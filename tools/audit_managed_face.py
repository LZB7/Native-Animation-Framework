#!/usr/bin/env python3
"""Audit the complete managed-face production surface for Fallout 4 1.11.221."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "BSFaceGenNiNode::UpdateMorphing": (2209487, 0x6E8BF0),
    "BSFaceGenAnimationData::Update": (2209139, 0x6D1310),
    "BSFaceGenAnimationData::UpdateMorphsFromLip": (2209124, 0x6D0560),
}

GUARDS = {
    "f4se-plugin/src/RE/BSFaceGenAnimationData.h": (
        "offsetof(BSFaceGenAnimationData, finalExp) == 0x18",
        "offsetof(BSFaceGenAnimationData, instanceData) == 0x2A0",
        "sizeof(BSFaceGenAnimationData) == 0x2E8",
    ),
    "f4se-plugin/src/FaceAnimation/FaceUpdateHook.h": (
        "BindManagedData_NonThreadSafe",
        "BodyAnimation::GraphHook::RegisterSet3DObserver(&OnSet3D)",
        "eyeGeoCache.erase(handle)",
        "EyeVector{ u * -0.25, v * 0.2 }",
        "LoadAndPlayAnimation",
        "StopAnimation",
        "SmartIdle::GetGraphTime",
        "FixupAfterSerialization",
        "state = std::make_unique<PersistentState>()",
    ),
    "f4se-plugin/src/FaceAnimation/Animation.h": (
        "timeElapsed += timeDelta",
        "animData->finalExp.exp[tl.morph]",
        "GameUtil::SetEyeCoords",
    ),
    "f4se-plugin/src/Data/User/FaceAnim.h": (
        '"frames"',
        '"key"',
        '"morph"',
        '"eyes"',
        "cereal::BinaryOutputArchive",
    ),
    "f4se-plugin/src/FaceAnimation/AnimationData.h": (
        'append_child("faceAnim")',
        "ConvertEyeCoordRange",
        "ToRuntimeData",
    ),
    "f4se-plugin/src/Serialization/Serialization.h": (
        "SAVE_PERSISTENT_STATE('FACE', 5",
        "case 'FACE':",
        "FaceAnimation::FaceUpdateHook::FixupAfterSerialization()",
        "FaceAnimation::FaceUpdateHook::Reset()",
    ),
    "f4se-plugin/src/Scripts/Papyrus.h": (
        "PAPYRUS_BIND(SetEyeCoordOverride);",
        "PAPYRUS_BIND(ClearEyeCoordOverride);",
        "PAPYRUS_BIND(PlayFaceAnimation);",
        "PAPYRUS_BIND(StopFaceAnimation);",
        "Registered all 45 production Papyrus natives",
    ),
    "ck/project_files/Scripts/Source/User/NAF.psc": (
        "Function SetEyeCoordOverride(Actor akActor, Float fEyeX, Float fEyeY) Native Global",
        "Function ClearEyeCoordOverride(Actor akActor) Native Global",
        "Bool Function PlayFaceAnimation(Actor akActor, String akAnimationId, Bool bLoop = False, Bool bBodySync = False) Native Global",
        "Function StopFaceAnimation(Actor akActor) Native Global",
    ),
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    data = args.database.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    database = dict(struct.iter_unpack("<QQ", memoryview(data)[8:8 + count * 16]))
    failures: list[str] = []

    for owner, (ident, expected_rva) in EXPECTED.items():
        actual = database.get(ident)
        if actual != expected_rva:
            failures.append(f"{owner}: ID {ident}, expected 0x{expected_rva:X}, got {actual!r}")
    for relative, selectors in GUARDS.items():
        source = (args.source_root / relative).read_text(encoding="utf-8", errors="replace")
        for selector in selectors:
            if selector not in source:
                failures.append(f"{relative}: missing {selector!r}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(
        f"PASS: {len(EXPECTED)} exact face records and {sum(map(len, GUARDS.values()))} "
        "managed-face ABI, lifecycle, parser, persistence, Set3D, and Papyrus guards matched"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
