#!/usr/bin/env python3
"""Verify exact Fallout 4 1.11.221 records selected by NAF's graph surface."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "IAnimationGraphManagerHolder::UpdateAnimationGraphManager": (2214536, 0x818970),
    "TESObjectREFR::Set3D": (2201055, 0x50C5B0),
    "IAnimationGraphManagerHolder::GetAnimationGraphProjectName": (2214534, 0x818730),
    "BGSAnimationSystemUtils::InitializeActorInstant": (2236393, 0xE13E40),
    "AnimationSystemUtils::IsActiveGraphInTransition": (2214305, 0x806370),
    "IAnimationGraphManagerHolder::SetGraphVariableBool": (2214543, 0x818DA0),
    "IAnimationGraphManagerHolder::SetGraphVariableFloat": (2214545, 0x818DE0),
    "IAnimationGraphManagerHolder::SetGraphVariableInt": (2214544, 0x818DC0),
    "IAnimationGraphManagerHolder::RevertAnimationGraphManager": (2214541, 0x818D10),
    "BGSAnimationSystemUtils::GetEventSourcePointersFromGraph": (2249847, 0x1090410),
    "BSModelDB::Demand<NiNode>": (2275153, 0x17B3E30),
    "BSModelDB::Demand<NiObject>": (2275154, 0x17B3FB0),
    "BSStringPool::Entry::release": (2268720, 0x167D180),
    "BSStringPool::GetEntry<char>": (2268729, 0x167DA00),
    "BSStringPool::GetEntry<wchar_t>": (2268730, 0x167DF80),
    "PlayerCharacter singleton": (2698073, 0x32D22E0),
}

SOURCE_GUARDS = {
    "f4se-plugin/src/BodyAnimation/GraphHook.h": (
        "REL::RelocationID(1492656, 2214536)",
        "REL::RelocationID(1216262, 2201055)",
    ),
    "f4se-plugin/src/Compat/Engine.h": (
        "REL::RelocationID(190119, 2214534)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/Utilities.h": (
        "REL::RelocationID(672857, 2236393)",
        "REL::RelocationID(839650, 2214305)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESObjectREFRs.h": (
        "REL::RelocationID(1437197, 2214543)",
        "REL::RelocationID(27400, 2214545)",
        "REL::RelocationID(732926, 2214544)",
        "REL::RelocationID(41382, 2214541)",
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
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    database = read_database(args.database)
    failures = []
    for owner, (reloc_id, expected_rva) in EXPECTED.items():
        actual = database.get(reloc_id)
        if actual != expected_rva:
            failures.append(
                f"{owner}: ID {reloc_id}, expected 0x{expected_rva:X}, "
                f"got {'missing' if actual is None else f'0x{actual:X}'}"
            )

    for relative_path, required_text in SOURCE_GUARDS.items():
        text = (args.source_root / relative_path).read_text(errors="replace")
        for required in required_text:
            if required not in text:
                failures.append(f"{relative_path}: missing selector {required}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1

    print(
        f"PASS: {len(EXPECTED)} graph relocation IDs matched exact expected RVAs; "
        f"{sum(map(len, SOURCE_GUARDS.values()))} source selectors matched"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
