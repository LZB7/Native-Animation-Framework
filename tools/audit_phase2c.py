import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOADER = (ROOT / "f4se-plugin/src/Data/JSONAnimation.h").read_text(encoding="utf-8")
API = (ROOT / "f4se-plugin/src/API/ExtendedAPI.h").read_text(encoding="utf-8")
GLOBAL = (ROOT / "f4se-plugin/src/Data/Global.h").read_text(encoding="utf-8")
DOC = (ROOT / "docs/NAF_EXTENDED_JSON_SCHEMA_V1_EXPERIMENTAL.md").read_text(encoding="utf-8")


def require(text: str, token: str, owner: str) -> None:
    if token not in text:
        raise SystemExit(f"FAIL: {owner} missing {token}")


def validate_actor(actor: dict) -> bool:
    if not isinstance(actor.get("file"), str) or not actor["file"]:
        return False
    body = actor.get("expectedBodyType", "Any")
    if not isinstance(body, str) or body.lower() not in {"masc", "femme", "any"}:
        return False
    if "role" in actor and (not isinstance(actor["role"], str) or actor["role"].lower() not in {"giver", "receiver"}):
        return False
    if "erection" in actor and not isinstance(actor["erection"], bool):
        return False
    legacy = actor.get("requirements", {}).get("legacyGameSex", actor.get("gender", "Any"))
    return isinstance(legacy, str) and legacy.lower() in {"any", "male", "female"}


def main() -> None:
    fixture = json.loads((ROOT / "tests/fixtures/phase2c_actor_metadata.naf.json").read_text(encoding="utf-8"))
    actors = fixture["animation"]["actors"]
    assert all(validate_actor(actor) for actor in actors)
    assert validate_actor({"file": "x.hkx"})
    assert validate_actor({"file": "x.hkx", "expectedBodyType": "Any", "erection": False})
    assert validate_actor({"file": "x.hkx", "gender": "Male", "expectedBodyType": "Femme"})
    assert not validate_actor({"file": "x.hkx", "expectedBodyType": "unknown"})
    assert not validate_actor({"file": "x.hkx", "role": "performer"})
    assert not validate_actor({"file": "x.hkx", "erection": 1})

    for token in (
        "kCacheVersion = 1", "kSchemaVersion = 1", "kMaxCacheBytes", "kMaxDefinitions",
        "relative(path, USERDATA_DIR)", "file_size(path)", "last_write_time(path)", "HashFile(path)",
        "header.payloadSize", "header.checksum", "MoveFileExW", "MOVEFILE_REPLACE_EXISTING",
        'warm ? "NAFPackCache.bin" : "source files"', '"slot.{}."', '"naf.extended.actor"',
    ):
        require(LOADER, token, "JSON/cache implementation")
    require(GLOBAL, "JSONAnimation::LoadFiles(jsonFiles, verbose)", "global load path")
    for token in ("std::stable_partition", "queuedStarts", "last committed resolved metadata", '"1.11.240-extended"'):
        require(API, token, "API lifecycle")
    for token in ("omission supplies", "slot.N.erection", "ABI-v1", "NAFPackCache.bin"):
        require(DOC, token, "Phase 2C documentation")
    print("PASS: Phase 2C cache, actor metadata, ABI-v1 exposure, and deterministic lifecycle guards are present")


if __name__ == "__main__":
    main()
