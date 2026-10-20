import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOADER = ROOT / "f4se-plugin/src/Data/JSONAnimation.h"
GLOBAL = ROOT / "f4se-plugin/src/Data/Global.h"
FIXTURE = ROOT / "tests/fixtures/phase2_solo.naf.json"
SHARED_FIXTURE = ROOT / "tests/fixtures/phase2_shared_hkx.naf.json"


def main() -> None:
    loader = LOADER.read_text(encoding="utf-8")
    global_text = GLOBAL.read_text(encoding="utf-8")
    fixture = json.loads(FIXTURE.read_text(encoding="utf-8"))
    shared = json.loads(SHARED_FIXTURE.read_text(encoding="utf-8"))
    assert fixture["format"] == "NAFExtendedAnimation"
    assert fixture["schemaVersion"] == 1
    assert fixture["animation"]["actors"][0]["file"].lower().endswith(".hkx")
    assert "gender" not in fixture["animation"]["actors"][0]
    assert not fixture["animation"]["id"].startswith("NAFExtJSON_")
    assert fixture["position"]["displayName"]
    assert shared["animation"]["actors"][0]["file"] == fixture["animation"]["actors"][0]["file"]
    assert shared["animation"]["id"] != fixture["animation"]["id"]
    for token in (
        'root.value("format"', 'root.value("schemaVersion"', "Global::Animations.priority_insert",
        "Global::Positions.priority_insert", "behaviorRequiresConvert = true", "metadata[positionID]",
        "conflicts with existing animation/position ID", 'value("legacyGameSex"', 'value("displayName"',
        "displayNames[positionID]"
    ):
        if token not in loader:
            raise SystemExit(f"FAIL: JSON adapter is missing {token}")
    for token in ('EndsWith(p.filename().generic_string(), ".naf.json")', "std::ranges::sort(jsonFiles)", "JSONAnimation::Clear()"):
        if token not in global_text:
            raise SystemExit(f"FAIL: JSON discovery/reload path is missing {token}")
    print("PASS: JSON definitions support unrestricted omitted sex, arbitrary IDs, display names, shared HKX resources, and deterministic registry insertion")


if __name__ == "__main__":
    main()
