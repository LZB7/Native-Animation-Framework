#!/usr/bin/env python3
import argparse, struct
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('--db', required=True)
p.add_argument('--rva', type=lambda x: int(x, 0), required=True)
a = p.parse_args()
data = Path(a.db).read_bytes()
count = struct.unpack_from('<Q', data)[0]
entries = list(struct.iter_unpack('<QQ', memoryview(data)[8:8 + count * 16]))
exact = [(i, r) for i, r in entries if r == a.rva]
prior = max(((r, i) for i, r in entries if r <= a.rva), default=(0, 0))
print(f'exact={exact} nearest_id={prior[1]} id_rva={prior[0]:#x} delta={a.rva-prior[0]:#x}')
