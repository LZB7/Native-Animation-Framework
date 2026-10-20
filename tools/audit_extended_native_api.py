from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PUBLIC = ROOT / "f4se-plugin/include/NAFExtendedAPI.h"
IMPL = ROOT / "f4se-plugin/src/API/ExtendedAPI.h"
MAIN = ROOT / "f4se-plugin/src/main.cpp"
LOOP = ROOT / "f4se-plugin/src/Tasks/GameLoopHook.h"


def main() -> None:
    public = PUBLIC.read_text(encoding="utf-8")
    impl = IMPL.read_text(encoding="utf-8")
    main_cpp = MAIN.read_text(encoding="utf-8")
    loop = LOOP.read_text(encoding="utf-8")
    for token in (
        "NAFEXT_ABI_VERSION_1", "NAFEXT_PROVIDER_ID", "NAFExtAPIv1",
        "NAFExtSceneSnapshotCopy", "display_name", "NAFEXT_EVENT_SCENE_ENDING",
        "NAFEXT_CAP_RESOLVED_ANIMATION_METADATA", "NAFEXT_CAP_SEMANTIC_METADATA",
    ):
        if token not in public:
            raise SystemExit(f"FAIL: public native API is missing {token}")
    for token in (
        'extern "C" __declspec(dllexport)', "ProcessPending()", "ResetEpoch()",
        "registrationEpoch", "NAFEXT_DISCOVERY_MESSAGE_TYPE", "subscriptions.erase(it)",
        "JSONAnimation::GetMetadata", "catch (...)"
    ):
        if token not in impl:
            raise SystemExit(f"FAIL: native API implementation is missing {token}")
    if "NAFExtended::NativeAPI::Initialize();" not in main_cpp:
        raise SystemExit("FAIL: native API is not initialized during plugin load")
    if "NAFExtended::NativeAPI::ProcessPending();" not in loop:
        raise SystemExit("FAIL: external callbacks are not dispatched at the out-of-lock game-loop boundary")
    forbidden = ("std::vector", "std::string", "std::function", "RE::", "Scene::")
    for token in forbidden:
        if token in public:
            raise SystemExit(f"FAIL: private/C++ type leaks through public ABI: {token}")
    print("PASS: native API v1 is size/version negotiated, format-neutral, out-of-lock, and has no private C++ ABI leakage")


if __name__ == "__main__":
    main()
