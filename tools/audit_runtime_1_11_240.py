#!/usr/bin/env python3
"""Verify the exact 1.11.240 database and critical NAF hook sites."""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path


EXPECTED_SHA256 = "fdcef37ac1230af6d0b0050eb2142b139ef3a867b37b9211fb6edfcc646072f8"
EXPECTED = {
    "NonRendersafeAI containing function": (2228937, 0xC365F0),
    "ProcessQueues": (2229181, 0xC45150),
    "UpdateAnimationGraphManager": (2214536, 0x818C80),
    "TESObjectREFR::Set3D": (2201055, 0x50C8D0),
    "BSFaceGenNiNode::UpdateMorphing": (2209487, 0x6E8F00),
    "BSFaceGenAnimationData::Update": (2209139, 0x6D1630),
    "BSFaceGenAnimationData::UpdateMorphsFromLip": (2209124, 0x6D0880),
    "Actor::EvaluatePackage": (2229805, 0xC695D0),
    "AIProcess::CheckforNewPackage": (2231539, 0xCEFE60),
    "FreeCameraState::Update": (2248280, 0x10223D0),
    "TESObjectREFR::FreeUpAllMarkers": (2202575, 0x55F7B0),
    "BSScript::Object::DecRef": (2314437, 0x20E9DF0),
}


def read_database(path: Path) -> dict[int, int]:
    data = path.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    if len(data) != 8 + count * 16:
        raise ValueError(f"invalid Address Library size: {path}")
    return dict(struct.iter_unpack("<QQ", memoryview(data)[8:]))


def pe_bytes(path: Path, rva: int, size: int) -> bytes:
    data = path.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    sections = pe + 24 + struct.unpack_from("<H", data, pe + 20)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    for index in range(count):
        section = sections + index * 40
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", data, section + 8)
        if virtual_address <= rva < virtual_address + max(virtual_size, raw_size):
            offset = raw_offset + rva - virtual_address
            return data[offset : offset + size]
    raise ValueError(f"RVA 0x{rva:X} is not mapped")


def decode_call_target(executable: Path, instruction_rva: int) -> int:
    encoded = pe_bytes(executable, instruction_rva, 5)
    if encoded[0] != 0xE8:
        raise ValueError(f"RVA 0x{instruction_rva:X} is not a direct call")
    return instruction_rva + 5 + struct.unpack_from("<i", encoded, 1)[0]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("executable", type=Path)
    args = parser.parse_args()
    failures: list[str] = []
    database = read_database(args.database)

    digest = hashlib.sha256(args.executable.read_bytes()).hexdigest()
    if digest != EXPECTED_SHA256:
        failures.append(f"Fallout4.exe SHA-256 mismatch: {digest}")
    for owner, (ident, expected_rva) in EXPECTED.items():
        actual = database.get(ident)
        if actual != expected_rva:
            failures.append(f"{owner}: ID {ident}, expected 0x{expected_rva:X}, got {actual!r}")

    direct_calls = {
        "game loop -> ProcessQueues": (0xC365F0 + 0x1E, 0xC45150),
        "face outer -> face update": (0x6E8F00 + 0xBD, 0x6D1630),
        "lip hook -> lip morph update": (0x6D1630 + 0x14B, 0x6D0880),
    }
    for owner, (site, expected_target) in direct_calls.items():
        try:
            actual_target = decode_call_target(args.executable, site)
        except ValueError as error:
            failures.append(f"{owner}: {error}")
            continue
        if actual_target != expected_target:
            failures.append(f"{owner}: expected target 0x{expected_target:X}, got 0x{actual_target:X}")

    freeup_signature = bytes.fromhex(
        "40 55 56 48 83 EC 28 48 8B B1 E0 00 00 00 48 8B E9 48 85 F6 0F 84 "
        "00 00 00 00 0F B6 46 1A 3C 2A 74 08 3C 37 0F 85 00 00 00 00 48"
    )
    freeup_mask = bytes.fromhex(
        "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF "
        "00 00 00 00 FF FF FF FF FF FF FF FF FF FF FF FF 00 00 00 00 FF"
    )
    actual = pe_bytes(args.executable, 0x55F7B0, len(freeup_signature))
    if any((left & mask) != (right & mask) for left, right, mask in zip(actual, freeup_signature, freeup_mask)):
        failures.append("TESObjectREFR::FreeUpAllMarkers masked entry signature mismatch")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(f"PASS: {len(EXPECTED)} exact 1.11.240 records, three direct-call hook sites, executable hash, and FreeUpAllMarkers entry")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
