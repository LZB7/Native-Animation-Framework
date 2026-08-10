#!/usr/bin/env python3
import argparse, struct
from pathlib import Path
import pefile

p = argparse.ArgumentParser()
p.add_argument('--exe', required=True)
p.add_argument('--db', required=True)
p.add_argument('--target', type=lambda x: int(x, 0), required=True)
p.add_argument('--mapped', action='store_true')
a = p.parse_args()

data = Path(a.exe).read_bytes()
dbdata = Path(a.db).read_bytes()
count = struct.unpack_from('<Q', dbdata)[0]
entries = list(struct.iter_unpack('<QQ', memoryview(dbdata)[8:8 + count * 16]))
by_rva = sorted((rva, ident) for ident, rva in entries)
pe = pefile.PE(a.exe, fast_load=True) if not a.mapped else None

def offset_to_rva(off):
    if a.mapped:
        return off
    return pe.get_rva_from_offset(off)

for off in range(len(data) - 5):
    if data[off] != 0xE8:
        continue
    call_rva = offset_to_rva(off)
    if call_rva is None:
        continue
    rel = struct.unpack_from('<i', data, off + 1)[0]
    if call_rva + 5 + rel != a.target:
        continue
    prior = max(((r, i) for r, i in by_rva if r <= call_rva), default=(0, 0))
    print(f'call_rva={call_rva:#x} nearest_id={prior[1]} id_rva={prior[0]:#x} delta={call_rva-prior[0]:#x}')
