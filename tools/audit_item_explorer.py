#!/usr/bin/env python3
"""Audit the complete production Item Explorer/equipment path on 1.11.221."""

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "TESDataHandler singleton": (4796135, 0x30DC080),
    "PlayerCharacter singleton": (2698073, 0x32D22E0),
    "BGSObjectInstance constructor": (2197563, 0x436830),
    "TESForm::GetFile": (2193103, 0x3120B0),
    "TESObjectREFR::GetInventoryObjectCount": (2200939, 0x502290),
    "TESObjectREFR::AddInventoryItem": (2200949, 0x502950),
    "ActorEquipManager singleton": (4798287, 0x31E33A8),
    "ActorEquipManager::EquipObject": (2231392, 0xCE5A90),
    "ActorEquipManager::UnequipObject": (2231395, 0xCE5F30),
    "UIMessageQueue singleton": (4796377, 0x30DDAA0),
    "UIMessageQueue::AddMessage": (2284929, 0x1A89170),
}

GUARDS = {
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESObjectREFRs.h": (
        "REL::RelocationID(1095748, 2197563)",
        "BGSObjectInstance(TESForm* a_object, TBO_InstanceData* a_instanceData)",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESForms.h": (
        "GetFilledSlots() const;",
        "GetPlayable([[maybe_unused]] const TBO_InstanceData* a_data) const",
        "REL::RelocationID(1376557, 2193103)",
    ),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/ItemExplorerHandler.h": (
        "GetFormArray<RE::TESObjectARMO>", "GetPlayable", "GetFormRace", "if (!filename.empty())",
        '"[Unnamed Armor]"', "Item Explorer opened", "source enumeration complete", "list populated",
        "form selected", "ordered equip action", "ordered remove action", "Item Explorer closed",
    ),
    "f4se-plugin/src/Scene/OrderedActionQueue.h": (
        "GetInventoryObjectCount", "AddInventoryItem", "RE::BGSObjectInstance(obj, instData)",
        "ActorEquipManager::GetSingleton()->EquipObject", "ActorEquipManager::GetSingleton()->UnequipObject",
        "RemoveItem(removeData)",
    ),
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    data = args.database.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    records = dict(struct.iter_unpack("<QQ", memoryview(data)[8:8 + count * 16]))
    failures: list[str] = []
    for owner, (ident, expected_rva) in EXPECTED.items():
        if records.get(ident) != expected_rva:
            failures.append(f"{owner}: ID {ident}, expected 0x{expected_rva:X}, got {records.get(ident)!r}")
    combined = ""
    for relative, tokens in GUARDS.items():
        source = (args.source_root / relative).read_text(encoding="utf-8", errors="replace")
        combined += source
        for token in tokens:
            if token not in source:
                failures.append(f"{relative}: missing {token!r}")
    if "REL::ID(1095748)" in combined:
        failures.append("Item Explorer/equipment path retains invariant OG constructor ID 1095748")
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(
        f"PASS: {len(EXPECTED)} exact Item Explorer/equipment AE records and "
        f"{sum(map(len, GUARDS.values()))} ABI/lifecycle guards matched"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
