#!/usr/bin/env python3
import argparse, collections, struct
from pathlib import Path
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from capstone.x86 import X86_OP_IMM

p = argparse.ArgumentParser()
p.add_argument('--exe', required=True)
p.add_argument('--db', required=True)
a = p.parse_args()
pe = pefile.PE(a.exe, fast_load=True)
image = Path(a.exe).read_bytes()
base = pe.OPTIONAL_HEADER.ImageBase
dbdata = Path(a.db).read_bytes()
count = struct.unpack_from('<Q', dbdata)[0]
by_rva = {rva: ident for ident, rva in struct.iter_unpack('<QQ', memoryview(dbdata)[8:8 + count * 16])}
text = next(s for s in pe.sections if s.Name.rstrip(b'\0') == b'.text')
code = image[text.PointerToRawData:text.PointerToRawData + text.SizeOfRawData]
md = Cs(CS_ARCH_X86, CS_MODE_64)
md.detail = True
md.skipdata = True
window = collections.deque(maxlen=24)
for ins in md.disasm(code, base + text.VirtualAddress):
    if ins.mnemonic == 'call' and ins.operands and ins.operands[0].type == X86_OP_IMM:
        prior = ' ; '.join(f'{x.mnemonic} {x.op_str}' for x in window)
        if ('0x3d' in prior and ('0xfd' in prior or ', 3' in prior or '0x3' in prior)):
            target = ins.operands[0].imm - base
            call_rva = ins.address - base
            print(f'call={call_rva:#x} target={target:#x} target_id={by_rva.get(target)}')
            print(prior)
    window.append(ins)
