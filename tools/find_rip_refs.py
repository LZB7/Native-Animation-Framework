#!/usr/bin/env python3
import argparse, struct
from pathlib import Path
import pefile

p = argparse.ArgumentParser()
p.add_argument('--exe', required=True)
p.add_argument('--target', type=lambda x: int(x, 0), required=True)
a = p.parse_args()
data = Path(a.exe).read_bytes()
pe = pefile.PE(a.exe, fast_load=True)
prefixes = {bytes.fromhex(x) for x in ('48 8b 05','48 8b 0d','48 8b 15','48 8b 1d','48 8b 35','48 8b 3d','48 89 05','48 89 0d','48 8d 05','48 8d 0d','48 8d 15','48 8d 1d','48 8d 35','48 8d 3d')}
for off in range(len(data) - 7):
    if data[off:off+3] not in prefixes:
        continue
    rva = pe.get_rva_from_offset(off)
    if rva is None:
        continue
    disp = struct.unpack_from('<i', data, off + 3)[0]
    if rva + 7 + disp == a.target:
        print(f'rva={rva:#x} bytes={data[off:off+7].hex(" ")}')
