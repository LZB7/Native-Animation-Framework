#!/usr/bin/env python3
"""Fail closed if a NAF HUD-stage relocation is absent or maps to another RVA."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


# The scope is intentionally bounded to plugin startup, the verified queue hook,
# HUD registration/construction, and operations directly reachable from NAFHUDMenu.
EXPECTED = {
    "MemoryManager::GetSingleton": (4471522, 0x1E02A0),
    "MemoryManager::Allocate": (2267872, 0x1657DE0),
    "MemoryManager::Deallocate": (2267874, 0x1658240),
    "MemoryManager::Reallocate": (2267873, 0x1658040),
    "MemoryManager::GetThreadScrapHeap": (2267850, 0x1657250),
    "BSStringPool::Entry::release": (2268720, 0x167D4A0),
    "BucketTable::GetSingleton": (2268736, 0x167EB20),
    "BSStringPool::GetEntry<char>": (2268729, 0x167DD20),
    "BSStringPool::GetEntry<wchar_t>": (2268730, 0x167E2A0),
    "HUDModeEvent::GetEventSource": (4801988, 0x3274910),
    "BSTGlobalEvent::GetSingleton": (4796078, 0x30E6EC0),
    "ApplyColorUpdateEvent::GetEventSource": (2707340, 0x3DBDCC0),
    "UI::GetSingleton": (4796314, 0x30E8930),
    "UI::RegisterMenu": (2284766, 0x1A826D0),
    "UIMessageQueue::GetSingleton": (4796377, 0x30E8B20),
    "UIMessageQueue::AddMessage": (2284929, 0x1A89630),
    "BSScaleformManager::GetSingleton": (4796889, 0x30F5D90),
    "BSScaleformManager::LoadMovie": (2287422, 0x1B1B1D0),
    "BSScaleformManager::InitMovieViewport": (4494254, 0x1B1BB30),
    "BSSystemFileStreamer::UncacheAll": (2268797, 0x16812A0),
    "bInvalidateOlderFiles setting": (2667999, 0x2F5B288),
    "IMenu::ShouldHandleEvent": (2287392, 0x1B19CC0),
    "IMenu::PassesRenderConditionText": (2287379, 0x1B198E0),
    "IMenu::OnSetSafeRect": (2287375, 0x1B19690),
    "IMenu::ProcessScaleformEvent": (2287395, 0x1B19FD0),
    "IMenu::RefreshPlatform": (2287374, 0x1B19500),
    "BSGFxObject::AcquireFlashObjectByMemberName": (2287014, 0x1B03D30),
    "BSGFxObject::AcquireFlashObjectByPath": (2287015, 0x1B03EC0),
    "BSGFxDisplayObject::RemoveChild": (2287327, 0x1B17D70),
    "BSGFxShaderFXTarget::ProcessEvent": (2287033, 0x1B04840),
    "BSGFxShaderFXTarget::AppendShaderFXInfos": (2287021, 0x1B04320),
    "BSGFxShaderFXTarget::CreateAndSetFiltersToColor": (2287028, 0x1B04550),
    "BSGFxShaderFXTarget::CreateAndSetFiltersToHUD": (2287027, 0x1B04500),
    "BSGFxShaderFXTarget::EnableShadedBackground": (2287022, 0x1B04350),
    "GameMenuBase::SetIsTopButtonBar": (2223204, 0xA72C30),
    "GameMenuBase::OnMenuDisplayStateChanged": (2223205, 0xA72CA0),
    "GameMenuBase::CacheShaderFXQuadsForRenderer_Impl": (2223200, 0xA72A40),
    "GameMenuBase::TransferCachedShaderFXQuadsForRenderer": (2223201, 0xA72AD0),
    "GameMenuBase::SetViewportRect": (2223202, 0xA72B40),
    "GameMenuBase::AppendShaderFXInfos": (2223203, 0xA72B90),
    "UIUtils::PlayMenuSound": (2249707, 0x1086570),
    "SendHUDMessage::PushHUDMode": (2222443, 0xA373A0),
    "SendHUDMessage::PopHUDMode": (2222444, 0xA373B0),
    "SendHUDMessage::ShowHUDMessage": (2222440, 0xA37270),
    "SendHUDMessage::ClearHUDMessages": (2222460, 0xA379E0),
    "NonRendersafeAI containing function": (2228937, 0xC365F0),
    "ProcessQueues": (2229181, 0xC45150),
    "GFx::Loader::CreateMovie": (2284723, 0x1A7F420),
    "GFx::Movie::Release": (2287230, 0x1B12000),
    "GFx::Value::ObjectAddRef": (2286228, 0x1AD6B70),
    "GFx::Value::ObjectRelease": (2286229, 0x1AD6BB0),
    "GFx::Value::HasMember": (2286078, 0x1ACE1A0),
    "GFx::Value::GetArraySize": (2285791, 0x1AC4A60),
    "GFx::Value::GetMember": (4494126, 0x1AC8AE0),
    "GFx::Value::SetMember": (2286589, 0x1AE6F00),
    "GFx::Value::Invoke": (2286101, 0x1AD02A0),
    "GFx::Value::PushBack": (2286424, 0x1ADD860),
    "GFx::Value::RemoveElements": (2286475, 0x1AE0E70),
    "GFx::Value::VisitMembers": (2286786, 0x1AF0CA0),
    "GFx::Value::GetDisplayInfo": (2285873, 0x1AC6E30),
    "Scaleform::Memory global heap": (2707353, 0x3DBDD80),
    "SWFToCodeFunctionHandler::RegisterCodeObject": (2287415, 0x1B1A620),
    "SWFToCodeFunctionHandler::MapCodeMethodToASFunction": (2287419, 0x1B1A920),
}


# The original audit verified only an enumerated database list. These guards
# also verify the source-level selector at the lazy Scaleform boundary, so an
# OG-only ID or 1.10.984 second member cannot pass merely because the correct AE
# record also happens to be listed above.
SOURCE_GUARDS = {
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Scaleform/GFx/GFx_Loader.h": (
        "REL::RelocationID(912291, 2284723)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Scaleform/GFx/GFx_Player.h": (
        "REL::RelocationID(788691, 2286078)",
        "REL::RelocationID(1517430, 4494126)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/MemoryManager.h": (
        "REL::RelocationID(343176, 4471522)",
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
            failures.append((owner, reloc_id, expected_rva, actual))

    if failures:
        for owner, reloc_id, expected, actual in failures:
            actual_text = "missing" if actual is None else f"0x{actual:X}"
            print(f"FAIL {owner}: ID {reloc_id}, expected 0x{expected:X}, got {actual_text}")
        return 1

    source_failures = []
    for relative_path, required_text in SOURCE_GUARDS.items():
        path = args.source_root / relative_path
        text = path.read_text(errors="replace")
        for required in required_text:
            if required not in text:
                source_failures.append(f"{relative_path}: missing {required}")
    if source_failures:
        for failure in source_failures:
            print(f"FAIL source selector: {failure}")
        return 1

    print(
        f"PASS: {len(EXPECTED)} HUD-stage relocation IDs matched exact expected RVAs; "
        f"{sum(map(len, SOURCE_GUARDS.values()))} source selectors matched"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
