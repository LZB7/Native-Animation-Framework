#!/usr/bin/env python3
"""Fail closed if a NAF HUD-stage relocation is absent or maps to another RVA."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


# The scope is intentionally bounded to plugin startup, the verified queue hook,
# HUD registration/construction, and operations directly reachable from NAFHUDMenu.
EXPECTED = {
    "MemoryManager::GetSingleton": (4471522, 0x1DFF80),
    "MemoryManager::Allocate": (2267872, 0x1657AC0),
    "MemoryManager::Deallocate": (2267874, 0x1657F20),
    "MemoryManager::Reallocate": (2267873, 0x1657D20),
    "MemoryManager::GetThreadScrapHeap": (2267850, 0x1656F30),
    "BSStringPool::Entry::release": (2268720, 0x167D180),
    "BucketTable::GetSingleton": (2268736, 0x167E800),
    "BSStringPool::GetEntry<char>": (2268729, 0x167DA00),
    "BSStringPool::GetEntry<wchar_t>": (2268730, 0x167DF80),
    "HUDModeEvent::GetEventSource": (4801988, 0x3269880),
    "BSTGlobalEvent::GetSingleton": (4796078, 0x30DBE40),
    "ApplyColorUpdateEvent::GetEventSource": (2707340, 0x3DA7440),
    "UI::GetSingleton": (4796314, 0x30DD8B0),
    "UI::RegisterMenu": (2284766, 0x1A82210),
    "UIMessageQueue::GetSingleton": (4796377, 0x30DDAA0),
    "UIMessageQueue::AddMessage": (2284929, 0x1A89170),
    "BSScaleformManager::GetSingleton": (4796889, 0x30EAD10),
    "BSScaleformManager::LoadMovie": (2287422, 0x1B1AD10),
    "BSScaleformManager::InitMovieViewport": (4494254, 0x1B1B670),
    "BSSystemFileStreamer::UncacheAll": (2268797, 0x1680F80),
    "bInvalidateOlderFiles setting": (2667999, 0x2F50298),
    "IMenu::ShouldHandleEvent": (2287392, 0x1B19800),
    "IMenu::PassesRenderConditionText": (2287379, 0x1B19420),
    "IMenu::OnSetSafeRect": (2287375, 0x1B191D0),
    "IMenu::ProcessScaleformEvent": (2287395, 0x1B19B10),
    "IMenu::RefreshPlatform": (2287374, 0x1B19040),
    "BSGFxObject::AcquireFlashObjectByMemberName": (2287014, 0x1B03870),
    "BSGFxObject::AcquireFlashObjectByPath": (2287015, 0x1B03A00),
    "BSGFxDisplayObject::RemoveChild": (2287327, 0x1B178B0),
    "BSGFxShaderFXTarget::ProcessEvent": (2287033, 0x1B04380),
    "BSGFxShaderFXTarget::AppendShaderFXInfos": (2287021, 0x1B03E60),
    "BSGFxShaderFXTarget::CreateAndSetFiltersToColor": (2287028, 0x1B04090),
    "BSGFxShaderFXTarget::CreateAndSetFiltersToHUD": (2287027, 0x1B04040),
    "BSGFxShaderFXTarget::EnableShadedBackground": (2287022, 0x1B03E90),
    "GameMenuBase::SetIsTopButtonBar": (2223204, 0xA72920),
    "GameMenuBase::OnMenuDisplayStateChanged": (2223205, 0xA72990),
    "GameMenuBase::CacheShaderFXQuadsForRenderer_Impl": (2223200, 0xA72730),
    "GameMenuBase::TransferCachedShaderFXQuadsForRenderer": (2223201, 0xA727C0),
    "GameMenuBase::SetViewportRect": (2223202, 0xA72830),
    "GameMenuBase::AppendShaderFXInfos": (2223203, 0xA72880),
    "UIUtils::PlayMenuSound": (2249707, 0x1086220),
    "SendHUDMessage::PushHUDMode": (2222443, 0xA37090),
    "SendHUDMessage::PopHUDMode": (2222444, 0xA370A0),
    "SendHUDMessage::ShowHUDMessage": (2222440, 0xA36F60),
    "SendHUDMessage::ClearHUDMessages": (2222460, 0xA376D0),
    "NonRendersafeAI containing function": (2228937, 0xC36260),
    "ProcessQueues": (2229181, 0xC44DC0),
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
    args = parser.parse_args()
    database = read_database(args.database)
    failures = []
    for owner, (reloc_id, expected_rva) in EXPECTED.items():
        actual = database.get(reloc_id)
        if actual != expected_rva:
            failures.append((owner, reloc_id, expected_rva, actual))

    if failures:
        for owner, reloc_id, expected, actual in failures:
            actual_text = "missing" if actual is None else f"0x{actual:X}"
            print(f"FAIL {owner}: ID {reloc_id}, expected 0x{expected:X}, got {actual_text}")
        return 1

    print(f"PASS: {len(EXPECTED)} HUD-stage relocation IDs matched exact expected RVAs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
