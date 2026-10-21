#!/usr/bin/env python3
"""Audit the node-level Body IK path reachable without NANIM playback."""

import struct
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    2214534: 0x818A40,  # GetAnimationGraphProjectName
    2214536: 0x818C80,  # graph update hook
    2201055: 0x50C8D0,  # Set3D hook
}
GUARDS = {
    "f4se-plugin/src/Scripts/Papyrus.h": (
        "PAPYRUS_BIND(SetIKChainTarget);",
        "PAPYRUS_BIND(SetIKChainEnabled);",
    ),
    "f4se-plugin/src/BodyAnimation/NodeAnimationGraph.h": (
        "case kIdle:",
        "ikManager.Update();",
        "SetGraphData",
        "UpdateNodes",
    ),
    "f4se-plugin/src/BodyAnimation/IK.h": (
        "SetChainEnabled",
        "SetChainTarget",
        "holderEnabled",
        "h->Update();",
    ),
    "f4se-plugin/src/BodyAnimation/GraphHook.h": (
        "REL::ID(1492656, 2214536)",
        "REL::ID(1216262, 2201055)",
        "EngineCompat::GetAnimationGraphProjectName",
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
    for relative, selectors in GUARDS.items():
        source = (ROOT / relative).read_text(errors="replace")
        for selector in selectors:
            if selector not in source:
                failures.append(f"{relative}: missing {selector}")
    if failures:
        print("\n".join(f"FAIL: {failure}" for failure in failures))
        return 1
    print("PASS: 3 exact AE graph/IK records, node-level IK update guards, and two IK natives matched")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
