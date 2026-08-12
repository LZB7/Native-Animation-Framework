#!/usr/bin/env python3
"""Audit the historical-v5 active-scene persistence and deferred fixup surface."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
GUARDS = {
    "f4se-plugin/src/Serialization/Serialization.h": (
        "SAVE_PERSISTENT_STATE('TASK', 5",
        "SAVE_PERSISTENT_STATE('SCNE', 5",
        "SAVE_PERSISTENT_STATE('EQPT', 5",
        "SAVE_PERSISTENT_STATE('UID', 5",
        "SAVE_PERSISTENT_STATE('FACE', 5",
        "SAVE_PERSISTENT_STATE('HUD', 5",
        "SAVE_PERSISTENT_STATE('PACK', 5",
        "SAVE_PERSISTENT_STATE('SHUD', 5",
        "RunRestoredSceneFixup",
        "iter->second->noUpdate = true",
        "FixupRestoredScenes",
		"FaceUpdateHook::FixupAfterSerialization",
		"FixupRestoredScenes();\n\t\tBodyAnimation::GraphHook::FinishSerializationFixup();\n\t\tTasks::TimerThread::GetSingleton()->Start();",
    ),
    "f4se-plugin/src/Scene/SceneBase.h": (
        "ResumeAfterSerialization() override",
        "PlayAnimations();",
        "noUpdate = false",
        "GetInputsToDisableForPlayerFreeCameraScene()",
		"controlSystem->OnSerializationLoad(this)",
    ),
	"f4se-plugin/src/Scene/ControlSystems.h": (
		"OnSerializationLoad(IControllable* scn) override",
		"parent = scn",
		"UpdateHUDState();",
	),
    "f4se-plugin/src/Scene/SceneManager.h": (
        "FixupRestoredScenes",
        "could not be safely restored; cancelling with production cleanup",
        "StopScene(scene->uid, true)",
    ),
	"f4se-plugin/src/FaceAnimation/FaceUpdateHook.h": (
		"FixupAfterSerialization",
		"state->managedDatas.clear()",
	),
}


def main() -> int:
    failures = []
    for relative, selectors in GUARDS.items():
        source = (ROOT / relative).read_text(errors="replace")
        for selector in selectors:
            if selector not in source:
                failures.append(f"{relative}: missing {selector}")
    serialization = (ROOT / "f4se-plugin/src/Serialization/Serialization.h").read_text(errors="replace")
    if "SAVE_PERSISTENT_STATE('BODY', 5" not in serialization:
        failures.append("BODY/NANIM historical-v5 state is not active")
    if failures:
        print("\n".join(f"FAIL: {failure}" for failure in failures))
        return 1
    print("PASS: nine historical-v5 records, deferred world-ready scene/BODY fixup, and fail-safe cancellation matched")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
