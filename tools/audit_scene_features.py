#!/usr/bin/env python3
"""Audit the bounded equipment, morph and ordered-action production surface."""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path


EXPECTED = {
    "ActorEquipManager singleton": (4798287, 0x31EE428),
    "ActorEquipManager::EquipObject": (2231392, 0xCE5E20),
    "ActorEquipManager::UnequipObject": (2231395, 0xCE62C0),
    "TESObjectREFR::GetInventoryObjectCount": (2200939, 0x5025B0),
    "TESObjectREFR::AddInventoryItem": (2200949, 0x502C70),
}

SOURCE_GUARDS = {
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/Actor.h": (
        "REL::ID(1174340, 4798287)",
        "REL::ID(988029, 2231392)",
        "REL::ID(1292493, 2231395)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESObjectREFRs.h": (
        "REL::ID(78185, 2200949)",
        "REL::ID(333415, 2200939)",
    ),
    "f4se-plugin/src/Scene/OrderedActionQueue.h": (
        "static void UnEquip",
        "static void ApplyMorphs",
        "static void InsertCustom",
        "ActorEquipManager::GetSingleton()->EquipObject",
        "ActorEquipManager::GetSingleton()->UnequipObject",
    ),
    "f4se-plugin/src/LooksMenu/LooksMenu.h": (
        'GetModuleHandleA("f4ee.dll")',
        "BCRYPT_SHA256_ALGORITHM",
        "expectedHash",
        "0x11400",
        "0x107E00",
        "0x11780",
        "0x10DD30",
        "0xCD840",
        "expectedApplyPrologue",
    ),
    "f4se-plugin/src/main.cpp": ("LooksMenu::Init();",),
}

LOOKSMENU_SHA256 = {
    "b562d636d768620760aad8c166ccfda22b014f490abace0b21e84334b26c56f0": "1.7.0.3",
    "2eb76a878bad00edee64d19431d19d072fffa096215deb04dd3180abe39fdb0f": "1.7.0.4",
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

    looks_source = (args.source_root / "f4se-plugin/src/LooksMenu/LooksMenu.h").read_text(errors="replace")
    if 'GetPluginInfo("F4EE")' in looks_source:
        failures.append("LooksMenu detection regressed to F4SE logical plugin-name metadata")

    if args.looksmenu:
        digest = hashlib.sha256(args.looksmenu.read_bytes()).hexdigest()
        if digest not in LOOKSMENU_SHA256:
            failures.append(f"unsupported LooksMenu SHA-256: {digest}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1

    suffix = f"; exact LooksMenu {LOOKSMENU_SHA256[digest]} SHA-256 matched" if args.looksmenu else ""
    print(f"PASS: {len(EXPECTED)} equipment/inventory AE records and {sum(map(len, SOURCE_GUARDS.values()))} feature guards matched{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
