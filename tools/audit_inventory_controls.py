#!/usr/bin/env python3
"""Audit the production Active Scenes inventory/equipment control path for AE."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "OpenContainerMenu dispatcher": (2248608, 0x103E540),
    "UI singleton": (4796314, 0x30E8930),
    "UI menu-map lock": (2707105, 0x3DBD370),
    "UIMessageQueue singleton": (4796377, 0x30E8B20),
    "UIMessageQueue::AddMessage": (2284929, 0x1A89630),
    "ActorEquipManager singleton": (4798287, 0x31EE428),
    "ActorEquipManager::EquipObject": (2231392, 0xCE5E20),
    "ActorEquipManager::UnequipObject": (2231395, 0xCE62C0),
    "TESObjectREFR::GetInventoryObjectCount": (2200939, 0x5025B0),
    "TESObjectREFR::AddInventoryItem": (2200949, 0x502C70),
    "TESObjectREFR::ApplyEffectShader": (2205201, 0x5E21A0),
    "ProcessLists singleton": (4796160, 0x30E71C0),
}

SOURCE_GUARDS = {
    "f4se-plugin/src/RE/ContainerMenu.h": (
        "REL::ID(1105206, 2248608)",
        "1.10.163-only",
    ),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/InventoryMenuHandler.h": (
        "OpenContainerMenu(a, 3, false)",
        "EngineCompat::ModifyKeyword(targetActor, Data::Forms::ShowWornItemsKW, false)",
        "state->restoreSubmenu = kInventories",
        "actor handle no longer resolves; suppressing menu open",
    ),
    "f4se-plugin/src/Compat/Engine.h": (
        "a_refr->AddKeyword(a_keyword)",
        "a_refr->RemoveKeyword(a_keyword)",
        "GetMenuMapRWLock()",
    ),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/ManageScenesHandler.h": (
        '"Speed (%): "',
        '"Change Position..."',
        '"Actor Inventories..."',
    ),
    "f4se-plugin/src/Menu/HighlightedObjectsContainer.h": (
        "EngineCompat::PlayEffectShader",
        "EngineCompat::StopEffectShader",
    ),
}

FORBIDDEN = (
    "REL::ID(1105206)",
    "REL::ID(1105206, 2193197)",
)


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

    # Guard the verified callable entry only; this is not a signature search.
    expected_entry = bytes.fromhex("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 8B")
    actual_entry = read_pe(args.executable, EXPECTED["OpenContainerMenu dispatcher"][1], len(expected_entry))
    if actual_entry != expected_entry:
        failures.append(f"OpenContainerMenu entry mismatch: {actual_entry.hex(' ')}")

    combined = ""
    for relative, guards in SOURCE_GUARDS.items():
        source = (args.source_root / relative).read_text(encoding="utf-8", errors="replace")
        combined += source
        for guard in guards:
            if guard not in source:
                failures.append(f"{relative}: missing source guard {guard!r}")
    for forbidden in FORBIDDEN:
        if forbidden in combined:
            failures.append(f"inventory surface retains stale selector {forbidden}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(
        f"PASS: {len(EXPECTED)} inventory/equipment AE records, OpenContainerMenu entry, "
        f"and {sum(map(len, SOURCE_GUARDS.values()))} source guards matched"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
