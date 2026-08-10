# Fallout 4 1.11.221 port

This branch is based on upstream NAF commit
`9d8efbfea0e2f6048a59f7a7af8ccf814dd61791` and targets Fallout 4
1.11.221 with F4SE 0.7.8. It is not yet runtime-ready.

## Dependency baseline

The upstream gitlinks establish these historical revisions:

- Deweh/CommonLibF4: `425588497bc4c176047bd6c77626be18c26da345`
- Deweh/ik: `b1700d9ad080c82143e2fb83e2594604bee1b9bb`
- zeux/pugixml: `6909df2478f7eb092e8e5b5cda097616b2595cc6`

The supplied Deweh/CommonLibF4 and ik archives match their expected commits
after normalizing archive/git line endings. The supplied pugixml archive is a
newer snapshot and is not used. The checked-out historical pugixml gitlink is
retained.

## CommonLibF4 strategy

The port pins alandtse/CommonLibF4 commit
`ba22620e455dffaf1d7021c16b0728dc28f1ddae`. This is a maintained continuation
of the original library that provides explicit legacy/NG relocation IDs and
runtime-selected layouts. The build is restricted to `ENABLE_FALLOUT_NG`.

The PANDA snapshot retains single-ID compatibility in many places. NAF's 47
distinct legacy IDs are all absent from the supplied 1.11.221 Address Library,
so that model cannot resolve NAF as written. Deweh/CommonLibF4-NG contains useful
NG relocation/layout work and is closely related to the maintained continuation,
but its head stopped in 2025, still declares 1.10.984 as latest, and does not map
NAF's custom relocation set. Its six unique post-common-ancestor commits improve
`hkRefPtr`, `IStackCallbackFunctor`, `NiQuaternion`, and a few Scaleform helpers.
Those are useful references for NAF's Havok/math-heavy code, but they do not add
1.11.221 runtime selection or solve the missing IDs. They are not merged merely
because they are newer than the fork point. It is evidence, not the selected
dependency.

The first compiler pass adds an important qualification: the maintained fork
dropped several Deweh APIs that NAF uses, while also adding declarations that
overlap NAF's local RE shims. PANDA retains examples of those Deweh APIs (notably
quaternion helpers and `Actor::GetSex`), but their single legacy IDs are not NG
evidence. The selected strategy therefore remains the maintained NG runtime
core, with only individually audited Deweh/PANDA declarations or local
implementations restored. A wholesale PANDA substitution is still unjustified.

## Compatibility evidence levels

- **Mechanically translated**: an authoritative old/NG ID pair exists and both
  IDs resolve in their respective Address Library databases.
- **Statically verified**: source declarations and the relevant F4SE/CommonLib
  ABI or layout agree, without claiming runtime behavior.
- **Verified by disassembly**: the exact instruction/function in the supplied
  1.11.221 binary has been inspected and matched semantically.
- **Requires runtime testing**: static evidence is sufficient to build, but game
  behavior still has to be exercised.
- **Unresolved**: evidence is insufficient; no value is guessed.

## Current audit

| Area | Status | Evidence / next action |
| --- | --- | --- |
| F4SE plugin discovery | Statically verified | F4SE 0.7.8 rejects DLLs without `F4SEPlugin_Version`; the port exports explicit 1.11.221 metadata. |
| CommonLib runtime selection | Statically verified | NG-only CMake configuration; 1.11.221 version constant added from F4SE 0.7.8 source. |
| NAF relocation IDs | Unresolved except noted mappings | All 47 legacy IDs are absent from the 1.11.221 database. Each requires an authoritative NG pair. |
| `EvaluatePackage` ID 1395257 | Mechanically translated | PANDA records NG ID 2229805 for the same named/signatured `Actor::EvaluatePackage`; NAF now uses the explicit pair. Hook semantics still require runtime testing. |
| Interior hook offsets | Unresolved | IDs 317245+0xBA, 3139+0x158, 338354+0x1E, and 1385889+0xA8 cannot be carried forward. Their legacy IDs do not exist in the NG namespace. |
| Vtables / RTTI | Unresolved | NAF custom types and all `stl::emplace_vtable` uses require NG ID/layout review. |
| Engine structure layouts | Unresolved | NAF's custom RE/Havok structures require field-by-field comparison and runtime tests. |
| Virtual calls | Unresolved | Slots and object layouts require NG validation. |
| LooksMenu 1.7.0.3 binary identity | Statically verified | Supplied and installed DLLs have identical SHA-256 `B562D636D768620760AAD8C166CCFDA22B014F490ABACE0B21E84334B26C56F0`. |
| LooksMenu `ApplyMorphsToShapes` | Verified by disassembly | MSVC RTTI locates the `BodyMorphInterface` vtable at RVA `0xC88B0`; source-declared virtual slot 18 resolves to RVA `0x11400`. A call at RVA `0x5AB19` targets that function. |
| LooksMenu `g_bodyMorphInterface` | Verified by disassembly | The same call site loads `this` from DLL RVA `0x107E00`; constructor/vtable evidence agrees. NAF now uses this RVA only for the exact checksum-gated DLL. |

## Relocation inventory

The following source IDs are unresolved in the NG namespace unless explicitly
listed otherwise above. "Absent" means the numeric legacy ID is not present in
the supplied 1.11.221 database; it does not mean the underlying function was
removed.

| Source area | Legacy IDs | Status |
| --- | --- | --- |
| `BGSAnimationSystemUtils.h` | 1349978, 839650, 672857, 686648, 35074, 177755, 897074, 639576, 22408 | Unresolved; legacy IDs absent |
| `BSAnimationGraphManager.h` | 849404, 424303, 16523, 1530664, 1065709, 1381136, 144578, 194777, 326555, 762973 | Unresolved; functions, global array, custom layouts and virtual traversal require matching |
| `TESObjectREFR_Events.h` | 1448133, 1182113, 1240328, 973940, 33618, 78267 | Unresolved; legacy IDs absent |
| `BSInputEnableManager.h` | 781703, 920956, 230159 | Unresolved; singleton and functions require matching |
| `GraphHook.h` | 1492656, 1216262 | Unresolved; detour entry points require disassembly |
| `FaceUpdateHook.h` | 317245+0xBA, 3139+0x158 | Unresolved; call sites require cross-version matching and disassembly |
| `GameLoopHook.h` | 338354+0x1E | Unresolved; call site requires cross-version matching and disassembly |
| `HUDManager.h` | 1317216, 1385889+0xA8 | Unresolved; function and call site require matching |
| `EvalHook.h` | 1395257→2229805, 609985 | EvaluatePackage mechanically translated; CheckForNewPackage unresolved |
| `CamHook.h` | 541110 | Unresolved; detour entry point requires matching |
| Other custom RE wrappers | 69733, 652173, 356321, 1105206, 1225688, 1286348, 992878 | Unresolved; legacy IDs absent |
| Studio settings/globals | 1302430, 644641 | Unresolved; global type and location require matching |

`tools/match_runtime.py` now performs PE-unwind-bounded candidate searches using
authoritative old/NG pairs from maintained headers as regional anchors. Its
current fuzzy results are search hints only: no additional ID or interior-hook
mapping has been promoted from that output because instruction/call semantics
have not yet been established.

CommonLib-provided RTTI/vtable constants used for `TESActionData` and
`BSAnimGraphVisit::BShkbVisitor` now have NG-aware ID representations in the
selected library. This is only mechanical selection. NAF's derived declarations,
slot order, and object layouts remain unresolved and require static comparison
plus runtime testing.

## Development environment and build attempt

- Fallout4.exe reference: independently verified file/product version
  1.10.163.0, SHA-256
  `55F57947DB9E05575122FAE1088F0B0247442F11E566B56036CAA0AC93329C36`.
- Fallout4.exe: 1.11.221.0, SHA-256
  `428F9996CC4248E26C0F62F9FDD3EAF0E5EB305834B67EE5996538E593218B61`.
- F4SE installation: loader and `f4se_1_11_221.dll` version 0.7.8; matching
  `f4se-0.7.8` source archive is present in the game `src` directory.
- Address Library: `version-1-11-221-0.bin` is installed.
- LooksMenu: installed `f4ee.dll` is version 1.7.0.3 and matches the supplied DLL.
- Python: available through the Codex workspace runtime, not normal `PATH`.
- CMake 4.4.2: installed workspace-locally and used for generation.
- Visual Studio 2022 Build Tools 17.14.37516.0: found at
  `D:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools`; MSVC v143
  19.44.35228 and Windows SDK 10.0.28000.0 are usable.
- vcpkg: `C:/Dev/vcpkg`, version 2026-07-27. The manifest's builtin baseline
  resolves successfully.

Configuration now succeeds in `f4se-plugin/build-1.11.221` with the Visual
Studio 2022 generator, NG-only options, and `COPY_BUILD=OFF`. The top-level
manifest needed `rapidcsv`, which CommonLibF4 requires but NAF did not list.
CMake 4 also requires a 3.5 policy floor for the pinned ik revision, and the
MSVC runtime expression was corrected so Release selects `MultiThreadedDLL`.

The first Release compile builds the pinned ik library and CommonLibF4, then
stops while compiling NAF. The initial errors are source/API migration issues:
NAF declarations now duplicate types present in the selected CommonLib
(`BSAnimationGraphEvent`, `BGSAnimationSystemUtils::ActiveSyncInfo`, and math
types), while NAF also relies on Deweh extensions absent from the selected
alandtse header set (`NiQuaternion` helpers and `Actor::GetSex`). This is
evidence that adopting alandtse unchanged is insufficient. No DLL has been
produced; compilation success and runtime compatibility remain separate gates.

The audit helper is maintained at `../tools/audit_runtime.py` in the project
workspace. Resolving the remaining custom legacy IDs without guessing requires
a Fallout4.exe 1.10.163.0 binary for cross-version function matching. A successful
compile is not a runtime compatibility claim.

## Source/API convergence pass (2026-08-10)

The compiler inventory was captured from a clean Release compile in
`f4se-plugin/build-1.11.221/compile-final-stage.log`. CommonLibF4 and the pinned
ik target compile; NAF's unity translation unit then reaches MSVC's 100-error
limit. Linking has not been reached. The errors group as follows:

| Classification | Current examples | Disposition |
| --- | --- | --- |
| Source/API migration with no runtime implication | `NiPoint4::pt`, pointer-handle `native_handle_const`, private enum storage access | Migrated to public maintained APIs. |
| Duplicate/obsolete NAF declaration | geometry/shader types, Papyrus `FunctionArgs`, animation/event/input/effect types | Geometry and `FunctionArgs` duplicates removed from active use. Others retained until semantic/layout differences are resolved. |
| Missing pure Deweh/PANDA helper | quaternion algebra, transform composition/inversion | Reimplemented locally from source evidence. No engine relocation was copied. |
| Expressible through supported CommonLibF4 | `Actor::GetSex` | Replaced with `Actor::GetNPC()` followed by maintained `TESNPC::GetSex()`. |
| Runtime-sensitive dependency | animation graph wrappers, input manager, effect spawning, hooks, camera state, package evaluation, graph project name, idle playback, HUD messages | Left unresolved; these require an authoritative NG relocation, supported replacement, layout/vtable evidence, or focused disassembly. |
| Uncertain | NAF-local hit/event layouts and some scene convenience methods | Kept visible for individual semantic comparison; no casts, stubs, or guessed values added. |

### Safe migrations completed

- Added `Compat/Quaternion.h`. `Add`, `Scale`, `Negate`, multiplication,
  inverse, matrix conversion, angle-axis conversion, SLERP, `Log`, `Exp`, and
  `Intermediate` are engine-independent mathematical implementations derived
  from the Deweh CommonLibF4-NG source and cross-checked against the historical
  Deweh/PANDA interfaces. The current alandtse `NiQuaternion` is storage-only;
  the historical Deweh/PANDA member implementations call engine relocations and
  were deliberately not copied. `FromEulerAnglesXYZ` uses the maintained
  CommonLib `NiMatrix3::FromEulerAnglesXYZ` API (explicit old/NG relocation
  pair) and then performs local matrix-to-quaternion conversion. It therefore
  remains runtime-test-required even though the quaternion conversion itself is
  local.
- Added local, engine-independent coordinate-space transform composition and
  world-to-local inversion in `MathUtil`; removed calls to deleted Deweh
  `NiTransform` convenience methods.
- Replaced all NAF `Actor::GetSex` calls with `actor->GetNPC()->GetSex()` and
  the maintained `RE::SEX` enum. Historical Deweh `Actor::GetSex` calls legacy
  relocation 1216256. The replacement instead obtains the NPC base through the
  maintained object-reference API and calls maintained `TESNPC::GetSex`, which
  carries the explicit pair `(1257181, 2207107)`. No legacy convenience ID was
  imported. This is a semantically equivalent modern API and requires runtime
  testing at the NAF call sites.
- Updated `NiPoint4` indexing and pointer-handle access to their public current
  APIs. Updated scene-state serialization to use `EnumSet::underlying()` while
  preserving the serialized `uint8_t` representation.

### Duplicate declaration audit

- NAF's local `BSGeometry`, `BSTriShape`, `BSShaderMaterial`, and
  `BSLightingShaderProperty` declarations are no longer included. Geometry
  reading now uses maintained CommonLib names/types and renderer vertex data.
  The basic geometry sizes/offsets used by NAF agree, but an ABI warning was
  found: the old local lighting-shader declaration placed projected-UV color
  and parameter fields in the opposite 0x70/0x80 order from maintained
  CommonLib. NAF does not directly access those fields in the migrated code.
  Removing the active local declaration also removes its unresolved
  `QShaderProperty` relocation 1286348.
- NAF's Papyrus `FunctionArgs` copy was byte-for-byte duplicative of the
  maintained header and used the same legacy-only ID 69733. The local header is
  removed and `GameUtil` includes the maintained declaration. ID 69733 itself
  is still unresolved; eliminating a duplicate declaration is not evidence that
  the engine call works on 1.11.221.
- `BSAnimationGraphEvent` is retained as a blocker: both declarations are 0x18,
  but NAF expects a `TESObjectREFR*` holder whereas current CommonLib declares a
  numeric holder ID, with additional const/semantic differences.
- Local `ReferenceEffect` cannot be merged by name: NAF's declaration derives
  directly from `NiObject`, while maintained `ReferenceEffect` derives from
  `BSTempEffect` and is 0x48. NAF only uses the local declaration to reach the
  unresolved transient-hit-effect engine function (legacy ID 652173).
- Animation graph manager/cache, input manager, and hit-event declarations are
  retained pending layout and call-semantics verification. Name or equal size
  alone is not being treated as ABI equivalence.

### Current compiler boundary

The next exact engine-facing blockers are:

- unresolved BGS animation-system and animation-graph manager functions,
  globals, visitor layouts, and event-holder semantics;
- `BSInputEnableManager` singleton/functions;
- transient hit-effect spawning (legacy ID 652173) and effect play/stop;
- custom hit/event layouts and dispatch wrappers;
- interior hook sites, including HUD `1385889+0xA8`, plus camera-state layout
  and camera hook 541110;
- graph lock/manager layout and `GetAnimationGraphProjectName`;
- historical `TESNPC::GetHeight` (legacy ID 674794), whose actor/race-dependent
  behavior is not equivalent merely to reading the maintained height fields;
- package/process methods (`ClearCurrentPackage`,
  `ClearCurrentDataForProcess`, `EvaluatePackage`), special-idle playback, HUD
  message clearing, and scene convenience APIs that may touch engine state.

These are the boundary requested for this stage: further progress on them would
require focused source comparison or binary analysis. The compiler also exposed
that several maintained convenience methods still contain legacy-only
`REL::ID` values under an NG-only build; their presence in a maintained header
is not accepted as 1.11.221 verification.

### Pinned ik source cleanliness

The historical ik CMake script increments and rewrites
`ik/cmake/build_number.txt` in its source directory (and its `file(STRINGS)`
read also consumes the historical comment). NAF now copies the exact pinned ik
source into the build tree and configures that copy. The original submodule is
unchanged and remains clean after builds; its expected numeric value was not
modified. The historical script may still warn while operating on the build
copy, but it no longer dirties pinned dependency content.
