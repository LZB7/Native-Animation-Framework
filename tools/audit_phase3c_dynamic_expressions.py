from pathlib import Path
import json
import re

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
face_creator = (ROOT / "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/FaceAnimCreatorHandler.h").read_text(encoding="utf-8")

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
for token in ("kMorphNames", "ResolveMorph", "is_number_unsigned", "is_string", "value < 0.0f", "value > 1.0f", "used.test(*morph)", "keeping the first definition"):
    require(presets, token, "preset validation")

mapping_match = re.search(r"kMorphNames\s*\{(.*?)\n\s*\};", presets, re.S)
if not mapping_match:
    raise SystemExit("FAIL: unable to extract canonical FaceGen morph table")
morph_names = re.findall(r'"([^"]+)"', mapping_match.group(1))
if len(morph_names) != 54 or len(set(morph_names)) != 54:
    raise SystemExit("FAIL: canonical FaceGen morph table must contain 54 unique names")
assert morph_names[0] == "Brow Squeeze"
assert morph_names[17] == "Left Smile"
assert morph_names[40] == "Right Smile"
assert morph_names[49] == "Tongue"
assert morph_names[50:54] == ["Extra 1", "Extra 2", "Extra 3", "Extra 4"]

creator_entries = re.findall(r'\{\s*"([^"]+):",\s*MORPH_BIND\((\d+)\)\s*\}', face_creator)
creator_indices = [int(index) for _, index in creator_entries]
if len(creator_indices) != 54 or sorted(creator_indices) != list(range(54)):
    raise SystemExit("FAIL: Face Creator must expose every expression morph index 0-53 exactly once")
for label, index in (("Squeeze", 0), ("Left Smile", 17), ("Right Smile", 40), ("Tongue", 49), ("Extra 4", 53)):
    if (label, str(index)) not in creator_entries:
        raise SystemExit(f"FAIL: Face Creator mapping changed for {label} / {index}")

name_to_index = {name: index for index, name in enumerate(morph_names)}

def normalize_identifier(value):
    # Mirrors the C++ contract: bool is not an integer JSON morph identifier.
    if isinstance(value, int) and not isinstance(value, bool):
        if 0 <= value < 54:
            return value
        raise ValueError("invalid index")
    if isinstance(value, str) and value in name_to_index:
        return name_to_index[value]
    raise ValueError("unknown name or invalid type")

def normalize_channels(channels):
    result = []
    used = set()
    for item in channels:
        morph = normalize_identifier(item["morph"])
        value = item["value"]
        if not isinstance(value, (int, float)) or isinstance(value, bool) or not 0.0 <= value <= 1.0:
            raise ValueError("invalid value")
        if morph in used:
            raise ValueError("duplicate")
        used.add(morph)
        result.append((morph, float(value)))
    return result

assert normalize_identifier(17) == normalize_identifier("Left Smile") == 17
assert normalize_identifier(40) == normalize_identifier("Right Smile") == 40
numeric_smile = normalize_channels([{"morph": 17, "value": 0.55}, {"morph": 40, "value": 0.55}])
named_smile = normalize_channels([{"morph": "Left Smile", "value": 0.55}, {"morph": "Right Smile", "value": 0.55}])
assert numeric_smile == named_smile
assert normalize_channels([{"morph": 17, "value": 0.55}, {"morph": "Right Smile", "value": 0.55}]) == numeric_smile

invalid_channel_sets = [
    [{"morph": 17, "value": 0.5}, {"morph": 17, "value": 0.6}],
    [{"morph": "Left Smile", "value": 0.5}, {"morph": "Left Smile", "value": 0.6}],
    [{"morph": 17, "value": 0.5}, {"morph": "Left Smile", "value": 0.6}],
]
for channels in invalid_channel_sets:
    try:
        normalize_channels(channels)
    except ValueError:
        pass
    else:
        raise SystemExit("FAIL: duplicate channel case was accepted")

for invalid in ("Left Smille", "", "17", None, True, 17.0, -1, 54, 100, 254, 255):
    try:
        normalize_identifier(invalid)
    except ValueError:
        pass
    else:
        raise SystemExit(f"FAIL: invalid morph identifier was accepted: {invalid!r}")

fixtures = {}
for fixture in (ROOT / "tests/fixtures/expressions").glob("*.nafexpr.json"):
    data = json.loads(fixture.read_text(encoding="utf-8"))
    assert data["format"] == "NAFExtendedExpressionPreset" and data["schemaVersion"] == 1
    assert data["id"] and data["channels"]
    fixtures[data["id"]] = (data["displayName"], normalize_channels(data["channels"]))
assert fixtures["dev.smile"][1] == fixtures["dev.smile.named"][1]
assert fixtures["dev.smile"][0] == "Development Smile"
assert fixtures["dev.smile.named"][0] == "Development Smile (Named Channels)"
require(harness, "VK_NUMPAD1", "runtime request control")
require(harness, "VK_NUMPAD5", "runtime release control")
require(harness, "NAFExt_GetFaceAPIv2", "v2 discovery")
print("PASS: Phase 3C presets accept and normalize all 54 exact canonical FaceGen names alongside numeric indices, reject invalid/duplicate identifiers, remain bounded and cached, and preserve the transient v2 expression lifecycle")
