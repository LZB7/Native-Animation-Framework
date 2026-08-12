#!/usr/bin/env python3
from phase_d_audit_common import audit

raise SystemExit(audit("LookAt Cam", {
    "f4se-plugin/src/Data/Settings.h": ('sLookAtCamTarget = "HEAD"',),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/SettingsMenuHandler.h": (
        '"LookAt Cam Target"', '"HEAD", "Chest", "COM"'),
    "f4se-plugin/src/CamHook/CamHook.h": (
        "lookAtBase->GetObjectByName", "lookAtNode->world.translate",
        "GetLookAtRotation(s->translation", "OriginalUpdate(s, a_nextState)",
        "s->translation.z += 1.0f", "s->translation.z -= 1.0f",
        "stateRotation.FromEulerAnglesZXY", "stateForward.Dot(desiredDirection)",
        "diagnosticCount <= 3", "cameraRootWorld=", "desired=", "stateForward=", "dot=", "distance="),
}, forbidden=(
    "camNode->local.rotate.FromEulerAnglesZXY(r.x, r.y, 0.0f);",
)))
