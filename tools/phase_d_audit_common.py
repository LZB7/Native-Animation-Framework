#!/usr/bin/env python3
"""Shared source guards for the NAF 1.0.14 Phase-D authoring audits."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def audit(name: str, guards: dict[str, tuple[str, ...]], forbidden: tuple[str, ...] = ()) -> int:
    failures: list[str] = []
    combined = ""
    count = 0
    for relative, tokens in guards.items():
        source = (ROOT / relative).read_text(encoding="utf-8", errors="replace")
        combined += source
        for token in tokens:
            count += 1
            if token not in source:
                failures.append(f"{relative}: missing {token!r}")
    for token in forbidden:
        if token in combined:
            failures.append(f"retains forbidden selector/gate {token!r}")
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print(f"PASS: {name}: {count} source/lifecycle guards matched")
    return 0
