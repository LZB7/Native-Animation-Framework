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
        "REL::RelocationID(0, 2270336)", "Main::WorldRootCamera",
        "PointRenderedCameraAt", "renderCamera->world.rotate.entry[0][0]",
        "renderForward.Dot(desiredDirection)", "diagnosticCount <= 3",
        "renderCamera=", "desired=", "stateForward=", "renderForward=", "renderDot=", "distance="),
}, forbidden=(
    "camNode->local.rotate.FromEulerAnglesZXY(r.x, r.y, 0.0f);",
)))
