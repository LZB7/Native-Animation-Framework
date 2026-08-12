#!/usr/bin/env python3
"""Bounded source audit for the enabled Papyrus/serialization lifecycle."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


# This is the bounded relocation surface added by Papyrus registration and
# serialization itself.  HUD and package native bodies are verified by their
# dedicated audits; the shared allocator/string/handle machinery is repeated
# here deliberately so this audit can fail independently before deployment.
EXPECTED = {
    "GameVM::GetSingleton (g_GameVM)": (4796420, 0x30E0288),
    "BSScript::NF_util::NativeFunctionBase::Call": (2314780, 0x20F9870),
    "MemoryManager::GetSingleton": (4471522, 0x1DFF80),
    "MemoryManager::Allocate": (2267872, 0x1657AC0),
    "MemoryManager::Deallocate": (2267874, 0x1657F20),
    "MemoryManager::Reallocate": (2267873, 0x1657D20),
    "MemoryManager::GetThreadScrapHeap": (2267850, 0x1656F30),
    "BSStringPool::Entry::release": (2268720, 0x167D180),
    "BSStringPool::BucketTable::GetSingleton": (2268736, 0x167E800),
    "BSStringPool::GetEntry<char>": (2268729, 0x167DA00),
    "BSStringPool::GetEntry<wchar_t>": (2268730, 0x167DF80),
    "ActorHandleManager::CreateHandle": (2188375, 0x21E3F0),
    "ActorHandleManager::GetHandle": (2188676, 0x22CA40),
    "ActorHandleManager::GetSmartPointer": (2188681, 0x22CD40),
    "Actor::GetActorHandle": (2229894, 0xC73410),
}

SOURCE_GUARDS = {
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/GameScript.h": (
        "REL::RelocationID(996227, 4796420)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/BSScript/NF_util/NativeFunctionBase.h": (
        "REL::RelocationID(571037, 2314780)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESForms.h": (
		"REL::RelocationID(422985, 4796465)",
		"REL::RelocationID(691815, 4796476)",
    ),
}


SAFE_NATIVES = {
    "SetDisableRescaler",
    "GetDisableRescaler",
    "SetPackageOverride",
    "ClearPackageOverride",
    "DrawRectangle",
    "DrawText",
    "DrawLine",
    "SetElementPosition",
    "TranslateElementTo",
    "StopElementTranslation",
    "RemoveElement",
    "GetElementWidth",
    "GetElementHeight",
    "MoveElementToFront",
    "MoveElementToBack",
    "SetElementMask",
    "AttachElementTo",
	"StartScene",
	"StopScene",
	"GetSceneFromActor",
	"IsSceneRunning",
	"ToggleMenu",
	"WalkToAndStartScene",
	"IsActorUsable",
	"SetActorUsable",
	"PlayDynamicIdle",
	"SetScenePosition",
	"GetSceneActors",
	"GetSceneHKXs",
	"GetSceneTags",
	"SetSceneSpeed",
	"GetSceneSpeed",
	"GetSceneProperty",
	"ValidateSceneParams",
	"ReEnablePlayerInput",
	"SetIKChainTarget",
	"SetIKChainEnabled",
	"GetLocalTransform",
    "SynchronizeAnimations",
    "PlayNANIM",
    "StopNANIM",
}

GATED_NATIVES = {
    "SetEyeCoordOverride",
    "ClearEyeCoordOverride",
    "PlayFaceAnimation",
    "StopFaceAnimation",
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    failures: list[str] = []

    data = args.database.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    if len(data) != 8 + count * 16:
        failures.append(f"invalid Address Library size: {args.database}")
        database = {}
    else:
        database = dict(struct.iter_unpack("<QQ", memoryview(data)[8:]))

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

    papyrus = (args.source_root / "f4se-plugin/src/Scripts/Papyrus.h").read_text(errors="replace")
    for name in SAFE_NATIVES:
        if f"PAPYRUS_BIND({name});" not in papyrus:
            failures.append(f"safe native is not registered: {name}")
    for name in GATED_NATIVES:
        if f"PAPYRUS_BIND({name});" in papyrus:
            failures.append(f"gated native is reachable: {name}")

    main_source = (args.source_root / "f4se-plugin/src/main.cpp").read_text(errors="replace")
    for selector in (
        "serialization->SetUniqueID('NAF')",
        "serialization->SetSaveCallback(Serialization::SaveCallback)",
        "serialization->SetLoadCallback(Serialization::LoadCallback)",
        "serialization->SetRevertCallback(Serialization::RevertCallback)",
        "papyrus->Register(Papyrus::RegisterFunctions)",
    ):
        if selector not in main_source:
            failures.append(f"missing F4SE lifecycle selector: {selector}")

    serialization = (args.source_root / "f4se-plugin/src/Serialization/Serialization.h").read_text(errors="replace")
    for selector in (
        "SAVE_PERSISTENT_STATE('HUD', 5",
        "SAVE_PERSISTENT_STATE('PACK', 5",
        "deferredRecords.push_back({ type, version, std::move(payload) })",
        "ReadRecordPayload(a_intfc, length, payload)",
		"SAVE_PERSISTENT_STATE('SCNE', 5",
		"SAVE_PERSISTENT_STATE('TASK', 5",
		"RunRestoredSceneFixup",
    ):
        if selector not in serialization:
            failures.append(f"missing serialization compatibility guard: {selector}")

    f4se = (args.source_root / "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/F4SE/Interfaces.h").read_text(errors="replace")
    if "using RegisterFunctions = bool F4SEAPI(RE::BSScript::IVirtualMachine* a_vm);" not in f4se:
        failures.append("F4SE Papyrus registration callback ABI changed")
    if "using EventCallback = void      F4SEAPI(const SerializationInterface* a_intfc);" not in f4se:
        failures.append("F4SE serialization callback ABI changed")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1

    print(
        f"PASS: {len(EXPECTED)} Papyrus/serialization relocation IDs matched exact RVAs; "
        f"{len(SAFE_NATIVES)} safe natives registered, {len(GATED_NATIVES)} natives gated; "
        "F4SE callback ABI, historical v5 active-scene schema and deferred-fixup guards matched"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
