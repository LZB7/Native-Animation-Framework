#!/usr/bin/env python3
"""Audit the bounded equipment, morph and ordered-action production surface."""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path


EXPECTED = {
    "ActorEquipManager singleton": (4798287, 0x31E33A8),
    "ActorEquipManager::EquipObject": (2231392, 0xCE5A90),
    "ActorEquipManager::UnequipObject": (2231395, 0xCE5F30),
    "TESObjectREFR::GetInventoryObjectCount": (2200939, 0x502290),
    "TESObjectREFR::AddInventoryItem": (2200949, 0x502950),
}

SOURCE_GUARDS = {
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/Actor.h": (
        "REL::RelocationID(1174340, 4798287)",
        "REL::RelocationID(988029, 2231392)",
        "REL::RelocationID(1292493, 2231395)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESObjectREFRs.h": (
        "REL::RelocationID(78185, 2200949)",
        "REL::RelocationID(333415, 2200939)",
    ),
    "f4se-plugin/src/Scene/OrderedActionQueue.h": (
        "kEquipmentActionsEnabled = true",
        "kMorphActionsEnabled = true",
        "kArbitraryCallbacksEnabled = false",
        "ActorEquipManager::GetSingleton()->EquipObject",
        "ActorEquipManager::GetSingleton()->UnequipObject",
    ),
    "f4se-plugin/src/LooksMenu/LooksMenu.h": (
        "0x11400",
        "0x107E00",
        "baseAddr + 0xC88B0",
        "expectedApplyPrologue",
    ),
    "f4se-plugin/src/main.cpp": ("LooksMenu::Init();",),
}

LOOKSMENU_SHA256 = "b562d636d768620760aad8c166ccfda22b014f490abace0b21e84334b26c56f0"


def read_database(path: Path) -> dict[int, int]:
    data = path.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    if len(data) != 8 + count * 16:
        raise ValueError(f"invalid Address Library size: {path}")
    return dict(struct.iter_unpack("<QQ", memoryview(data)[8:]))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("--looksmenu", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    database = read_database(args.database)
    failures: list[str] = []

    for owner, (reloc_id, expected_rva) in EXPECTED.items():
        actual = database.get(reloc_id)
        if actual != expected_rva:
            failures.append(f"{owner}: ID {reloc_id}, expected 0x{expected_rva:X}, got {actual!r}")

    for relative_path, selectors in SOURCE_GUARDS.items():
        source = (args.source_root / relative_path).read_text(errors="replace")
        for selector in selectors:
            if selector not in source:
                failures.append(f"{relative_path}: missing selector {selector}")

    if args.looksmenu:
        digest = hashlib.sha256(args.looksmenu.read_bytes()).hexdigest()
        if digest != LOOKSMENU_SHA256:
            failures.append(f"LooksMenu 1.7.0.3 SHA-256 mismatch: {digest}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1

    suffix = "; exact LooksMenu 1.7.0.3 SHA-256 matched" if args.looksmenu else ""
    print(f"PASS: {len(EXPECTED)} equipment/inventory AE records and {sum(map(len, SOURCE_GUARDS.values()))} feature guards matched{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
