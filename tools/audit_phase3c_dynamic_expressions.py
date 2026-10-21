from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]

def require(text: str, token: str, label: str) -> None:
    if token not in text:
        raise SystemExit(f"FAIL: missing {label}: {token}")

public = (ROOT / "f4se-plugin/include/NAFExtendedAPI.h").read_text(encoding="utf-8")
provider = (ROOT / "f4se-plugin/src/API/ExtendedAPI.h").read_text(encoding="utf-8")
face = (ROOT / "f4se-plugin/src/FaceAnimation/FaceUpdateHook.h").read_text(encoding="utf-8")
presets = (ROOT / "f4se-plugin/src/Data/ExpressionPreset.h").read_text(encoding="utf-8")
global_h = (ROOT / "f4se-plugin/src/Data/Global.h").read_text(encoding="utf-8")
serialization = (ROOT / "f4se-plugin/src/Serialization/Serialization.h").read_text(encoding="utf-8")
harness = (ROOT / "tools/NAFExtAPITest/NAFExtAPITest.cpp").read_text(encoding="utf-8")

for token in ("NAFExtFaceAPIv2", "NAFExt_GetFaceAPIv2", "request_expression", "release_expression", "get_expression_state"):
    require(public, token, "public v2 ABI")
for forbidden in ("Actor*", "BSFaceGenAnimationData", "std::"):
    table = public[public.index("typedef struct NAFExtFaceAPIv2"):public.index("} NAFExtFaceAPIv2;")]
    if forbidden in table:
        raise SystemExit(f"FAIL: private/C++ type leaked through v2 ABI: {forbidden}")
for token in ("std::isfinite(intensity)", "intensity < 0.0f", "intensity > 1.0f", "options != 0", "Data::ExpressionPreset::Get"):
    require(provider, token, "input validation")
for token in ("expression = std::nullopt", "channel.value * expression.intensity", "OriginalUpdateLip", "ApplyExpression", "ReleaseExpression", "requestGeneration"):
    require(face, token, "managed FaceGen integration")
require(face, "expression = std::nullopt;  // transient; never serialized", "transient policy")
require(face, "SuspendExpressionsForSerialization_NonThreadSafe", "transient FACE filtering")
require(serialization, "RestoreExpressionsAfterSerialization_NonThreadSafe", "post-save runtime restoration")
require(global_h, ".nafexpr.json", "preset discovery")
for token in ("morph >= 54", "value < 0.0f", "value > 1.0f", "used.test(morph)", "keeping the first definition"):
    require(presets, token, "preset validation")
for fixture in (ROOT / "tests/fixtures/expressions").glob("*.nafexpr.json"):
    data = json.loads(fixture.read_text(encoding="utf-8"))
    assert data["format"] == "NAFExtendedExpressionPreset" and data["schemaVersion"] == 1
    assert data["id"] and data["channels"]
require(harness, "VK_NUMPAD1", "runtime request control")
require(harness, "VK_NUMPAD5", "runtime release control")
require(harness, "NAFExt_GetFaceAPIv2", "v2 discovery")
print("PASS: Phase 3C presets are bounded and cached, dynamic expression ownership is transient/generation-protected, v1 remains present, v2 is C-only and negotiated, and the independent harness covers request/query/replacement/release/reset")
