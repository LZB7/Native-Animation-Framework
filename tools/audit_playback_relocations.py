#!/usr/bin/env python3
"""Verify the exact 1.11.221 relocation surface reached by basic scene playback."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "BSScript::ArrayWrapper<Variable>::ReplaceArray": (2250435, 0x10B6100),
    "BGSAnimationSystemUtils::InitializeActorInstant": (2236393, 0xE13E40),
    "AnimationSystemUtils::IsActiveGraphInTransition": (2214305, 0x806370),
    "AnimationSystemUtils::GetDefaultObjectForActionInitializeToBaseState": (2214309, 0x806750),
    "TESActionData::DoIt": (2234273, 0xDBFEC0),
    "AIProcess::SetupSpecialIdle": (2231704, 0xCFAD40),
    "IAnimationGraphManagerHolder::SetGraphVariableBool": (2214543, 0x818DA0),
    "hkbContext::hkbContext(hkbCharacter*,...)": (2258328, 0x13A9B10),
    "hkbContext::~hkbContext": (2258329, 0x13A9B90),
    "hkbBehaviorGraph::setActiveGeneratorLocalTime": (2260451, 0x147C170),
    "hkbBehaviorGraph::getNodeClone": (2260454, 0x147C5A0),
}

SOURCE_GUARDS = {
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/BSScript/ArrayWrapper.h": (
        "REL::RelocationID(445184, 2250435)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/Utilities.h": (
        "REL::RelocationID(672857, 2236393)",
        "REL::RelocationID(839650, 2214305)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/Actor.h": (
        "REL::RelocationID(1446774, 2231704)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESObjectREFRs.h": (
        "REL::RelocationID(1437197, 2214543)",
    ),
    "f4se-plugin/src/RE/BGSAnimationSystemUtils.h": (
        "REL::RelocationID(639576, 2214309)",
        "REL::RelocationID(390914, 2234273)",
    ),
    "f4se-plugin/src/RE/BSAnimationGraphManager.h": (
        "REL::RelocationID(1381136, 2258328)",
        "REL::RelocationID(144578, 2258329)",
    ),
    "f4se-plugin/src/RE/hk.h": (
        "REL::RelocationID(992878, 2260451)",
        "REL::RelocationID(326555, 2260454)",
    ),
}

FORBIDDEN = (
    "REL::ID(445184)",
    "REL::ID(1381136)",
    "REL::ID(144578)",
    "REL::ID(992878)",
)


def read_database(path: Path) -> dict[int, int]:
    data = path.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    return dict(struct.iter_unpack("<QQ", memoryview(data)[8:8 + count * 16]))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()

    database = read_database(args.database)
    failures: list[str] = []
    for owner, (reloc_id, expected_rva) in EXPECTED.items():
        actual = database.get(reloc_id)
        if actual != expected_rva:
            failures.append(f"{owner}: ID {reloc_id}, expected 0x{expected_rva:X}, got {actual!r}")

    combined = ""
    for relative, guards in SOURCE_GUARDS.items():
        text = (args.source_root / relative).read_text(encoding="utf-8")
        combined += text
        for guard in guards:
            if guard not in text:
                failures.append(f"{relative}: missing source guard {guard!r}")
    for forbidden in FORBIDDEN:
        if forbidden in combined:
            failures.append(f"playback surface retains stale invariant relocation {forbidden}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(f"Playback relocation audit passed: {len(EXPECTED)} exact 1.11.221 records/source guards verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
