#!/usr/bin/env python3
"""Verify exact Fallout 4 1.11.221 records selected by NAF's package surface."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "Actor::EvaluatePackage": (2229805, 0xC69240),
    "AIProcess::CheckforNewPackage": (2231539, 0xCEFAD0),
    "Actor::GetActorHandle": (2229894, 0xC73410),
    "AIProcess::ClearCurrentPackage": (2231582, 0xCF1340),
    "AIProcess::ClearCurrentDataForProcess": (2232435, 0xD394F0),
    "AIProcess::SetupSpecialIdle": (2231704, 0xCFAD40),
    "TESObjectREFR_Events::RegisterForPackage": (2201672, 0x52D7B0),
    "TESObjectREFR_Events::UnregisterForPackage": (2201673, 0x52D7E0),
    "ActorHandleManager::CreateHandle": (2188375, 0x21E3F0),
    "ActorHandleManager::GetHandle": (2188676, 0x22CA40),
    "ActorHandleManager::GetSmartPointer": (2188681, 0x22CD40),
    "Actor vtable": (1455516, 0x25602C0),
}

SOURCE_GUARDS = {
    "f4se-plugin/src/PackageOverride/EvalHook.h": (
        "REL::RelocationID(1395257, 2229805)",
        "REL::RelocationID(609985, 2231539)",
    ),
    "f4se-plugin/src/RE/TESObjectREFR_Events.h": (
        "REL::RelocationID(1448133, 2201672)",
        "REL::RelocationID(1182113, 2201673)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/Actor.h": (
        "REL::RelocationID(1145222, 2229894)",
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
    failures: list[str] = []

    for owner, (reloc_id, expected_rva) in EXPECTED.items():
        actual = database.get(reloc_id)
        if actual != expected_rva:
            failures.append(
                f"{owner}: ID {reloc_id}, expected 0x{expected_rva:X}, "
                f"got {'missing' if actual is None else f'0x{actual:X}'}"
            )

    for relative_path, selectors in SOURCE_GUARDS.items():
        source = (args.source_root / relative_path).read_text(errors="replace")
        for selector in selectors:
            if selector not in source:
                failures.append(f"{relative_path}: missing selector {selector}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1

    print(
        f"PASS: {len(EXPECTED)} package relocation IDs matched exact expected RVAs; "
        f"{sum(map(len, SOURCE_GUARDS.values()))} source selectors matched"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
