#!/usr/bin/env python3
"""List literal relocation selectors whose selected ID is absent from a database."""

import argparse
import re
import struct
from pathlib import Path


parser = argparse.ArgumentParser()
parser.add_argument("database", type=Path)
parser.add_argument("roots", nargs="+", type=Path)
args = parser.parse_args()
data = args.database.read_bytes()
count = struct.unpack_from("<Q", data)[0]
records = dict(struct.iter_unpack("<QQ", memoryview(data)[8:8 + count * 16]))
pair = re.compile(r"REL::RelocationID\(\s*(\d+)\s*,\s*(\d+)\s*\)")
single = re.compile(r"REL::ID\(\s*(\d+)\s*\)")

seen: set[tuple[Path, int, str, int]] = set()
for root in args.roots:
    files = root.rglob("*") if root.is_dir() else (root,)
    for path in files:
        if not path.is_file() or path.suffix.lower() not in {".h", ".hpp", ".cpp", ".cxx"}:
            continue
        try:
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        except OSError:
            continue
        for line_number, line in enumerate(lines, 1):
            spans = []
            for match in pair.finditer(line):
                selected = int(match.group(2))
                spans.append(match.span())
                if selected not in records:
                    seen.add((path, line_number, f"pair {match.group(1)}/{selected}", selected))
            for match in single.finditer(line):
                if any(start <= match.start() < finish for start, finish in spans):
                    continue
                selected = int(match.group(1))
                if selected not in records:
                    seen.add((path, line_number, f"single {selected}", selected))

for path, line_number, kind, selected in sorted(seen, key=lambda item: (str(item[0]), item[1], item[3])):
    print(f"{path}:{line_number}: {kind} missing")
print(f"missing_literal_selectors={len(seen)}")
