#!/usr/bin/env python3
"""Bounded host-side JSON v2 source/normalized parsing benchmark."""

import json
import time
import tracemalloc


def package(count: int) -> dict:
    return {
        "format": "NAFExtendedAnimationPack",
        "schemaVersion": 2,
        "pack": {"id": f"benchmark.{count}"},
        "defaults": {"requirements": {"skeleton": "Human"}, "expectedBodyType": "Any"},
        "animations": [
            {"id": f"benchmark.animation.{i}", "actors": [{"file": f"benchmark\\clip{i}.hkx"}]}
            for i in range(count)
        ],
        "positions": [
            {"id": f"benchmark.position.{i}", "animation": f"benchmark.animation.{i}"}
            for i in range(count)
        ],
    }


def timed_load(payload: bytes, repetitions: int = 20) -> float:
    started = time.perf_counter()
    for _ in range(repetitions):
        json.loads(payload)
    return (time.perf_counter() - started) * 1000.0 / repetitions


def main() -> None:
    print("Synthetic host-side benchmark; these are not in-game NAF timings.")
    for count in (10, 100, 1000):
        source = json.dumps(package(count), separators=(",", ":")).encode()
        normalized = json.dumps({"cacheVersion": 2, "schemaVersion": 2, "definitions": [package(count)]}, separators=(",", ":")).encode()
        tracemalloc.start()
        json.loads(source)
        _, peak = tracemalloc.get_traced_memory()
        tracemalloc.stop()
        print(
            f"{count:4d} animations source={len(source):8d} bytes "
            f"source_parse={timed_load(source):8.3f} ms "
            f"normalized_parse={timed_load(normalized):8.3f} ms "
            f"parse_peak={peak:9d} bytes"
        )


if __name__ == "__main__":
    main()
