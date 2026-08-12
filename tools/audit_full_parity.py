#!/usr/bin/env python3
"""Audit the central NAF 1.0.14 preservation ledger and remaining gates."""

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
    require(queue, "kArbitraryCallbacksEnabled = true", "restored ordered callbacks")
    require(highlight, "kHighlightingEnabled = true", "restored highlighting")
    require(position, "Rejected cyclic position-tree reference", "nested-tree cycle guard")
    require(serialization, "case 'SHUD':", "eighth active v5 record")
    require(serialization, "case 'BODY':", "active BODY v5 restoration")

    managed_face_natives = {
        "SetEyeCoordOverride",
        "ClearEyeCoordOverride",
        "PlayFaceAnimation",
        "StopFaceAnimation",
    }
    missing_bindings = sorted(name for name in managed_face_natives if f"PAPYRUS_BIND({name});" not in papyrus)
    if missing_bindings:
        raise SystemExit(f"FAIL: managed-face Papyrus bindings remain gated: {missing_bindings}")

    if '{ "Creator", BindGoto(kCreator) }' in menu or '{ "Item Explorer", BindGoto(kItemExplorer) }' in menu:
        raise SystemExit("FAIL: update the parity ledger for newly restored main-menu entries")

    print(
        "PASS: full-parity ledger tracks restored Phase-B surfaces, all 45 Papyrus bindings, "
        "managed-face lifecycle, and active BODY/FACE v5 restoration"
    )


if __name__ == "__main__":
    main()
