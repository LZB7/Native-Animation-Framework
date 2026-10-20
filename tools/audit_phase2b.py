from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(text: str, token: str, owner: str) -> None:
    if token not in text:
        raise SystemExit(f"FAIL: {owner} is missing {token}")


def main() -> None:
    harness = (ROOT / "tools/NAFExtAPITest/NAFExtAPITest.cpp").read_text(encoding="utf-8")
    cmake = (ROOT / "f4se-plugin/CMakeLists.txt").read_text(encoding="utf-8")
    cache = (ROOT / "f4se-plugin/src/Data/Cache/XMLCache.h").read_text(encoding="utf-8")
    global_h = (ROOT / "f4se-plugin/src/Data/Global.h").read_text(encoding="utf-8")
    new_scene = (ROOT / "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/NewSceneHandler.h").read_text(encoding="utf-8")

    for token in (
        'GetProcAddress(module, "NAFExt_GetAPI")', "NAFEXT_DISCOVERY_MESSAGE_TYPE",
        "subscribe_scene_events", "Self-unregistration succeeded", "ReconcileLateScenes()",
        "ValidatePacked", "NAFExtAPITest_Shutdown", "Controlled negotiation tests",
        "CreateFileW", "WriteFile", "API acquisition deferred until PostLoad",
    ):
        require(harness, token, "API harness")
    load_body = harness.split("F4SEPlugin_Load", 1)[1].split("NAFExtAPITest_Shutdown", 1)[0]
    if "TryDirectAcquisition();" in load_body:
        raise SystemExit("FAIL: harness performs direct provider acquisition inside F4SEPlugin_Load")
    if "std::ofstream" in harness or "std::filesystem" in harness:
        raise SystemExit("FAIL: harness restored loader-phase C++ stream/filesystem logging")
    require(cmake, "add_library(NAFExtAPITest SHARED", "harness target")
    if "CommonLibF4::CommonLibF4" in cmake.split("add_library(NAFExtAPITest SHARED", 1)[1]:
        raise SystemExit("FAIL: independent harness still links CommonLibF4")
    require(cmake, "target_link_libraries(NAFExtAPITest PRIVATE Shell32)", "raw-F4SE harness linkage")
    require(cache, "currentFiles.size() != primaryCache.files.size()", "cache deletion/addition invalidation")
    require(cache, "currentFiles.contains", "cache exact manifest validation")
    require(global_h, "JSONAnimation::Clear()", "JSON reload invalidation")
    require(global_h, "std::ranges::sort(jsonFiles)", "deterministic JSON order")
    require(new_scene, "JSONAnimation::GetDisplayName", "scene selector display name")
    print("PASS: Phase 2B harness, cache manifest invalidation, deterministic JSON reload, and display-name UI path are present")


if __name__ == "__main__":
    main()
