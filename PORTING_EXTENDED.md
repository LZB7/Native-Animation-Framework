# NAF Extended port notes

## Runtime Database migration

NAF Extended branches from preservation commit `d39f3225e9a2d91799aa2ccb2a4b6c0e060804b8` on `extended/1.11.240`. The dependency starts from CommonLibF4RD `b092bcc70f690b2c51b980c0b251fa56bf2d3460` and retains the preservation fork's verified RE/F4SE declarations. This hybrid is necessary because upstream CommonLibF4RD does not contain the complete NAF-facing ABI surface or the port's verified Get3D, Papyrus, FaceGen, actor, graph, and singleton corrections.

The relocation backend is Runtime Database-only. `Data/F4SE/Plugins/f4rd-runtime.bin` (or its exact-version variant) is required. The CommonLibF4RD legacy `version-*.bin` loader was removed; Address Library cannot silently satisfy a missing Runtime Database. Missing/corrupt database data and unresolved required IDs fail closed with a diagnostic.

All NAF declarations now use `REL::ID`. The game-loop, outer-face, and lip-morph interior hooks use `AUTO_CALLSITE(target).or_offset(...)`: discovery must be unique, while the known 1.11.221/1.11.240 offset remains an exact-version fallback. Existing opcode, decoded-target, executable-range, prologue, and ABI validation remains in place. `FreeUpAllMarkers` resolves canonically as `REL::ID(1324833, 2202575)` and retains its masked 43-byte entry check; there is no raw-RVA fallback.

F4SE metadata advertises signature-based address independence, explicitly disables Address Library, retains layout dependence, explicitly allowlists the verified Fallout 4 `1.11.240.0` ABI, and requires F4SE 0.7.9. The first Runtime Database build omitted that compatible-layout entry and was rejected by F4SE before plugin initialization; the runtime allowlist fixes loader acceptance without reintroducing runtime-specific address selection. Runtime flexibility does not imply unverified layout independence. Optional subsystems remain lazily resolved.

The first loader-accepted build reached `kGameDataReady` but crashed while checking for `NAF.esp`. Runtime Database ID `4796135` resolved correctly to the verified 1.11.240 `g_TESDataHandler` RVA `0x30E7100`. An initial defensive correction made the inherited full, light, and VR file-collection lookup loops skip null `TESFile*` slots, but the follow-up crash proved that null elements were not the root cause: the collection object itself was being addressed incorrectly.

A follow-up crash at the same source path exposed a deeper merge-contract issue: CommonLibF4RD's two-way `RelocateMember` and `RelocateVirtual` helpers interpreted their arguments as OG/AE, while the inherited CommonLibF4 headers pass SE-or-AE/VR offsets and vtable slots. Consequently `GetCompiledFileCollection(this, 0xFC0, 0)` selected offset zero on AE and treated the start of `TESDataHandler` as a file collection. The no-version two-way helpers now retain the preservation CommonLibF4 SE/VR contract; explicit versioned two/three-family overloads remain available for genuine OG/NG/AE selection. The bounded audit found ten inherited member-layout callsites and fifteen virtual-dispatch callsites relying on the restored SE/VR meaning, including `TESDataHandler`, `NiNode`, `NiAVObject`, and `BSFadeNode`.

Build: Release/x64/MSVC v143, `COPY_BUILD=OFF`. Output remains under `NAF Extended/Codex Production Files`; nothing is staged to Nexus or deployed. Runtime acceptance requires 1.11.240 startup without Address Library, clean failure with Runtime Database absent even if Address Library is present, all hook installations, HUD/Papyrus, solo/paired/staged scenes, FreeCam cleanup, face/lip, NANIM, active-scene persistence, inventory/highlighting, Creator/Studio, serialization, and shutdown. Diagnostic `.trace`, `.mapping`, and `.mapping.fail` marker files are development-only and must not ship.

## Native API architecture

Phase 2 is design-only. See `docs/NAF_EXTENDED_NATIVE_API_PROPOSAL.md` and `docs/api/NAFExtendedAPI_draft.h`. No API provider, facial reset, furniture merge, or other gameplay feature is implemented in this migration.
