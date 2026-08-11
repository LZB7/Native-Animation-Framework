#!/usr/bin/env python3
"""Print PE vtable entries as RVAs for focused ABI audits."""

import argparse
import struct
from pathlib import Path


parser = argparse.ArgumentParser()
parser.add_argument("--exe", type=Path, required=True)
parser.add_argument("--rva", type=lambda value: int(value, 0), required=True)
parser.add_argument("--count", type=int, required=True)
args = parser.parse_args()

data = args.exe.read_bytes()
pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
section_count = struct.unpack_from("<H", data, pe_offset + 6)[0]
optional_size = struct.unpack_from("<H", data, pe_offset + 20)[0]
optional = pe_offset + 24
image_base = struct.unpack_from("<Q", data, optional + 24)[0]
sections = optional + optional_size


def read_rva(rva: int, size: int) -> bytes:
    for index in range(section_count):
        section = sections + index * 40
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", data, section + 8)
        if virtual_address <= rva < virtual_address + max(virtual_size, raw_size):
            offset = raw_offset + rva - virtual_address
            return data[offset:offset + size]
    raise ValueError(f"RVA 0x{rva:X} is outside PE sections")


for slot in range(args.count):
    pointer = struct.unpack("<Q", read_rva(args.rva + slot * 8, 8))[0]
    print(f"slot=0x{slot:X} byte=0x{slot * 8:X} rva=0x{pointer - image_base:X}")
