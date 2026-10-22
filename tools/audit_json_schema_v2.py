#!/usr/bin/env python3
"""Static and fixture audit for NAF Extended JSON animation-pack schema v2."""

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOADER = (ROOT / "f4se-plugin/src/Data/JSONAnimation.h").read_text(encoding="utf-8")
GLOBAL = (ROOT / "f4se-plugin/src/Data/Global.h").read_text(encoding="utf-8")
API = (ROOT / "f4se-plugin/src/API/ExtendedAPI.h").read_text(encoding="utf-8")
V2 = json.loads((ROOT / "tests/fixtures/json-v2/multi-dance-pack.naf.json").read_text(encoding="utf-8"))
ERRORS = json.loads((ROOT / "tests/fixtures/json-v2/error-isolation.naf.json").read_text(encoding="utf-8"))
V1 = json.loads((ROOT / "tests/fixtures/phase2_solo.naf.json").read_text(encoding="utf-8"))


def require(text: str, token: str, owner: str) -> None:
    if token not in text:
        raise SystemExit(f"FAIL: {owner} missing {token}")


def main() -> None:
    assert V1["format"] == "NAFExtendedAnimation" and V1["schemaVersion"] == 1
    assert V2["format"] == "NAFExtendedAnimationPack" and V2["schemaVersion"] == 2
    animations = {item["id"]: item for item in V2["animations"]}
    positions = {item["id"]: item for item in V2["positions"]}
    groups = {item["id"]: item for item in V2["animationGroups"]}
    trees = {item["id"]: item for item in V2["positionTrees"]}
    assert len(animations) == 3 and len(positions) == 5 and len(groups) == 1 and len(trees) == 1
    assert all(stage["animation"] in animations for stage in groups["jsonv2.dance-sequence"]["stages"])
    assert positions["jsonv2.staged.position"]["animationGroup"] in groups
    assert positions["jsonv2.tree.position"]["positionTree"] in trees
    assert V2["defaults"]["requirements"]["skeleton"] == "Human"
    assert V2["animations"][0]["actors"][0]["role"] == "solo"
    assert V2["animations"][2]["actors"][0]["role"] == "solo"
    assert "role" not in V2["animations"][1]["actors"][0]
    assert "erection" not in V2["animations"][0]["actors"][0]
    assert "erection" not in V2["animations"][1]["actors"][0]
    assert groups["jsonv2.dance-sequence"]["stages"][0]["animation"] == "jsonv2.belly"
    assert groups["jsonv2.dance-sequence"]["stages"][2]["animation"] == "jsonv2.macarena"
    assert trees["jsonv2.dance-tree"]["root"]["position"] == "jsonv2.belly.position"
    assert all(item["actors"][0]["file"].lower().endswith(".hkx") for item in V2["animations"])

    valid_roles = {"giver", "receiver", "solo"}
    valid_error_animations = [a for a in ERRORS["animations"] if a.get("id") and a.get("actors") and all(slot.get("file") and ("role" not in slot or slot["role"].lower() in valid_roles) for slot in a["actors"])]
    first_ids = []
    for animation in valid_error_animations:
        if animation["id"].lower() not in {value.lower() for value in first_ids}:
            first_ids.append(animation["id"])
    assert first_ids == ["jsonv2.valid-among-errors"]
    assert ERRORS["positions"][0]["animation"] in first_ids
    assert ERRORS["positions"][1]["animation"] not in first_ids

    for token in (
        'kCacheVersion = 2', 'kPackSchemaVersion = 2', '"NAFExtendedAnimationPack"',
        'NormalizeV1', 'NormalizeV2', 'RegisterDocuments', 'RegisterAnimationDefinition',
        'RegisterGroupDefinition', 'RegisterTreeDefinition', 'RegisterPositionDefinition',
        'PackageRecords', 'AnimationRecords', 'stageMetadata', 'treeNodeMetadata',
        'candidatePositions', 'first loaded definition', 'normalizedKind',
        'return "solo"',
        'MoveFileExW', 'nlohmann::json::to_cbor', 'nlohmann::json::from_cbor',
    ):
        require(LOADER, token, "schema-v2 loader/cache")
    for token in ('AnimationGroups', 'PositionTrees', 'JSONAnimation::Clear()', 'std::ranges::sort(jsonFiles)'):
        require(GLOBAL, token, "shared runtime/reload")
    for token in ('GetMetadata(result->positionID', 'result->stageIndex', 'metadataTreeNodeID'):
        require(API, token, "ABI-v1 scoped metadata snapshot")
    print("PASS: JSON schema v2 fixture provides 3 animations, 5 positions, a sequential 3-stage group, a recursive tree, shared defaults, scoped metadata, v1 dispatch, cache-v2 guards, and definition-level error isolation")


if __name__ == "__main__":
    main()
