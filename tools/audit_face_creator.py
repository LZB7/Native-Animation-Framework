#!/usr/bin/env python3
from phase_d_audit_common import audit

raise SystemExit(audit("Face Creator", {
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/FaceAnimCreatorHandler.h": (
        "SetMorph", "SetEyes", '"Emissive Mult:"', "TogglePreview", "ToggleHavokSync",
        "FaceUpdateHook::StartAnimation", "FaceUpdateHook::StopAnimation", "FaceUpdateHook::VisitAnimation"),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/CreatorMenuHandler.h": (
        '"[+] New Face Animation"', '"[->] Open in Face Animator"'),
    "f4se-plugin/src/FaceAnimation/AnimationData.h": ('append_child("faceAnim")', "ToRuntimeData"),
}))
