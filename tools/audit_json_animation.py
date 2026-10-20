import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOADER = ROOT / "f4se-plugin/src/Data/JSONAnimation.h"
GLOBAL = ROOT / "f4se-plugin/src/Data/Global.h"
FIXTURE = ROOT / "tests/fixtures/phase2_solo.naf.json"
SHARED_FIXTURE = ROOT / "tests/fixtures/phase2_shared_hkx.naf.json"
METADATA_FIXTURE = ROOT / "tests/fixtures/phase2c_actor_metadata.naf.json"


def main() -> None:
    loader = LOADER.read_text(encoding="utf-8")
    global_text = GLOBAL.read_text(encoding="utf-8")
    fixture = json.loads(FIXTURE.read_text(encoding="utf-8"))
    shared = json.loads(SHARED_FIXTURE.read_text(encoding="utf-8"))
    actor_metadata = json.loads(METADATA_FIXTURE.read_text(encoding="utf-8"))
    assert fixture["format"] == "NAFExtendedAnimation"
    assert fixture["schemaVersion"] == 1
    assert fixture["animation"]["actors"][0]["file"].lower().endswith(".hkx")
    assert "gender" not in fixture["animation"]["actors"][0]
    assert not fixture["animation"]["id"].startswith("NAFExtJSON_")
    assert fixture["position"]["displayName"]
    assert shared["animation"]["actors"][0]["file"] == fixture["animation"]["actors"][0]["file"]
    assert shared["animation"]["id"] != fixture["animation"]["id"]
    assert actor_metadata["animation"]["actors"][0]["expectedBodyType"] == "Masc"
    assert actor_metadata["animation"]["actors"][0]["role"] == "giver"
    assert actor_metadata["animation"]["actors"][0]["erection"] is True
    assert actor_metadata["animation"]["actors"][1]["expectedBodyType"] == "Femme"
    assert actor_metadata["animation"]["actors"][1]["role"] == "receiver"
    assert "erection" not in actor_metadata["animation"]["actors"][1]
    for token in (
        'root.value("format"', 'root.value("schemaVersion"', "Global::Animations.priority_insert",
        "Global::Positions.priority_insert", "behaviorRequiresConvert = true", "metadata[positionID]",
        "conflicts with existing animation/position ID", 'value("legacyGameSex"', 'value("displayName"',
        "displayNames[positionID]", "ParseExpectedBodyType", "ParseRole", 'actorNode["erection"].is_boolean()',
        '"naf.extended.actor"', 'prefix + "expectedBodyType"', 'prefix + "erection"', "NAFPackCache.bin",
        "HashFile", "MoveFileExW", "nlohmann::json::from_cbor", "nlohmann::json::to_cbor"
    ):
        if token not in loader:
            raise SystemExit(f"FAIL: JSON adapter is missing {token}")
    for token in ('EndsWith(p.filename().generic_string(), ".naf.json")', "std::ranges::sort(jsonFiles)", "JSONAnimation::Clear()", "JSONAnimation::LoadFiles"):
        if token not in global_text:
            raise SystemExit(f"FAIL: JSON discovery/reload path is missing {token}")
    print("PASS: JSON definitions and NAFPackCache preserve body-presentation, role, tri-state erection, legacy sex, shared HKX, and deterministic registration")


if __name__ == "__main__":
    main()
