#!/usr/bin/env python3
"""Audit the bounded production animation-group and position-tree surface."""

from __future__ import annotations

import argparse
import struct
import xml.etree.ElementTree as ET
from pathlib import Path


PLAY_MENU_SOUND_ID = 2249707
PLAY_MENU_SOUND_RVA = 0x1086220

SOURCE_GUARDS = {
    "f4se-plugin/src/Data/User/Position.h": (
        "case kAnimationGroup:",
        "case kPositionTree:",
        "std::ranges::all_of(group->stages",
		"Rejected cyclic position-tree reference",
    ),
    "f4se-plugin/src/Scene/ControlSystems.h": (
        "class AnimationGroupControlSystem",
        "class PositionTreeControlSystem",
        "TransitionToAnimation",
		"GetControlSystem(p)",
		"subSystem->OnSerializationLoad(this)",
        'RE::UIUtils::PlayMenuSound("UIMenuOK")',
    ),
    "f4se-plugin/src/Scene/OrderedActionQueue.h": (
        "kArbitraryCallbacksEnabled = true",
    ),
}


def read_database(path: Path) -> dict[int, int]:
    data = path.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    if len(data) != 8 + count * 16:
        raise ValueError(f"invalid Address Library size: {path}")
    return dict(struct.iter_unpack("<QQ", memoryview(data)[8:]))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("--chak-group", type=Path)
    parser.add_argument("--chak-position", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    failures: list[str] = []

    actual = read_database(args.database).get(PLAY_MENU_SOUND_ID)
    if actual != PLAY_MENU_SOUND_RVA:
        failures.append(
            f"UIUtils::PlayMenuSound: ID {PLAY_MENU_SOUND_ID}, expected 0x{PLAY_MENU_SOUND_RVA:X}, got {actual!r}"
        )

    for relative_path, selectors in SOURCE_GUARDS.items():
        source = (args.source_root / relative_path).read_text(errors="replace")
        for selector in selectors:
            if selector not in source:
                failures.append(f"{relative_path}: missing selector {selector}")

    if bool(args.chak_group) != bool(args.chak_position):
        failures.append("CHAK group and position XML must be supplied together")
    elif args.chak_group:
        group_root = ET.parse(args.chak_group).getroot()
        position_root = ET.parse(args.chak_position).getroot()
        group = group_root.find(".//animationGroup[@id='CHAK_Hugs_Staged']")
        stages = [] if group is None else [stage.get("animation") for stage in group.findall("stage")]
        if group is None or group.get("sequential") != "true" or stages != ["CHAK_Hug_01", "CHAK_Hug_02"]:
            failures.append(f"unexpected CHAK_Hugs_Staged definition: sequential={None if group is None else group.get('sequential')}, stages={stages}")
        position = position_root.find(".//position[@id='(CHAK) *Staged* Hugs']")
        if position is None or position.get("animationGroup") != "CHAK_Hugs_Staged":
            failures.append("staged CHAK position does not reference CHAK_Hugs_Staged")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1

    suffix = "; CHAK staged Hugs linkage matched" if args.chak_group else ""
    print(f"PASS: group/tree production guards and UIUtils::PlayMenuSound AE record matched{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
