#!/usr/bin/env python3
"""Audit the bounded production NAFMenu entry/navigation surface for 1.11.221."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "BSScript::Stack::GetPageForFrame": (2314680, 0x20F32F0),
    "BSScript::Stack::GetStackFrameVariable": (2314681, 0x20F3360),
    "UI singleton": (4796314, 0x30DD8B0),
    "UI menu-map lock": (2707105, 0x3DA6AF0),
    "UI::RegisterMenu": (2284766, 0x1A82210),
    "UIMessageQueue singleton": (4796377, 0x30DDAA0),
    "UIMessageQueue::AddMessage": (2284929, 0x1A89170),
    "ControlMap singleton": (4799307, 0x31E5B08),
    "ControlMap::PushInputContext": (2268335, 0x166F430),
    "ControlMap::PopInputContext": (2268336, 0x166F500),
    "ControlMap::SetTextEntryMode": (4491359, 0x166F9A0),
    "PlayerControls singleton": (4799306, 0x31E5B00),
    "BSScaleformManager singleton": (4796889, 0x30EAD10),
    "BSScaleformManager::LoadMovie": (2287422, 0x1B1AD10),
    "BSGFxShaderFXTarget::CreateAndSetFiltersToHUD": (2287027, 0x1B04040),
    "SendHUDMessage::ShowHUDMessage": (2222440, 0xA36F60),
    "SendHUDMessage::PushHUDMode": (2222443, 0xA37090),
    "SendHUDMessage::PopHUDMode": (2222444, 0xA370A0),
    "UIUtils::PlayMenuSound": (2249707, 0x1086220),
    "TESDataHandler singleton": (4796135, 0x30DC080),
    "TESForm all-forms map": (4796465, 0x30E0E48),
    "TESForm all-forms lock": (4796476, 0x30E0E98),
    "TESObjectREFR::GetDisplayFullName": (2201126, 0x513310),
    "TESObjectREFR::SetLinkedRef": (2202684, 0x5645B0),
}

SOURCE_GUARDS = {
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/BSScript/Stack.h": (
        "REL::RelocationID(1429302, 2314680)",
        "REL::RelocationID(897539, 2314681)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/ControlMap.h": (
        "REL::RelocationID(325206, 4799307)",
        "REL::RelocationID(1270079, 4491359)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/PlayerControls.h": (
        "REL::RelocationID(544871, 4799306)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESObjectREFRs.h": (
        "REL::RelocationID(192840, 2202684)",
    ),
    "f4se-plugin/src/Menu/HighlightedObjectsContainer.h": (
        "kHighlightingEnabled = false",
    ),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/MainMenuHandler.h": (
        '{ "New Scene", BindGoto(kNewScene) }',
        '{ "Active Scenes", BindGoto(kManageScenes) }',
    ),
    "f4se-plugin/src/Scripts/Papyrus.h": ("PAPYRUS_BIND(ToggleMenu)",),
}

FORBIDDEN = (
    "REL::ID(1429302)",
    "REL::RelocationID(544871, 2692013)",
    "REL::RelocationID(325206, 2692014)",
    "REL::ID(192840)",
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()

    data = args.database.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    database = dict(struct.iter_unpack("<QQ", memoryview(data)[8:8 + count * 16]))
    failures: list[str] = []

    for owner, (reloc_id, expected_rva) in EXPECTED.items():
        actual = database.get(reloc_id)
        if actual != expected_rva:
            failures.append(f"{owner}: ID {reloc_id}, expected 0x{expected_rva:X}, got {actual!r}")

    combined = ""
    for relative, guards in SOURCE_GUARDS.items():
        source = (args.source_root / relative).read_text(errors="replace")
        combined += source
        for guard in guards:
            if guard not in source:
                failures.append(f"{relative}: missing source guard {guard!r}")
    for forbidden in FORBIDDEN:
        if forbidden in combined:
            failures.append(f"menu surface retains stale selector {forbidden}")

    manage = (args.source_root / "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/ManageScenesHandler.h").read_text(errors="replace")
    for forbidden_entry in ('"Change Position..."', '"Actor Inventories..."', '"Speed (%): "'):
        # Functions remain compiled for later restoration; only the binding in
        # the reachable active-scene menu must be absent.
        active_block = manage[manage.index("case kManageScene:"):manage.index("case kManageWalkInstance:")]
        if forbidden_entry in active_block:
            failures.append(f"active-scene menu exposes gated entry {forbidden_entry}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(f"PASS: {len(EXPECTED)} production-menu relocation IDs and bounded source guards verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
