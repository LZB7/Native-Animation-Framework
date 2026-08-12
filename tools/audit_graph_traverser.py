#!/usr/bin/env python3
"""Ensure legacy visitor helpers remain fail-closed and are not an authoring dependency."""

from pathlib import Path

root = Path(__file__).resolve().parents[1]
declaration = root / "f4se-plugin/src/RE/BSAnimationGraphManager.h"
hits: list[str] = []
for path in (root / "f4se-plugin/src").rglob("*"):
    if path.suffix not in {".h", ".cpp"} or path == declaration:
        continue
    text = path.read_text(encoding="utf-8", errors="replace")
    for token in ("GraphTraverser", "BShkbAnimationGraph::VisitGraph", "BShkbVisitor::Traverse"):
        if token in text:
            hits.append(f"{path.relative_to(root)}: {token}")
if hits:
    for hit in hits:
        print(f"FAIL: unexpected live legacy traversal dependency: {hit}")
    raise SystemExit(1)
print("PASS: Studio/Creator use NodeAnimationGraph; legacy GraphTraverser declarations have no live Phase-D call site")
