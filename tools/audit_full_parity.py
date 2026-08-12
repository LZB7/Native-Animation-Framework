#!/usr/bin/env python3
"""Fail if a known NAF 1.0.14 preservation gate disappears from the parity ledger."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def require(text: str, token: str, owner: str) -> None:
    if token not in text:
        raise SystemExit(f"FAIL: expected {owner} token is missing: {token}")


def main() -> None:
    porting = (ROOT / "PORTING.md").read_text(encoding="utf-8")
    papyrus = (ROOT / "f4se-plugin/src/Scripts/Papyrus.h").read_text(encoding="utf-8")
    queue = (ROOT / "f4se-plugin/src/Scene/OrderedActionQueue.h").read_text(encoding="utf-8")
    highlight = (ROOT / "f4se-plugin/src/Menu/HighlightedObjectsContainer.h").read_text(encoding="utf-8")
    position = (ROOT / "f4se-plugin/src/Data/User/Position.h").read_text(encoding="utf-8")
    menu = (ROOT / "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/MainMenuHandler.h").read_text(encoding="utf-8")
    serialization = (ROOT / "f4se-plugin/src/Serialization/Serialization.h").read_text(encoding="utf-8")

    require(porting, "## Full NAF 1.0.14 -> NAF AE parity matrix", "parity ledger")
    require(queue, "kArbitraryCallbacksEnabled = false", "ordered callback gate")
    require(highlight, "kHighlightingEnabled = false", "highlight gate")
    require(position, "position->posType == kPositionTree", "nested-tree gate")
    require(serialization, "case 'SHUD':", "eighth active v5 record")
    if "case 'BODY':" in serialization:
        raise SystemExit("FAIL: BODY became active without updating the parity audit")

    gated_natives = {
        "PlayNANIM",
        "StopNANIM",
        "SetEyeCoordOverride",
        "ClearEyeCoordOverride",
        "PlayFaceAnimation",
        "StopFaceAnimation",
    }
    unexpectedly_bound = sorted(name for name in gated_natives if f"PAPYRUS_BIND({name});" in papyrus)
    if unexpectedly_bound:
        raise SystemExit(f"FAIL: update the parity ledger for newly bound natives: {unexpectedly_bound}")

    if '{ "Creator", BindGoto(kCreator) }' in menu or '{ "Item Explorer", BindGoto(kItemExplorer) }' in menu:
        raise SystemExit("FAIL: update the parity ledger for newly restored main-menu entries")

    print(
        "PASS: full-parity ledger tracks 8 explicit AE gates, 6 remaining upstream "
        "Papyrus bindings, active v5 records, and passive BODY state"
    )


if __name__ == "__main__":
    main()
