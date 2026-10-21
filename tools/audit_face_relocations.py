#!/usr/bin/env python3
"""Verify exact Fallout 4 1.11.240 records selected by NAF's face/lip hooks."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "BSFaceGenNiNode::UpdateMorphing": (2209487, 0x6E8F00),
    "BSFaceGenAnimationData::Update": (2209139, 0x6D1630),
    "BSFaceGenAnimationData::UpdateMorphsFromLip": (2209124, 0x6D0880),
}

SOURCE_GUARDS = {
    "face update containing function": "REL::ID faceOwner{ 317245, 2209487 }",
    "face update target": "REL::ID faceTarget{ 3139, 2209139 }",
    "face update automatic callsite": "REL::AUTO_CALLSITE(faceTarget)",
    "face update tested fallback": ".or_offset(0xBD, REL::Version{ 1, 11, 221, 0 }, REL::Version{ 1, 11, 240, 0 })",
    "lip update target": "REL::ID lipTarget{ 1550953, 2209124 }",
    "lip update automatic callsite": "REL::AUTO_CALLSITE(lipTarget)",
    "lip update tested fallback": ".or_offset(0x14B, REL::Version{ 1, 11, 221, 0 }, REL::Version{ 1, 11, 240, 0 })",
}


def read_database(path: Path) -> dict[int, int]:
    data = path.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    entries = dict(struct.iter_unpack("<QQ", memoryview(data)[8 : 8 + count * 16]))
    if len(entries) != count:
        raise ValueError("duplicate or malformed Address Library entries")
    return entries


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path("f4se-plugin/src"))
    args = parser.parse_args()

    records = read_database(args.database)
    failures: list[str] = []
    for owner, (ident, expected_rva) in EXPECTED.items():
        actual = records.get(ident)
        if actual != expected_rva:
            failures.append(f"{owner}: ID {ident} expected {expected_rva:#x}, got {actual!r}")

    source = (args.source_root / "FaceAnimation/FaceUpdateHook.h").read_text(encoding="utf-8")
    for owner, selector in SOURCE_GUARDS.items():
        if selector not in source:
            failures.append(f"{owner}: missing source selector {selector!r}")

    if failures:
        print("FAIL:")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print(f"PASS: {len(EXPECTED)} face/lip relocation IDs matched exact expected RVAs; {len(SOURCE_GUARDS)} source selectors matched")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
