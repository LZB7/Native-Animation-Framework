#!/usr/bin/env python3
import argparse, re, struct
from pathlib import Path
import pefile

p = argparse.ArgumentParser()
p.add_argument('--exe', required=True)
p.add_argument('--db', required=True)
p.add_argument('--pattern', required=True, help='space-separated hex bytes; ?? is wildcard')
a = p.parse_args()
data = Path(a.exe).read_bytes()
parts = a.pattern.split()
pat = re.compile(b''.join(b'.' if x == '??' else re.escape(bytes([int(x, 16)])) for x in parts), re.DOTALL)
dbdata = Path(a.db).read_bytes()
count = struct.unpack_from('<Q', dbdata)[0]
entries = list(struct.iter_unpack('<QQ', memoryview(dbdata)[8:8 + count * 16]))
by_rva = {rva: ident for ident, rva in entries}
sorted_rvas = sorted(by_rva)
pe = pefile.PE(a.exe, fast_load=True)
for m in pat.finditer(data):
    rva = pe.get_rva_from_offset(m.start())
    prior = max((x for x in sorted_rvas if x <= rva), default=0)
    print(f'offset={m.start():#x} rva={rva:#x} id={by_rva.get(rva)} nearest_id={by_rva.get(prior)} id_rva={prior:#x} delta={rva-prior:#x}')
