#!/usr/bin/env python3
"""Bounded source audit for the enabled Papyrus/serialization lifecycle."""

from __future__ import annotations

import argparse
from pathlib import Path


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
}

GATED_NATIVES = {
    "ToggleMenu",
    "IsActorUsable",
    "SetActorUsable",
    "PlayDynamicIdle",
    "StartScene",
    "WalkToAndStartScene",
    "StopScene",
    "SetScenePosition",
    "GetSceneActors",
    "GetSceneHKXs",
    "GetSceneTags",
    "SetSceneSpeed",
    "GetSceneSpeed",
    "GetSceneFromActor",
    "IsSceneRunning",
    "GetSceneProperty",
    "ValidateSceneParams",
    "ReEnablePlayerInput",
    "PlayNANIM",
    "StopNANIM",
    "SetIKChainTarget",
    "SetIKChainEnabled",
    "SetEyeCoordOverride",
    "ClearEyeCoordOverride",
    "PlayFaceAnimation",
    "StopFaceAnimation",
    "GetLocalTransform",
    "SynchronizeAnimations",
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    failures: list[str] = []

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
        f"PASS: {len(SAFE_NATIVES)} safe natives registered, {len(GATED_NATIVES)} natives gated; "
        "F4SE callback ABI and historical v5/deferred-record guards matched"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
