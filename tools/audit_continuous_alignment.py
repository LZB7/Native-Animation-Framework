#!/usr/bin/env python3
"""Audit ordinary-scene continuous alignment and bounded transform exposure."""

import struct
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    2229653: 0xC5CE00,  # actor collision disable
    2231882: 0xD02600,  # player 3D update
}
GUARDS = {
    "f4se-plugin/src/Scene/SceneBase.h": (
        "MathUtil::ApplyOffsetToLocalSpace",
        "SetAngleOnReference(actorAngle)",
        "SetPosition(actorLoc, true)",
        "EngineCompat::DisableCollision(currentActor)",
        "EngineCompat::SetNoCollision(currentActor, true)",
        "EngineCompat::UpdatePlayer3D(player)",
    ),
    "f4se-plugin/src/Scripts/Papyrus.h": (
        "PAPYRUS_BIND(GetLocalTransform);",
        "PAPYRUS_BIND(SynchronizeAnimations);",
    ),
    "f4se-plugin/src/Tasks/GameLoopHook.h": (
        "Scene::SceneManager::UpdateScenes();",
    ),
}


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} ADDRESS_LIBRARY")
    data = Path(sys.argv[1]).read_bytes()
    database = dict(struct.iter_unpack("<QQ", memoryview(data)[8:]))
    failures = []
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
    print("PASS: ordinary-scene alignment cadence, 2 exact AE records, and bounded transform/sync natives matched")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
