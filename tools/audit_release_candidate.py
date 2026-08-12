#!/usr/bin/env python3
"""Release-candidate invariants for the Fallout 4 1.11.221 NAF port."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8", errors="replace")


def main() -> int:
    failures: list[str] = []
    source_root = ROOT / "f4se-plugin/src"
    source = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in source_root.rglob("*.[ch]pp"))
    source += "\n" + "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in source_root.rglob("*.h"))

    forbidden_gates = (
        "kGameplaySubsystemsEnabled", "kHUDSubsystemEnabled", "kGameplayUpdateWorkEnabled",
        "kEquipmentActionsEnabled", "kMorphActionsEnabled", "kArbitraryCallbacksEnabled",
        "kHighlightingEnabled", "DISABLE_SERIALIZATION",
    )
    for token in forbidden_gates:
        if token in source:
            failures.append(f"temporary compatibility gate remains: {token}")

    node_graph = read("f4se-plugin/src/BodyAnimation/NodeAnimationGraph.h")
    if 'GetModuleHandleW(L"ocbp.dll")' not in node_graph:
        failures.append("OCBP optional integration is not detected by exact module identity")
    if 'GetPluginInfo("OCBP plugin")' in node_graph:
        failures.append("OCBP retains noisy obsolete F4SE logical-name lookup")

    main = read("f4se-plugin/src/main.cpp")
    for token in ("RUNTIME_1_11_221", "ValidateFreeUpAllMarkers", "UsesSigScanning(false)"):
        if token not in main:
            failures.append(f"startup fail-closed guard missing: {token}")

    papyrus = read("f4se-plugin/src/Scripts/Papyrus.h")
    if papyrus.count("PAPYRUS_BIND(") != 46:  # macro definition plus 45 registrations
        failures.append("Papyrus registration is not exactly 45 production natives")

    serialization = read("f4se-plugin/src/Serialization/Serialization.h")
    for record in ("TASK", "SCNE", "EQPT", "UID", "FACE", "HUD", "PACK", "SHUD", "BODY"):
        if f"'{record}'" not in serialization:
            failures.append(f"historical v5 record is missing: {record}")

    menu = read("f4se-plugin/src/Menu/NAFMenu/MenuHandlers/MainMenuHandler.h")
    creator = read("f4se-plugin/src/Menu/NAFMenu/MenuHandlers/CreatorMenuHandler.h")
    for entry in ("New Scene", "Active Scenes", "Settings", "Creator", "Item Explorer"):
        if entry not in menu:
            failures.append(f"upstream menu entry missing: {entry}")
    if "Open in Animation Studio" not in creator:
        failures.append("upstream Creator-to-Studio entry missing")

    cam = read("f4se-plugin/src/CamHook/CamHook.h")
    for noisy in ("LookAt Cam update #", "renderDot=", "LookAt Cam resolved target node"):
        if noisy in cam:
            failures.append(f"development LookAt diagnostic remains: {noisy}")

    # The sole project-specific RVA fallback must remain isolated to the verified
    # exact-runtime FreeUpAllMarkers helper.
    raw_rva_literals: list[tuple[str, str]] = []
    for path in source_root.rglob("*.h"):
        text = path.read_text(encoding="utf-8", errors="replace")
        if "0x55F490" in text:
            raw_rva_literals.append((str(path.relative_to(ROOT)), "0x55F490"))
        if "0x55F4B0" in text:
            failures.append(f"withdrawn interior FreeUpAllMarkers address remains in {path.relative_to(ROOT)}")
    if raw_rva_literals != [("f4se-plugin\\src\\Compat\\Engine.h", "0x55F490")]:
        failures.append(f"unexpected FreeUpAllMarkers raw-RVA locations: {raw_rva_literals}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print("PASS: RC gates, optional integration, startup guards, 45 natives, nine v5 records, UI parity, logging, and sole raw-RVA exception")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
