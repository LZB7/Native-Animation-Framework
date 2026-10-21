#!/usr/bin/env python3
"""Audit NAF Extended Phase 3A facial-reset ownership and public access."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def require(text: str, token: str, owner: str) -> None:
    if token not in text:
        raise SystemExit(f"FAIL: {owner} is missing {token!r}")


def main() -> None:
    face = (ROOT / "f4se-plugin/src/FaceAnimation/FaceUpdateHook.h").read_text(encoding="utf-8")
    native = (ROOT / "f4se-plugin/src/Scripts/Papyrus/NAF.h").read_text(encoding="utf-8")
    bindings = (ROOT / "f4se-plugin/src/Scripts/Papyrus.h").read_text(encoding="utf-8")
    script = (ROOT / "ck/project_files/Scripts/Source/User/NAF.psc").read_text(encoding="utf-8")
    settings = (ROOT / "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/SettingsMenuHandler.h").read_text(encoding="utf-8")
    serialization = (ROOT / "f4se-plugin/src/Serialization/Serialization.h").read_text(encoding="utf-8")

    guards = {
        "face ownership": (
            "PendingRequestKind::kReset",
            "pending->second.generation != generation",
            "EraseManagedActor_NonThreadSafe(targetActor)",
            "eyeGeoCache.erase(targetActor)",
            "resetState->pendingResets.insert(targetActor)",
            "ApplyPendingResetsAfterSerialization",
            "consumePersistence = !persistAcrossLoad",
            "resetState->pendingResets.contains(handle)",
        ),
        "neutral composition": (
            "std::fill(std::begin(data->finalExp.exp), std::end(data->finalExp.exp), 0.0f)",
            "data->instanceData.lock",
            "GameUtil::SetEyeCoords(eyeGeo, 0.0f, 0.0f)",
        ),
        "legacy stop": (
            "void StopAnimation(RE::ActorHandle targetActor, bool animOverride = false)",
            "bool ResetFaceExpression(RE::ActorHandle targetActor, bool persistAcrossLoad = true)",
        ),
    }
    for owner, tokens in guards.items():
        for token in tokens:
            require(face, token, owner)

    require(native, "bool ResetFaceExpression(std::monostate, RE::Actor* a_actor)", "Papyrus implementation")
    require(bindings, "PAPYRUS_BIND(ResetFaceExpression);", "Papyrus binding")
    require(script, "Bool Function ResetFaceExpression(Actor akActor) Native Global", "Papyrus source")
    require(settings, '"Reset Player Expression"', "player menu action")
    require(serialization, "SAVE_PERSISTENT_STATE('FRST', 1", "face-reset persistence")
    require(serialization, "case 'FRST':", "face-reset restoration")
    require(serialization, "version >= 5 || (type == 'FRST' && version >= 1)", "face-reset record version gate")
    require(serialization, "FaceUpdateHook::ApplyPendingResetsAfterSerialization()", "world-ready reset fixup")

    if "std::fill(std::begin(data->baseExp.exp)" in face or "std::fill(std::begin(data->modExp.exp)" in face:
        raise SystemExit("FAIL: reset must not erase engine-owned baseExp/modExp")

    print("PASS: Phase 3A reset invalidates pending loads, removes persistent NAF ownership, "
          "neutralizes only finalExp under the FaceGen lock, preserves legacy stop semantics, "
          "persists a one-load reset marker, and is exposed through Papyrus and the NAF Settings menu")


if __name__ == "__main__":
    main()
