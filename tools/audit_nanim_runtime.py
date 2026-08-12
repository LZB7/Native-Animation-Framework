#!/usr/bin/env python3
"""Audit the production NANIM runtime and historical-v5 BODY restoration path."""

import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    2214534: 0x818730, 2214536: 0x818970, 2201055: 0x50C5B0,
    2258328: 0x13A9B10, 2258329: 0x13A9B90,
    2260451: 0x147C170, 2260454: 0x147C5A0,
}
GUARDS = {
    "f4se-plugin/src/BodyAnimation/GraphHook.h": (
        "pendingRestores", "FixupAfterSerialization", "FinishSerializationFixup",
        "restoredAnimations", "generator.localTime = info.animTime",
        "TryRestorePendingForReference(a_ref)",
    ),
    "f4se-plugin/src/BodyAnimation/NodeAnimationGraph.h": (
        "PushOutput(generator.output)", "a_output[i].ToComplex(nodes[i]->local)",
        "ikManager.Update(a_output)", "kUnloaded3D",
    ),
    "f4se-plugin/src/Serialization/Serialization.h": (
        "SAVE_PERSISTENT_STATE('BODY', 5", "case 'BODY':",
        "BodyAnimation::GraphHook::FixupAfterSerialization();",
        "BodyAnimation::GraphHook::FinishSerializationFixup();",
        "BodyAnimation::GraphHook::Reset();",
    ),
    "f4se-plugin/src/Scripts/Papyrus.h": (
        "PAPYRUS_BIND(PlayNANIM);", "PAPYRUS_BIND(StopNANIM);",
    ),
    "f4se-plugin/src/RE/BSAnimationGraphManager.h": (
        "REL::RelocationID(1381136, 2258328)",
        "REL::RelocationID(144578, 2258329)",
        "static_assert(sizeof(hkbContext) == 0x68)",
    ),
    "f4se-plugin/src/RE/hk.h": (
        "REL::RelocationID(992878, 2260451)",
        "REL::RelocationID(326555, 2260454)",
        "sizeof(hkbGeneratorSyncInfo) == 0xA4",
        "offsetof(hkbCharacter, hkbCharacter::behaviorGraph) == 0x80",
        "offsetof(hkbBehaviorGraph, hkbBehaviorGraph::rootGenerator) == 0xC0",
    ),
}

def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} ADDRESS_LIBRARY")
    data = Path(sys.argv[1]).read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    database = dict(struct.iter_unpack("<QQ", memoryview(data)[8:]))
    failures = []
    if len(data) != 8 + count * 16:
        failures.append("invalid Address Library size")
    for reloc_id, rva in EXPECTED.items():
        if database.get(reloc_id) != rva:
            failures.append(f"ID {reloc_id}: expected 0x{rva:X}, got {database.get(reloc_id)!r}")
    combined = ""
    for relative, selectors in GUARDS.items():
        source = (ROOT / relative).read_text(errors="replace")
        combined += source
        for selector in selectors:
            if selector not in source:
                failures.append(f"{relative}: missing {selector}")
    for stale in ("REL::ID(1381136)", "REL::ID(144578)", "REL::ID(992878)", "REL::ID(326555)"):
        if stale in combined:
            failures.append(f"stale runtime-invariant NANIM-adjacent relocation: {stale}")
    if failures:
        print("\n".join(f"FAIL: {failure}" for failure in failures))
        return 1
    print("PASS: NANIM transform runtime, 7 exact AE records, Havok layout guards, 2 natives, and active BODY v5 fixup matched")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
