#!/usr/bin/env python3
"""Audit the AE hit-event to Papyrus-object cleanup path."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    "BSScript::Object::DecRef": (2314437, 0x20E23C0),
    "BSScript::Object::GetHandle": (2314431, None),
    "BSScript::Object::IncRef": (2314436, None),
    "TESHitEvent::GetEventSource": (2201886, 0x533C80),
}


def read_database(path: Path) -> dict[int, int]:
    data = path.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    if len(data) != 8 + count * 16:
        raise ValueError(f"invalid Address Library size: {path}")
    return dict(struct.iter_unpack("<QQ", memoryview(data)[8:]))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    args = parser.parse_args()
    database = read_database(args.database)
    failures: list[str] = []

    if 541793 in database:
        failures.append("legacy BSScript::Object::DecRef ID 541793 unexpectedly exists in AE database")
    for owner, (reloc_id, expected_rva) in EXPECTED.items():
        actual = database.get(reloc_id)
        if actual is None:
            failures.append(f"{owner}: exact AE ID {reloc_id} is missing")
        elif expected_rva is not None and actual != expected_rva:
            failures.append(f"{owner}: ID {reloc_id} expected RVA 0x{expected_rva:X}, got 0x{actual:X}")

    object_header = (args.source_root / "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/BSScript/Object.h").read_text(errors="replace")
    events_header = (args.source_root / "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/Events.h").read_text(errors="replace")
    event_proxy = (args.source_root / "f4se-plugin/src/Scene/EventProxy.h").read_text(errors="replace")
    scene = (args.source_root / "f4se-plugin/src/Scene/SceneBase.h").read_text(errors="replace")

    guards = (
        (object_header, "REL::RelocationID(541793, 2314437)", "AE DecRef selector"),
        (object_header, "REL::RelocationID(1452752, 2314431)", "AE GetHandle selector"),
        (object_header, "REL::RelocationID(461710, 2314436)", "AE IncRef selector"),
        (events_header, "static_assert(sizeof(DamageImpactData) == 0x40)", "DamageImpactData layout"),
        (events_header, "static_assert(sizeof(HitData) == 0xE0)", "HitData layout"),
        (events_header, "static_assert(sizeof(TESHitEvent) == 0x110)", "TESHitEvent layout"),
        (events_header, "NiPointer<TESObjectREFR> target;", "maintained TESHitEvent target"),
        (event_proxy, "VisitAllScenesWithRef(a_event.target.get()", "hit target resolution"),
        (event_proxy, "return RE::BSEventNotifyControl::kContinue;", "non-consuming event sink"),
        (scene, "F4SE::GetTaskInterface()->AddTask", "game-thread stop dispatch"),
    )
    for text, token, owner in guards:
        if token not in text:
            failures.append(f"{owner}: missing source guard {token}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(
        "PASS: hit-event layouts/target, non-consuming sink behavior, game-thread stop dispatch, "
        "and exact AE BSScript object refcount helpers are guarded"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
