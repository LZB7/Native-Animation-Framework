#!/usr/bin/env python3
"""Focused static audit for the additive Phase 3B managed-face C service."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(text: str, token: str, owner: str) -> None:
    if token not in text:
        raise SystemExit(f"FAIL: {owner} is missing {token!r}")


def main() -> None:
    public = (ROOT / "f4se-plugin/include/NAFExtendedAPI.h").read_text(encoding="utf-8")
    provider = (ROOT / "f4se-plugin/src/API/ExtendedAPI.h").read_text(encoding="utf-8")
    face = (ROOT / "f4se-plugin/src/FaceAnimation/FaceUpdateHook.h").read_text(encoding="utf-8")
    harness = (ROOT / "tools/NAFExtAPITest/NAFExtAPITest.cpp").read_text(encoding="utf-8")
    papyrus = (ROOT / "ck/project_files/Scripts/Source/User/NAF.psc").read_text(encoding="utf-8")

    for token in (
        "#define NAFEXT_FACE_API_VERSION_1 1u",
        "typedef struct NAFExtFaceAPIv1",
        "NAFExt_GetFaceAPI(uint32_t requested_version, uint32_t caller_table_size)",
        "NAFEXT_CAP_FACE_PLAYBACK",
        "NAFEXT_CAP_FACE_STOP",
        "NAFEXT_CAP_FACE_RESET",
        "NAFEXT_CAP_FACE_QUERY",
        "uint32_t actor_form_id",
    ):
        require(public, token, "public C header")

    for forbidden in ("std::", "RE::", "FaceAnimation::", "Actor*", "BSFaceGenAnimationData"):
        face_table = public[public.index("typedef struct NAFExtFaceAPIv1"):public.index("} NAFExtFaceAPIv1;")]
        if forbidden in face_table:
            raise SystemExit(f"FAIL: private/C++ ABI type {forbidden!r} crosses the facial table")

    for token in (
        "inline const NAFExtAPIv1 api{",
        "inline const NAFExtFaceAPIv1 faceAPI{",
        "F4SE::GetTaskInterface()->AddTask",
        "ResolveActor(actorFormID)",
        "FaceUpdateHook::LoadAndPlayAnimation",
        "FaceUpdateHook::StopAnimation",
        "FaceUpdateHook::ResetFaceExpression",
        "GetRequestState(actorFormID)",
    ):
        require(provider, token, "provider")

    for token in (
        "pending->second.generation == generation",
        "SetRequestStateIfCurrent(actorFormID, generation",
        "pendingResets.erase(targetActor)",
        "requestStates.clear()",
        "BindManagedData_NonThreadSafe(handle, faceData)",
    ):
        require(face, token, "managed-face lifecycle")

    for token in (
        "GetProcAddress(module, \"NAFExt_GetFaceAPI\")",
        "Facial negotiation tests:",
        "F6=play A",
        "PlayFace(faceAnimationA); PlayFace(faceAnimationB)",
        "GetAsyncKeyState(VK_INSERT)",
    ):
        require(harness, token, "independent harness")

    natives = [line for line in papyrus.splitlines() if " Native Global" in line and " Function " in f" {line}"]
    if len(natives) != 46:
        raise SystemExit(f"FAIL: Papyrus surface changed unexpectedly ({len(natives)} natives, expected 45+1)")

    if provider.count("extern \"C\" __declspec(dllexport)") < 2:
        raise SystemExit("FAIL: additive face export is missing")

    print("PASS: Phase 3B facial service is additive, version/size/capability negotiated, C-only, "
          "task-dispatched, actor-FormID based, query synchronized, and generation protected; scene ABI and 45+1 Papyrus remain intact")


if __name__ == "__main__":
    main()

