#!/usr/bin/env python3
"""Dump one PE unwind function and annotate Address Library call targets."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path

import pefile
from capstone import CS_ARCH_X86, CS_MODE_64, Cs
from capstone.x86 import X86_OP_IMM


def read_database(path: Path) -> tuple[dict[int, int], dict[int, list[int]]]:
    data = path.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    forward = dict(struct.iter_unpack("<QQ", memoryview(data)[8:]))
    if len(forward) != count:
        raise ValueError("duplicate or malformed Address Library entries")
    reverse: dict[int, list[int]] = {}
    for ident, rva in forward.items():
        reverse.setdefault(rva, []).append(ident)
    return forward, reverse


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--db", type=Path, required=True)
    parser.add_argument("--id", type=int, required=True)
    parser.add_argument("--mapped", action="store_true", help="input is a loaded-module image indexed by RVA")
    parser.add_argument("--length", type=lambda value: int(value, 0), help="override unwind-derived byte length")
    args = parser.parse_args()

    forward, reverse = read_database(args.db)
    target = forward[args.id]
    pe = pefile.PE(str(args.exe), fast_load=False)
    base = pe.OPTIONAL_HEADER.ImageBase
    image = args.exe.read_bytes() if args.mapped else None
    if args.mapped:
        directory = pe.OPTIONAL_HEADER.DATA_DIRECTORY[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXCEPTION"]]
        runtime_functions = (
            struct.unpack_from("<III", image, offset)
            for offset in range(directory.VirtualAddress, directory.VirtualAddress + directory.Size, 12)
        )
        entry = next(((start, finish) for start, finish, _ in runtime_functions if start <= target < finish), None)
        begin, end = entry if entry else (target, target + 0x100)
        code = image[begin:end]
    else:
        entry = next(
            (entry for entry in pe.DIRECTORY_ENTRY_EXCEPTION
             if entry.struct.BeginAddress <= target < entry.struct.EndAddress),
            None,
        )
        begin = entry.struct.BeginAddress if entry else target
        end = entry.struct.EndAddress if entry else target + 0x100
        code = pe.get_data(begin, end - begin)
    if args.length is not None:
        begin = target
        end = target + args.length
        code = image[begin:end] if args.mapped else pe.get_data(begin, end - begin)
    print(f"id={args.id} target={target:#x} function={begin:#x}-{end:#x} delta={target-begin:#x}")

    disassembler = Cs(CS_ARCH_X86, CS_MODE_64)
    disassembler.detail = True
    disassembler.skipdata = True
    for instruction in disassembler.disasm(code, base + begin):
        annotation = ""
        if instruction.mnemonic in {"call", "jmp"} and instruction.operands and instruction.operands[0].type == X86_OP_IMM:
            call_rva = instruction.operands[0].imm - base
            annotation = f" ; target={call_rva:#x} ids={reverse.get(call_rva, [])}"
        marker = ">>" if instruction.address == base + target else "  "
        print(f"{marker} +{instruction.address-base-begin:04x} {instruction.mnemonic:8} {instruction.op_str}{annotation}")


if __name__ == "__main__":
    main()
