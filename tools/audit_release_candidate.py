#!/usr/bin/env python3
"""Release-candidate invariants for the Fallout 4 1.11.240 NAF port."""

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
    if 'if (GetModuleHandleW(L"ocbp.dll") != nullptr)' not in node_graph or 'CallGlobalPapyrusFunction("OCBP_API"' not in node_graph:
        failures.append("OCBP call is not strictly guarded by optional module presence")

    looks_menu = read("f4se-plugin/src/LooksMenu/LooksMenu.h")
    for token in ('if (!module)', 'isInstalled && actor && detail::g_bodyMorphInterface', 'expectedHash'):
        if token not in looks_menu:
            failures.append(f"LooksMenu soft-dependency guard missing: {token}")

    main = read("f4se-plugin/src/main.cpp")
    for token in ("RUNTIME_1_11_240", "ValidateFreeUpAllMarkers", "UsesSigScanning(true)", "UsesAddressLibrary(false)"):
        if token not in main:
            failures.append(f"startup fail-closed guard missing: {token}")
    if "spdlog::level::trace" in main or "flush_on(spdlog::level::trace)" in main:
        failures.append("Release logger still enables or flushes trace-level hot-path diagnostics")
    if "log->set_level(spdlog::level::info)" not in main:
        failures.append("Release logger is not configured at the support-oriented info level")

    if "MCM" in source:
        failures.append("native DLL source contains a direct MCM dependency")

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

    # FreeUpAllMarkers must use its exact Runtime Database identity; the retired
    # preservation raw-RVA fallback must not return.
    engine = read("f4se-plugin/src/Compat/Engine.h")
    if "REL::ID(1324833, 2202575)" not in engine:
        failures.append("FreeUpAllMarkers exact relocation selector is missing")
    for path in source_root.rglob("*.h"):
        text = path.read_text(encoding="utf-8", errors="replace")
        for retired in ("0x55F490", "0x55F4B0"):
            if retired in text:
                failures.append(f"retired FreeUpAllMarkers raw RVA {retired} remains in {path.relative_to(ROOT)}")

    readme = read("README.md")
    for token in (
        "community-maintained continuation and expansion", 'cgf "NAF.ToggleMenu"',
        "Install the Extended `NAF.dll` and `NAF.pex`", "LookAt Cam",
        "Original NAF was created by Snapdragon/Deweh", "independently maintained", "GPL-3.0",
    ):
        if token not in readme:
            failures.append(f"public source/distribution documentation missing: {token}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print("PASS: Extended RC gates, optional integration, Runtime Database startup guards, 45 natives, nine v5 records, UI parity, logging, and exact FreeUpAllMarkers relocation")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
