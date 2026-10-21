#!/usr/bin/env python3
"""Audit the production Face Creator/preview path on Fallout 4 1.11.240."""

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "alternate head-part map anchor": (2662368, 0x2F09278),
    "PlayerCharacter singleton": (2698073, 0x32DD370),
    "BSFaceGenNiNode::UpdateMorphing": (2209487, 0x6E8F00),
    "BSFaceGenAnimationData::Update": (2209139, 0x6D1630),
    "BSFaceGenAnimationData::UpdateMorphsFromLip": (2209124, 0x6D0880),
    "Console pick-ref array": (2701391, 0x3312800),
    "Console selected pick ref": (2701395, 0x3312820),
}

GUARDS = {
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESBoundAnimObjects.h": (
        "REL::ID(1306546, 2662368)",
        "GetAlternateHeadPartListMap",
        "GetHeadParts(bool a_alternate = true)",
    ),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/FaceAnimCreatorHandler.h": (
        "SetMorph", "SetEyes", '"Emissive Mult:"', "TogglePreview", "ToggleHavokSync",
        "FaceUpdateHook::StartAnimation", "FaceUpdateHook::StopAnimation",
        "FaceUpdateHook::VisitAnimation", "Console::GetPickRef", "pickedHandle.get().get()",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/IMenu.h": (
        "REL::ID(170742, 2701395)",
        "REL::ID(875116, 2701391)",
    ),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/CreatorMenuHandler.h": (
        '"[+] New Face Animation"', '"[->] Open in Face Animator"',
    ),
    "f4se-plugin/src/Misc/GameUtil.h": (
        "npc->GetHeadParts()", "GetObjectByName(partName)", "SetEyeCoords(RE::BSGeometry*",
    ),
    "f4se-plugin/src/FaceAnimation/AnimationData.h": (
        'append_child("faceAnim")', "ToRuntimeData",
    ),
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    data = args.database.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    records = dict(struct.iter_unpack("<QQ", memoryview(data)[8:8 + count * 16]))
    failures: list[str] = []
    for owner, (ident, expected_rva) in EXPECTED.items():
        if records.get(ident) != expected_rva:
            failures.append(f"{owner}: ID {ident}, expected 0x{expected_rva:X}, got {records.get(ident)!r}")
    combined = ""
    for relative, tokens in GUARDS.items():
        source = (args.source_root / relative).read_text(encoding="utf-8", errors="replace")
        combined += source
        for token in tokens:
            if token not in source:
                failures.append(f"{relative}: missing {token!r}")
    if "REL::ID(1306546)" in combined:
        failures.append("Face Creator path retains invariant OG alternate-head-part ID 1306546")
    if "REL::ID(170742)" in combined or "REL::ID(875116)" in combined:
        failures.append("Face Creator path retains invariant OG console pick-ref globals")
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(
        f"PASS: {len(EXPECTED)} exact Face Creator/managed-face AE records and "
        f"{sum(map(len, GUARDS.values()))} ABI/lifecycle guards matched"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
