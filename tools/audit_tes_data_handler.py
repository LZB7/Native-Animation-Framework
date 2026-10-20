#!/usr/bin/env python3
"""Regression checks for safe TESDataHandler file collection traversal."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESDataHandler.h"


def main() -> None:
    text = HEADER.read_text(encoding="utf-8", errors="ignore")
    if "REL::ID(711558, 4796135)" not in text:
        raise SystemExit("FAIL: verified TESDataHandler singleton IDs are missing")
    if text.count("if (!file)") < 5:
        raise SystemExit("FAIL: full/VR TESFile collection traversals are not null-safe")
    if text.count("if (!smallFile)") < 2:
        raise SystemExit("FAIL: light TESFile collection traversals are not null-safe")
    print("PASS: TESDataHandler uses the verified singleton ID and skips null full/light file slots")


if __name__ == "__main__":
    main()
