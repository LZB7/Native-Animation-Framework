#!/usr/bin/env python3
"""Audit the 1.11.221 Papyrus surface reached by production scene events."""

from __future__ import annotations

import argparse
import re
import struct
from pathlib import Path


EXPECTED = {
    "BSScript::Internal::VirtualMachine vtable": (614600, 0x28F1D70),
    "g_GameVM": (4796420, 0x30E0288),
    "BSStringPool::Entry::release": (2268720, 0x167D180),
    "BSStringPool::GetEntry<char>": (2268729, 0x167DA00),
    "ScrapHeap::Allocate": (2267983, 0x165BF20),
    "ScrapHeap::Deallocate": (2267984, 0x165C4F0),
    "ArrayWrapper<Variable>::ReplaceArray (adjacent compatibility path)": (2250435, 0x10B6100),
    "Stack::GetPageForFrame (adjacent native path)": (2314680, 0x20F32F0),
    "Stack::GetStackFrameVariable (adjacent native path)": (2314681, 0x20F3360),
    "VirtualMachine::CreateArray(RawType,...)": (2315147, 0x2112220),
    "VirtualMachine::CreateArray(TypeInfo,...)": (2315148, 0x21123A0),
}

VM_SLOTS = {
    0x19: ("CreateArray(TypeInfo,...)", 0x21123A0),
    0x1A: ("CreateArray(RawType,...)", 0x2112220),
    0x2B: ("SendEvent", 0x2114AD0),
    0x2C: ("DispatchStaticCall", 0x21150F0),
    0x2D: ("DispatchMethodCall(Object,...)", 0x2115600),
    0x2E: ("DispatchMethodCall(handle,...)", 0x2115480),
    0x2F: ("DispatchUnboundMethodCall", 0x2115630),
}


def read_database(path: Path) -> dict[int, int]:
    data = path.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    return dict(struct.iter_unpack("<QQ", memoryview(data)[8:8 + count * 16]))


def pe_info(path: Path) -> tuple[bytes, int, list[tuple[int, int, int]]]:
    data = path.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    image_base = struct.unpack_from("<Q", data, optional + 24)[0]
    section_table = optional + optional_size
    sections = []
    for index in range(count):
        entry = section_table + index * 40
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", data, entry + 8)
        sections.append((virtual_address, max(virtual_size, raw_size), raw_offset))
    return data, image_base, sections


def read_pointer(data: bytes, sections: list[tuple[int, int, int]], rva: int) -> int:
    for virtual_address, size, raw_offset in sections:
        if virtual_address <= rva < virtual_address + size:
            return struct.unpack_from("<Q", data, raw_offset + rva - virtual_address)[0]
    raise ValueError(f"RVA 0x{rva:X} is outside PE sections")


def check_order(text: str, first: str, second: str, owner: str, failures: list[str]) -> None:
    if first not in text or second not in text or text.index(first) >= text.index(second):
        failures.append(f"{owner}: declaration order does not preserve {first!r} before {second!r}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--plugin", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()

    failures: list[str] = []
    database = read_database(args.database)
    for owner, (reloc_id, expected_rva) in EXPECTED.items():
        actual = database.get(reloc_id)
        if actual != expected_rva:
            failures.append(f"{owner}: ID {reloc_id}, expected 0x{expected_rva:X}, got {actual!r}")

    data, image_base, sections = pe_info(args.executable)
    vtable_rva = EXPECTED["BSScript::Internal::VirtualMachine vtable"][1]
    for slot, (owner, expected_rva) in VM_SLOTS.items():
        actual = read_pointer(data, sections, vtable_rva + slot * 8) - image_base
        if actual != expected_rva:
            failures.append(f"VM slot 0x{slot:X} {owner}: expected 0x{expected_rva:X}, got 0x{actual:X}")

    game_util = (args.source_root / "f4se-plugin/src/Misc/GameUtil.h").read_text(encoding="utf-8")
    if '#include "RE/Bethesda/BSScriptUtil.h"' not in game_util or "argsPacked" not in game_util:
        failures.append("GameUtil scene events do not use the maintained AE packed-argument path")
    if "Papyrus::FunctionArgs" in game_util or "RE/Bethesda/FunctionArgs.h" in game_util:
        failures.append("GameUtil retains the OG FunctionArgs/BSTThreadScrapFunction path")

    headers = (
        args.source_root / "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/BSScript/IVirtualMachine.h",
        args.source_root / "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/BSScript/Internal/VirtualMachine.h",
    )
    for header in headers:
        text = header.read_text(encoding="utf-8")
        # MSVC reverses members within these overload sets when laying out the vftable.
        check_order(text, "CreateArray(TypeInfo::RawType", "CreateArray(const TypeInfo&", str(header), failures)
        check_order(text, "DispatchMethodCall(std::uint64_t", "DispatchMethodCall(const BSTSmartPointer<Object>&", str(header), failures)

    type_info = (
        args.source_root / "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/BSScript/TypeInfo.h"
    ).read_text(encoding="utf-8")
    if "data.rawType -= RawType::kArrayStart" not in type_info:
        failures.append("TypeInfo::SetArray(false) does not remove the verified +0xA primitive-array tag")

    if args.plugin:
        plugin = args.plugin.read_bytes()
        call_prefix = rb"\x48\x8B\x03\x4C\x8D\x4D.\x48\x8D\x55.\x48\x8B\xCB\xFF\x90"
        if not re.search(call_prefix + rb"\xC8\x00\x00\x00", plugin, re.DOTALL):
            failures.append("compiled Actor-array packer does not call physical VM slot 0x19 (+0xC8)")
        if re.search(call_prefix + rb"\xD0\x00\x00\x00", plugin, re.DOTALL):
            failures.append("compiled Actor-array packer still calls RawType slot 0x1A (+0xD0) with the TypeInfo ABI")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(
        f"PASS: {len(EXPECTED)} exact scene-event records and "
        f"{len(VM_SLOTS)} physical VM slots/source ABI guards verified"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
