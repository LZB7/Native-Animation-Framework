#!/usr/bin/env python3
from phase_d_audit_common import audit

raise SystemExit(audit("Item Explorer", {
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/MainMenuHandler.h": ('{ "Item Explorer", BindGoto(kItemExplorer) }',),
    "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/ItemExplorerHandler.h": (
        "GetFormArray<RE::TESObjectARMO>", "GetPlayable", "GetFormRace", "AddEquipment",
        "RemoveEquipment", "if (!filename.empty())", '"[Unnamed Armor]"'),
}))
