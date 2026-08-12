#!/usr/bin/env python3
from phase_d_audit_common import audit

raise SystemExit(audit("LookAt Cam", {
    "f4se-plugin/src/Data/Settings.h": ('sLookAtCamTarget = "HEAD"',),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/SettingsMenuHandler.h": (
        '"LookAt Cam Target"', '"HEAD", "Chest", "COM"'),
    "f4se-plugin/src/CamHook/CamHook.h": (
        "lookAtBase->GetObjectByName", "lookAtNode->world.translate",
        "GetLookAtRotation(camNode->local.translate", "OriginalUpdate(s, a_nextState)",
        "s->translation.z += 1.0f", "s->translation.z -= 1.0f",
        "diagnosticCount <= 3", "actorRoot=", "direction=", "distance="),
}))
