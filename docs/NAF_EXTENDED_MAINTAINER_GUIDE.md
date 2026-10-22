# NAF Extended maintainer guide

## Purpose and lineage

NAF Extended is a community-maintained continuation and expansion built on the
complete NAF 1.0.14 preservation port for Fallout 4 1.11.240. Original author
Snapdragon acknowledged the compatibility port and confirmed GPLv3 distribution
of modified versions. Extended remains independently maintained. The
preservation branch is the behavioral reference; Extended adds maintainable
address resolution and opt-in integration surfaces without replacing the
original scene, authoring, Papyrus, or serialization architecture.

Read `PORTING.md` for the preservation evidence ledger and
`PORTING_EXTENDED.md` for phase history. This guide is the subsystem map.

## Runtime and address resolution

- `f4se-plugin/src/main.cpp` owns plugin metadata, startup ordering, hooks,
  messaging, serialization, Papyrus registration, and API initialization.
- `f4se-plugin/src/Compat/Engine.h` contains the remaining NAF-specific engine
  compatibility wrappers and critical byte validation.
- `f4se-plugin/extern/CommonLibF4` is the pinned CommonLibF4RD fork. Runtime
  Database is the only relocation backend; Address Library fallback is
  intentionally disabled.
- Direct hooks still validate instruction shape, decoded targets, executable
  range, prologues, and ABI assumptions. Runtime Database supplies identity and
  location, not permission to skip hook-site validation.

The plugin advertises layout dependence and explicitly supports Fallout 4
1.11.240/F4SE 0.7.9. A future runtime needs independent layout and hook-site
verification even when Runtime Database can resolve every symbol. Optional
engine paths must remain lazy so an unavailable optional mapping cannot abort
startup.

## Native API

Public ABI: `f4se-plugin/include/NAFExtendedAPI.h`  
Provider: `f4se-plugin/src/API/ExtendedAPI.h`  
Examples: `docs/api/`  
Harness: `tools/NAFExtAPITest/`

The header is C-compatible and depends only on standard integer/size headers.
Consumers discover exports (`NAFExt_GetAPI`, `NAFExt_GetFaceAPI`, and
`NAFExt_GetFaceAPIv2`) or the F4SE `NAFX` announcement, then validate structure
size, version, capability bits, required function pointers, and provider identity.

Scene callbacks are delivered from NAF's verified game-loop processing path.
Callback view pointers are borrowed and expire on return. Retained data must be
copied through the size/copy API, whose packed result contains offsets and is
caller-owned. A nonzero requested revision returning `NAFEXT_E_BUSY` means the
scene changed between sizing and copying. Enumeration supports consumers that
load late; its registration epoch changes across reset/revert boundaries.

Unregistration prevents future delivery, but an already-entered callback may
finish. Consumers own their callback context and must keep it alive until that
call returns. Never invoke consumer code while holding a NAF scene lock.

ABI obligations: preserve the C calling convention, enum values, structure
prefixes, capability meanings, and existing function-table layout. Extend by a
new version or append-only negotiated structure—never by silently reordering v1.

## JSON animations and pack cache

Implementation: `f4se-plugin/src/Data/JSONAnimation.h`  
Schemas: `docs/NAF_EXTENDED_JSON_SCHEMA_V2.md` and
`docs/NAF_EXTENDED_JSON_SCHEMA_V1_EXPERIMENTAL.md`
Tests: `tools/audit_json_animation.py`, `tools/audit_json_schema_v2.py`, and
`tools/audit_phase2c.py`

Discovery finds `.naf.json` sources under NAF user data. Explicit format/version
dispatch preserves v1 and normalizes v2 package documents into pointer-free
animation, position, group/stage, and recursive-tree collections. Registration
is phased across all lexically ordered documents: animations, groups, trees,
then positions. This permits forward references without making parser order a
runtime property. XML registers first and wins; reservation sets preserve
first-JSON-wins behavior across source and array order. V1 retains its atomic
animation/position collision boundary, while v2 isolates invalid definitions
and their dependents.

Package defaults apply only to skeleton, legacy game sex, expected body type,
and additive tags. Role and tri-state erection remain explicit per slot. IDs
are global and never silently package-qualified. Scoped package, animation,
slot, position, group, active-stage, tree, and active-node metadata is composed
into the existing ABI-v1 snapshot metadata array; no public ABI changed.

Actor-slot role normalization is case-insensitive and accepts only `giver`,
`receiver`, and `solo`. The field remains optional and is copied as scoped
metadata; it is not consulted by eligibility, assignment, grouping, trees, or
playback. Adding a role spelling therefore does not alter cache or API layouts.

`NAFPackCache.bin` format version 2 stores a versioned CBOR encoding of
normalized documents plus a source manifest (relative path, size, modification
value, and content fingerprint). Header, version, bounds, checksum, manifest,
or decode failure rejects the whole cache and rebuilds from source. Atomic
replacement avoids publishing partial data. Cached objects never contain engine
pointers, handles, locks, or other process-local state. V1 and v2 share the
cache, and hot reload clears every scoped metadata map before reconstruction.

## Managed face and expressions

Implementation: `f4se-plugin/src/FaceAnimation/FaceUpdateHook.h`  
Preset parser: `f4se-plugin/src/Data/ExpressionPreset.h`  
Contracts: `docs/NAF_EXTENDED_FACIAL_API_V1.md` and
`docs/NAF_EXTENDED_DYNAMIC_EXPRESSIONS.md`

Legacy stop ends authored playback but preserves upstream stop semantics.
Explicit reset is stronger: it cancels current ownership, clears NAF eye and
dynamic-expression state, neutralizes the composed output, and lets vanilla
face/lip processing resume. Reset can defer until face data is ready.

Asynchronous authored loads carry a monotonically increasing request generation.
Only the current generation may publish, preventing an older load from reviving
state after stop/reset/replacement. Facial API v1 exposes authored play/stop,
reset, and query. v2 repeats that prefix and appends dynamic preset request,
intensity, release, and query.

Dynamic expressions overlay selected `finalExp` channels during the verified
FaceGen update context while retaining vanilla lip/eye behavior. They are
transient and intentionally absent from historical FACE serialization. The
removed experimental `FRST` record must not be restored: reset intent is an
operation, not persistent facial state.

## Furniture aggregation

Implementation: `f4se-plugin/src/Data/User/Furniture.h` and
`f4se-plugin/src/Data/Global.h`  
Policy: `docs/NAF_EXTENDED_FURNITURE_MERGING.md`

Original generic registry insertion replaced or ignored a complete group on an
ID collision. Extended uses an additive furniture-only map: all form and keyword
contributions survive, while one deterministic priority/source winner supplies
group-level start/stop metadata. Text identities are normalized and deduplicated
before linking; resolved pointers are deduplicated afterward. Sorting removes
parallel-parser arrival order from the result. Cold source loads, cache loads,
and configuration reloads must all rebuild through the same merge path.

## Build and release invariants

- Target: Fallout 4 1.11.240, F4SE 0.7.9, MSVC v143, Release/x64.
- Configure with `COPY_BUILD=OFF`; release builds should also set
  `BUILD_NAFEXT_API_TEST=OFF`.
- Runtime Database is installed separately and must not be packaged.
- Release files are `NAF.dll` and the matching Extended `NAF.pex`. Publish
  `NAF.psc` for Papyrus authors. Original NAF supplies unchanged assets.
- Do not ship fixtures, harness DLLs, presets used only by tests, PDBs, caches,
  Runtime Database data, build directories, or reference binaries.

## Required regression checks

Run `tools/audit_full_parity.py` plus the Extended audits for Runtime Database,
native API, JSON/cache, facial reset/APIs/expressions, and furniture merging.
For runtime changes, cover startup, a solo/paired/staged scene, player FreeCam,
active-scene save/load, Creator/Studio, managed face, and clean teardown. For API
changes, run the independent harness through discovery, negotiation, late
enumeration, snapshots, callback ordering, and unregistration. For loader/cache
changes, test cold, warm, corrupt, stale, and reload paths.

The preservation branches and historical v5 record layout are not development
shortcuts: keep them unchanged unless a separately scoped compatibility task
demonstrates a defect.
