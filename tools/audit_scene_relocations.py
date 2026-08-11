#!/usr/bin/env python3
"""Verify the bounded Fallout 4 1.11.221 basic-scene execution surface."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "Actor::DisableCollision": (2229653, 0xC5CA70),
    "Actor::EnableCollision": (2229652, 0xC5C9E0),
    "Actor::GetCanDoFavor": (2230148, 0xC930B0),
    "Actor::SetCanDoFavor": (2230149, 0xC93130),
    "Actor::SetRestrained": (2229910, 0xC73E30),
    "Actor::StopAlarmOnActor": (2229667, 0xC5CEF0),
    "Actor::StopInteractingQuick": (2231227, 0xCDBC10),
    "Actor::StopMoving": (2230262, 0xC9B9F0),
    "Actor::ClearLookAtTarget": (2230291, 0xC9CB70),
    "Actor::TurnOffHeadtracking": (2230240, 0xC9AB10),
    "Actor::WarpToRef": (2229838, 0xC6EB80),
    "PlayerCharacter::Update3D": (2231882, 0xD02270),
    "PlayerCamera::Force3rdPerson": (2248348, 0x1026A40),
    "DisableInputForPlayer": (2200872, 0x4FE440),
    "ReenableInputForPlayer": (2200873, 0x4FE540),
    "PlayerCharacter::SetAIControlled": (2233087, 0xD6EDF0),
    "BGSAnimationSystemUtils::InitializeActorInstant": (2236393, 0xE13E40),
    "AnimationSystemUtils::IsActiveGraphInTransition": (2214305, 0x806370),
    "AnimationSystemUtils::GetDefaultObjectForActionInitializeToBaseState": (2214309, 0x806750),
    "TESActionData::DoIt": (2234273, 0xDBFEC0),
    "TESActionData vtable": (780200, 0x245F6E8),
    "TESForm all-forms map": (4796465, 0x30E0E48),
    "TESForm all-forms lock": (4796476, 0x30E0E98),
    "TESForm editor-ID map": (4796466, 0x30E0E50),
    "TESForm editor-ID lock": (4796477, 0x30E0EA0),
    "TESObjectREFR::SetAngleOnReference": (2201134, 0x5137A0),
    "TESObjectREFR::SetScale": (2200893, 0x4FF5A0),
    "TESHitEvent source": (2201886, 0x533C80),
    "Actor-location event registration": (2201562, 0x52B800),
    "TESDeathEvent source": (2201833, 0x531170),
    "AIProcess::SetupSpecialIdle": (2231704, 0xCFAD40),
    "AIProcess::ClearCurrentPackage": (2231582, 0xCF1340),
    "AIProcess::ClearCurrentDataForProcess": (2232435, 0xD394F0),
    "Actor::EvaluatePackage": (2229805, 0xC69240),
    "TESDataHandler singleton": (4796135, 0x30DC080),
    "TESObjectREFR::SetLinkedRef": (2202684, 0x5645B0),
    "ControlMap singleton": (4799307, 0x31E5B08),
    "ControlMap::PushInputContext": (2268335, 0x166F430),
    "ControlMap::PopInputContext": (2268336, 0x166F500),
    "ControlMap::SetTextEntryMode": (4491359, 0x166F9A0),
}

SOURCE_GUARDS = {
    "f4se-plugin/src/Tasks/GameLoopHook.h": (
        "kGameplayUpdateWorkEnabled = true",
        "REL::RelocationID(338354, 2228937)",
    ),
    "f4se-plugin/src/RE/BGSAnimationSystemUtils.h": (
        "REL::RelocationID(639576, 2214309)",
        "REL::RelocationID(390914, 2234273)",
    ),
    "f4se-plugin/src/Scene/OrderedActionQueue.h": (
        "kEquipmentActionsEnabled = false",
        "kMorphActionsEnabled = false",
        "kArbitraryCallbacksEnabled = false",
    ),
    "f4se-plugin/src/Scene/SceneManager.h": ("IsBasicSceneCompatible()",),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESForms.h": (
        "REL::RelocationID(422985, 4796465)",
        "REL::RelocationID(691815, 4796476)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESDataHandler.h": (
        "REL::RelocationID(711558, 4796135)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESObjectREFRs.h": (
        "REL::RelocationID(192840, 2202684)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/ControlMap.h": (
        "REL::RelocationID(325206, 4799307)",
        "REL::RelocationID(1270079, 4491359)",
    ),
    "f4se-plugin/src/Scripts/Papyrus.h": (
        "PAPYRUS_BIND(ToggleMenu)",
        "PAPYRUS_BIND(WalkToAndStartScene)",
    ),
    "f4se-plugin/src/Menu/Menu.h": ("RegisterProductionSceneMenu",),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/MainMenuHandler.h": (
        '{ "New Scene", BindGoto(kNewScene) }',
        '{ "Active Scenes", BindGoto(kManageScenes) }',
    ),
}

FORBIDDEN = (
    "REL::ID(686648)",
    "REL::ID(22408)",
    "REL::ID(639576)",
    "REL::RelocationID(422985, 2689178)",
    "REL::RelocationID(691815, 2689189)",
    "REL::RelocationID(711558, 2688883)",
    "REL::RelocationID(325206, 2692014)",
    "REL::ID(192840)",
)


def read_database(path: Path) -> dict[int, int]:
    data = path.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    return dict(struct.iter_unpack("<QQ", memoryview(data)[8:8 + count * 16]))


def read_pe_data(path: Path, rva: int, size: int) -> tuple[bytes, int]:
    data = path.read_bytes()
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    section_count = struct.unpack_from("<H", data, pe_offset + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe_offset + 20)[0]
    optional = pe_offset + 24
    image_base = struct.unpack_from("<Q", data, optional + 24)[0]
    sections = optional + optional_size
    for index in range(section_count):
        section = sections + index * 40
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", data, section + 8)
        if virtual_address <= rva < virtual_address + max(virtual_size, raw_size):
            offset = raw_offset + rva - virtual_address
            return data[offset:offset + size], image_base
    raise ValueError(f"RVA 0x{rva:X} is outside PE sections")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    database = read_database(args.database)
    failures: list[str] = []

    for owner, (reloc_id, expected_rva) in EXPECTED.items():
        actual = database.get(reloc_id)
        if actual != expected_rva:
            failures.append(f"{owner}: ID {reloc_id}, expected 0x{expected_rva:X}, got {actual!r}")

    vtable_rva = EXPECTED["TESActionData vtable"][1]
    do_it_pointer, image_base = read_pe_data(args.executable, vtable_rva + 5 * 8, 8)
    if struct.unpack("<Q", do_it_pointer)[0] - image_base != EXPECTED["TESActionData::DoIt"][1]:
        failures.append("TESActionData physical vslot 0x05 does not target TESActionData::DoIt")

    do_it, _ = read_pe_data(args.executable, EXPECTED["TESActionData::DoIt"][1], 3)
    if do_it != bytes.fromhex("48 8B D1"):
        failures.append(f"TESActionData::DoIt prologue mismatch: {do_it.hex(' ')}")

    combined = ""
    for relative, guards in SOURCE_GUARDS.items():
        text = (args.source_root / relative).read_text(encoding="utf-8")
        combined += text
        for guard in guards:
            if guard not in text:
                failures.append(f"{relative}: missing source guard {guard!r}")
    for forbidden in FORBIDDEN:
        if forbidden in combined:
            failures.append(f"scene surface retains forbidden stale relocation {forbidden}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(f"Scene relocation audit passed: {len(EXPECTED)} exact 1.11.221 records and ABI/source guards verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
