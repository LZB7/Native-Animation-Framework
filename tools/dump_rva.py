#!/usr/bin/env python3
import argparse
from pathlib import Path
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

p = argparse.ArgumentParser()
p.add_argument('--exe', required=True)
p.add_argument('--rva', type=lambda x: int(x, 0), required=True)
p.add_argument('--length', type=lambda x: int(x, 0), default=0x200)
a = p.parse_args()
pe = pefile.PE(a.exe, fast_load=True)
data = Path(a.exe).read_bytes()
off = pe.get_offset_from_rva(a.rva)
md = Cs(CS_ARCH_X86, CS_MODE_64)
for ins in md.disasm(data[off:off + a.length], pe.OPTIONAL_HEADER.ImageBase + a.rva):
    print(f'+{ins.address-(pe.OPTIONAL_HEADER.ImageBase+a.rva):04x} {ins.mnemonic:9} {ins.op_str}')
