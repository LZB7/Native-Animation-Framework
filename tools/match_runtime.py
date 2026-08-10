#!/usr/bin/env python3
"""Cross-match NAF's legacy Fallout 4 functions against a newer runtime.

Function boundaries come from the PE exception directory. Fingerprints preserve
instruction and register semantics while removing image-relative addresses.
Results are candidates for manual disassembly review, never automatic proof.
"""

from __future__ import annotations

import argparse
import collections
import difflib
import hashlib
import re
import struct
from dataclasses import dataclass
from pathlib import Path

import pefile
from capstone import CS_ARCH_X86, CS_MODE_64, Cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG, X86_REG_RIP


ID_RE = re.compile(r"REL::ID\(\s*(\d+)\s*\)")
PAIR_RES = (
    re.compile(r"RelocationID\s*[\({]\s*(\d+)\s*,\s*(\d+)"),
    re.compile(r"SelectVersionID\s*\(\s*(\d+)\s*,\s*(\d+)"),
)


def read_database(path: Path) -> dict[int, int]:
    data = path.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    if len(data) != 8 + count * 16:
        raise ValueError(f"invalid Address Library size: {path}")
    return dict(struct.iter_unpack("<QQ", memoryview(data)[8:]))


def source_ids(root: Path) -> dict[int, list[str]]:
    result: dict[int, list[str]] = {}
    for path in root.rglob("*"):
        if path.suffix.lower() not in {".h", ".hpp", ".cpp"}:
            continue
        rel = path.relative_to(root).as_posix()
        for line_no, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
            for match in ID_RE.finditer(line):
                result.setdefault(int(match.group(1)), []).append(f"{rel}:{line_no}")
    return result


def anchor_pairs(roots: list[Path], old_db: dict[int, int], new_db: dict[int, int]):
    result = {}
    for root in roots:
        for path in root.rglob("*"):
            if path.suffix.lower() not in {".h", ".hpp", ".cpp"}:
                continue
            text = path.read_text(errors="replace")
            for pattern in PAIR_RES:
                for old_id, new_id in pattern.findall(text):
                    old_id, new_id = int(old_id), int(new_id)
                    if old_id in old_db and new_id in new_db:
                        result[(old_id, new_id)] = (old_db[old_id], new_db[new_id])
    return result


@dataclass(frozen=True)
class Function:
    begin: int
    end: int
    tokens: tuple[str, ...]

    @property
    def size(self) -> int:
        return self.end - self.begin

    @property
    def digest(self) -> str:
        return hashlib.sha256("\n".join(self.tokens).encode()).hexdigest()

    @property
    def coarse_tokens(self) -> tuple[str, ...]:
        result = []
        for token in self.tokens:
            mnemonic, _, operands = token.partition(":")
            shapes = []
            for operand in operands.split(",") if operands else ():
                if operand.startswith("M:rsp") or operand.startswith("M:rbp"):
                    shapes.append("STACK")
                elif operand.startswith("M:rip"):
                    shapes.append("RIP")
                elif operand.startswith("M:"):
                    shapes.append("MEM")
                elif operand.startswith("I:"):
                    shapes.append(operand if mnemonic in {"cmp", "test", "and", "or"} else "IMM")
                elif operand in {"IMAGE", "IMM"}:
                    shapes.append("IMM")
                else:
                    shapes.append("REG")
            result.append(f"{mnemonic}:{','.join(shapes)}")
        return tuple(result)


def operand_token(insn, operand, image_base: int, image_size: int) -> str:
    if operand.type == X86_OP_REG:
        return insn.reg_name(operand.reg)
    if operand.type == X86_OP_IMM:
        value = operand.imm
        if image_base <= value < image_base + image_size:
            return "IMAGE"
        if -0x1000 <= value <= 0x1000:
            return f"I:{value}"
        return "IMM"
    if operand.type == X86_OP_MEM:
        mem = operand.mem
        base = insn.reg_name(mem.base) if mem.base else ""
        index = insn.reg_name(mem.index) if mem.index else ""
        if mem.base == X86_REG_RIP:
            disp = "IMAGE"
        elif -0x200 <= mem.disp <= 0x200:
            disp = str(mem.disp)
        else:
            disp = "D"
        return f"M:{base}:{index}:{mem.scale}:{disp}"
    return f"O:{operand.type}"


def load_functions(path: Path) -> tuple[pefile.PE, dict[int, Function]]:
    pe = pefile.PE(str(path), fast_load=False)
    base = pe.OPTIONAL_HEADER.ImageBase
    size = pe.OPTIONAL_HEADER.SizeOfImage
    md = Cs(CS_ARCH_X86, CS_MODE_64)
    md.detail = True
    md.skipdata = True
    functions: dict[int, Function] = {}
    for entry in pe.DIRECTORY_ENTRY_EXCEPTION:
        begin = entry.struct.BeginAddress
        end = entry.struct.EndAddress
        if end <= begin:
            continue
        tokens = []
        for insn in md.disasm(pe.get_data(begin, end - begin), base + begin):
            if insn.id == 0:  # Capstone skip-data pseudo instruction
                continue
            operands = ",".join(operand_token(insn, op, base, size) for op in insn.operands)
            tokens.append(f"{insn.mnemonic}:{operands}")
        if tokens:
            functions[begin] = Function(begin, end, tuple(tokens))
    return pe, functions


def containing(functions: dict[int, Function], rva: int) -> Function | None:
    exact = functions.get(rva)
    if exact:
        return exact
    # Only 47 lookups are performed, so a linear fallback is acceptable.
    return next((fn for fn in functions.values() if fn.begin <= rva < fn.end), None)


def ngrams(tokens: tuple[str, ...], n: int = 3) -> set[tuple[str, ...]]:
    if len(tokens) < n:
        return {(tokens,)}
    return {tokens[i : i + n] for i in range(len(tokens) - n + 1)}


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", type=Path, required=True)
    ap.add_argument("--old-exe", type=Path, required=True)
    ap.add_argument("--new-exe", type=Path, required=True)
    ap.add_argument("--old-db", type=Path, required=True)
    ap.add_argument("--new-db", type=Path, required=True)
    ap.add_argument("--top", type=int, default=5)
    ap.add_argument("--anchor-root", type=Path, action="append", default=[])
    ap.add_argument("--local", action="store_true", help="rank functions near projections from known ID pairs")
    args = ap.parse_args()

    old_db = read_database(args.old_db)
    new_db = read_database(args.new_db)
    new_offset_to_ids: dict[int, list[int]] = collections.defaultdict(list)
    for ident, offset in new_db.items():
        new_offset_to_ids[offset].append(ident)
    ids = source_ids(args.source)
    _, old_functions = load_functions(args.old_exe)
    _, new_functions = load_functions(args.new_exe)
    print(f"old functions={len(old_functions)} new functions={len(new_functions)}")

    anchors = anchor_pairs(args.anchor_root, old_db, new_db)
    anchor_values = list(anchors.items())

    if args.local:
        for ident in sorted(ids):
            old_rva = old_db.get(ident)
            fn = containing(old_functions, old_rva) if old_rva is not None else None
            print(f"\nID {ident} old_rva={old_rva and hex(old_rva)} locations={';'.join(ids[ident])}")
            if fn is None or not anchor_values:
                print("  no bounded legacy function or anchor evidence")
                continue
            nearest = sorted(anchor_values, key=lambda item: abs(item[1][0] - fn.begin))[:2]
            ranked_by_begin = {}
            for (old_id, new_id), (anchor_old, anchor_new) in nearest:
                projection = anchor_new + (fn.begin - anchor_old)
                distance = abs(fn.begin - anchor_old)
                radius = min(0x40000, max(0x8000, distance // 2))
                print(
                    f"  anchor {old_id}->{new_id} old={anchor_old:#x} new={anchor_new:#x} "
                    f"distance={fn.begin-anchor_old:+#x} projection={projection:#x} radius={radius:#x}"
                )
                for candidate in new_functions.values():
                    if abs(candidate.begin - projection) > radius:
                        continue
                    ratio = difflib.SequenceMatcher(None, fn.coarse_tokens, candidate.coarse_tokens, autojunk=False).ratio()
                    size_ratio = min(fn.size, candidate.size) / max(fn.size, candidate.size)
                    score = ratio * 0.85 + size_ratio * 0.15
                    previous = ranked_by_begin.get(candidate.begin)
                    if previous is None or score > previous[0]:
                        ranked_by_begin[candidate.begin] = (score, ratio, size_ratio, candidate)
            ranked = sorted(ranked_by_begin.values(), reverse=True, key=lambda item: item[0])
            for score, ratio, size_ratio, candidate in ranked[: args.top]:
                print(
                    f"  LOCAL new={candidate.begin:#x}-{candidate.end:#x} ids={new_offset_to_ids.get(candidate.begin, [])} "
                    f"score={score:.4f} sequence={ratio:.4f} length={size_ratio:.4f}"
                )
        return

    digest_index: dict[str, list[Function]] = collections.defaultdict(list)
    gram_index: dict[tuple[str, ...], set[int]] = collections.defaultdict(set)
    for fn in new_functions.values():
        digest_index[fn.digest].append(fn)
        for gram in ngrams(fn.coarse_tokens):
            gram_index[gram].add(fn.begin)

    for ident in sorted(ids):
        old_rva = old_db.get(ident)
        fn = containing(old_functions, old_rva) if old_rva is not None else None
        print(f"\nID {ident} old_rva={old_rva and hex(old_rva)} locations={';'.join(ids[ident])}")
        if fn is None:
            print("  no PE unwind function contains the legacy RVA")
            continue
        print(f"  old_function={fn.begin:#x}-{fn.end:#x} size={fn.size:#x} delta={old_rva-fn.begin:#x} tokens={len(fn.tokens)}")
        exact = digest_index.get(fn.digest, [])
        if exact:
            for candidate in exact:
                print(f"  EXACT new={candidate.begin:#x}-{candidate.end:#x} ids={new_offset_to_ids.get(candidate.begin, [])}")
            continue

        old_grams = ngrams(fn.coarse_tokens)
        votes: collections.Counter[int] = collections.Counter()
        for gram in old_grams:
            for begin in gram_index.get(gram, ()):
                votes[begin] += 1
        ranked = []
        for begin, shared in votes.most_common(250):
            candidate = new_functions[begin]
            cand_grams = ngrams(candidate.coarse_tokens)
            union = len(old_grams | cand_grams)
            score = shared / union if union else 0.0
            length_ratio = min(fn.size, candidate.size) / max(fn.size, candidate.size)
            combined = score * 0.8 + length_ratio * 0.2
            ranked.append((combined, score, length_ratio, candidate))
        ranked.sort(reverse=True, key=lambda item: item[0])
        for combined, score, length_ratio, candidate in ranked[: args.top]:
            print(
                f"  CAND new={candidate.begin:#x}-{candidate.end:#x} ids={new_offset_to_ids.get(candidate.begin, [])} "
                f"combined={combined:.4f} ngram={score:.4f} length={length_ratio:.4f}"
            )


if __name__ == "__main__":
    main()
