#!/usr/bin/env python3
"""Audit Phase-B callbacks, transforms, nested trees, and AE highlighting."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "TESObjectREFR::ApplyEffectShader": (2205201, 0x5E1E80),
    "ProcessLists singleton": (4796160, 0x30DC140),
}

SOURCE_GUARDS = {
    "f4se-plugin/src/Scene/OrderedActionQueue.h": (
        "kArbitraryCallbacksEnabled = true",
        "std::queue<std::function<bool()>> equipQueue",
        "equipQueue = std::queue<std::function<bool()>>()",
        "RE::NiPointer<RE::Actor>(a)",
    ),
    "f4se-plugin/src/Scene/ControlSystems.h": (
        "GetControlSystem(p)",
        "Position tree '{}' contains a cyclic nested-tree reference",
        "subSystem->OnSerializationLoad(this)",
        "parent->ReportSystemCompletion()",
    ),
    "f4se-plugin/src/Data/User/Position.h": (
        "Rejected cyclic position-tree reference",
        "GetBaseAnimation(visitedTrees)",
    ),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/ManageScenesHandler.h": (
        '"Speed (%): "',
        '"Change Position..."',
        '"Actor Inventories..."',
    ),
    "f4se-plugin/src/Menu/HighlightedObjectsContainer.h": (
        "kHighlightingEnabled = true",
        "EngineCompat::PlayEffectShader",
        "EngineCompat::StopEffectShader",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESObjectREFRs.h": (
        "REL::RelocationID(652173, 2205201)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/ProcessLists.h": (
        "REL::RelocationID(1569706, 4796160)",
    ),
}


def read_database(path: Path) -> dict[int, int]:
    data = path.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    if len(data) != 8 + count * 16:
        raise ValueError(f"invalid Address Library size: {path}")
    return dict(struct.iter_unpack("<QQ", memoryview(data)[8:]))


def read_pe(path: Path, rva: int, size: int) -> bytes:
    data = path.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    section_count = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    sections = pe + 24 + optional_size
    for index in range(section_count):
        section = sections + index * 40
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", data, section + 8)
        if virtual_address <= rva < virtual_address + max(virtual_size, raw_size):
            offset = raw_offset + rva - virtual_address
            return data[offset:offset + size]
    raise ValueError(f"RVA 0x{rva:X} is outside PE sections")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    failures: list[str] = []
    database = read_database(args.database)

    for owner, (reloc_id, expected_rva) in EXPECTED.items():
        actual = database.get(reloc_id)
        if actual != expected_rva:
            failures.append(f"{owner}: ID {reloc_id}, expected 0x{expected_rva:X}, got {actual!r}")

    expected_prologue = bytes.fromhex("48 89 5C 24 18 57 48 83 EC 30")
    actual_prologue = read_pe(args.executable, EXPECTED["TESObjectREFR::ApplyEffectShader"][1], len(expected_prologue))
    if actual_prologue != expected_prologue:
        failures.append(f"ApplyEffectShader prologue mismatch: {actual_prologue.hex(' ')}")

    for relative, guards in SOURCE_GUARDS.items():
        source = (args.source_root / relative).read_text(encoding="utf-8")
        for guard in guards:
            if guard not in source:
                failures.append(f"{relative}: missing source guard {guard!r}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(
        f"PASS: {len(EXPECTED)} AE records, ApplyEffectShader entry bytes, and "
        f"{sum(map(len, SOURCE_GUARDS.values()))} Phase-B source guards matched"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
