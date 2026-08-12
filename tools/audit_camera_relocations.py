#!/usr/bin/env python3
"""Verify the bounded Fallout 4 1.11.221 production camera surface."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


EXPECTED = {
    "FreeCameraState::Update": (2248280, 0x1022040),
    "FreeCameraState vtable": (21725, 0x25A26F8),
    "TESCameraState vtable": (67299, 0x2475288),
    "TESCamera::Update": (2214740, 0x827970),
    "PlayerCamera singleton": (4796065, 0x30DBDD8),
    "PlayerCamera::ToggleFreeCameraMode": (2248368, 0x1027590),
    "PlayerCamera::Force3rdPerson": (2248348, 0x1026A40),
    "PlayerCamera::SetState": (2214742, 0x827AC0),
    "Main::WorldRootCamera": (2228956, 0xC37A40),
    "NiCamera::LookAtWorldPoint": (2270336, 0x16D0960),
}

SOURCE_GUARDS = {
    "f4se-plugin/src/CamHook/CamHook.h": (
        "using UpdateSig = void(RE::FreeCameraState*, RE::BSTSmartPointer<RE::TESCameraState>&)",
        "REL::RelocationID(541110, 2248280)",
        "REL::RelocationID(0, 2270336)",
        "RE::Main::WorldRootCamera()",
    ),
    "f4se-plugin/extern/CommonLibF4/CommonLibF4/include/RE/Bethesda/TESCamera.h": (
        "REL::RelocationID(1171980, 4796065)",
        "REL::RelocationID(224913, 2248368)",
        "REL::RelocationID(858847, 2214742)",
        "bool stateActive;  // 24",
    ),
    "f4se-plugin/src/Compat/Engine.h": (
        "REL::RelocationID(162201, 2248348)",
    ),
    "f4se-plugin/src/Misc/GameUtil.h": (
        "camera->GetState(RE::CameraState::kFree)",
        "camera->ToggleFreeCameraMode(false)",
        "camera->currentState.get() == freeState.get()",
    ),
    "f4se-plugin/src/Scene/IScene.h": (
        "GetInputsToDisableForPlayerFreeCameraScene",
        "~static_cast<uint32_t>(USER_EVENT_FLAG::kMovement)",
    ),
    "f4se-plugin/src/Scene/SceneBase.h": (
        "GameUtil::SetFlyCam(true)",
        "GetInputsToDisableForPlayerFreeCameraScene()",
        "GameUtil::SetFlyCam(false)",
    ),
}


def read_database(path: Path) -> dict[int, int]:
    data = path.read_bytes()
    count = struct.unpack_from("<Q", data)[0]
    if len(data) != 8 + count * 16:
        raise ValueError(f"invalid Address Library size: {path}")
    return dict(struct.iter_unpack("<QQ", memoryview(data)[8:]))


def read_pe_data(path: Path, rva: int, size: int) -> tuple[bytes, int]:
    data = path.read_bytes()
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    section_count = struct.unpack_from("<H", data, pe_offset + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe_offset + 20)[0]
    optional = pe_offset + 24
    if struct.unpack_from("<H", data, optional)[0] != 0x20B:
        raise ValueError("expected a PE32+ executable")
    image_base = struct.unpack_from("<Q", data, optional + 24)[0]
    section_table = optional + optional_size
    for index in range(section_count):
        section = section_table + index * 40
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", data, section + 8)
        if virtual_address <= rva < virtual_address + max(virtual_size, raw_size):
            offset = raw_offset + rva - virtual_address
            return data[offset:offset + size], image_base
    raise ValueError(f"RVA 0x{rva:X} is outside PE sections")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("database", type=Path)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    database = read_database(args.database)
    failures: list[str] = []

    for owner, (reloc_id, expected_rva) in EXPECTED.items():
        actual = database.get(reloc_id)
        if actual != expected_rva:
            failures.append(
                f"{owner}: ID {reloc_id}, expected 0x{expected_rva:X}, "
                f"got {'missing' if actual is None else f'0x{actual:X}'}"
            )

    # TESCameraState inherits the nine BSInputEventUser virtuals.  Update is
    # therefore physical slot 0x0B; FreeCameraState overrides that exact slot.
    pointer_data, image_base = read_pe_data(args.executable, EXPECTED["FreeCameraState vtable"][1] + 0x0B * 8, 8)
    update_pointer = struct.unpack("<Q", pointer_data)[0]
    update_rva = update_pointer - image_base
    if update_rva != EXPECTED["FreeCameraState::Update"][1]:
        failures.append(
            f"FreeCameraState physical vslot 0x0B: expected RVA 0x1022040, got 0x{update_rva:X}"
        )

    expected_prologue = bytes.fromhex("48 89 7C 24 20 55 48 8D 6C 24 A9 48 81 EC 90 00 00 00")
    actual_prologue, _ = read_pe_data(args.executable, EXPECTED["FreeCameraState::Update"][1], len(expected_prologue))
    if actual_prologue != expected_prologue:
        failures.append(
            "FreeCameraState::Update prologue mismatch: "
            f"expected {expected_prologue.hex(' ')}, got {actual_prologue.hex(' ')}"
        )

    for relative_path, selectors in SOURCE_GUARDS.items():
        source = (args.source_root / relative_path).read_text(errors="replace")
        for selector in selectors:
            if selector not in source:
                failures.append(f"{relative_path}: missing selector {selector}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1

    print(
        f"PASS: {len(EXPECTED)} camera relocation IDs matched exact RVAs; "
        "FreeCameraState physical vslot 0x0B and guarded prologue matched RVA 0x1022040; "
        f"{sum(map(len, SOURCE_GUARDS.values()))} source selectors matched"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
