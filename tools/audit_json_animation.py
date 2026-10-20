import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOADER = ROOT / "f4se-plugin/src/Data/JSONAnimation.h"
GLOBAL = ROOT / "f4se-plugin/src/Data/Global.h"
FIXTURE = ROOT / "tests/fixtures/phase2_solo.naf.json"


def main() -> None:
    loader = LOADER.read_text(encoding="utf-8")
    global_text = GLOBAL.read_text(encoding="utf-8")
    fixture = json.loads(FIXTURE.read_text(encoding="utf-8"))
    assert fixture["format"] == "NAFExtendedAnimation"
    assert fixture["schemaVersion"] == 1
    assert fixture["animation"]["actors"][0]["file"].lower().endswith(".hkx")
    for token in (
        'root.value("format"', 'root.value("schemaVersion"', "Global::Animations.priority_insert",
        "Global::Positions.priority_insert", "behaviorRequiresConvert = true", "metadata[positionID]",
        "conflicts with existing animation/position ID"
    ):
        if token not in loader:
            raise SystemExit(f"FAIL: JSON adapter is missing {token}")
    for token in ('EndsWith(p.filename().generic_string(), ".naf.json")', "std::ranges::sort(jsonFiles)", "JSONAnimation::Clear()"):
        if token not in global_text:
            raise SystemExit(f"FAIL: JSON discovery/reload path is missing {token}")
    print("PASS: experimental .naf.json definitions validate and feed the existing Animation/Position registries deterministically")


if __name__ == "__main__":
    main()
