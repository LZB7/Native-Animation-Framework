#!/usr/bin/env python3
"""Regression checks for safe TESDataHandler file collection traversal."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESDataHandler.h"
RELOCATION = ROOT / "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/REL/Relocation.h"


def main() -> None:
    text = HEADER.read_text(encoding="utf-8", errors="ignore")
    relocation = RELOCATION.read_text(encoding="utf-8", errors="ignore")
    if "REL::ID(711558, 4796135)" not in text:
        raise SystemExit("FAIL: verified TESDataHandler singleton IDs are missing")
    if text.count("if (!file)") < 5:
        raise SystemExit("FAIL: full/VR TESFile collection traversals are not null-safe")
    if text.count("if (!smallFile)") < 2:
        raise SystemExit("FAIL: light TESFile collection traversals are not null-safe")
    required_layout_contract = (
        "const auto offset = Module::IsVR() ? a_vr : a_f4AndNG;",
        "const auto vtableOffset = Module::IsVR() ? a_vrVtableOffset : a_f4AndNGVtableOffset;",
        "const auto index = Module::IsVR() ? a_vrIndex : a_f4AndNGIndex;",
    )
    for token in required_layout_contract:
        if token not in relocation:
            raise SystemExit(f"FAIL: preservation SE/VR layout contract is missing {token!r}")
    if "return RelocateMember<T>(a_self, Module::get().version(), a_og, a_ae);" in relocation:
        raise SystemExit("FAIL: two-offset member helper still interprets preservation SE/VR offsets as OG/AE")
    print(
        "PASS: TESDataHandler traversal is null-safe and inherited two-way member/vtable helpers "
        "preserve the CommonLibF4 SE/VR contract"
    )


if __name__ == "__main__":
    main()
