#!/usr/bin/env python3
"""Audit transitive authoring reference-selection helpers on Fallout 4 1.11.240."""

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "Console current pick index": (2701382, 0x33127A0),
    "Console pick-ref array": (2701391, 0x3312800),
    "Console selected pick ref": (2701395, 0x3312820),
    "Console::SetCurrentPickREFR": (2248551, 0x1038820),
    "PlayerCharacter singleton": (2698073, 0x32DD370),
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
    for owner, (ident, rva) in EXPECTED.items():
        if records.get(ident) != rva:
            failures.append(f"{owner}: ID {ident}, expected 0x{rva:X}, got {records.get(ident)!r}")

    header = (args.source_root / "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/IMenu.h").read_text(errors="replace")
    face = (args.source_root / "f4se-plugin/src/Menu/NAFMenu/MenuHandlers/FaceAnimCreatorHandler.h").read_text(errors="replace")
    required = (
        (header, "REL::ID(1407033, 2701382)"),
        (header, "REL::ID(875116, 2701391)"),
        (header, "REL::ID(170742, 2701395)"),
        (header, "REL::ID(79066, 2248551)"),
        (face, "Console::GetPickRef()"),
        (face, "pickedHandle.get().get()"),
        (face, "ref->As<RE::Actor>()"),
        (face, "UpdatePreviewTarget(a)"),
    )
    for source, token in required:
        if token not in source:
            failures.append(f"missing transitive reference-selection guard {token!r}")
    for stale in ("REL::ID(170742)", "REL::ID(875116)"):
        if stale in header:
            failures.append(f"retains invariant OG selector {stale}")
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(f"PASS: {len(EXPECTED)} exact AE reference-selection records and {len(required)} transitive call/ABI guards matched")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
