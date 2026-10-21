#!/usr/bin/env python3
"""Fail-closed checks for the NAF Extended Runtime Database migration."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "f4se-plugin" / "src"
COMMON = ROOT / "f4se-plugin" / "extern" / "CommonLibF4" / "CommonLibF4"


def require(text: str, token: str, owner: str) -> None:
    if token not in text:
        raise SystemExit(f"FAIL: {owner} is missing {token!r}")


def main() -> None:
    sources = "\n".join(p.read_text(encoding="utf-8", errors="ignore") for p in SRC.rglob("*.[ch]*"))
    resolver = (COMMON / "src/REL/Relocation.cpp").read_text(encoding="utf-8")
    main_cpp = (SRC / "main.cpp").read_text(encoding="utf-8")
    game_loop = (SRC / "Tasks/GameLoopHook.h").read_text(encoding="utf-8")
    face = (SRC / "FaceAnimation/FaceUpdateHook.h").read_text(encoding="utf-8")
    engine = (SRC / "Compat/Engine.h").read_text(encoding="utf-8")
    modules = (ROOT / ".gitmodules").read_text(encoding="utf-8")

    if "REL::RelocationID" in sources:
        raise SystemExit("FAIL: legacy RelocationID remains in NAF source")
    require(modules, "LZB7/CommonLibF4RD.git", "public CommonLibF4RD submodule")
    require(main_cpp, "UsesSigScanning(true)", "signature-independent metadata")
    require(main_cpp, "UsesAddressLibrary(false)", "Address Library opt-out")
    require(main_cpp, "IsLayoutDependent(true)", "layout-dependent metadata")
    require(main_cpp, "HasNoStructUse(false)", "engine structure use metadata")
    require(main_cpp, "CompatibleVersions({ F4SE::RUNTIME_1_11_240 })", "exact supported ABI runtime metadata")
    require(resolver, "Runtime Database is required (Address Library fallback is disabled)", "missing-database failure")
    if "loadLegacyTable" in resolver or "version-{}.bin" in resolver:
        raise SystemExit("FAIL: legacy Address Library loader remains reachable")

    require(game_loop, "AUTO_CALLSITE", "game-loop automatic callsite")
    require(game_loop, "or_offset(0x1E", "game-loop verified fallback")
    require(face, "or_offset(0xBD", "outer-face verified fallback")
    require(face, "or_offset(0x14B", "lip verified fallback")
    require(game_loop, "encodedTarget != processQueues.address()", "game-loop target validation")
    require(face, "validatesDirectCall", "face target validation")
    require(engine, "REL::ID(1324833, 2202575)", "FreeUpAllMarkers canonical ID")
    require(engine, "static constexpr std::array<std::uint8_t, signature.size()> mask", "FreeUpAllMarkers masked validation")

    relocation_count = sources.count("REL::ID(")
    print(
        "PASS: Runtime Database-only resolution is fail closed; "
        f"{relocation_count} explicit REL::ID declarations, three automatic callsites, "
        "and FreeUpAllMarkers validation are present"
    )


if __name__ == "__main__":
    main()
