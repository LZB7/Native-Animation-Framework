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
| `EvaluatePackage` ID 1395257 | Verified by disassembly | NG ID 2229805 preserves the boolean arguments, virtual slot 0x600 gate, control-flow transfer into the evaluation body, and return behavior. Hook behavior still requires runtime testing. |
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
| `EvalHook.h` | 1395257→2229805, 609985 | EvaluatePackage disassembly-verified; CheckForNewPackage unresolved |
| `CamHook.h` | 541110 | Unresolved; detour entry point requires matching |
| Other custom RE wrappers | 69733, 356321, 1225688, 1286348, 992878 | Unresolved; legacy IDs absent (`1105206`/OpenContainerMenu and `652173`/ApplyEffectShader are resolved below) |
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

## Focused engine-interface recovery (2026-08-10)

This pass began from concrete compiler blockers and did not run a broad
relocation sweep. It established one safe layout migration and one important
limitation of the supplied historical binary.

### Animation graph manager and input declarations

NAF's local `BSAnimationGraphVariableCache` and `BSAnimationGraphManager`
declarations were compared field-by-field with maintained CommonLibF4. The
inheritance, member sequence, offsets (`graph` 0x40, `updateLock` 0xC8,
`activeGraph` 0xD8), alignment, and total sizes (0x40 and 0xE0) agree. NAF now
uses the maintained declarations. This is **statically verified layout/API**;
the contained `BShkbAnimationGraph`/Havok layouts and operations remain
unresolved. Current `hkRefPtr` exposes its pointer as `_ptr`, so graph/root
generator access was migrated without changing ownership or adding an engine
call.

The NAF-local `BSInputEnableManager` declaration had no call sites anywhere in
NAF; it was included only through `ExtraREClasses.h`. It has been removed rather
than reconstructing three unused legacy relocations (781703, 920956, 230159).
Maintained CommonLib does declare the 0x180 manager and implements
`ClearForcedState`/`ForceUserEventEnabled` locally under its lock, but its
singleton still uses legacy-only ID 781703. No singleton mapping is asserted.

### Historical executable is not a static disassembly image

The supplied 1.10.163 executable is version-correct and retains its previously
recorded SHA-256, but its `.text` section is SteamStub-encrypted: `.text` spans
RVA 0x1000-0x2C0BF8F and has measured entropy 8.000 bits/byte. It also contains
the `.bind` section typical of that wrapper. At Address Library ID 1446774
(RVA 0xE35510), the on-disk bytes decode as random/invalid instructions rather
than a function prologue. By contrast, the supplied 1.11.221 image has a normal
`.text` entropy of 6.475 and ID 2231704 at RVA 0xCFAD40 disassembles as a
coherent 0x390-byte function.

Consequently the requested old/new instruction-level comparison cannot be
performed from the current historical file. PE unwind boundaries and similarity
scores derived from encrypted old bytes are not semantic evidence and all such
earlier candidate output is disregarded. Focused analysis tools are retained in
`tools/dump_function.py` and `tools/focused_interfaces.h` so the audit can be
repeated against an unpacked image.

PANDA records the following explicit pairs, and both sides resolve in their
respective Address Library databases:

- `AIProcess::SetupSpecialIdle`: 1446774 -> 2231704
- `AIProcess::ClearCurrentPackage`: 241540 -> 2231582
- `AIProcess::ClearCurrentDataForProcess`: 577581 -> 2232435

These remain **mechanically translated only**. PANDA's declaration is useful
source evidence, and the proposed NG targets contain executable code, but the
historical implementations cannot yet be compared for signature, argument use,
side effects, or call-graph identity. They have therefore not been added to NAF.
PANDA leaves `GetAnimationGraphProjectName` (190119), actor/race-dependent
`TESNPC::GetHeight` (674794), HUD `ClearMessages` (973227), and the input
singleton (781703) as legacy-only IDs as well.

### Unresolved interface table

| NAF call site | Historical mechanism | 1.10.163 evidence | Proposed 1.11.221 mechanism | Grade | Remaining uncertainty |
| --- | --- | --- | --- | --- | --- |
| `GameUtil::AcquireGraphLock` | Direct manager fields and Havok pointers | Deweh layout/source | Maintained manager layout; local Havok traversal | Manager layout statically verified; traversal unresolved | `BShkbAnimationGraph`, character, behavior graph, root generator layouts and called functions |
| `SmartIdle` / `DynamicIdle` | `AIProcess::PlayIdle(actor*, 0x35, idle)` | Historical NAF call plus Deweh wrapper | Maintained `PlayIdle` -> `SetupSpecialIdle`, PANDA pair 1446774->2231704 | Mechanically translated only | Old executable must be decrypted to verify argument mapping, default action and side effects |
| `PackageOverride::Manager` | `ClearCurrentPackage`, `ClearCurrentDataForProcess`, then actor package evaluation | Deweh signatures; old IDs 241540/577581 | PANDA pairs 2231582/2232435; EvaluatePackage pair previously recorded | Mechanically translated only | Instruction/call semantic comparison and operation ordering |
| graph-name users | Engine wrapper ID 190119, output `BSFixedString&` | Deweh/PANDA agree on signature | No established target | Unresolved | Focused old/new function and caller analysis |
| `GameUtil::CalcActorScale` | `TESNPC::GetHeight(refr, race)`, ID 674794 | Deweh/PANDA agree on actor/race arguments | No maintained equivalent or established target | Unresolved | Must preserve actor/race-dependent calculation, not substitute raw NPC height fields |
| `SceneBase::ClearAnimObjects` | Construct event with holder pointer and notify graph event sources | Deweh event field is `TESObjectREFR*` | Maintained event field is `const uint64_t holderID` | Unresolved ABI/semantics | Determine whether field is pointer bits, handle/ID, or changed ownership; verify source dispatch function |
| `OrderedActionQueue` | HUD `ClearMessages`, ID 973227 | Deweh/PANDA legacy wrapper | No established target | Unresolved | Focused UI call graph and message-queue global analysis |
| camera/HUD hooks | Detour or interior call-site patches | Historical IDs/offsets only; historical `.text` encrypted | No established targets | Unresolved | Requires unpacked old code, then independent instruction-boundary/register/trampoline audit |
| hit/effect call sites | Custom event structures and transient-effect ID 652173 | Deweh layouts/wrappers | Maintained event/effect types exist but differ | Unresolved ABI/semantics | Constructor/access pattern and dispatch analysis; minimize fields used by NAF |

### Build boundary after this pass

A Release `ClCompile` pass still reaches the 100-error cap (102 reported) and
does not reach linking. The previous `BSAnimationGraphManager` redefinition and
`hkRefPtr::get` errors are gone, as are all NAF-local `BSInputEnableManager`
errors. The leading failures are now the duplicate animation event/utility
declarations, hit/effect declarations, and the unresolved engine-facing methods
listed above. No new 1.11.221 relocation, virtual slot, or interior offset was
promoted in this pass.

To continue instruction-level recovery without weakening the evidence standard,
the exact missing input is an **unpacked/decrypted Fallout4.exe 1.10.163.0** (or
a full module image dumped from a running 1.10.163 process after SteamStub has
decrypted `.text`). It should retain the normal PE image layout/RVAs so the
1.10.163 Address Library remains applicable. The live 1.11.221 installation
does not need to be modified.

## Decrypted mapped-image continuation (2026-08-10)

The historical-binary limitation above is superseded by
`Fallout4_1.10.163_memory.bin`, a complete 0x6CE6000-byte loaded-module image.
The analysis tools now support direct RVA indexing and parse `.pdata` from the
mapped image. ID 1446774 at RVA 0xE35510 disassembles as valid x64 code and
agrees with the WinDbg sanity check. The clean target is the supplied
`Reference Binaries/Fallout 1.11.221/Fallout4.exe`; the live installation is not
used for static analysis.

### Disassembly-verified actor/process functions

| Interface | 1.10.163 | 1.11.221 | Evidence |
| --- | --- | --- | --- |
| `AIProcess::SetupSpecialIdle` | ID 1446774, RVA 0xE35510 | ID 2231704, RVA 0xCFAD40 | Matching six-argument flow, process/actor gates, virtual slot 0x198, reference-counting path, idle construction, process fields 0x571/0x572, and boolean return. |
| `AIProcess::ClearCurrentPackage` | ID 241540, RVA 0xE2A670 | ID 2231582, RVA 0xCF1340 | Identical `this + 0x18` tail-call thunk; paired callees occupy the corresponding package helper region. |
| `AIProcess::ClearCurrentDataForProcess` | ID 577581, RVA 0xE77520 | ID 2232435, RVA 0xD394F0 | Matching process high-data checks and fields, idle-stop call, package/container cleanup, actor checks, virtual slot 0x120, flags, and follow-up operations. NG allocator/refcount cleanup is more heavily inlined but preserves the operation sequence. |
| `Actor::EvaluatePackage` | ID 1395257, RVA 0xD7F980 | ID 2229805, RVA 0xC69240 | Matching two boolean arguments, virtual slot 0x600 call with forced `dl=1`, branch into the corresponding evaluation body, and return convention. |
| `TESNPC::GetHeight` | ID 674794, RVA 0x5BA6F0 | ID 2207473, RVA 0x65A130 | Matching NPC interpolation fields 0x284/0x288, reference form-ID input, race fallback 0x1B8, sex lookup (1257181->2207107), race height array 0x180, and final multiplication. |

These explicit pairs were added selectively to the maintained CommonLibF4
dependency. NAF idle calls now use maintained `AIProcess::PlayIdle`, which
passes `DEFAULT_OBJECT::kActionIdle`, condition testing enabled, and a null
target to the verified `SetupSpecialIdle` function. Package override call sites
use the verified clear/evaluate interfaces without changing their original
operation order. All require runtime testing in the restored 1.11.221 game.

The Release compiler inventory no longer contains errors for `PlayIdle`,
`ClearCurrentPackage`, `ClearCurrentDataForProcess`, `EvaluatePackage`, or
`TESNPC::GetHeight`. Compilation still reaches the 100-error cap and has not
reached linking.

### Rejected candidates and remaining focus

- `GetAnimationGraphProjectName` old ID 190119 calls the holder's virtual slot
  0x20 to obtain a graph manager, then derives a `BSFixedString`. Candidate NG
  ID 2216182 was rejected: it does not consume the holder argument and instead
  operates on a TLS/global object.
- `SendHUDMessage::ClearMessages` candidate NG ID 2222001 was rejected. The old
  function lazily initializes a global queue and forwards a stack boolean to a
  queue operation; the candidate constructs and destroys a large stack object
  from an input object.
- No graph-name or HUD relocation is promoted from similarity alone. Animation
  event ownership, Havok graph internals, hit/effect ABI, camera/HUD hooks, and
  all interior sites remain unresolved as listed above.

## Compile-queued event recovery (2026-08-10)

This pass used `compile-engine-interface-3.log` as its work queue. One new
relocation was promoted:

| Interface | 1.10.163 | 1.11.221 | Grade and evidence |
| --- | --- | --- | --- |
| `BGSAnimationSystemUtils::GetEventSourcePointersFromGraph` | ID 897074, RVA 0x12CE440 | ID 2249847, RVA 0x1090410 | **Disassembly-verified function**: both adjust the reference to its graph-holder base at +0x48, call virtual slot 0x20 for the manager, forward the manager and output scrap array to the corresponding helper, release the smart pointer, and return the helper boolean. |

NAF's old `BSAnimationGraphEvent` and maintained CommonLib's event are both
0x18 with three pointer-sized fields in the same order. The historical first
field was typed `TESObjectREFR*`; maintained CommonLib exposes it as const
`uint64_t holderID`. NAF now stores the same pointer bit pattern in that opaque
pointer-sized field and does not retain or release ownership. Tag and payload
remain `BSFixedString`. This is **statically verified ABI adaptation**; the
manual `AnimObjUnequip` dispatch still requires runtime testing.

Unused duplicate wrappers (`GetActiveSyncInfo`, `WillEventChangeState`, and
`GetAnimationMaxDuration`) were removed. `InitializeActorInstant` and
`IsActiveGraphInTransition` now resolve to maintained declarations, but their
maintained legacy-only IDs 672857 and 839650 are still **unresolved** for NG;
their compile errors disappearing is not runtime verification. Top candidates
2238120 and 2216792 were rejected because their argument/data flows do not
match. Candidates for `IsIdleLoading` and `RunActionOnActor` were likewise
rejected.

NAF only consumed `TESHitEvent`'s victim/target reference. The custom 0x38/
0xD8/0x108 damage, hit-data, and event declarations were removed. Maintained
CommonLib documents changed sizes 0x40/0xE0/0x110 and exposes the required
reference as `TESHitEvent::target` at 0xE0. Registration now uses maintained
`TESHitEvent::GetEventSource` pair (989868, 1411899); death registration uses
maintained pair (1465690, 2201833). NAF no longer depends on the remaining hit
payload layout. This is a **maintained CommonLib API migration** and requires
runtime event-delivery testing.

| Remaining call site | Historical mechanism | Proposed NG mechanism | Grade | Compile impact / uncertainty |
| --- | --- | --- | --- | --- |
| animation initialization | Function ID 672857 | Maintained declaration, NG target unknown | Unresolved | Compiles, but cannot run safely until mapped |
| transition polling | Function ID 839650 | Maintained declaration, NG target unknown | Unresolved | Compiles, but scene transitions require mapping |
| idle loading | Function ID 686648 | No accepted candidate | Unresolved | Direct compile blocker remains |
| default reset action / action dispatch | IDs 639576 / 22408 | No accepted candidates | Unresolved | Direct compile blockers remain |
| graph project name | ID 190119 | No accepted replacement | Unresolved | Three direct compile errors remain |
| effect highlighting | transient effect ID 652173 and finish path | Maintained effect types differ | Unresolved ABI/function | Leading compiler block after event cleanup |
| HUD clearing | global queue wrapper ID 973227 | No accepted replacement | Unresolved | Four direct errors remain |
| camera/HUD/interior hooks | detours and interior instruction sites | No established targets | Unresolved | High-risk; unchanged in this pass |

After the hit/event migration, Release compilation reports **98 errors** with
one warning. This clears MSVC's 100-error cap for the first time. Linking has
not been reached. The leading errors are now the effect wrapper, BSModelDB,
HUD clearing, graph project name, and camera layout/hooks.

## Full compile-error burn-down (2026-08-10)

MSBuild printed every diagnostic twice (once during compilation and once in the
failure summary). Deduplicating by file, line, code, and message gives the
recorded starting total of 98. The primary-cluster inventory was:

| Primary cluster | Errors | Translation units / call sites | Shared cause and disposition |
| --- | ---: | --- | --- |
| transient effect / highlighting | 11 | `RE/TESEffectShader.h`, `Menu/HighlightedObjectsContainer.h` | Obsolete duplicate `ReferenceEffect` plus missing Deweh play/finish conveniences; eliminated using maintained effect APIs and layouts. |
| `BSModelDB` | 1 | `RE/BSModelDB.h` | NAF declared a namespace where maintained CommonLib declares `BSModelDB`; duplicate eliminated, exposing two real call-argument errors. |
| HUD/UI | 13 | `OrderedActionQueue`, `HUDManager`, three menu classes, inventory/menu mode calls | Four HUD-clear errors remain unresolved; duplicate HUD declaration and three renamed input overrides were eliminated; two `UI::UpdateMenuMode` errors remain. |
| `GetAnimationGraphProjectName` | 3 | `GraphHook`, `NAFStudioMenu`, `CreatorMenuHandler` | Legacy-only engine wrapper; unresolved. |
| camera state/layout and hooks | 16 | `CamHook` | Old scalar member names did not match the maintained aggregate names; all layout-name errors eliminated. Hook target remains unresolved. |
| animation initialization / transition / idle / action | 0 direct | Runtime paths documented below | The declarations currently compile, but the four legacy-only engine functions remain runtime blockers and are not counted as current diagnostics. |
| ordinary source/API drift | 5 | three menu input overrides and two format cascades | Three eliminated by the maintained override name; two format errors remain secondary to missing getters. |
| unresolved runtime-sensitive interfaces | 49 | chiefly `SceneBase`, `SceneManager`, studio/menu and Papyrus call sites | Missing Deweh actor/player/reference conveniences; each requires maintained-API replacement or focused recovery. |
| other | 0 | — | — |

The duplicate declarations are represented in their functional clusters above
rather than double-counted: `ReferenceEffect`, the NAF `BSModelDB` namespace,
and the local partial `HUDMenu` declaration were all obsolete against maintained
CommonLib.

### Verified source and layout migrations

Highlight play now calls maintained `TESObjectREFR::ApplyEffectShader`. It is
the maintained name for historical ID 652173 and returns the maintained
`ShaderReferenceEffect`. Historical `ProcessLists::FinishShaderEffect` (ID
631860, RVA 0xF0E060) was independently disassembled: it locks the magic-effect
array, selects a shader effect by target handle and `effectData`, and writes
`ReferenceEffect::finished = true`. NAF now performs that same narrow operation
through maintained `ProcessLists::ForEachShaderEffect`, which supplies the lock
and maintained 1.11.221 layouts. This is a **maintained CommonLib API plus
disassembly-verified semantic replacement**; effect appearance and teardown
still require runtime testing. The entire 11-error cluster disappeared (98 ->
87).

`FreeCameraState` retains size 0x58. Its historical scalars `x/y/z` at
0x28-0x33 and `pitch/yaw` at 0x34-0x3B are represented by maintained
`translation` (`NiPoint3`) and `rotation` (`BSTPoint2<float>`) at those exact
offsets. NAF now uses the maintained member names with the same pitch/yaw axis
ordering. This is a **statically verified layout/API migration** and removed all
16 camera layout diagnostics (88 -> 72). It does not verify legacy update hook
ID 541110 or any camera interior hook.

The partial NAF `HUDMenu` declaration was removed. Its only use was testing the
active `Pipboy` mode; maintained `HUDMenu::hudModes` and
`HUDModeType::modeString` express that test directly. This is a **source-level
semantic replacement using maintained layout** and removed four diagnostics
(72 -> 68). The three menu input handlers now override maintained
`BSInputEventUser::OnButtonEvent` rather than the historical `HandleEvent`
name, removing three ordinary API errors (68 -> 65).

### Intentionally unresolved compile blockers

The obsolete NAF `BSModelDB` namespace and its incorrect two-argument wrapper
were removed. Maintained CommonLib exposes the same historical Demand identity
(ID 1225688) with the actual 12-byte `DBTraits::ArgsType`, not NAF's opaque
`int64_t` default. The two skeleton-load calls are deliberately left as compile
errors until the required request flags for this use are established; no
aggregate defaults were guessed.

HUD clear ID 973227 at old RVA 0xAE26E0 was re-examined. It lazily initializes
a global queue object (callee ID 1175527), then passes a stack `false` byte to
the queue operation at old ID 1109561. This confirms the previous behavioral
description but does not establish either target identity in 1.11.221, so the
four call-site diagnostics remain. Candidate 2222001 remains rejected.

The final Release `ClCompile` pass reports **65 errors** and one command-line
warning. Linking has not been reached. The largest remaining repeated roots are
`Actor::ModifyKeyword` (8), `GetScale` (5 across actor/player call sites),
`SetAIControlled` (4), `SetNoCollision` (4),
`PlayerCharacter::ReenableInputForPlayer` (3), and multiple two-call-site
actor/player helpers. The smallest named high-priority blocker set is:

| Call site / interface | Current compile impact | Grade / remaining uncertainty |
| --- | ---: | --- |
| actor/player/reference Deweh conveniences | 50+ direct or cascading errors | Unresolved runtime-sensitive engine interfaces; audit individually by payoff. |
| HUD `ClearMessages` | 4 | Historical mechanism established; 1.11.221 queue owner/operation unresolved. |
| graph project name | 3 | Historical holder data flow established; replacement unresolved. |
| `BSModelDB::Demand` request | 2 | Maintained API known; correct skeleton request arguments unresolved. |
| `UI::UpdateMenuMode` | 2 | Maintained equivalent or focused recovery required. |
| format diagnostics | 2 | Cascades from missing `GetScale`/`GetBaseFullName`; not independent runtime interfaces. |

No new relocation pair, virtual slot, detour target, or interior hook was
promoted in this pass. `InitializeActorInstant`,
`IsActiveGraphInTransition`, `IsIdleLoading`, and `RunActionOnActor` retain
their previous unresolved status and rejected candidates. Compilation does not
currently force their recovery, and successful future linking would not make
them runtime-safe.

## Targeted convenience-API recovery (2026-08-10)

The complete 65-error set was used as the work queue. Release/x64/v143 now
reports **27 compiler errors**. Linking has not been reached.

| Old Deweh API | NAF call sites | Historical semantics | Replacement | Runtime dependency / evidence | Runtime test |
| --- | --- | --- | --- | --- | --- |
| `ModifyKeyword` | 8 | ID 1504561 selects add or remove from its boolean argument | Narrow NAF dispatcher to maintained `AddKeyword` / `RemoveKeyword` | Add uses maintained pair 640674/2200860. Remove pair 281170/2200861 is **disassembly verified** as the inverse adjacent implementation. | Required |
| `GetScale` | 5 | `refScale * 0.01`; NPC references additionally multiply `TESNPC::GetHeight(ref, visualsRace)` | Local field/API composition | **Disassembly-verified historical semantics**; maintained fields plus previously verified GetHeight pair 674794/2207473 | Required |
| `SetNoCollision` | 4 | Sets or clears bit 0x0800 in `loadedData->flags`; it does not call Havok or the character controller | Narrow local field helper | **Disassembly-verified field operation**; maintained `LOADED_REF_DATA` preserves flags at +0x20 | Required |
| cell `GetWorldSpace` | 1 | Null for interior cells, otherwise `worldSpace` at +0xC8 | Local maintained-layout accessor | **Disassembly/static layout verified** | Required |
| `IsFurniture` | 1 | Base form type is FURN or TERM | Local form-type check | **Disassembly-verified local semantics** | Low |
| `GetBaseFullName` | 1 plus 2 format cascades | Full name of the reference's base form | Maintained `TESFullName::GetFullName` | **Maintained CommonLib API** | Low |
| actor collision enable/disable | 4 | Compound actor/process collision transition, not the no-collision reference bit | Narrow relocations 1274117/2229652 and 475952/2229653 | **Disassembly-verified functions**: identical process acquisition, controller toggle argument, vslot 0x120 use, collision-state bit operation, and update dispatch | Required |
| `GetCanDoFavor` / `SetCanDoFavor` | 3 | Query/set actor commandability bit with player/package side effects on clear | Narrow relocations 46113/2230148 and 391670/2230149 | **Disassembly-verified functions**: same +0x43C/+0x440/+0x130 state, branches, player test, and paired calls | Required |
| `SetRestrained` | 2 | Changes actor interaction state through virtual slot 0x828 and stops movement when enabling | Narrow relocation 316742/2229910 | **Disassembly-verified function**, including vslot, state test and StopMoving pair | Required |
| `StopAlarmOnActor` | 2 | Stops alarm/combat process state then dispatches an actor/process update | Narrow relocation 1564830/2229667 | **Disassembly-verified function** with identical actor +0x300 access, arguments and corresponding callees | Required |
| `StopInteractingQuick` | 1 | Compound interaction teardown using three boolean arguments | Narrow relocation 129904/2231227 | **Disassembly-verified function**: actor flags, +0x300/+0x100 paths, lock sequence and argument consumption correspond | Required |
| `StopMoving` | 1 | Stops actor movement using a deceleration multiplier | Narrow relocation 940952/2230262 | **Disassembly-verified function**: TLS state, actor state mask, float argument and corresponding downstream path match | Required |
| `ClearLookAtTarget` | 1 | Resolves the actor look-at object then clears it | Narrow relocation 1345971/2230291 | **Disassembly-verified function**: same accessor call followed by corresponding clear operation | Required |
| `TurnOffHeadtracking` | 1 | Clears actor headtracking state and updates the process when active | Narrow relocation 149104/2230240 | **Disassembly-verified function**: same +0x130 bit, +0x300 process, +0x418 state and branches | Required |
| `SetAIControlled` | 4 | Compound player transition: several engine calls, a global state clear, input restoration on disable, and bit 1 at player +0xDFE | None | **Unresolved**; not safely reducible to a flag write | N/A until recovered |
| `ReenableInputForPlayer` | 3 | Resolves and releases two extra-data/input-layer objects and invalidates reference state | None | **Unresolved**; not equivalent to a blanket input-manager clear | N/A until recovered |

Error reductions by coherent group were: 65 to 48 for `ModifyKeyword`,
`GetScale`, and `SetNoCollision`; 48 to 42 after the cell/form/name local
migrations and one pointer correction; 42 to 31 for collision, favor,
restraint, and alarm functions; and 31 to 27 for interaction, movement,
look-at, and headtracking. No historical CommonLib surface was restored
wholesale.

The remaining 27 diagnostics are grouped as follows:

| Root cause | Errors | Status |
| --- | ---: | --- |
| HUD `ClearMessages` | 4 | Historical queue mechanism known; NG owner/operation unresolved |
| `BSModelDB::Demand` | 2 | Maintained API known; skeleton request `ArgsType` values unresolved |
| `GetAnimationGraphProjectName` | 3 | Holder-based historical behavior known; NG accessor/function unresolved |
| `SetAIControlled` | 4 | Compound runtime interface unresolved |
| disable/reenable player input | 4 | Compound input-layer runtime interfaces unresolved |
| graph-variable setting | 2 | Maintained declaration still carries legacy-only ID 1437197; NG target unresolved |
| `UI::UpdateMenuMode` | 2 | Maintained equivalent or NG target unresolved |
| `SetLinkedRef` | 1 | Maintained declaration narrows the second reference to `Actor*`; historical signature accepted any `TESObjectREFR*`; ABI/API correction still required |
| camera `Force3rdPerson` | 1 | Runtime-sensitive camera operation unresolved |
| `UpdatePlayer3D` | 1 | Legacy-only engine function unresolved |
| `WarpToRef` | 1 | Historical local wrapper depends on unresolved `WarpTo` plus worldspace semantics |
| `FreeUpAllMarkers` | 1 | Engine function ID 1324833; NG target unresolved |
| Total | **27** | Linking not reached |

The two BSModelDB calls, HUD clearing, and graph-project-name calls are
unchanged: no argument flags, queue target, or project accessor were guessed.
The four animation functions explicitly deprioritized for this stage remain
unchanged because none is a current compiler diagnostic. No virtual slot or
interior-hook location was newly promoted in this pass.

## Final compile-blocker pass (2026-08-10)

Release/x64/v143 started at **27 diagnostics** and now reports **19**. Linking
has not been reached. The reductions were: `SetLinkedRef` 1, `WarpToRef` 1,
graph-variable setting 2, `UpdatePlayer3D` 1, and
`GetAnimationGraphProjectName` 3.

| Family | Start | End | Result / evidence |
| --- | ---: | ---: | --- |
| `SetLinkedRef` | 1 | 0 | Historical Deweh and PANDA declarations accept `const TESObjectREFR*`; the maintained/NG `Actor*` narrowing was corrected. Pointer ABI is unchanged. The legacy engine ID 192840 itself still lacks an established NG pair, so the call remains runtime-unresolved. |
| `Force3rdPerson` | 1 | 1 | Old ID 162201 performs conditional transition bookkeeping, target update, third-person state preparation, and `SetState`; maintained `SetState` alone is not equivalent. Unresolved. |
| `UpdatePlayer3D` | 1 | 0 | Old ID 894747 is only a wrapper loading `currentProcess` and tail-calling core 986782 with `(process, player, true)`. Core **986782 -> 2231882** is instruction-level equivalent, including TLS marker, actor/process fields, branches, and corresponding callees. Disassembly verified; runtime test required. |
| `WarpToRef` | 1 | 0 | Old ID 728864 maps to refactored NG wrapper **2229838**. It consumes the same destination position, Z angle, zero X rotation, cell, exterior worldspace, and three false flags before delegating to the NG move core. Disassembly verified; cell/physics behavior requires runtime testing. |
| `FreeUpAllMarkers` | 1 | 1 | Old ID 1324833 releases FURN/TERM marker state and removes extra data type 0x12. The equivalent enlarged NG function was found immediately before ID 2202576 at RVA 0x55F4E0, but its entry has no Address Library ID. No unsafe RVA relocation was added. |
| graph bool setter | 2 | 0 | Old wrapper 1437197 and NG **2214543** are byte/instruction equivalent typed wrappers and call corresponding typed cores. Disassembly verified; runtime test required. |
| `UI::UpdateMenuMode` | 2 | 2 | Old ID 241336 locks the menu map, changes flag 1 at menu +0x58, maintains UI menu-mode count +0x1E0, and dispatches two events. Candidates 2283112 and 4506182 were rejected because they do not implement that behavior. |
| `BSModelDB::Demand` | 2 | 2 | Maintained API requires a 12-byte `DBTraits::ArgsType`. NAF's old wrapper supplied a reference to an 8-byte integer default (`3`), leaving byte 8 outside the object; that undefined historical call cannot establish the six request bits. No flags were guessed. |
| graph project name | 3 | 0 | **190119 -> 2230292** is disassembly verified. NG inlines the former helper: holder vslot 0x20, manager lifetime, lock +0xC8, active index +0xD8, graph array +0x40/+0x48/+0x50, and project-string leaf all correspond. Runtime test required for returned string ownership. |
| HUD clear | 4 | 4 | Old coupled IDs 973227/1109561 remain understood only historically; the NG queue owner and operation are unresolved. Candidate 2222001 remains rejected. |
| AI/input cluster | 8 | 8 | Historical `SetAIControlled` calls several actor/player operations, clears global state, conditionally invokes input restoration, and changes bit 1 at player +0xDFE. Disable/reenable input manages named input-layer/extra-data objects and invalidation/release paths. It is not equivalent to guessed flag writes or a blanket input-manager toggle; NG targets remain unresolved. |

No virtual slot or interior hook was newly promoted. The smallest remaining
compile-blocker set is the AI/input state machine (8 diagnostics), HUD queue
clearing (4), `BSModelDB` request construction (2), `UI::UpdateMenuMode` (2),
and one each for `Force3rdPerson` and `FreeUpAllMarkers` (19 total). The bounded
candidate search for these exact interfaces timed out without producing a
candidate; no evidence grade was changed as a result.

## Targeted final blocker recovery (2026-08-10)

This pass started at **19 MSVC diagnostics / 18 root call sites**. The current
Release/x64/v143 pass reports **7 diagnostics / 5 root call sites**. Linking
has not been reached.

### Resolved interfaces

- `PlayerCamera::Force3rdPerson` is **162201 -> 2248348**. The functions are
  instruction-level equivalents: both test the current state ID, perform the
  same conditional Pip-Boy/transition path, update `cameraTarget` at +0x64,
  prepare the third-person state at +0x120, and invoke the verified SetState
  pair 858847/2214742. This is disassembly verified and runtime-test-required.
- `DisableInputForPlayer` is **53346 -> 2200872**. Both resolve or allocate the
  reference's named input layer, invalidate through vslot +0x68 with
  `0x80000000`, resolve the stored layer, disable the requested user-event
  mask, and balance the layer reference. Disassembly verified.
- `ReenableInputForPlayer` is **478350 -> 2200873**. Both resolve two stored
  layer objects, invalidate the reference once, and release both layers. NG
  uses its current destruction path after the refcount reaches zero.
  Disassembly verified.
- `SetAIControlled` is **1431972 -> 2233087**. The enter path calls the same
  corresponding player-control and actor operations and clears global +0x88.
  The leave path calls the verified input cleanup, restores the player-control
  subsystem, and both paths update bit 1 at player +0xDFE. Disassembly
  verified. NAF's existing enter/leave ordering is unchanged, so cleanup
  symmetry is preserved. All eight AI/input call-site errors are eliminated.
- The old `UI::UpdateMenuMode` relocation was replaced with a maintained-layout
  composition. Under `UI::GetMenuMapRWLock()` it finds and retains the menu,
  changes only `UI_MENU_FLAGS::kPausesGame`, updates `UI::menuMode`, notifies
  `MenuModeCounterChangedEvent`, and notifies `MenuModeChangeEvent` only when
  the aggregate counter crosses zero. This exactly follows old ID 241336's
  control flow and offsets and is a **source-level semantic replacement using
  maintained API/layout**. Both UI errors are eliminated; runtime testing is
  required.

### Remaining blocker evidence

`BSModelDB::Demand` remains two root errors. Legitimate old actor/player model
callers establish the 12-byte request as `lodFadeMult = kActors (3)`,
`loadLevel = 0`, with `prepareAfterLoad`, `useErrorMarker`, `performProcess`,
`createFadeNode`, and `loadTextures` set. `faceGenModel` is deliberately
preserved from caller context. NAF's two calls inspect race skeletons outside a
face-generation context, which supports the non-facegen variant, but the
maintained Demand entries 1225688/1066398 still have no established NG IDs.
No call was added until both the request and callable NG entry are established.

HUD clearing remains four diagnostics at two call sites. Old ID 973227 has one
engine caller (old ID 1088722, a menu construction path). It lazily obtains the
queue and invokes 1109561 with a false byte. The queue operation locks, walks
the queued pointer array, rebuilds its membership/index structure, and then
performs cleanup. Structural searches around the verified ShowHUDMessage pair
1163005/2222440 did not establish the NG queue identity or public equivalent;
2222001 remains rejected.

`FreeUpAllMarkers` remains one error. The semantic NG body near RVA 0x55F4E0
has no Address Library ID, no direct call xrefs, no maintained method, and no
Address-Library-addressed wrapper/thunk was found. Removing only
`ExtraUsedMarkers` through maintained extra-data APIs would omit the
furniture-manager release and is not equivalent. A module-base-plus-RVA call
would require adopting and justifying a separate relocation policy, so work
stopped before introducing it as requested.

The corrected `SetLinkedRef` signature still uses legacy-only ID 192840 and
remains runtime-unresolved. No new virtual slot or interior hook was promoted.
The smallest remaining compile set is HUD clearing (4 diagnostics / 2 call
sites), `BSModelDB::Demand` (2), and `FreeUpAllMarkers` (1).

## Residual-interface recovery (2026-08-10)

This pass started at **7 compiler diagnostics**. Release/x64/v143 now reports
**1 compiler diagnostic**, the single `FreeUpAllMarkers` call site. Compilation
therefore has not reached the linker and there is no linker inventory yet.

### BSModelDB model demand

Both historical entry shapes are recovered:

- handle/database demand **1066398 -> 2275153**;
- `NiPointer<NiNode>` demand **1225688 -> 2275154**.

Old 1066398 performs path validation/canonicalization, selects the resource
database/cache path, forwards the 12-byte request, waits/processes the returned
entry as required, and returns the database handle/error. Old 1225688 wraps it,
extracts the entry's `NiNode` smart pointer, clears the output on failure, and
balances handle ownership. NG retains both ABI entry shapes, but 2275154 has
absorbed more of the former core path rather than simply calling 2275153.
Corresponding path, database globals, request forwarding, failure branches,
entry processing, `NiNode` extraction and refcount behavior establish both
pairs as **disassembly verified**.

NAF now uses a real `DBTraits::ArgsType`: `kActors`, load level zero,
non-facegen, with prepare-after-load, error marker, processing, fade-node and
texture loading enabled. This matches the verified NG player-skeleton caller
at ID 2232933 and eliminates both model-load compile errors. Runtime testing is
required for cache behavior and the two race skeleton variants.

### HUD message clearing

HUD clear is **973227 -> 2222460**, disassembly verified. The old subsystem
uses a lazily allocated synchronized queue with a pending-pointer array,
membership/index array, lock and clear-command object; 973227 submits the
false/clear command through 1109561. NG moved construction into the public
wrapper and uses static queue storage. ID 2222460 initializes that exact clear
queue and submits its clear-command object through addressed operation 2222507.

Data identity is corroborated from the enqueue side: old ShowHUDMessage
1163005 constructs the message, lazily gets the same subsystem and enqueues via
999295; NG ShowHUDMessage 2222440 constructs the corresponding message and
calls wrapper 2222482, which initializes the sibling static queue state and
enqueues through 2222510. The NG queue operations retain locking, pending-array
processing, membership/index reconstruction, ownership transfer and cleanup.
Candidate 2222001 remains rejected. Both NAF HUD call sites now compile;
message destruction and visible HUD behavior require runtime testing.

### FreeUpAllMarkers final blocker

The NG cleanup is a distinct unaddressed function beginning at RVA 0x55F490;
RVA 0x55F4B0 is an interior form-type branch and RVA 0x55F4E0 is its
marker-loop body. It is adjacent to, but not an interior
branch of, addressed ID 2202574. No direct calls, tail calls, jump-table entry,
vtable/function-pointer entry, callback registration, maintained wrapper or
higher-level addressed operation reaches this function.

Its behavior is now fully decomposed. For furniture/terminal references, and
subject to a manager configuration flag at +0x360, it iterates 22 marker
indices. ID 2198041 tests whether the base object exposes each marker; ID
2202566 obtains its reference/handle; the function resolves and validates the
handle generation and referenced object; qualifying furniture state is
released through ID 2229314; all temporary references are balanced. It then
locks the reference extra-data list, removes type 0x12 (`ExtraUsedMarkers`) via
ID 2190082, and unlocks it. The order is manager/marker release first,
extra-data removal second.

Several components are Address-Library-addressed, but the handle-table lookup,
generation validation, qualifying-object test and configuration-global policy
are not exposed as a maintained API composition. Calling only 2229314 is not
safe without reproducing that validation; removing only `ExtraUsedMarkers`
omits manager cleanup. The NAF call occurs after cross-cell actor warps and
before scene attachment/begin, on the selected furniture reference, so the full
cleanup remains behaviorally required. No raw RVA, partial helper or no-op was
introduced.

The smallest exact compile blocker is therefore one call to
`TESObjectREFR::FreeUpAllMarkers` in `SceneBase.h`. Reaching the linker requires
either an Address Library entry/upstream public wrapper for the NG function, or
an explicit project-level decision to adopt a separately justified relocation
mechanism for functions omitted from Address Library. Runtime-unresolved items
listed earlier, including SetLinkedRef, camera/HUD hooks, animation functions,
manual AnimObjUnequip behavior and effect/highlighting validation, retain their
existing grades.

## Exact-runtime fallback and first linked diagnostic artifact

The project-level exception for `FreeUpAllMarkers` is implemented only for
Fallout4.exe **1.11.221.0**, at module RVA **0x55F490**. The formerly proposed
RVA 0x55F4B0 was rejected after direct inspection: it begins with a conditional
branch and consumes flags/register state established by the real entry at
0x55F490. Address Library ID 2202576 at neighboring RVA 0x55F640 independently
confirmed the PE RVA conversion.

The x64 entry takes `TESObjectREFR*` in RCX and returns `void`, matching the
historical non-static member call. From the entry, control reaches the analyzed
form check and marker loop. The complete function retains handle-generation
validation, furniture-manager release, balanced temporary references, then the
locked removal of `ExtraUsedMarkers` (type 0x12). This is graded
**disassembly verified / exact-runtime fallback / runtime guarded**.

`EngineCompat::FreeUpAllMarkers` is the sole raw-RVA call site. It adds the
module base to 0x55F490, preserving ASLR. Both plugin load and the call boundary
require `REL::Module::version() == 1.11.221.0`. A fixed local masked signature
checks the verified prologue, RCX-based object/base-form access and FURN/TERM
tests; only the two relative branch displacements are masked. Validation never
scans elsewhere. Failure logs a critical diagnostic, rejects plugin load, and
the scene call also aborts rather than invoking mismatched code. No other raw
RVA exception was introduced.

### Fresh configure and compile convergence

The corrected fallback removed the final previously known compile diagnostic.
A fresh Visual Studio/vcpkg configure exposed ordinary source drift hidden by
the older cached build. These were resolved without engine relocations:

- UI depth priorities now assign the maintained enum directly.
- serializable handles use maintained typed-handle construction, equality,
  truth testing and public native values.
- fixed-string maps use a local case-insensitive comparator.
- `EnumSet` co-save data passes through `underlying()` and its public
  assignment operation rather than private storage.
- engine-independent `NiPoint3`/`NiQuaternion` cereal serialization was
  restored locally.
- the quaternion spline segment has a deterministic valid default, resolving
  an LTCG uninitialized-value warning.

The ending compiler diagnostic count is **zero**.

### First linker inventory

The complete initial linker failure set was:

| Symbol | Classification | Disposition |
| --- | --- | --- |
| `RE::ActionInput::~ActionInput()` | Missing CommonLib implementation | Defaulted in the CommonLib declaration; this performs normal destruction of its maintained `NiPointer` members and adds no engine call. |
| `RE::BSScript::IStackCallbackFunctor::~IStackCallbackFunctor()` | Missing CommonLib implementation | Defaulted in the CommonLib declaration; the abstract interface has no owned members beyond its maintained intrusive-refcount base. |

After those fixes, LTCG reported one ordinary `C4701` warning in the local
quaternion spline; deterministic initialization resolved it. The subsequent
Release/x64/v143 link succeeded and produced
`f4se-plugin/build-1.11.221/Release/NAF.dll`. The configured triplet is
`x64-windows-static-md` and `COPY_BUILD=OFF`; nothing was deployed.

### Static runtime-readiness audit

Successful linking does not make this DLL load-ready. The unresolved inventory
is classified as follows:

| Classification | Interfaces | Static disposition |
| --- | --- | --- |
| **A — must resolve before loading** | Game-loop interior hook `338354+0x1E`; graph detours `1492656` and `1216262`; face interior hooks `317245+0xBA` and `3139+0x158`; HUD interior hook `1385889+0xA8`; package hook `609985`; camera hook `541110` | All are installed synchronously by `InitializeHooking()` during `F4SEPlugin_Load`. Their NG targets/instruction boundaries or detour entries remain unresolved, so the current DLL is **not safe to load**, even diagnostically. |
| **B — safe only if subsystem is not exercised, after A is fixed** | `SetLinkedRef` legacy ID 192840; `InitializeActorInstant` 672857; `IsActiveGraphInTransition` 839650; `IsIdleLoading` 686648; `RunActionOnActor` 22408 and reset-action path 639576; remaining animation-graph/Havok wrappers and globals; custom reference/hit-event wrappers; movement/container/geometry wrappers; studio globals 1302430/644641 | These are reached from walking, scene initialization/transition, idle/action, hit/effect, container, geometry or studio paths rather than DLL entry. They remain unresolved and those paths must be disabled or recovered before use. |
| **C — suitable for focused diagnostic runtime testing once A is fixed** | Exact-runtime `FreeUpAllMarkers` fallback; disassembly-verified relocation pairs recorded above, including model demand, HUD clear, graph project name, player input/AI, camera third-person, graph setter and actor/process helpers | Static evidence supports the call shape and target. Tests must be narrow, symmetric where state is changed, and verify logs/cleanup. |
| **D — only runtime can complete validation** | `AnimObjUnequip` holder-ID dispatch; effect/highlighting lifetime and visual cleanup; HUD message destruction/visible behavior; graph-project string ownership; player AI/input restoration symmetry; camera transition behavior; model cache/skeleton results | These compile and have static support but their observable lifetime, ownership or game-state behavior cannot be proven statically. |

The `TESActionData` local layout/vtable and other `stl::emplace_vtable` custom
types also retain the earlier ABI audit requirement. Until every class-A hook
is recovered or explicitly prevented from registering, the produced artifact
must remain an undeployed build artifact and must not be placed in the live
F4SE plugin directory.

## Main-menu startup diagnostic gate

The complete synchronous initialization order was audited as:

1. `F4SEPlugin_Query` creates the log and rejects non-1.11.221 runtimes.
2. `F4SEPlugin_Load` calls `F4SE::Init`, which initializes `REL::Module`, loads
   the Address Library database and captures F4SE interfaces.
3. The exact-runtime `FreeUpAllMarkers` prologue guard is checked.
4. Historical `InitializeHooking` allocated the trampoline and registered, in
   order: game loop; graph update/Set3D; face update/lip; HUD mode; package
   evaluate/check; free-camera update.
5. Messaging, serialization and Papyrus callbacks were registered, game data
   was initialized, and the game-data-ready message later registered menus and
   entered additional subsystems.

Registration of each detour/interior hook resolves and dereferences its target
synchronously. Detour creation reads and patches the target prologue; call-site
hooks patch the interior call instruction. Registration does not deliberately
invoke the original target. The game-loop callback can nevertheless execute on
the next engine frame; graph Set3D/update, facial update, HUD mode, package
evaluation and camera update callbacks execute when their engine events occur.
There are no ordering dependencies among these hooks for basic F4SE startup.

For the first load only, `kStartupDiagnosticMode` is compile-time enabled in
`main.cpp`. `InitializeHooking` returns before trampoline allocation or any
relocation construction. The registered F4SE message listener logs message
types and returns without game-data/menu initialization. Plugin load returns
after messaging registration, before serialization, Papyrus, `Data::Global`,
menus, scenes or gameplay systems are initialized. This is an explicit test
configuration, not a claim that the hooks are obsolete or permanently
optional. Every affected subsystem is logged as unavailable.

### Hook matrix

| Historical target | Purpose and historical hook semantics | NG target / offset | Evidence grade | Normal timing / diagnostic timing | Load status |
| --- | --- | --- | --- | --- | --- |
| `338354 + 0x1E` | Interior `call` from `Main::UpdateNonRenderSafeAITasks` to `TaskQueueInterface::ProcessQueues`; wrapper calls the original, then advances scenes, ordered actions and forced actor alignment | Unresolved | Historical source semantics; NG site unresolved | Every frame / registration deferred and scene scheduler unavailable | C: subsystem-only, but callback would execute immediately after load |
| `1492656` | Entry detour of `IAnimationGraphManagerHolder::UpdateAnimationGraphManager`; observes graph update timing, calls original and updates NAF node graphs under NAF locks | Unresolved | Historical signature/source; NG entry unresolved | Graph update / deferred, body-animation graph subsystem unavailable | C |
| `1216262` | Entry detour of `TESObjectREFR::Set3D`; calls original and refreshes registered NAF node graphs for the reference | Unresolved | Historical signature/source; NG entry unresolved | Reference 3D assignment / deferred, body-animation graph subsystem unavailable | C |
| `317245 + 0xBA` | Interior call in `BSFaceGenNiNode::UpdateMorphing` to `BSFaceGenAnimationData::Update`; wrapper applies managed facial animation around the original update | Unresolved | Historical source/site description; NG instruction boundary unresolved | Face update / deferred, face animation unavailable | C |
| `3139 + 0x158` | Interior call in `BSFaceGenAnimationData::Update` to `UpdateMorphsFromLip`; wrapper controls lip/morph processing for managed animation | Unresolved | Historical source/site description; NG instruction boundary unresolved | Lip update / deferred, face animation unavailable | C |
| `1385889 + 0xA8` | Interior call in `HUDMenu::OnHUDMessage_SetMode` to `OnHUDModeChanged`; wrapper hides/restores NAF HUD around Pip-Boy mode, then calls original | Unresolved; distinct from verified HUD queue clear | Historical source/site description; NG instruction boundary unresolved | HUD mode message / deferred, NAF HUD unavailable | C |
| `1395257 -> 2229805` plus `609985` | Paired package interception: suppresses actor package evaluation and new-package checks while NAF disallows evaluation; both wrappers conditionally call originals | EvaluatePackage pair is disassembly verified; CheckForNewPackage remains unresolved | Mixed: verified entry plus unresolved paired entry | Actor AI/package evaluation / entire pair deferred, package override unavailable | C; partial installation is unsafe |
| `541110` | Entry detour of `FreeCameraState::Update`; calls original then applies active NAF free-camera translation/look-at input and node update | Unresolved | Historical signature/source; NG entry unresolved | Free-camera frame update / deferred, NAF camera controls unavailable | C |

No hook was replaced by a maintained event in this pass: available F4SE task
dispatch does not establish the post-`ProcessQueues` ordering required by the
scene scheduler, and no maintained graph Set3D, face morph, HUD-mode, package
decision or free-camera event was found that preserves the wrapper semantics.
No new relocation or NG interior offset was promoted.

With startup diagnostic mode enabled, no unresolved relocation is constructed,
dereferenced, patched or called during synchronous initialization or the
game-data-ready message. Release/x64/v143 still compiles and links with zero
diagnostics, `COPY_BUILD=OFF`. This artifact is statically safe for a strictly
main-menu-only diagnostic load: launch 1.11.221 through F4SE 0.7.8, reach the
main menu without loading a save or opening NAF UI, exit, then inspect the F4SE
loader and NAF logs for runtime/Address Library/guard/init success and the
expected diagnostic-gate messages.

## Game-loop update diagnostic

The first main-menu-only runtime diagnostic passed: F4SE 0.7.8 discovered and
loaded NAF 1.0.14.0, exact-runtime and startup gates remained active, Address
Library/CommonLib initialization and messaging registration succeeded,
lifecycle messages were logged without entering gameplay initialization, and
the game reached and exited the main menu normally.

### Historical workload and timing

Legacy ID 338354 is `Main::UpdateNonRenderSafeAITasks`, RVA 0xD3B4E0 in the
mapped 1.10.163 image. Its `+0x1E` instruction is one complete five-byte direct
`call` to legacy ID 882638, the task-queue `ProcessQueues` operation. Before the
call the containing function loads the queue singleton into RCX, a fixed float
into XMM1, and 8 into R8D. The entry has saved RBX and allocated 0x20 bytes, so
the call observes the normal aligned x64 stack and volatile-register ABI. No
incoming flags are consumed at the site. Continuation is exactly `+0x23`, the
next independent engine call; no RIP-relative instruction is relocated by the
five-byte call rewrite.

NAF's wrapper forwards all three arguments, calls `ProcessQueues` first, then:

- snapshots the scene map under its shared lock, detaches uniquely releasable
  queued scenes, and updates active scenes whose first actor is in a loaded
  cell, under each scene lock;
- drains the ordered equipment/morph action queue under its mutex, including
  frame-counted delays and deferred equip/unequip/inventory/HUD work;
- applies studio-requested actor alignment under a separate mutex, correcting
  angle/position and collision state.

Thus it is the normal game-thread post-task-queue scheduler for scene
advancement, deferred action execution, cleanup/detachment and forced
alignment. It does not directly service `TimerThread`, and has no local pause,
menu, save or load branch: its frequency and those transition semantics are
inherited from `UpdateNonRenderSafeAITasks`. The historical placement was
chosen so F4SE/engine queued work completes before NAF observes and advances
state in the same update pass.

F4SE 0.7.8 exposes only one-shot task and UI-task enqueue operations. They use
the very queue being processed and do not guarantee an immediately-after-
`ProcessQueues` callback. Self-rescheduling from a task also lacks a documented
same-pass-versus-next-pass guarantee and could recurse within the current drain.
No maintained CommonLib per-frame event preserves this exact ordering. These
mechanisms are therefore not semantically equivalent replacements.

### Verified 1.11.221 hook

Focused comparison found a normalized instruction-for-instruction match at NG
Address Library ID **2228937**, RVA **0xC36260**. Manual disassembly confirms
the same containing-function control flow and the same call program point at
independently verified offset **+0x1E**. The NG call prepares RCX, XMM1 and R8D
identically and targets NG ID **2229181**, whose `ProcessQueues` implementation
consumes the same `(queue, float, uint32)` arguments and returns bool in AL.
The verified pairs are:

| Historical | 1.11.221 | Grade |
| --- | --- | --- |
| `338354` / RVA 0xD3B4E0 | `2228937` / RVA 0xC36260 | Disassembly-verified containing function |
| `338354 + 0x1E` | `2228937 + 0x1E` | Disassembly-verified interior call site |
| task queue `882638` | task queue `2229181` | Disassembly-verified callee ABI and semantics |

Installation fails closed unless the addressed byte is opcode E8 and its
decoded rel32 target equals ID 2229181. Only then does the trampoline replace
that five-byte call and retain the original callee. No fuzzy scan or raw RVA is
used.

For the second diagnostic, the wrapper calls the original queue processor but
returns before all historical NAF work. It uses only atomic counters, the OS
thread ID and the logger. It logs installation, callback 1, callbacks 2 and 3,
then callbacks 10 and 60 with the initial/current thread IDs. Scenes, ordered
actions, alignment, serialization, Papyrus, menus, graph, face, HUD, package
and camera systems remain gated and unreachable.

The second test is limited to launching 1.11.221 through F4SE 0.7.8, reaching
the main menu, waiting long enough to observe multiple callbacks, loading no
save, exiting normally, and inspecting F4SE/NAF logs. Success establishes the
update hook as **runtime-startup verified**, not gameplay verified; normal NAF
work must remain disabled until its downstream interfaces are ready.

### Second diagnostic result and world-load gate

The second diagnostic loaded and installed `2228937 + 0x1E` without a crash,
but produced no callback-count entries while the game remained at the main
menu. Evidence is therefore split: installation and startup stability are
runtime verified; invocation is not yet runtime verified.

Static xref searches found no direct call edge to either the old or NG
containing function. NG does have one instruction-aligned tail-call wrapper,
ID `2227597` / RVA `0xBD5DE0`: it performs a preparatory manager operation (ID
`2192054`), loads the update object from a global, restores its frame and jumps
to `2228937`. No direct call or jump reaches that wrapper, so the outer schedule
is indirect. A second raw-byte `E9` hit inside ID `2230727` was rejected after
instruction-boundary disassembly showed that it falls inside another
instruction and is not executable as a jump. The containing function's complete
body is a world/update bundle: after processing the task queue it advances
multiple AI/game systems, accesses world and manager globals, and finishes with
`Main` state handling. Combined with the main-menu negative runtime observation,
this supports the conclusion that the indirect dispatch phase normally starts
only after a game world/save is active, rather than being conditional on the
task queue merely containing work. The verified hook site and trampoline are
unchanged.

The third diagnostic adds only first-callback context. After calling the
original `ProcessQueues`, it logs the current Windows thread ID, maintained
`Main::threadID`, `Main::QGameDataLoaded()`, `Main::gameActive` and
`Main::inMenuMode`, then follows the existing atomic count schedule. It still
returns before all NAF scene/update work. These are maintained CommonLib NG
globals/layout fields; no gated NAF subsystem is initialized.

Third test plan:

1. launch Fallout 4 1.11.221 through F4SE 0.7.8;
2. load an ordinary save without opening NAF UI or starting an NAF scene;
3. remain idle for several seconds, optionally open/close pause once;
4. quit normally and inspect NAF/F4SE logs;
5. require callbacks 1, 2, 3, 10 and 60, a consistent current/initial thread
   ID, and first-callback world-state context consistent with a loaded game.

A pass will grade the hook **runtime-startup/world-update verified**, while its
normal scene/action/alignment workload remains disabled and not gameplay
verified.

### Third diagnostic: worker-thread execution

The save-loaded third diagnostic runtime test invoked the recovered hook
repeatedly without a crash. Callback 1 ran on Windows thread `215544`, callback
2 on `206108`, callback 3 on `215544`, callback 10 on `200192`, and callback 60
on `206108`. The first callback observed `Main::threadID == 59924`,
`QGameDataLoaded() == true`, `gameActive == true`, and `inMenuMode == false`.
Installation, execution, world-load activation, and repeated execution are now
runtime verified. Single-thread or game-main-thread execution is contradicted.

`Main::threadID` is not an inferred label. In the 1.11.221 `Main` constructor,
ID `2228905` / RVA `0xC2F5C0`, the engine calls imported
`KERNEL32!GetCurrentThreadId` and stores EAX at `Main + 0x40`. The field therefore
records the Windows thread that constructs `Main`; it is not updated for each
job callback. The diagnostic's `GetCurrentThreadId` values use the same Windows
identity domain, so the differing IDs prove that `NonRendersafeAI` ran on job
workers rather than the `Main` construction/game thread.

The scheduling identity is also explicit in both binaries. In 1.10.163 the
function/name pair table contains wrapper ID `691099` / RVA `0xCBDEF0` beside
the string `NonRendersafeAI`; the wrapper performs preparatory call ID `731640`,
loads `Main`, and tail-jumps to ID `338354`. In 1.11.221 the corresponding table
entry contains wrapper ID `2227597` / RVA `0xBD5DE0` beside the same
`NonRendersafeAI` string; it performs preparatory call ID `2192054`, loads
`Main`, and tail-jumps to ID `2228937`. The surrounding paired job names and
ordering also correspond (`VMRenderSafe`, `PostSwap`, `IOSync`,
`NonRendersafeAI`, `BreakableFXManager`, and later physics/animation jobs).
The NG table is consumed by job-list construction ID `2227625`, which registers
the callback graph with the engine job system. This is disassembly evidence that
worker scheduling and possible thread migration are historical semantics, not a
side effect of the NG mapping.

The table contains `NonRendersafeAI` once, establishing one node execution per
job-list cycle. It does not, by itself, prove that job-list cycles equal rendered
frames or that successive cycles cannot overlap. The original source comment
says scene `Update` occurs between frames, and `delayCounter` decrements once per
hook invocation, demonstrating that NAF expected a serialized once-per-update
cadence rather than main-thread identity. The third test's rapid count and thread
migration do not resolve cycle cadence or overlap.

Original callback thread-affinity audit:

| Work | NAF synchronization | Engine/thread-affinity assessment |
|---|---|---|
| Scene-map snapshot and detach selection | shared `scenesMapLock`; shared ownership snapshot | B: NAF-protected; no main-thread-only operation by itself |
| Per-scene advancement and detach cleanup | recursive `IScene::lock` serializes each scene | B/D/E: NAF state is protected, but graph queries, idle playback, actor state and event work assume the serialized engine update context; main-thread affinity is not established |
| Ordered queue and frame delays | one recursive `safe_mutex` protects queue, persistent equipment state and `delayCounter` | B/D: NAF-serialized; cadence assumes one invocation per update cycle |
| Equip/unequip and inventory changes | executed while ordered-queue lock is held | D/E: actor/equip manager operations were intentionally run from `NonRendersafeAI`, but their independent NG worker-safety is not yet proven |
| Morph and HUD work in ordered actions | ordered-queue lock only | E: NAF serialization does not prove LooksMenu/UI thread safety; these must remain disabled |
| Scene animation synchronization and player-3D refresh | per-scene lock | D/E: historically placed in the non-render-safe AI job; engine graph/API affinity still requires runtime validation |
| Studio forced alignment and collision correction | `alignActorsLock` | B/D/E: vector access is protected; actor transform, collision and process calls require the verified update context and are not proven safe under overlapping callbacks |

There is no outer NAF lock covering scene advancement, ordered actions and
alignment as one transaction, and no TLS-based thread selection. Per-scene,
queue and alignment locks prevent same-container races, but callbacks could
interleave between stages if successive engine job-list cycles overlap. The
original implementation therefore appears to rely on serialized execution of
the named engine job node, not on a fixed Windows thread. That serialization is
not yet runtime verified, so reconnecting gameplay work remains unsafe.

The fourth diagnostic retains the verified hook and all gameplay gates. It adds
an atomic in-flight counter spanning only the natural original `ProcessQueues`
call, a maximum-in-flight counter, QPC entry/duration/delta timestamps, thread
ID, and the opaque queue pointer value (logged but never dereferenced). It logs
samples 1, 2, 3, 10, 60 and 120. This can establish natural overlap, worker
migration, stable queue identity, and approximate job-cycle cadence without
sleeping, widening the callback, or initializing another subsystem. A
`maxInFlight` greater than one proves concurrent callbacks; a value of one over
a sufficiently long loaded-save run supports natural serialization but does not
prove that the longer historical NAF workload could never extend across the next
cycle.

### Fourth diagnostic result and incremental workload plan

The fourth loaded-save diagnostic reached at least callback 120. Worker identity
continued to migrate and remained distinct from `Main::threadID`, while the
opaque task-queue pointer stayed stable. Every sampled callback reported
`entryInFlight=1`, `remainingInFlight=0`, and `maxInFlight=1`; the immediate
overlap warning never fired. The recovered node is therefore runtime-executed,
world-update active, worker-thread scheduled, thread-migrating, and empirically
serialized for the observed run. This is not promoted to a formal no-overlap
engine guarantee, nor does the evidence prove one invocation per rendered
frame.

Static registration nevertheless supports serialization as an intended engine
property. Both runtimes register exactly one `NonRendersafeAI` node in the
corresponding job graph, surrounded by named nodes and synchronization stages,
and the callback performs a fixed update bundle. NAF contains no reentrancy
test or outer callback mutex; instead it locks each state domain independently.
Together with the runtime result, this strongly indicates that the original
implementation relied on the job graph to schedule one logical invocation at a
time, while allowing that node to migrate among workers. This remains an
inference from graph identity, source structure and observation, not a formally
documented scheduler contract.

Historical post-`ProcessQueues` order and disposition:

| Order | Work | Synchronization | Important calls/dependencies | Class |
|---:|---|---|---|---|
| 1 | Snapshot scene map | shared `scenesMapLock`; snapshot owns `shared_ptr`s | local container copy only | A, but not selected alone because it does not advance state |
| 2 | Test queued detach eligibility | per-scene recursive `safe_mutex`; `shared_ptr::use_count` | local state/read-only bookkeeping | A for the test only |
| 3 | Perform eligible detach | scene lock plus exclusive scene-map lock | removes map entry, walks actors, `EngineCompat::ModifyKeyword` | B; keyword/actor mutation is engine-facing |
| 4 | Test scene update eligibility | scene lock | actor handle resolution and cell/loaded-data reads | B; layout is maintained, worker safety still needs validation |
| 5 | Advance active scene | scene lock | animation transition/loading queries, idle playback, graph time, event delivery, actor transforms, collision, `UpdatePlayer3D` | C until graph and other unresolved interfaces are recovered; individual calls also require B validation |
| 6 | Ordered-action delay test/decrement | `OrderedActionQueue::lock` | local `delayCounter` only | A |
| 7 | Invoke queued custom/equipment/morph functions | same ordered-queue lock | arbitrary custom callback; equip manager; inventory; LooksMenu morph path | B for equip/inventory; C for arbitrary, morph and LooksMenu-sensitive work |
| 8 | Deferred HUD cleanup reached by equipment actions | ordered-queue lock does not establish UI affinity | `SendHUDMessage::ClearMessages` and related UI state | C; HUD/UI remains gated |
| 9 | Studio alignment snapshot/iteration | `alignActorsLock` | `NiPointer` vector and coordinate comparisons | A for container access only, but no useful state transition without actor work |
| 10 | Apply studio alignment/collision correction | same alignment lock | `SetAngleOnReference`, `SetPosition`, collision/process compatibility calls | B/C; visible gameplay effect and unresolved subsystem dependencies |

`InsertDelay(frames)` does not consult rendering. It enqueues a local lambda that
adds `frames` to `delayCounter` when the ordered queue reaches it. Each later
invocation of this exact game-loop callback decrements the counter once and
returns before executing another queued action. Thus "frames" historically
means `NonRendersafeAI` update cycles/callback ticks. Preserving that behavior
does not require proving equivalence to rendered frames.

The fifth diagnostic selects only order 6. At registration it takes the existing
ordered-queue mutex and refuses to install unless the queue is empty and
`delayCounter` is zero. It then seeds `delayCounter=3`. After each verified
`ProcessQueues` call, a diagnostic-only helper takes the same mutex and performs
only the historical `delayCounter > 0` decrement. It never invokes, removes or
inspects the contents of a queued function. Three low-volume entries must show
`3 -> 2`, `2 -> 1`, and `1 -> 0`, with `queued engine actions=0`. The existing
threading samples and overlap warning remain active.

Everything else remains gated: scene snapshot/update/detach, graph and animation
work, actor/player updates, equipment and inventory operations, morphs,
HUD/UI, studio alignment/collision, Papyrus, serialization, menus, package,
camera and face systems. This diagnostic therefore validates NAF's own locked
tick bookkeeping on the recovered worker node without causing gameplay-visible
effects or entering an unresolved interface.

Fifth runtime plan:

1. launch Fallout 4 1.11.221 through F4SE 0.7.8 and load an ordinary save;
2. remain idle for 30-60 seconds, optionally pause/unpause once;
3. do not open NAF UI or start a scene;
4. exit normally and inspect `NAF.log`;
5. require the three ordered delay transitions, zero queued engine actions, no
   overlap warning, `maxInFlight=1`, and the normal messages confirming all
   gameplay initialization remains gated.

### Fifth diagnostic result and first engine-read boundary

The fifth loaded-save diagnostic produced exactly `3 -> 2`, `2 -> 1`, and
`1 -> 0`; every transition reported zero queued engine actions. No queued
function was invoked or dereferenced. The queue pointer remained stable, worker
identity continued to migrate, sampled callbacks retained `maxInFlight=1`, and
all gameplay initialization remained gated. The recovered `NonRendersafeAI`
execution, serialized worker scheduling for the tested runtime, NAF's
ordered-action mutex use in this callback, and the delay-counter tick semantics
are now **runtime validated**. Serialization remains an empirical property of
the tested 1.11.221 run rather than a documented universal scheduler guarantee.

Engine-facing candidate risk order:

| Rank | Candidate | Historical use/frequency | Reversibility and verified surface | Worker/subsystem risk |
|---:|---|---|---|---|
| 1 | Actor/reference loaded-3D read | `ForEachActor` eligibility on every active-scene update | read-only pointer result; maintained player singleton relocation and maintained `TESObjectREFR::Get3D` virtual ABI | lowest; no returned-object dereference or subsystem entry |
| 2 | Actor keyword add/remove | detach and scene membership transitions | reversible in principle; compatibility helper established | mutates actor keyword state/container and requires reliable inverse cleanup |
| 3 | Collision query/toggle | scene positioning and forced alignment | toggle can be reversed | touches actor process/physics state and may cascade |
| 4 | Actor transform setters | active scene and studio alignment, potentially every tick | old value could be captured/restored | visible movement, physics/process updates and multiple engine objects |
| 5 | Player-3D refresh | active scene update, potentially every tick | maintained composition exists | large visible regeneration with equipment/node side effects |
| 6 | Equipment/inventory | ordered actions when queued | inverse operation is possible but stateful | inventory/equip managers, messages and HUD cleanup; rollback is not trivial |
| 7 | Animation/idle/graph work | scene setup and every active update | not generally reversible | unresolved graph hooks/functions and animation locking |
| 8 | Morph/LooksMenu | ordered morph action | state can theoretically be restored | LooksMenu ABI/thread affinity and face dependencies remain gated |
| 9 | HUD/UI | deferred equipment cleanup and UI state | observable UI state | known higher thread-affinity risk and unresolved HUD hook |

The sixth diagnostic selects rank 1: one call to
`PlayerCharacter::Get3D() const`. The exact target surface is the maintained
player singleton relocation `REL::RelocationID(303410, 2690919)` and the
maintained `TESObjectREFR::Get3D` virtual declaration (slot comment `0x8C` in
historical CommonLibF4, CommonLibF4-NG, and maintained CommonLibF4). No new raw
address or relocation is introduced. This is graded **maintained CommonLib API / runtime-test-required**, not independently disassembly-verified as a new slot.

Historical reachability is direct: recovered `NonRendersafeAI` call site ->
`HookedGameLoop` after original `ProcessQueues` -> `SceneManager::UpdateScenes`
-> active `IScene::Update` -> `IScene::ForEachActor` ->
`GameUtil::ActorIsEnabled` -> `Actor::Get3D`. The call is conditional on an
attached, active scene with a resolved actor and uses the default `require3d`
argument. It runs while the per-scene recursive mutex is held; the actor is kept
alive by the resolved `NiPointer`. The call only returns the current scene-root
pointer and has no NAF cleanup path because it is observational.

For the controlled test, the lifetime-stable player singleton replaces a scene
actor, avoiding scene initialization and handle resolution. The call runs once,
only after `QGameDataLoaded()` and `Main::gameActive` are true. Logs record the
worker thread and player pointer before the call, then the player and returned
3D pointer after it. The returned pointer is never dereferenced or retained.
`loaded` and `not-loaded` are both safe query outcomes, although a normally
loaded save is expected to return a non-null root. No mutation occurs, so no
rollback is required.

The fifth diagnostic's three locked delay ticks and all overlap/queue/thread
monitoring remain enabled. Full ordered execution is still unsafe because the
queue stores arbitrary functions; equipment/inventory changes are stateful;
morph and HUD paths have unresolved thread affinity; and scene actions reach
unresolved animation hooks, actor transforms, collision, player-3D refresh and
other gated systems.

Sixth runtime plan:

1. launch through F4SE 0.7.8 and load an ordinary save;
2. remain idle for 30-60 seconds without opening NAF UI or starting a scene;
3. optionally pause/unpause once, then exit normally;
4. require one matching before/after `PlayerCharacter::Get3D` pair, the same
   non-null player pointer in both lines, no crash, the three delay transitions,
   stable queue identity, no overlap warning, and `maxInFlight=1`;
5. do not generalize a pass beyond this single read-only virtual operation.

### Sixth diagnostic crash: invalid singleton acquisition

The sixth diagnostic did **not** reach `TESObjectREFR::Get3D`. Its final log
recorded `player=0x700000008`; the fault at `mov rax, [rsi]` was the attempted
vtable load from that invalid value. This is an invalid-object/singleton
relocation failure, not evidence that `Get3D` is unsafe on the serialized
`NonRendersafeAI` worker. The recovered update hook and its observed
serialization are not implicated.

Focused old/new analysis established the singleton representation and exposed
a maintained-library relocation defect:

| Fact | 1.10.163 | 1.11.221 | Evidence |
|---|---:|---:|---|
| Player singleton storage | ID `303410`, RVA `0x5AA4388` | ID `2698073`, RVA `0x32D22E0` | disassembly-verified global identity and initialization |
| Player initializer | ID `1142639`, RVA `0xD3E620` | ID `2229084`, RVA `0xC3E550` | matched `0xE10` allocation, constructor, intrusive retain/store/release sequence |
| Player constructor | ID `129780` | ID `2232883`, RVA `0xD524E0` | initializer call target and allocation/initialization context |

The storage is one pointer-sized intrusive `NiPointer<PlayerCharacter>` global,
not an extra `PlayerCharacter**` wrapper. Legitimate engine callers perform one
load from the global and commonly null-check the resulting object. Both
initializers allocate `0xE10` bytes, construct the player, increment the new
object's intrusive count at `+0x28`, store the pointer directly in the global,
and release the prior value.

Maintained CommonLib declared NG ID `2690919`, but that ID is absent from the
supplied 1.11.221 Address Library. `IDDB::id2offset` used `lower_bound` without
checking exact equality on non-VR runtimes, so the missing request silently
selected the next entry: ID `2697707`, RVA `0x32961F0`. The unrelated global at
that address contained `0x700000008` at runtime (low/high 32-bit values `8` and
`7`). Its precise engine meaning is unresolved, but it is demonstrably packed
non-pointer state obtained from the wrong relocation; it is not another player
indirection and must never be cast or accepted heuristically.

CommonLib now uses ID `2698073` and rejects every inexact Address Library lookup
on NG as well as legacy/VR. The corrected diagnostic resolves the explicit
`NiPointer<PlayerCharacter>` storage, logs the relocation/global address
separately from the resolved object, and validates the object before virtual
dispatch: committed readable storage plus a first pointer inside Fallout 4's
`.rdata` segment. Failure logs a critical diagnostic and suppresses the call;
there is no scan, fallback relocation, cast-around, or object dereference.
After validation it calls `Get3D` exactly once and logs—but does not dereference
or retain—the returned pointer.

`Get3D` remains a separate evidence item. Historical Deweh CommonLib,
CommonLibF4-NG, and maintained CommonLib agree on virtual slot `0x8C`, and the
1.11.221 executable contains widespread legitimate virtual calls through byte
offset `0x460`. This supports ABI continuity but is not yet a narrowly
disassembly-verified semantic proof of the slot, and worker-context safety is
still untested.

Corrected sixth diagnostic plan:

1. load an ordinary save with all existing gameplay gates retained;
2. require the logged singleton storage to resolve through ID `2698073` and a
   plausible object/vtable before dispatch;
3. require exactly one matching before/after `Get3D` pair, with no dereference
   or retention of its result;
4. retain the delay, queue-identity, worker-thread and overlap diagnostics;
5. treat any guard rejection as a safe diagnostic failure, and do not infer
   `Get3D` thread safety unless the virtual call actually returns.

## Addictol reference findings

Audit provenance (reference-only; neither repository is a NAF dependency):

- Dear-Modding-FO4/Addictol commit `d570d05acdda38585db8699dbecd2514eb397170`;
  pinned Dear-Modding-FO4/commonlibf4 commit
  `54c5f576e31410f393abebeebca366e970e824f4`.
- Dear-Modding-FO4/AddictolCrashLogger commit
  `a4abbd341b8acdb2c6160a47f9f59e1518763bb6`; pinned CommonLib commit
  `a08c731eead4dfc6ea55aea4649c22aec05a47fe`.
- Crash Logger's supplied `1.11.221.0/Fallout4.pdb` is 4,632,576 bytes,
  SHA-256 `6D217CD371F081A7E490523F5D33EDCCB2777A716B2E7227AD9CECEFD7B2A918`.
  Its repository history describes the audited revision as an updated
  community PDB with 42,970 symbols. This is PDB-derived/community reverse-
  engineering data, not an official Bethesda PDB.

### Address model and safety policy

Addictol's CommonLib defines three runtime indices in fixed order: OG
`1.10.163`, NG through `1.10.984`, and AE above `1.10.984` (including
`1.11.221`). `REL::VariantID{ og, ng, ae }` stores either an Address Library ID
or an explicit `REL::Offset` per runtime. If fewer than three operands are
provided, the last operand is repeated for later runtimes. Consequently a
two-element `{ old, new }` entry explicitly assumes the same Address Library ID
for NG and AE; it is not evidence that the code RVA or ABI is unchanged.

The Addictol Address Library loader is fail-closed: its sorted mapping lookup
requires exact ID equality, and its newer dense format rejects out-of-range or
zero entries. It also validates the library version/hash. This independently
supports the NAF CommonLib fix that made exact-ID checking unconditional; the
old NAF dependency's non-VR `lower_bound` fall-through was not a safe
multi-runtime behavior.

Addictol uses a mixture of function-entry IDs, runtime-specific interior
offsets, and a small number of documented direct RVAs. Quality is not uniform
across every module. Its stronger recent patch paths validate exact per-runtime
signatures, sometimes require a signature to be unique in `.text`, install
nothing on mismatch, preserve original bytes, and roll back partially applied
multi-site changes. Its module lifecycle also separates query/install stages,
allows subsystem deferral to F4SE lifecycle messages, logs disabled modules,
and wraps stages in structured exception handling. Useful NAF practices are:
exact runtime gates, pre-patch byte validation, all-or-nothing multi-site
installation, retained original bytes, explicit deferred subsystem stages, and
fail-closed logging. Addictol's occasional unvalidated raw patch sites are not
a reason to relax NAF's evidence policy.

### Player singleton

Addictol CommonLib declares:

```text
PlayerCharacter singleton IDs: OG 303410 / NG 2690919 / AE 4798212
storage type: REL::Relocation<NiPointer<PlayerCharacter>*>
accessor: singleton->get()
```

This resolves the apparent conflict with NAF's crashed test. In Addictol,
`2690919` is explicitly the **1.10.984 NG** ID, not the `1.11.221 AE` ID.
Working AE-targeted source selects the third entry, `4798212`. NAF's former
two-runtime declaration selected `2690919` for 1.11.221 and its faulty inexact
lookup then returned unrelated packed state `0x700000008`.

Addictol agrees on the representation: a pointer-sized intrusive
`NiPointer<PlayerCharacter>` global and one `get()` operation, not an extra
`PlayerCharacter**` dereference. Its player-dependent modules generally obtain
the object through `PlayerCharacter::GetSingleton`; later-lifecycle code often
null-checks it, although at least one game-data-ready call site dereferences it
directly. Addictol's AE ID belongs to its newer three-runtime Address Library
namespace, while NAF's supplied 1.11.221 library exposes independently verified
ID `2698073` for RVA `0x32D22E0`. The identifiers are therefore not directly
interchangeable. The shared storage type, runtime classification, and failure
explanation are **strong corroborating source evidence**; NAF's ID/RVA remains
**disassembly verified** through the old/new initializer comparison.

### PDB-derived 1.11.221 symbols relevant to NAF

The Crash Logger installs a runtime-selected `Fallout4.pdb` next to the plugin
and uses Microsoft DIA (`msdia140.dll`) to open it, query public/private symbols
by RVA, demangle names, and optionally report parameters/source lines. The
following RVAs account for `.text` section RVA `0x1000`; raw CodeView section
offsets are not module RVAs until that section base is added.

| 1.11.221 RVA | PDB symbol | NAF relevance | Grade/disposition |
|---:|---|---|---|
| `0x55F490` | `TESObjectREFR::FreeUpAllMarkers(void)` | exact-runtime fallback | PDB corroborates the already disassembly-verified entry/signature; no change |
| `0x5645B0` | `TESObjectREFR::SetLinkedRef(TESObjectREFR*, BGSKeyword*)` | unresolved NG relocation | named candidate; signature matches maintained wrapper, but relocation still requires focused binary verification |
| `0x818730` | `IAnimationGraphManagerHolder::GetAnimationGraphProjectName(BSFixedString&)` | unresolved graph-project accessor | strong named candidate and output signature; holder adjustment/data flow still requires disassembly verification |
| `0xE13E40` | `BGSAnimationSystemUtils::InitializeActorInstant(Actor&, bool)` | runtime-unresolved animation initialization | named candidate; Addictol source also maps old ID `672857` to new ID `2236393`; ABI/semantics not yet promoted |
| `0x806370` | `AnimationSystemUtils::IsActiveGraphInTransition(TESObjectREFR&)` | runtime-unresolved transition query | named candidate; Addictol maps old ID `839650` to `2214305`; const/reference difference must be checked |
| `0xC44DC0` | `TaskQueueInterface::ProcessQueues(float, uint)` | recovered worker hook context | symbol identifies the queue processor; it does not by itself establish NAF's wrapper/interior hook ABI |
| `0x50DA00`, `0x50D9A0` | two `TESObjectREFR::Get3D` implementations/overloads | test-six virtual call | useful implementation candidates; virtual slot `0x8C` still requires separate vtable/caller verification |
| `0x16775A0` | `BSInputEnableManager::AllocateNewLayer(...)` | player-input state machine | named function candidate; maintained/Addictol wrapper uses old/new IDs `537494/2268244` |
| `0x167A690` | `BSInputEnableManager::AllocateNewLayerImpl(...)` | input-layer ownership | names internal allocation path; candidate only |
| `0x167A300` | `BSInputEnableManager::EnableUserEvent(uint,uint,bool,UserEvents::SENDER_ID)` | input restore/disable | named candidate; Addictol wrapper uses `1432984/2268263` |
| `0x1677A00` | `BSInputEnableManager::OtherEventEnabled(uint)` | input state query | named candidate only |

The PDB also names `ExtraDataList::SetLinkedRefData` at `0x2830F0`,
`ExtraLinkedRef::SetLinkedRef` at `0x2A53A0`, and
`ExtraDataList::SetLinkedRefsTransient` at `0x283210`. These clarify the linked-
reference subsystem but are not substitutes for the public
`TESObjectREFR::SetLinkedRef` semantics. It contains no symbols named
`IsIdleLoading` or `RunActionOnActor`; absence is not proof those behaviors no
longer exist.

Additional Addictol CommonLib candidates overlapping the unresolved inventory:

| Interface | Addictol mapping/declaration | Grade |
|---|---|---|
| `BSModelDB::Demand1` | `1066398 / 2275153` | corroborating source candidate; existing NAF recovery remains authoritative |
| `BSModelDB::Demand2` | `1225688 / 2275154` | corroborating source candidate; existing NAF recovery remains authoritative |
| `BSInputEnableManager` singleton | `781703 / 2689007 / 4796297`, `BSInputEnableManager**` | layout/API candidate; requires target-runtime verification before new use |
| `BSInputEnableManager` layout | singleton base at `0x108`, locks/flags/layers through size `0x180` | matching maintained-style layout; ABI-sensitive and not promoted from headers alone |
| `PlayerCamera` | singleton plus push/pop/set-state APIs; declared size `0x1B0` | useful camera recovery surface, not evidence for NAF's unresolved detour ID `541110` |
| HUD messaging | `ShowHUDMessage`, `PushHUDMode`, `PopHUDMode`, HUD event types including `ClearHUDMessagesEvent` | suggests an event-oriented maintained path; does not yet identify NAF's historical queue clear operation |
| package/process | `AIProcess`, `TESPackageEvent`, and a package-location patch module | useful class/call-site context; no direct mapping of NAF's legacy package hook `609985` found |
| hit/effect | `AttachLightHitEffectVisitor` vtable hook and magic-effect event modules | useful RTTI/event reference; no proof of NAF's transient-effect wrapper ABI |
| Papyrus | VM declarations plus GC/max-ops patches and lifecycle-deferred binding | useful lifecycle/patch examples; does not resolve NAF serialization/Papyrus readiness by itself |
| animation subgraphs | runtime-specific calls, signatures, and transactional patch/rollback code | useful methodology and call-graph evidence; not a direct match for NAF's two graph detours yet |

### Crash Logger object and thread semantics

The Crash Logger does not identify `Main`, `TESObjectCELL`, or
`BSJobs::JobThread` from one hard-coded offset database. Function names come
from the selected PDB. Object recognition first validates that a candidate
vtable is in a loaded module's `.rdata`, reads the preceding MSVC Complete
Object Locator, resolves its TypeDescriptor, verifies the module's `type_info`,
and then demangles RTTI. Fallout-specific filters are keyed by mangled RTTI
names; `PlayerCharacter`, `TESObjectREFR`, and `TESObjectCELL` filters interpret
selected CommonLib fields such as form ID/type, base object, extra data, and
parent cell inside exception guards.

The PDB names two `BSJobs::JobThread` functions at RVAs `0x16879C0` and
`0x1687A60` after section-base adjustment, and Addictol CommonLib supplies its
RTTI/vtable identity. This can help label crash stacks and validate object
identity, but it does not prove `Main::threadID` semantics or the scheduler
contract. The crash logger's field reads inherit the pinned CommonLib layouts;
successful diagnostic output is runtime evidence for the fields exercised,
while the declarations alone remain **reference layout evidence**.

Overall rule: Addictol and its PDB may identify high-quality candidates and
provide independent corroboration. Any NAF engine-facing relocation, virtual
slot, ABI-sensitive layout, detour entry, or interior instruction site remains
subject to the existing old/new binary semantic verification threshold.

## Dear-Modding-FO4/CommonLibF4 reference audit

Audited standalone commit `54c5f576e31410f393abebeebca366e970e824f4`
with `commonlib-shared` commit
`dfd28a41a832108c4de1367608bd42e1ba477212`. This is the same CommonLib
revision pinned by the audited Addictol checkout; AddictolCrashLogger pins the
older CommonLib commit `a08c731eead4dfc6ea55aea4649c22aec05a47fe`.
The repository remains reference-only and was not substituted for NAF's
selected dependency.

### Runtime and relocation architecture

The fork explicitly models three runtime classes:

| Runtime index | Version range used by the code | Intended label |
|---:|---|---|
| 0 | exactly `1.10.163.0` | OG |
| 1 | greater than OG through `1.10.984.0` | NG |
| 2 | greater than `1.10.984.0`, including `1.11.221.0` | AE |

`REL::VariantID` accepts a runtime-ordered mixture of Address Library IDs and
`REL::Offset` RVAs. Missing trailing operands repeat the last supplied value,
so `{ og, ng }` means “use the NG identifier for AE too”; it does not perform a
semantic translation. Three-operand declarations are used where AE has a
distinct ID namespace. The fork's recent history explicitly includes runtime
`1.11.221.0` support and synchronization of libxse's 1.11.221 data.

The Address Library lookup is fail-closed. Older sorted formats require
`lower_bound` to return an entry whose ID is exactly equal. New dense format 5
requires the ID to be in range and its resolved offset to be nonzero. The
loader also validates runtime version and the library hash. This is materially
safer than the pre-fix alandtse lookup that returned the next sorted record for
a missing NG ID, and independently validates the strict check now carried by
NAF's selected dependency.

The AE identifiers are not numerically compatible with NAF's supplied legacy-
format 1.11.221 database. For example, the fork declares player singleton AE
ID `4798212`; looking that number up in NAF's supplied database yields RVA
`0x31E2DD0`, not the player global. The fork expects its newer AE namespace/
database format. Numeric IDs must therefore never be copied between these
schemes without resolving them using the matching database.

### PlayerCharacter singleton reconciliation

The fork declares:

```text
OG ID 303410 / NG ID 2690919 / AE ID 4798212
REL::Relocation<NiPointer<PlayerCharacter>*>
return singleton->get()
```

This agrees with historical Deweh, PANDA, alandtse and Addictol on the storage
model: one intrusive `NiPointer<PlayerCharacter>` global and one pointer
extraction, not an extra global-wrapper dereference. It materially clarifies
runtime selection: `2690919` belongs to 1.10.984, while 1.11.221 selects the
third entry. The fork does not ship the matching AE Address Library in its
source tree, so AE ID `4798212` cannot be resolved directly with the audited
source alone. It must not be interpreted through NAF's supplied older
namespace, where it maps elsewhere.

The fork therefore does not independently produce numeric RVA `0x32D22E0` from
its own files. It does strongly corroborate the runtime-classification and
indirection analysis that led to that RVA. NAF's mapping remains independently
**disassembly verified** as old ID `303410` / RVA `0x5AA4388` to supplied-library
ID `2698073` / RVA `0x32D22E0`, using the matched constructors/initializers and
global retain/store/release sequence. There is no semantic conflict: the two
AE IDs belong to different Address Library namespaces.

### TESObjectREFR::Get3D and a declaration-order defect

All compared families label the no-argument virtual `Get3D()` as slot `0x8C`
(byte offset `0x460`). Historical Deweh and PANDA place a separate preceding
overload in slot `0x8B`, then the no-argument method in `0x8C`. The audited
Dear-Modding fork, CommonLibF4-NG, and the currently selected alandtse header
instead declare the no-argument method first while retaining comments
`// 8C` and `// 8B`:

```text
declared first:  Get3D() const                 // comment 8C
declared second: Get3D(bool) const             // comment 8B
```

C++ vtable order follows declaration order, not comments. This is an ABI-
significant discrepancy, not cosmetic naming drift.

The Addictol Crash Logger PDB distinguishes concrete 1.11.221 symbols:

- RVA `0x50D9A0`: `TESObjectREFR::Get3D(void)`;
- RVA `0x50DA00`: `TESObjectREFR::Get3D(BSScrapArray<NiAVObject*>&)`;
- RVA `0xD5BCC0`: `PlayerCharacter::Get3D(void)`.

Independent inspection of the clean 1.11.221 executable and NAF's supplied
Address Library resolves the primary `TESObjectREFR` vtable (ID `179707`) to
RVA `0x249CBC8`; entry `0x8C` points to PDB-named RVA `0x50D9A0`. The primary
`PlayerCharacter` vtable (ID `1400465`) resolves to RVA `0x2564848`; entry
`0x8C` points to PDB-named RVA `0xD5BCC0`. Thus the no-argument call is now
**statically verified with Address Library, vtable, clean-binary and PDB
evidence** at slot `0x8C` / byte offset `0x460`.

The selected header's source ordering at the time of this audit would dispatch
a no-argument source call through physical slot `0x8B`, despite its `0x8C`
comment. The PDB-named array-reference overload at `0x50DA00` initially made
the slot-`0x8B` signature suspect; the subsequent vtable/disassembly audit
below established that it is a separate non-virtual helper and that physical
slot `0x8B` really is the first-person `bool` virtual. This does not establish
worker-thread safety. It does establish why corrected test six could not be
rerun using ordinary virtual dispatch from the then-misordered declaration.
No diagnostic was changed during this reference-only audit.

### Unresolved-interface findings

| Interface | Dear-Modding declaration/address evidence | Comparison and grade |
|---|---|---|
| `FreeUpAllMarkers` | no wrapper/ID in CommonLib | PDB alone names RVA `0x55F490`; existing disassembly-verified guarded fallback remains authoritative |
| `SetLinkedRef` | `void TESObjectREFR::SetLinkedRef(Actor*, BGSKeyword*)`, IDs `192840 / 2202684`; AE repeats the latter | supplied 1.11.221 DB resolves `2202684` to PDB-named RVA `0x5645B0`; strong address/signature candidate, still needs focused caller/body verification because header first parameter is narrower than PDB's `TESObjectREFR*` |
| `GetAnimationGraphProjectName` | absent from holder wrapper and IDs | PDB-named RVA `0x818730` remains the best candidate; no new callable declaration |
| `InitializeActorInstant` | `bool(Actor&, bool)`, IDs `672857 / 2236393` | supplied DB resolves `2236393` to PDB-named RVA `0xE13E40`; independent source/address agreement, still binary-semantic verification required |
| `IsActiveGraphInTransition` | wrapper takes `const TESObjectREFR*`, IDs `839650 / 2214305` | supplied DB resolves to PDB-named RVA `0x806370`; PDB names a reference parameter, so exact const/pointer ABI semantics require checking |
| `IsIdleLoading` | not found | unresolved; no evidence of removal or replacement |
| `RunActionOnActor` | not found | unresolved; no evidence of removal or replacement |
| graph variable setters | holder wrappers and IDs `1437197/2214543`, `27400/2214545`, `732926/2214544` | agrees with already migrated maintained API surface; source/address evidence |
| graph manager holder | virtual slots `0x00-0x19`, including manager get/set at `0x04/0x05` | broad declaration agreement; slot comments remain declaration-only until a used slot is checked |
| `BSModelDB::Demand` | overloads returning `BSResource::ErrorCode`; IDs `1066398/2275153` and `1225688/2275154`; 12-byte `ArgsType` | agrees with NAF's recovered signatures/argument layout; supplied DB resolves target RVAs `0x17B3E30` and `0x17B3FB0` |
| `BSInputEnableManager` | singleton `781703/2689007/4796297`, size `0x180`, allocation and enable wrappers | useful three-runtime layout/API evidence; AE ID namespace must be resolved with matching DB before use |
| HUD clearing | empty `ClearHUDMessagesEvent` plus complete RTTI/vtable identities | strengthens event-subsystem identity but exposes no send/clear helper equivalent to NAF's historical stateful queue operation |
| player camera | size `0x1B0`; singleton, push/pop/set-state and mode operations | useful callable surface; does not identify legacy camera detour `541110` or prove transition equivalence |
| package/process | extensive `AIProcess`, actor package and `TESPackageEvent` declarations | useful reference surface but no direct NAF package-hook mapping |
| hit/effect | `DamageImpactData`, `HitData`, `TESHitEvent`, reference effect and art/shader APIs | layouts agree with selected CommonLib; no new proof for NAF's transient-effect behavior |
| Papyrus/events | maintained VM/event source wrappers with F4SE lifecycle support | useful API confirmation; no direct resolution of gated NAF registration semantics |
| collision/player 3D | filter no-collision helper, reference 3D update APIs, PlayerCharacter 3D overrides | callable candidates only; `Get3D` slot issue above is load-test relevant |

### Structure and Havok comparison

The following sizes and substantive member offsets agree with NAF's selected
alandtse dependency: `PlayerCharacter 0xE10`, `TESObjectREFR 0x110`,
`BSAnimationGraphManager 0xE0`, `BSAnimationGraphVariableCache 0x40`,
`BSAnimationGraphEvent 0x18`, `DamageImpactData 0x40`, `HitData 0xE0`,
`TESHitEvent 0x110`, `FreeCameraState 0x58`, `BSInputEnableManager 0x180`, and
`BSLightingShaderProperty 0xE8`. Graph-manager arrays, cache pointer/lock,
active-graph fields, hit-data offsets and input-manager locks/layers also match.
Most differences are namespace/type-wrapper spelling (`REX::TEnumSet` versus
`REX::EnumSet`) or header organization and do not justify churn.

The Dear-Modding fork supplies extensive generated Havok RTTI/vtable IDs but
does not declare the detailed `hkbCharacter`, `hkbBehaviorGraph`, generator or
root-generator layouts NAF historically depended upon. It is therefore less
useful than NAF's selected CommonLib plus the historical/PANDA/NG sources for
those ABI-sensitive interiors. Generated RTTI ID agreement is object-identity
evidence, not layout evidence.

The important structural disagreement exposed by this audit is the
`TESObjectREFR::Get3D` overload declaration order/signature. No other audited
size difference warrants changing NAF declarations at this stage.

### Addictol upstreaming and disposition

This repository is the generalized engine layer Addictol consumes, rather than
an independent implementation isolated from Addictol. Its history includes
explicit 1.11.221/libxse synchronization, hundreds of OG mappings, three-way
RTTI IDs, AE singleton mappings, player/camera/input helpers, animation-system
wrappers, hit-event layouts and model-demand APIs subsequently used by Addictol.
That makes it valuable corroborating collaborative RE evidence, but the shared
dependency lineage means agreement between Addictol and this fork is not two
fully independent sources.

Audit disposition:

- player singleton understanding is strengthened but its verified NAF RVA does
  not change;
- the fork's AE ID cannot be used with NAF's supplied Address Library namespace;
- `Get3D(void)` slot `0x8C` is now independently statically verified;
- the current selected header's declaration order is unsafe for the planned
  source-level virtual call;
- test six must remain on hold until that ABI issue is deliberately corrected
  or the verified slot is invoked through an equally narrow, guarded mechanism;
- no runtime code, diagnostic, hook, relocation or build artifact was changed
  during this reference-only audit.

## TESObjectREFR Get3D ABI correction and corrected test-six build

### Root cause and physical vtable reconstruction

The original test-six crash remains classified as an invalid-object failure,
not a `Get3D` worker-thread failure. The two independent defects were:

1. a two-ID relocation selected `2690919`, which is a 1.10.984 mapping in the
   Dear-Modding three-runtime model and resolves to unrelated packed data in
   NAF's supplied 1.11.221 Address Library; the resulting value was
   `0x700000008`;
2. even after correcting the singleton to supplied-library ID `2698073`, the
   selected CommonLib declaration placed the no-argument virtual before the
   preceding first-person overload, so ordinary C++ dispatch selected physical
   slot `0x8B` instead of verified slot `0x8C`.

The old mapped module uses loaded base `0x7FF6CD6D0000`. Old Address Library ID
`179707` resolves the `TESObjectREFR` primary vtable to RVA `0x2C87AE8`.
The supplied 1.11.221 library resolves the same vtable ID to RVA `0x249CBC8`;
ID `1400465` resolves the `PlayerCharacter` primary vtable to RVA `0x2564848`.
Reading the physical entries from those images gives:

| slot | byte | 1.10.163 `TESObjectREFR` target | 1.11.221 `TESObjectREFR` target | 1.11.221 `PlayerCharacter` target | established operation/evidence |
|---:|---:|---:|---:|---:|---|
| `0x88` | `0x440` | `0x404B30` | `0x50C5B0` | `0xD5BD60` | `Set3D(NiAVObject*, bool)`; exact PDB names for old/base NG/player NG where present, matching prologues |
| `0x89` | `0x448` | `0x405C30` | `0x50D700` | `0x50D700` | `ShouldBackgroundClone()` declaration; identical inherited NG target, old/NG bodies have matching member/global tail-call form |
| `0x8A` | `0x450` | `0x405C50` | `0x50D720` | `0xD777D0` | `IsReadyForAttach(IO_TASK_PRIORITY const&)`; base returns true, player override is PDB named at exact RVA `0xD777D0` |
| `0x8B` | `0x458` | `0x42AD90` | `0x524B60` | `0xD5BD40` | `Get3D(bool firstPerson) const`; base thunk dispatches slot `0x8C`; player override consumes `DL`, returns `[this+0xB78]` when true, otherwise tail-calls base no-argument implementation |
| `0x8C` | `0x460` | `0x405E50` | `0x50D9A0` | `0xD5BCC0` | `NiAVObject* Get3D() const`; exact old/base-NG/player-NG PDB names and direct vtable evidence; **disassembly verified virtual slot** |
| `0x8D` | `0x468` | `0x405EA0` | `0x50D9E0` | `0x50D9E0` | `NiAVObject* GetFullyLoaded3D() const`; inherited NG target; returns the 3D pointer through `[this+0xF0]+8` |
| `0x8E` | `0x470` | `0x42B9C0` | `0x524FE0` | `0xD8A540` | `Is3rdPersonVisible() const`; base returns true, player override reads a player-state bit |
| `0x8F` | `0x478` | `0x430510` | `0x527CF0` | `0xD62450` | `PopulateGraphProjectsToLoad(...) const`; player target has the exact PDB name/signature; old/NG base bodies have corresponding substantial implementations |

Some Crash Logger PDB records cover ranges containing smaller functions and do
not give exact public symbols for every base slot. Therefore the table does not
promote incidental containing-symbol names for `0x89`, `0x8B`, `0x8D`, or
`0x8E`. Their identities come from declaration continuity plus instruction and
override behavior, with the `0x8B` and `0x8C` pair independently established at
the instruction level.

### Actual Get3D overloads

The verified virtual pair is:

```cpp
virtual NiAVObject* Get3D(bool a_firstPerson) const; // physical 0x8B
virtual NiAVObject* Get3D() const;                   // physical 0x8C
```

The base slot-`0x8B` implementation at NG RVA `0x524B60` ignores the base
implementation's `DL` value and jumps through `[vtable+0x460]`; the
`PlayerCharacter` override at `0xD5BD40` consumes `DL`, returns the first-person
root at `this+0xB78` when requested, and otherwise jumps to base RVA
`0x50D9A0`. This supplies direct ABI/signature evidence for the `bool` overload.

PDB-named `TESObjectREFR::Get3D(BSScrapArray<NiAVObject*>&)` is a separate,
non-virtual helper at old RVA `0x405EC0` and NG RVA `0x50DA00` (supplied NG
Address Library ID `2201064`). It receives the reference in `RCX` and output
array in `RDX`, adjusts to the embedded object at `this+0x48`, and tail-dispatches
that object's array-population method at its vtable byte offset `0x30`.
Legitimate callers construct the scrap array, call this helper, then consume
the array; they do not consume `RAX`. The supported signature is therefore
`void Get3D(BSScrapArray<NiAVObject*>&)`. It is not a vtable member in the
`0x88-0x8F` region and does not replace the `bool` overload. Source-level
`const` qualification is not encoded by the observed machine call and is not
needed by NAF; no new wrapper was added.

### CommonLib correction and ABI guard

The selected CommonLib header now declares the `bool` overload first at
physical slot `0x8B`, followed by the no-argument overload at `0x8C`. No
synthetic virtual or padding slot was introduced. Slots `0x88-0x8A` and
`0x8D-0x8F` retain their prior declaration order, so the correction restores
the verified layout without shifting a neighboring method.

The corrected diagnostic retains normal C++ virtual dispatch but adds a
fail-closed development guard before the one bounded call. It:

- requires exact runtime `1.11.221`;
- resolves the singleton with supplied-library ID `2698073` and exactly one
  `NiPointer<PlayerCharacter>::get()`;
- logs the relocation/global address, raw intrusive pointer, resolved player,
  vtable, and physical slot-`0x8C` target separately;
- requires committed/readable player memory;
- requires the vtable and full slot access to lie inside Fallout4.exe `.rdata`;
- requires slot `0x8C` to equal module base plus verified player override RVA
  `0xD5BCC0`;
- suppresses the virtual call with a critical diagnostic if any check fails.

This guard detects future declaration/runtime drift; it is not an alternate raw
slot invocation. When all checks pass, the code calls
`playerCharacter->Get3D()` exactly once through corrected normal virtual
dispatch and logs the returned pointer without dereferencing or retaining it.

### Narrow NAF-used virtual audit

The audit was limited to virtual surfaces NAF actually calls or reaches in the
currently ported code:

| surface | result |
|---|---|
| `TESObjectREFR`/`Actor`/`PlayerCharacter::Get3D()` | declaration-order defect corrected; base and player physical `0x8C` targets verified |
| `TESObjectREFR::GetFullyLoaded3D()` | physical neighbor `0x8D` remains in the correct position; body/member-access semantics agree across old/NG |
| `Actor::DrawWeaponMagicHands` / `Actor::SetPosition` | adjacent declared slots `0xC9/0xCA` remain ascending and agree across selected, PANDA and historical headers; no declaration-order defect observed, but this audit does not newly verify their runtime behavior |
| `IAnimationGraphManagerHolder::GetAnimationGraphManagerImpl` | holder virtual region is ascending and the used method remains declared at `0x04` across compared forks; no overload-order discrepancy observed |
| camera and package/process calls currently used by NAF | the audited call sites use maintained non-virtual wrappers/relocations or already tracked hooks rather than a newly suspect declaration-order pair |
| event-holder access used by NAF | maintained inheritance/event-source access does not expose a descending overload pair at the used call sites |

The selected `TESObjectREFR` header still contains descending comment/order
pairs for `GetBiped` (`0xA2/0xA1`), `IsExplosion` (`0xB4/0xB3`), and
`IsProjectile` (`0xB6/0xB5`). NAF does not call these methods, so they were
recorded as **declaration-order suspect but out of the NAF-used surface** and
were not changed. No second NAF-used declaration-order defect was found in
this narrow pass. Declaration agreement alone remains weaker than binary slot
verification.

### Build and corrected test-six plan

CommonLibF4 and NAF were rebuilt as Release/x64/v143 with `COPY_BUILD=OFF`.
Compilation and linking completed with zero errors. The existing pinned `ik`
timestamp generator still emits its known build-information warning from the
contained build copy; neither the pinned `ik` submodule nor its numeric build
value was modified.

Corrected test six remains bounded to the verified serialized
`NonRendersafeAI` diagnostic callback after `QGameDataLoaded()` and
`Main::gameActive`. It retains delay bookkeeping/concurrency monitoring and all
gameplay subsystem gates. A successful rerun requires the logged singleton,
player, vtable and exact `0x8C` target validation; one before/after log pair;
no crash; and `maxInFlight=1`. Only that result may classify this one
`PlayerCharacter::Get3D()` read as runtime-safe in the observed worker context.
The DLL was built but was not deployed or executed in this stage.

## Exact-ID startup failure 1378547 and bounded relocation audit

The corrected test-six DLL failed closed on runtime `1.11.221.0` before the
bounded player diagnostic with:

```text
REL/IDDB.cpp(145): Failed to find the id within the address library: 1378547
```

This is a successful detection by the exact-ID hardening, not evidence about
`PlayerCharacter::Get3D()`: the virtual call was never reached. Approximate or
next-record lookup remains prohibited.

### Owner, startup path, and namespace

ID `1378547` belonged to the selected CommonLib declaration
`RE::CombatUtilities::fWorldGravity` in `RE/Bethesda/Utilities.h`. Its original
form was a namespace-scope `REL::Relocation<float>` initialized with the single
old-runtime ID. Because `RE/Fallout.h` includes this header, MSVC emitted a CRT
dynamic initializer into NAF even though NAF never reads world gravity. The
effective path was:

```text
Windows loader -> CRT _initterm -> fWorldGravity initializer
  -> REL::Relocation<float> -> REL::ID::address -> exact IDDB lookup -> fail
```

The supplied databases resolve the semantic pair as follows:

| runtime | supplied-library ID | RVA | evidence |
|---|---:|---:|---|
| 1.10.163 | `1378547` | `0x5AD87C0` | exact old database record; selected historical declaration |
| 1.11.221 | `2700340` | `0x33058E8` | exact current database record; Dear/CommonLib independently pairs and names the global |

Dear/CommonLib represents `WorldGravity` as `{ 1378547, 2700340 }` and repeats
the current entry for its NG/AE variants. Consequently this failure is **not**
another case where the second member of a two-runtime pair was actually a
1.10.984-only ID. It was an OG-only single ID incorrectly treated as
runtime-invariant. Evidence grade: **source/address-database corroborated**;
the current global's semantic name is not promoted as PDB/disassembly proof.

`CombatUtilities::GetWorldGravity()` now performs lazy function-local
resolution using `REL::RelocationID(1378547, 2700340)`. This both records the
correct supplied-library namespace and prevents an unused engine global from
being resolved before `F4SEPlugin_Load`.

### Eager relocation hazards

The pre-fix Release PDB and PE CRT initializer table exposed ten CommonLib
engine globals that were resolved merely by loading the DLL:

| declaration | old selected addressing | disposition |
|---|---|---|
| full-screen blur disable | `372489 / 2713225` | lazy accessor; current ID exists exactly |
| get-hit effect disable | `1523708 / 2713237` | lazy accessor; current ID exists exactly |
| power-armor Pip-Boy image flag | old-only `1065542` | lazy accessor with corroborated current `2713203` |
| radial-blur disable | `1500590 / 2713225` | lazy accessor; current ID exists exactly |
| temporal-AA mask | old-only `775377` | lazy accessor with corroborated current `2713052` |
| power-armor battery setting | old-only `1134204` | lazy accessor; retained as unresolved until invoked because Dear points at a different current global |
| world gravity | old-only `1378547` | lazy accessor; corrected current ID `2700340` |
| workshop placement data | old-only `1279207` | lazy accessor; deferred and unresolved if invoked |
| workshop current row | old-only `833923` | lazy accessor; deferred and unresolved if invoked |
| workshop placement handle | old-only `526727` | lazy accessor; deferred and unresolved if invoked |

The Dear AE workshop candidates (`4797261`, `4797160`, and `4797235`) were not
copied: their exact current records are useful candidates, but NAF does not use
these interfaces and their semantics have not been independently verified.
Likewise, the battery-setting ID is not changed merely because another fork
names a candidate.

After a full clean rebuild, none of the ten engine-global initializer symbols
remains in the Release PDB. Hook state objects still have ordinary CRT
construction, but their default-constructed relocation members do not resolve
an ID; registration of all other gameplay hooks remains gated. Therefore no
deferred subsystem address is known to resolve eagerly during startup.

### Bounded two-ID audit

The first pass was deliberately limited to interfaces transitively reachable
from plugin initialization, the diagnostic `NonRendersafeAI` hook, its guards,
the player singleton, and the one bounded `Get3D()` call.

| interface | old/current IDs | current status |
|---|---|---|
| full-screen blur disable | `372489 / 2713225` | exact current record; now unreachable at startup |
| get-hit effect disable | `1523708 / 2713237` | exact current record; now unreachable at startup |
| radial-blur disable | `1500590 / 2713225` | exact current record; now unreachable at startup |
| game-loop containing function | `338354 / 2228937` | exact, disassembly- and runtime-verified |
| `ProcessQueues` original | `882638 / 2229181` | exact, disassembly- and runtime-verified |
| `Main` singleton | `756304 / 2698043` | exact and exercised by prior diagnostics |
| `QGameDataLoaded` | `384264 / 2228956` | exact and exercised by prior diagnostics |
| player singleton | `303410 / 2698073` | exact; disassembly-verified `NiPointer<PlayerCharacter>` storage at RVA `0x32D22E0` |

Eight NAF/startup-diagnostic reachable two-ID interfaces were audited. All
eight select an exact record in the supplied 1.11.221 namespace; five engine
interfaces have the stronger existing static/runtime evidence listed above,
and the three image-space globals have exact-record plus independent-source
corroboration but are no longer startup reachable. Zero current second IDs in
this bounded set are 1.10.984-only and zero bounded-path relocations remain
unresolved. The already-corrected `2690919` player failure remains the known
historical example of a 1.10.984 ID incorrectly selected as AE; it is not part
of the current set.

`REL::IDDB::id2offset()` still requires `lower_bound` to return a record whose
ID equals the requested ID. Missing records fail closed with the requested ID
and runtime. No nearest-record fallback or fuzzy lookup was restored.

### Build and test-six readiness

A clean Release/x64/v143 rebuild completed with zero compiler and linker
errors and `COPY_BUILD=OFF`. The clean rebuild was necessary because an
initial incremental artifact retained the old precompiled-header initializer;
the final PDB dry audit confirms its removal. The known pinned-`ik` timestamp
warning remains confined to the generated build copy and does not alter the
historical submodule.

The corrected test-six behavior is unchanged: it resolves exact player ID
`2698073`, validates player memory, the Fallout4.exe `.rdata` vtable, and slot
`0x8C == base + 0xD5BCC0`, then performs exactly one normal C++ `Get3D()` call
without dereferencing the result. With the bounded startup relocation set now
clean, this diagnostic DLL is suitable to rerun but has not been deployed or
executed here.

## Corrected test-six result and test-seven eligibility diagnostic

The corrected sixth runtime diagnostic passed on Fallout 4 `1.11.221.0`.
Runtime evidence established all of the following in the tested environment:

- exact player singleton ID `2698073` resolved the expected
  `NiPointer<PlayerCharacter>` storage;
- one `get()` produced a plausible, readable player object;
- the player vtable lay in Fallout4.exe `.rdata` and physical slot `0x8C`
  equaled the verified `PlayerCharacter::Get3D(void)` target at RVA
  `0xD5BCC0`;
- corrected normal C++ virtual dispatch returned a valid, non-null 3D pointer;
- the call completed without a crash from the serialized, thread-migrating
  `NonRendersafeAI` callback;
- delay bookkeeping again completed `3 -> 2 -> 1 -> 0` and sampled callbacks
  retained `maxInFlight=1`.

The player singleton, corrected `TESObjectREFR`/`PlayerCharacter` virtual
ordering, slot `0x8C`, and this one bounded worker-context `Get3D()` call are
therefore **runtime validated for the tested 1.11.221 environment**. This does
not establish safety for other actor virtuals or arbitrary scene work.

### Historical read path and mutation boundary

The normal historical path after `ProcessQueues` is:

```text
SceneManager::UpdateScenes
  -> lock-protected scene-map snapshot
  -> per-scene recursive lock
  -> detach/status/noUpdate/actor-count reads
  -> resolve first actor NiPointer
  -> actor->parentCell and parentCell->loadedData reads
  -> IScene::Update                         [subsystem-specific work begins]
  -> IScene::ForEachActor
  -> resolve each actor NiPointer
  -> GameUtil::ActorIsEnabled
       -> TESForm::IsDeleted
       -> TESForm::IsDisabled
       -> TESObjectREFR::Get3D
  -> caller-provided scene update operation [first mutation varies by scene]
```

`UpdateScenes` itself can mutate before `IScene::Update` when it services a
queued detach, so invoking it is outside this diagnostic. `IScene::Update` is
also a virtual subsystem boundary and may immediately advance state, start
animation work, process packages, or perform transforms. Test seven therefore
constructs no scene and calls neither function. It reproduces only the common
cell-readiness and `ActorIsEnabled` reads on the lifetime-stable player.

### Exact test-seven read inventory

| order | read | mechanism/evidence | mutation or affinity assessment |
|---:|---|---|---|
| 1 | game loaded/active | `Main::QGameDataLoaded()` IDs `384264/2228956`; `Main::GetSingleton()` IDs `756304/2698043` | exact current IDs and previously runtime exercised; observational gate |
| 2 | player singleton | IDs `303410/2698073`, one `NiPointer::get()` | exact current ID, disassembly verified and test-six runtime validated |
| 3 | player object/vtable | `VirtualQuery`, module `.rdata`, physical slot inspection | local validation only; fails closed |
| 4 | parent cell | inline `TESObjectREFR::GetParentCell()`, member offset `0xB8` | direct field read; offset agrees across historical Deweh, NG, PANDA, selected and Dear declarations |
| 5 | cell loaded data | `TESObjectCELL::loadedData`, member offset `0xD0` | direct field read after committed/readable cell validation; same offset across the compared declarations |
| 6 | form flags | inline `TESForm::GetFormFlags()`, member offset `0x10` | direct local field read; no relocation or virtual call |
| 7 | deleted state | `formFlags` bit 5 through inline `IsDeleted()` | pure mask; identical historical/NG/PANDA/selected/Dear implementation |
| 8 | disabled state | `formFlags` bit 11 through inline `IsDisabled()` | pure mask; identical historical/NG/PANDA/selected/Dear implementation |
| 9 | loaded 3D | normal virtual `PlayerCharacter::Get3D()` at physical slot `0x8C` | disassembly verified and test-six runtime validated; returned pointer is logged only |

There are no new relocations or virtual slots in test seven. All relocations
reachable by the diagnostic and its update hook have exact records in the
supplied 1.11.221 database and retain their prior evidence grades. No reachable
method depends on an eager unresolved global.

The field reads are genuinely local in the selected headers and compile to no
engine call. `IsDeleted()` and `IsDisabled()` are not source-qualified `const`,
but their bodies only mask `formFlags`; the missing qualifier does not imply a
hidden mutation. `Get3D()` remains the only engine-dispatched/read-looking call.
Its bounded use is runtime validated, but that result remains specific to this
object and observed worker context. Reading `parentCell->loadedData` does not
request or create loaded data; the diagnostic rejects an unreadable cell rather
than attempting recovery or lazy loading.

### Test-seven implementation and plan

The one-shot diagnostic runs only after `QGameDataLoaded()` and
`Main::gameActive`. It validates the player exactly as test six did, validates
the complete parent-cell object range with `VirtualQuery`, then records:

- player and worker thread;
- raw form flags and decoded deleted/disabled bits;
- parent cell and existing loaded-data pointer;
- current player 3D pointer;
- separate `cellReady`, `actorEnabled`, and combined synthetic eligibility;
- an explicit `mutationPerformed=false` marker.

The `Get3D()` call is short-circuited exactly like `ActorIsEnabled` if deleted
or disabled. Any failed player, vtable, slot, or cell validation suppresses the
remaining sequence. No pointer is retained and no returned object is
dereferenced. Scene progression, detach processing, animation, packages,
ordered callbacks, equipment, collision, transforms, player-3D refresh,
morphs, LooksMenu, HUD/UI, studio, Papyrus, serialization, camera and all other
gameplay hooks remain gated. Existing delay and concurrency diagnostics remain
active.

Test-seven runtime plan:

1. launch Fallout 4 1.11.221 through F4SE 0.7.8 and load an ordinary save;
2. remain idle briefly without opening NAF UI or starting a scene;
3. exit normally and inspect `NAF.log`;
4. require one `Test-seven eligibility diagnostic complete` entry with valid
   pointers, `mutationPerformed=false`, continued delay transitions, no
   concurrency warning, and `maxInFlight=1`.

A pass validates only this synthetic player eligibility read sequence. It does
not yet authorize real scene iteration, handle resolution for arbitrary actors,
or any state mutation.

## Test-seven result and test-eight mutation selection audit

Test seven passed on Fallout 4 `1.11.221.0`. The one-shot diagnostic reported
`deleted=false`, `disabled=false`, readable parent-cell and loaded-cell-data
pointers, a non-null player 3D, `cellReady=true`, `actorEnabled=true`,
`syntheticEligible=true`, and `mutationPerformed=false`. Concurrency monitoring
remained `maxInFlight=1`.

The following exact path is now **runtime validated in the tested environment**:

- player singleton ID `2698073` and one `NiPointer<PlayerCharacter>::get()`;
- `TESForm::formFlags` at `+0x10` and deleted/disabled bit decoding;
- `TESObjectREFR::parentCell` at `+0xB8`;
- `TESObjectCELL::loadedData` at `+0xD0`;
- corrected `PlayerCharacter::Get3D()` physical slot `0x8C`;
- the combined synthetic actor-eligibility result;
- this bounded sequence from the observed serialized, thread-migrating
  `NonRendersafeAI` worker context.

This promotion remains specific to these reads and does not extend to other
actor virtuals, arbitrary references, scene iteration, or mutation.

### Reversible-mutation ranking

| rank | candidate | forward/rollback evidence | target and side-effect assessment | disposition |
|---:|---|---|---|---|
| 1 | reference keyword add/remove | `AddKeyword` old/current IDs `640674/2200860`; `RemoveKeyword` `281170/2200861` | membership operations are adjacent inverses, but both mark the reference changed with flag `0x80000000`; no isolated reference/test keyword pair is currently available | rejected for this build |
| 2 | local no-collision bit | direct `LOADED_REF_DATA::flags` bit `0x0800`, disassembly-verified historical behavior | exact bit pre-state and rollback are possible, but the only bounded object is the player and player collision is explicitly excluded | blocked on controlled non-player reference |
| 3 | actor favor state | query/set IDs `46113/2230148` and `391670/2230149`, disassembly verified | setter has player/package/process side effects when clearing; historical scene path excludes the player | rejected |
| 4 | restrained state | setter IDs `316742/2229910`, disassembly verified | enabling invokes additional interaction/movement behavior, so a false call does not restore the complete pre-state | rejected |
| 5 | collision disable/enable | IDs `475952/2229653` and `1274117/2229652` | character-controller/physics transition has broader state and player collision is excluded | rejected |
| 6 | transform/scale | maintained or recovered setters | visible physics/3D consequences and expressly outside test-eight scope | rejected |

Animation, inventory/equipment, morphs, HUD/UI, camera, packages, player-3D
refresh, studio, Papyrus and serialization remain excluded without further
ranking because the requested stage already classifies them as too broad.

### Keyword operation evidence and rollback defect

Focused old/current disassembly independently verifies the add mapping:

| operation | 1.10.163 | 1.11.221 | semantic evidence |
|---|---:|---:|---|
| add reference keyword | ID `640674`, RVA `0x3F7040` | ID `2200860`, RVA `0x4FDDE0` | instruction-identical control flow: null guard, `IKeywordFormBase::HasKeyword`, extra-keyword update with `true`, virtual change mark `0x80000000`, common post-update call |
| remove reference keyword | ID `281170`, RVA `0x3F70E0` | ID `2200861`, RVA `0x4FDE80` | same structure with the membership test reversed and extra-keyword update with `false`; independently verified earlier and reconfirmed here |

The functions are true membership inverses, not merely similarly named
candidates. They are not complete state inverses: a successful add and a
successful remove both invoke the reference change-mark path with
`0x80000000`. Membership can be restored immediately, but the diagnostic
cannot truthfully assert that the reference's change/save state equals its
pre-test state without manipulating additional internal change tracking. Such
manipulation would expand rather than bound the mutation.

The project contains NAF-specific keywords, but they have actual framework
semantics (`NAFInScene`, walk-linked-reference use, exclusion, and related
flows). None is a dedicated diagnostic-only form. Mutating one on the player
would violate the requirement not to use the player's gameplay keyword state.
Creating a synthetic keyword through `IFormFactory` was also rejected: the
selected CommonLib's factory-array second ID `2689177` is absent from the
supplied 1.11.221 database (the exact current record is `4796464` at RVA
`0x30E0950`), and object creation/destruction plus worker-thread allocation
would introduce new lifetime and thread-affinity questions solely to enable the
test. No ID was changed or promoted for that unused path.

### Test-eight disposition and exact blocker

No mutation is enabled and no test-eight DLL is produced in this stage. The
existing test-seven DLL must not be relabeled as a mutation diagnostic.

The smallest missing input is a disposable, non-player reference in a test
save whose identity and lifecycle are controlled. For the lowest-risk
candidate, it should be a nonessential actor/reference with loaded
`LOADED_REF_DATA`, not used by quests, packages, workshops, ownership,
dialogue, factions or persistence-sensitive gameplay. Its exact FormID/load
order (or a small dedicated test plugin containing it) is required. With that
target, the direct no-collision bit can be tested transactionally by capturing
bit `0x0800`, toggling it once, verifying the changed bit, restoring the exact
original bit, and verifying restoration. No Havok function or new relocation
would be involved.

Until such an isolated target exists, crossing the mutation boundary would
either modify prohibited player state or falsely claim rollback completeness.

## Micro-diagnostic retirement and production HUD restoration

The one-field/one-operation diagnostic phase ended after test seven. Test seven
passed and promoted the player singleton, corrected `Get3D` ABI, bounded player
eligibility reads, recovered `NonRendersafeAI` hook, observed serialized worker
scheduling, and local delay bookkeeping as recorded above. Those engine and ABI
fixes remain in place.

Test-only callback sampling, in-flight counters, QPC/thread sampling, seeded
`delayCounter`, synthetic eligibility, player/vtable logging, and one-shot
`Get3D` behavior have now been removed. The verified game-loop patch and its
instruction/target sanity guard remain installed, but its scene, ordered-action,
and studio workloads are explicitly gated by `kGameplayUpdateWorkEnabled`.
This preserves the verified update infrastructure without allowing the HUD
stage to enter unrelated gameplay systems.

### HUD dependency graph and initialization boundary

The production HUD-only lifecycle is:

1. exact runtime/F4SE metadata and exact-ID Address Library initialization;
2. guarded `FreeUpAllMarkers` RVA validation;
3. verified `NonRendersafeAI::ProcessQueues` hook installation, with NAF gameplay
   update work disabled;
4. F4SE messaging registration;
5. on `kGameDataReady(true)`, obtain `UI`, register the NAF HUD manager for
   `MenuOpenCloseEvent` and `HUDModeEvent`, and register only `NAFHUDMenu`;
6. when the engine opens `HUDMenu`, enqueue `NAFHUDMenu` through
   `UIMessageQueue`; load `Interface/NAFHUDMenu.swf` through
   `BSScaleformManager::LoadMovieEx`; update translations through the real
   menu's `AdvanceMovie` callback.

Serialization, Papyrus, `Data::Global` game-form linking, NAF main/studio menus,
scenes, graph/animation hooks, face/Lip, package hooks, camera, LooksMenu/morph,
studio alignment, and arbitrary ordered gameplay actions remain gated. The HUD
overlay's own persistent state currently exists only for the process lifetime
because serialization is deliberately not restored in this stage.

### Historical HUD hook and maintained-event replacement

Legacy ID `1385889` resolves in 1.10.163 to RVA `0x1279160`, PDB-named
`HUDMenu::OnHUDMessage_SetMode(HUDData&)`. At `+0xA5` the function moves the
`HUDMenu*` held in `RSI` into `RCX`; the five-byte direct call beginning at
`+0xA8` invokes old ID `1224312`, RVA `0x1279770`, PDB-named
`HUDMenu::OnHUDModeChanged(void)`. NAF replaced that call, scanned
`HUDMenu::hudModes` for `"Pipboy"`, set the NAF HUD alpha to zero or one, and
then invoked the original function. The hook therefore observed HUD-mode state;
it did not alter the engine's mode transition.

The binary interior patch is retired rather than translated. Maintained
CommonLib models `HUDModeEvent` as a pointer to the current
`BSTArray<HUDModeType>`, which supplies precisely the state consumed by the old
hook. Dear-Modding's three-runtime mapping identifies AE event-source storage;
the supplied 1.11.221 database independently resolves exact ID `4801988` to RVA
`0x3269880`. The 1.11.221 PDB also names the event source, its sink registration
function, and multiple HUD data-model consumers. NAF now subscribes to that
source and performs the same `Pipboy` membership/alpha transition. This is a
maintained-event semantic replacement; there is intentionally no NG interior
offset or trampoline for the old hook.

Evidence grade: historical hook is disassembly/PDB verified; the replacement
event layout and source are maintained-CommonLib plus PDB and exact-address
verified. The observable alpha behavior remains runtime-test-required.

### Exact 1.11.221 HUD relocation audit

The audit exposed several selected CommonLib two-ID declarations whose second
ID described 1.10.984, not 1.11.221. Exact lookup correctly showed them missing.
The reachable HUD surface now uses the following exact records:

| interface | old ID | exact 1.11.221 ID / RVA | evidence |
|---|---:|---:|---|
| `BSTGlobalEvent` singleton | `1424022` | `4796078` / `0x30DBE40` | Dear three-runtime mapping; exact DB |
| `HUDModeEvent` source storage | `683142` | `4801988` / `0x3269880` | Dear mapping, PDB event symbols, exact DB |
| `ApplyColorUpdateEvent` source | `421543` | `2707340` / `0x3DA7440` | Dear mapping; exact DB |
| `UI` singleton | `548587` | `4796314` / `0x30DD8B0` | Dear mapping; exact DB |
| `UI::RegisterMenu` | `1519575` | `2284766` / `0x1A82210` | Dear mapping; exact DB |
| `UIMessageQueue` singleton | `82123` | `4796377` / `0x30DDAA0` | PDB `g_UIMessageQueue`; exact DB |
| `UIMessageQueue::AddMessage` | `1182019` | `2284929` / `0x1A89170` | PDB function name; exact DB |
| `BSScaleformManager` singleton | `106578` | `4796889` / `0x30EAD10` | PDB `g_BSScaleformManager`; exact DB |
| `BSScaleformManager::LoadMovie` | `1526234` | `2287422` / `0x1B1AD10` | PDB function name; exact DB |
| `GetMovieFilename` | `1191277` | `2287428` / `0x1B1B500` | Dear mapping; exact DB |
| `InitMovieViewport` | `206895` | `4494254` / `0x1B1B670` | Dear AE mapping; exact DB |
| file-uncache setting | `454316` | `2667999` / `0x2F50298` | Dear mapping; exact DB |
| `BSSystemFileStreamer::UncacheAll` | `40925` | `2268797` / `0x1680F80` | Dear mapping; exact DB |
| `BSGFxShaderFXTarget::CreateAndSetFiltersToHUD` | `876001` | `2287027` / `0x1B04040` | existing maintained mapping; exact DB |
| `SendHUDMessage::ShowHUDMessage` | `1163005` | `2222440` / `0xA36F60` | previously recovered; exact DB |
| HUD message clear wrapper | `973227` | `2222460` / `0xA376D0` | disassembly verified earlier; exact DB |

The recovered clear implementation continues through ID `2222507`; enqueue
siblings remain `2222482/2222510`. Clearing is not reached by HUD-only startup,
because its existing NAF callers are in the gated ordered-action workload.
Exact-ID lookup remains fail-closed, with no nearest-record fallback and no new
raw-RVA exception.

### Build and HUD subsystem test plan

Release x64/v143 builds and links with `COPY_BUILD=OFF`. Output:
`f4se-plugin/build-1.11.221/Release/NAF.dll`. SHA-256:
`A7537C99100933A1B3A2EB339219125FC5F6817E5E179AB00409EDFB99A38454`.

No file was deployed. The source tree contains `ck/project_files/NAF.esp` but
does not contain `Interface/NAFHUDMenu.swf`; runtime testing must retain the
original NAF interface assets, including that SWF, alongside this DLL.

HUD subsystem runtime test:

1. deploy only the newly built `NAF.dll` over a complete NAF installation that
   already supplies `NAF.esp` and `Interface/NAFHUDMenu.swf`;
2. launch Fallout 4 1.11.221 through F4SE 0.7.8 and reach the main menu;
3. load an ordinary save and confirm the log records HUD event/menu registration
   and `NAF HUD menu loaded`;
4. exercise normal HUD visibility without starting a NAF scene: open and close
   the Pip-Boy, and open/close ordinary menus that hide/show `HUDMenu`;
5. confirm the overlay follows `HUDMenu`, hides during Pip-Boy mode, restores
   afterward, and the game exits normally;
6. preserve `NAF.log`, F4SE log, and any Addictol Crash Logger report.

This is a production HUD-path test, not a release-readiness claim. HUD mode
event delivery, Scaleform asset loading, overlay hide/show, alpha restoration,
and shutdown cleanup remain runtime-test-required. All unrelated subsystem
hooks and gameplay initialization remain disabled.

## HUD restoration regression audit

### Runtime failure and exact owner of ID 2193197

The first HUD-enabled launch failed closed after `kGameDataReady(true)` and
before HUD event registration completed:

```text
REL/IDDB.cpp(145): Failed to find the id within the address library: 2193197
```

The owner is `MemoryManager::GetSingleton()` in selected CommonLibF4's
`MemoryManager.h`. The historical declaration was
`RelocationID(343176, 2193197)`. Dear-Modding/CommonLibF4 and Addictol model it
explicitly as the three-runtime tuple `{343176, 2193197, 4471522}`: `2193197`
is the 1.10.984 ID, while `4471522` is the 1.11.221 entry. In NAF's supplied
1.11.221 database, `2193197` is absent and exact ID `4471522` resolves to RVA
`0x1DFF80`.

The exact runtime call chain was:

```text
F4SE kGameDataReady(true)
 -> MessageHandler
 -> HUDManager::RegisterHUDModeEvents
 -> HUDModeEvent::GetEventSource (ID 4801988)
 -> null source requires new BSTGlobalEvent::EventSource
 -> CommonLib engine allocator
 -> MemoryManager::GetSingleton
 -> exact lookup of stale ID 2193197
```

This relocation is lazy but genuinely required. It was not an unused eager
global. Focused 1.10.163/1.11.221 disassembly verifies the replacement: both
functions test the initialization state for `2`, obtain the manager storage,
invoke their runtime-specific initialization helper when needed, and return
the manager. Old ID `343176` is RVA `0x48A0`; current ID `4471522` is RVA
`0x1DFF80`. Evidence grade: **three-runtime namespace identification plus
disassembly-verified function**.

### Regression and build-tree comparison

The HUD commits `c1560a7`, `41eb44c`, and `7deba89`, and CommonLib commit
`ef33245`, did not revert the earlier safety changes. The actual headers and
sources used by the generated Visual Studio projects retain:

- exact-match/fail-closed `IDDB::id2offset`; there is no nearest-record path;
- `PlayerCharacter::GetSingleton()` IDs `303410/2698073`, with
  `NiPointer<PlayerCharacter>` storage and one `get()`; `2690919` is absent;
- physical `TESObjectREFR` declaration order `Get3D(bool)` at slot `0x8B`, then
  `Get3D()` at slot `0x8C`;
- the ten function-local lazy engine-global relocations from CommonLib commit
  `d4eb940`, including `CombatUtilities::fWorldGravity` IDs
  `1378547/2700340`;
- the sole raw-RVA exception at `FreeUpAllMarkers` entry `0x55F490`, with its
  exact-runtime and prologue guards; `0x55F4B0` is not a callable entry;
- the verified game-loop containing ID `2228937 + 0x1E`, including direct-call
  validation against `ProcessQueues` ID `2229181`.

The failed DLL was built from CommonLib commit `ef33245`, whose ancestry
contains `d4eb940`, `66ad564`, and `6bfc088`. `CMakeCache.txt`, `NAF.vcxproj`,
and `CommonLibF4.vcxproj` all resolve includes and sources to the checked-out
`f4se-plugin/extern/CommonLibF4/CommonLibF4` tree. No vcpkg, PANDA,
Dear-Modding, alternate alandtse checkout, or stale generated include directory
is selected.

### Bounded reachable relocation sweep

The failure exposed one three-runtime collapse and the bounded sweep found 16
additional OG-only declarations on the newly reachable menu path before a
second launch. They are now old/current pairs:

| group | corrected 1.11.221 IDs |
|---|---|
| `IMenu` | `ShouldHandleEvent` `2287392`; `PassesRenderConditionText` `2287379`; `OnSetSafeRect` `2287375`; `ProcessScaleformEvent` `2287395`; `RefreshPlatform` `2287374` |
| Scaleform wrappers | `BSGFxObject::AcquireFlashObjectByPath` `2287015`; `BSGFxShaderFXTarget::ProcessEvent` `2287033` |
| `GameMenuBase` virtuals | `SetIsTopButtonBar` `2223204`; `OnMenuDisplayStateChanged` `2223205`; shader-quad cache/transfer `2223200/2223201`; viewport `2223202`; append shader information `2223203` |
| menu behavior | `UIUtils::PlayMenuSound` `2249707`; `SendHUDMessage::PushHUDMode` `2222443`; `PopHUDMode` `2222444` |

These IDs exist exactly in the supplied database at the Dear-mapped RVAs; PDB
names independently agree where public symbols are present. No Dear/Addictol
AE numeric ID was copied across namespaces: every promoted number is the exact
record in NAF's supplied database. Commit `435bc9d` contains these CommonLib
corrections, including `MemoryManager::GetSingleton` ID `4471522`.

`tools/audit_hud_relocations.py` records the bounded startup/game-loop/HUD
surface and requires both exact ID presence and the expected RVA. It checks 47
interfaces: memory allocation and string pooling, event sources, UI/menu queue,
Scaleform construction, active menu virtual helpers, HUD message functions,
and the verified queue hook. The supplied 1.11.221 database passes all 47.
The audit deliberately excludes still-gated animation, face, package, camera,
studio, scene, Papyrus, and serialization surfaces.

### HUD header and ABI audit

The selected and Dear declarations agree on the NAF-used virtual order and on
the sizes of `IMenu` (`0x70`), `GameMenuBase` (`0xE0`), `BSGFxObject`
(`0x38`), `BSGFxDisplayObject` (`0x50`), `BSGFxShaderFXTarget` (`0xB0`),
`BSScaleformManager` (`0x40`), `UIMessageQueue` (`0x220`), and `HUDModeEvent`
(`0x08`). The HUD change introduced no vtable reordering or overload change.
The engine-invoked `IMenu`/`GameMenuBase` declarations keep matching physical
order; only their callable relocation bodies were corrected.

Dear's AE `UI` is `0x260`, while the selected header remains `0x250`, because
the tail containing the timer/boolean state differs. NAF does not construct or
size `UI`, nor access those tail fields. It uses the unchanged leading
event-source bases, `GetSingleton`, `RegisterSink`, and engine
`RegisterMenu`. This discrepancy is documented as an **inactive layout
concern/runtime-test-required**, not papered over with speculative padding.
`HUDMenu` internals are no longer used because `HUDModeEvent` replaced the
legacy interior hook.

### Fix and clean build

CommonLib commit `435bc9d` corrects the one 1.10.984-as-AE mapping and the 16
reachable OG-only declarations. Exact lookup remains unchanged. A full CMake
reconfigure followed by Release clean and rebuild compiled all CommonLib and
NAF objects and linked with zero errors and `COPY_BUILD=OFF`. The pinned `ik`
timestamp script still emits its previously documented development warnings;
it does not alter the pinned dependency tree or affect the DLL result.

The production HUD test remains the next runtime goal. The corrected build is
statically safe to rerun under the same HUD-only gates; no additional gameplay
subsystem was enabled by this audit. Output:
`f4se-plugin/build-1.11.221/Release/NAF.dll`; SHA-256:
`636E67D740BBEBAAF5B0CB090222A47B0597FB196CABC5A89EC32A6B83445BF3`.

## Post-registration HUD/Scaleform reachable-path audit

### Runtime progress and ID 912291

The next production test confirms the preceding fix: NAF loaded, installed the
verified queue hook, registered both HUD event sinks, and logged
`Production NAF HUD registered`. Approximately twelve seconds later, when the
registered menu was first requested, exact lookup rejected ID `912291`.

ID `912291` belongs to `Scaleform::GFx::Loader::CreateMovie` in
`GFx_Loader.h`. It is the 1.10.163 ID (old RVA `0x2041120`), not a valid
1.11.221 record. Dear-Modding and Addictol map the same function as
`{912291, 2284723}`. Supplied-database ID `2284723` resolves to RVA
`0x1A7EF60`, and the 1.11.221 PDB names that RVA
`Scaleform::GFx::Loader::CreateMovie(char*, uint, unsigned __int64)` with the
same arguments and return role. Evidence grade: **exact database, independent
Dear/Addictol mapping, and exact PDB symbol/signature**.

The delayed call chain is:

```text
HUDMenu opens after world load
 -> HUDManager::ProcessEvent(MenuOpenCloseEvent)
 -> UIMessageQueue::AddMessage("NAFHUDMenu", kShow)
 -> engine UI message dispatch invokes registered NAFHUDMenu::Create
 -> NAFHUDMenu constructor
 -> BSScaleformManager::LoadMovieEx
 -> Loader::CreateMovie
 -> stale OG-only ID 912291
```

No lookup occurs at registration because `CreateMovie` is a function-local
lazy relocation. The timing therefore identifies first menu construction/SWF
load, not static initialization or an unrelated subsystem.

### Why the 47-entry audit missed it

The earlier audit followed NAF through the Bethesda UI and
`BSScaleformManager` wrappers, but treated `LoadMovieEx` as the end of the
engine-facing boundary. Its local implementation crosses into the separate
`RE/Scaleform` header tree and calls `Loader::CreateMovie`; the enumerated audit
did not inspect that inline wrapper. Thus the path was statically reachable,
but hidden behind a source-tree/API boundary and a lazy inline relocation.

The reusable tool now checks the deeper Scaleform surface and also verifies
source-level selector text for known three-runtime hazards. This prevents a
database inventory from passing merely because the correct AE record is listed
while the compiled header still selects an OG or 1.10.984 ID.

### Deeper lazy relocation inventory

Sixteen additional exact records were added to the bounded audit:

| lazy interface/group | first production use | exact 1.11.221 mapping/status |
|---|---|---|
| `GFx::Loader::CreateMovie` | SWF load | `2284723` / `0x1A7EF60`; PDB verified |
| `GFx::Movie::Release` | movie-definition/movie reference balancing | `2287230` / `0x1B11B40`; exact |
| `GFx::Value::ObjectAddRef/ObjectRelease` | managed Scaleform value lifetime | `2286228/2286229`; exact |
| `GFx::Value::HasMember` | shader-target construction (`height`, `width`, background flag) | `2286078` / `0x1ACDCE0`; exact PDB symbol |
| `GFx::Value::GetMember` | same shader-target construction | `4494126` / `0x1AC8620`; Dear triple and exact PDB symbol |
| remaining used/value helpers | menu ActionScript calls and value operations | `GetArraySize 2285791`, `SetMember 2286589`, `Invoke 2286101`, `PushBack 2286424`, `RemoveElements 2286475`, `VisitMembers 2286786`, `GetDisplayInfo 2285873`; all exact |
| Scaleform global heap | Scaleform-managed lifetime | `2707353` / `0x3DA7500`; exact |
| SWF code registration | `SetMenuCodeObject` and first-frame function mapping | `RegisterCodeObject 2287415`, `MapCodeMethodToASFunction 2287419`; exact PDB symbols |

The sweep proactively found two additional bad declarations:

- `GFx::Value::HasMember` treated OG ID `788691` as invariant; it now uses
  old/current IDs `788691/2286078`.
- `GFx::Value::GetMember` used `2285936` as its second two-runtime ID. Dear's
  explicit tuple `{1517430, 2285936, 4494126}` establishes that `2285936` is
  1.10.984-only; the supplied 1.11.221 mapping is `4494126`.

Together with `CreateMovie`, CommonLib commit `1a7da43` corrects three deeper
Scaleform selectors. The audit now covers 63 exact HUD-stage records and four
source selector guards. It passes against the supplied 1.11.221 database.

### Deeper ABI/header review

The selected and Dear Scaleform declarations agree on the relevant physical
virtual order and signatures for `Loader`, `MovieDef`, and `Movie`, including
`MovieDef::CreateInstance`, `Movie::SetViewScaleMode`, `Advance`,
`SetBackgroundAlpha`, and `GetHeap`. Sizes agree for `Loader` (`0x20`),
`MovieDef` (`0x20`), `Movie` (`0x20`), `GFx::Value` (`0x20`), and
`Value::ObjectInterface` (`0x10`). `GFx::Value` wrappers also agree with Dear's
signatures, while the PDB independently confirms the three directly relevant
callable entries above. No new virtual-order, overload, or structure-layout
defect was found.

### Rebuild and next production test

A CMake reconfigure and full Release x64/v143 clean rebuild compiled CommonLib
and NAF and linked with zero errors. `COPY_BUILD=OFF`; no deployment occurred.
Output: `f4se-plugin/build-1.11.221/Release/NAF.dll`. SHA-256:
`38767BECC1509C4CB220E08ACFB04999603914265566C451F44B40E690AE7A72`.

The same production HUD test is safe to rerun. Expected progression is HUD
event registration, registered menu creation, successful SWF load and
`NAF HUD menu loaded`, followed by Pip-Boy/HUD visibility exercise. All
non-HUD gameplay subsystems remain gated.

## Production HUD runtime validation and graph-infrastructure restoration

### HUD runtime result

The production HUD build passed its subsystem runtime test on Fallout 4
1.11.221/F4SE 0.7.8. NAF loaded, the verified `NonRendersafeAI` hook remained
active, production menu registration completed, and `NAFHUDMenu` loaded and
rendered correctly in game. The exercised HUD/Scaleform path produced no
Address Library failure. This promotes HUD registration, SWF loading/rendering,
and the exercised visibility behavior to **runtime validated**. It does not
promote unrelated UI paths or any still-gated gameplay subsystem.

### Production graph dependency map

The minimum real graph path is:

```text
engine IAnimationGraphManagerHolder update
 -> NAF update detour
 -> original engine update
 -> update an already-registered NAF NodeAnimationGraph, if present

engine TESObjectREFR::Set3D
 -> NAF Set3D detour
 -> original engine Set3D
 -> refresh nodes for already-registered NAF graphs, if present
```

Graph-manager/project lookup, graph-variable setters, event-source retrieval,
and model demand are dependencies of real NAF graph construction. Engine graph
event sinks remain owned by the graph-manager lifecycle. NAF's manual animation
event dispatch and actor initialization/transition helpers are reached by scene
or playback behavior and remain gated at this stage. No synthetic graph or
actor was created merely to exercise the hooks.

### Recovered production graph hooks

| historical target | 1.11.221 target | purpose and evidence | disposition |
|---|---|---|---|
| ID `1492656`, RVA `0x81CF50` | ID `2214536`, RVA `0x818970` | PDB names both entries `IAnimationGraphManagerHolder::UpdateAnimationGraphManager(BSAnimationUpdateData&)`; old/new disassembly agrees on the update-data flag, holder virtuals, manager update, and intrusive release | disassembly verified; production detour enabled |
| ID `1216262`, RVA `0x404B30` | ID `2201055`, RVA `0x50C5B0` | PDB names both entries `TESObjectREFR::Set3D(NiAVObject*, bool)`; old/new bodies agree on argument and reference-state handling | disassembly verified; production detour enabled |

Both targets are exact Address Library lookups on 1.11.221 and are guarded by
small fixed-entry byte checks before DetourXS patches them. The update entry is
checked for `48 89 5C 24 08 48 89 74 24 18 57 48 83 EC 20 40`; Set3D is
checked for `44 88 44 24 18 48 89 54 24 10 48 89 4C 24 08 55`. Validation is
local and fail-closed; there is no pattern scan or alternate-address search.

The historical float overload constructs `BSAnimationUpdateData` with delta
time as its first field before calling the hooked overload. The hook now uses
the exact `BSAnimationUpdateData&` signature and preserves the engine `bool`
return value. The former NAF callback returned `void`; that was a real detour
ABI defect even though its pointer-sized arguments happened to line up.

One-time production logs confirm first natural graph-update and Set3D traffic
without adding a synthetic diagnostic path.

### Graph interfaces and ABI audit

The following current mappings were established or corrected:

| interface | historical | 1.11.221 | grade |
|---|---:|---:|---|
| `GetAnimationGraphProjectName` | `190119` / `0x81CE00` | `2214534` / `0x818730` | PDB plus old/new argument/data-flow comparison |
| `InitializeActorInstant` | `672857` / `0xF925F0` | `2236393` / `0xE13E40` | exact PDB name/signature and Dear corroboration |
| `IsActiveGraphInTransition` | `839650` / `0x802A30` | `2214305` / `0x806370` | exact PDB name/signature and Dear corroboration |
| `SetGraphVariableFloat` | `27400` | `2214545` / `0x818DE0` | exact current mapping |
| `SetGraphVariableInt` | `732926` | `2214544` / `0x818DC0` | exact current mapping |
| `RevertAnimationGraphManager` | `41382` | `2214541` / `0x818D10` | exact current mapping |

The prior project-name candidate `2230292` resolves to RVA `0xC9CB90` in an
unrelated Actor function region and was removed. `SetGraphVariableBool`
(`2214543`) and `GetEventSourcePointersFromGraph` (`2249847`) were already
correct. `BSModelDB::Demand` remains on verified IDs `2275153/2275154`.

The selected and Dear graph declarations agree on the physical
`IAnimationGraphManagerHolder` order. Disassembly independently confirms the
manager getter at slot `0x04` (byte `0x20`), `ShouldUpdate` at `0x0E`
(`0x70`), `PreUpdate` at `0x16` (`0xB0`), and `PostUpdate` at `0x17`
(`0xB8`). The interface size is `0x8`.

Relevant selected/Dear layout agreement was also confirmed for
`BSAnimationGraphEvent` (`0x18`), `BSAnimationGraphVariableCache` (`0x40`),
and `BSAnimationGraphManager` (`0xE0`), including its graph array, event queue,
variable cache, locks, active-graph index, and generation depth. No new
CommonLib graph declaration-order, signature, inheritance, or size defect was
found. Havok graph/character/root-generator wrappers remain unpromoted until
actual NAF playback exercises them.

`IsIdleLoading` and `RunActionOnActor` have named historical implementations,
but no sufficiently verified 1.11.221 counterpart was established. Their
playback call sites remain gated.

### Reachable relocation audit and build

`tools/audit_graph_relocations.py` checks 16 exact 1.11.221 graph-surface
records and nine source selector guards. It covers both hook entries, project
name, initialization/transition, graph setters/revert, event-source access,
model demand, resource helpers, and the verified player singleton. It passes
against the supplied database. The 63-record HUD audit also still passes.

A clean Release x64/v143 build completed with zero compiler/linker errors and
`COPY_BUILD=OFF`; no deployment occurred. Output:
`f4se-plugin/build-1.11.221/Release/NAF.dll`. SHA-256:
`D4DD444111215172B715D81DC10B972CDFAE192A6532BF896DB78C7003C97D17`.

### Production graph runtime test plan

1. Launch Fallout 4 1.11.221 through F4SE 0.7.8 and load an ordinary save.
2. Confirm logs report installation of graph-update ID `2214536` and Set3D ID
   `2201055`, followed by the one-time natural graph-update observation.
3. Move, idle, draw/holster a weapon, and change normal actor animation state.
   A normal 3D rebuild/cell transition may be used to exercise Set3D without
   invoking a NAF scene.
4. Confirm the existing production HUD still loads/renders and exercise its
   previously tested Pip-Boy/HUD visibility behavior.
5. Do not start a NAF scene. Exit normally and collect `NAF.log`, F4SE logs,
   and any crash report.

Production graph infrastructure and its two verified hooks are enabled. Full
scene playback, face/lip, package hooks, camera, studio behavior, Papyrus, and
serialization remain gated. Animation action/idle paths and the custom Havok
playback surface remain unresolved or runtime-test-required.

## Production face/lip subsystem restoration

### Known-good baseline

The production graph test passed on Fallout 4 1.11.221/F4SE 0.7.8. Both graph
hooks installed at IDs `2214536` and `2201055`; natural Set3D and graph-update
traffic reached the callbacks; no crash or Address Library failure occurred;
and the production HUD continued to initialize and render normally. HUD plus
the game-loop and graph infrastructure are therefore the current known-good
production baseline.

### Historical face/lip dependency map

The original face path is:

```text
BSFaceGenNiNode::UpdateMorphing(NiUpdateData&)
 -> BSFaceGenAnimationData::Update(float delta, bool force)
    -> blinking/emotional-idle/lip state processing
    -> BSFaceGenAnimationData::UpdateMorphsFromLip(float delta)
 -> NAF post-processes only data registered in managedDatas
    -> custom expression timelines write finalExp[54]
    -> optional eye texture-coordinate/emissive updates
    -> optional graph-time synchronization
```

The outer hook calls the engine update first, then applies NAF-managed facial
timelines and eye overrides while holding NAF's state lock and the engine face
instance-data spin lock. The inner hook passes vanilla lip updates through for
unmanaged data, but returns success without calling the engine lip routine for
managed data. This prevents vanilla lip morphs from overwriting NAF's managed
expression output. These suppress-and-postprocess semantics are not exposed by
a maintained CommonLib, F4SE, Addictol, or LooksMenu event/API, so neither hook
can be replaced by a public event source.

LooksMenu body morphing is not involved in this path. The only visual mutation
outside the face-expression arrays is direct eye shader material state used by
NAF eye animation. Full LooksMenu scene/body-morph behavior remains gated.

### Hook recovery

#### Face update hook

Historical ID `317245` is RVA `0x686040`, PDB-named
`BSFaceGenNiNode::UpdateMorphing(NiUpdateData&)`. At historical `+0xBA`, the
five-byte direct call has:

- `RCX = [RDI+0x170]`, the node's `BSFaceGenAnimationData*`;
- `XMM1`, the update delta selected by the containing function;
- `R8B = 1`, the force/update flag;
- return `AL` saved into `R15B`;
- continuation at `+0xBF`.

The call target is old ID `3139`, RVA `0x6689D0`, PDB-named
`BSFaceGenAnimationData::Update(float,bool)`. The original NAF declaration had
an extra fourth floating argument; it was unused by the engine under the x64
ABI but was not the true signature.

The exact 1.11.221 containing function is ID `2209487`, RVA `0x6E8BF0`, with
the same PDB name and matching control/data flow. The equivalent direct call is
at **`+0xBD`**, not the historical offset. It passes the same object and values
to ID `2209139`, RVA `0x6D1310`, and consumes returned `AL`. The production
hook uses the corrected three-argument signature and continues at `+0xC2`.

#### Lip-morph hook

Inside historical `BSFaceGenAnimationData::Update`, the old `+0x158` direct
call passes `RCX = RDI` (the face data) and `XMM1 = XMM7` (delta) to old ID
`1550953`, RVA `0x667E50`, `UpdateMorphsFromLip(float)`. Returned `AL` is saved
in `ESI`, with continuation at `+0x15D`.

In 1.11.221 the same semantic call is at **ID `2209139 + 0x14B`**. It passes
`RCX = RDI` and `XMM1 = XMM9`, calls ID `2209124` at RVA `0x6D0560`, stores
returned `AL` in `EBP`, and continues at `+0x150`. Although the reduced AE PDB
does not publish the callee name, old/new disassembly agrees on lip-resource
handle validation at `+0x2C0`, the three morph arrays at `0x18/0xF0/0x1C8`,
54-morph processing, face-state fields, and boolean return behavior. This is
**disassembly-verified** as the current `UpdateMorphsFromLip` implementation.

Both installation sites are validated as `E8 rel32` calls whose decoded target
must equal the separately exact-resolved callee. Validation is fail-closed and
does not scan for another location. The hooks retain the original call-site
shape because it limits interception to the same two engine paths as historical
NAF; detouring the entire callee would broaden behavior to unrelated callers.

### Face ABI and layout audit

The old local `BSFaceGenAnimationData` declaration ended at `0x2D8`. Both
1.10.163 and 1.11.221 update/lip implementations access state through `0x2E0`,
and PANDA independently reports total size `0x2E8`. NAF's used prefix is
confirmed:

- `finalExp` at `0x18`;
- second and third 54-float arrays at `0xF0` and `0x1C8`;
- `FaceEmotionalIdles::InstanceData` at `0x2A0`;
- its spin lock at `0x2B4`.

The local declaration now retains this verified prefix and an opaque `0x10`
tail, with size `0x2E8`. NAF does not allocate, construct, destroy, or virtually
invoke this engine object.

For managed eye animation, selected and Dear declarations agree on
`NiAVObject::GetObjectByName` at physical slot `0x2E`. The PDB names its target
RVA `0x16C8330`; occurrences in derived vtables are consistently at byte
offset `0x170`. The 1.11.221 `BSGeometry` vtable is exact ID `1436492`, RVA
`0x267E0C8`; slot `0x08` points to RVA `0x16D5640`, whose implementation is
`mov rax, rcx; ret`, independently confirming `BSGeometry::IsGeometry()`.
Selected and Dear Actor/process layouts also agree on `Actor::currentProcess`
at `0x300`, `AIProcess::middleHigh` at `0x08`, and
`MiddleHighProcessData::faceAnimationData` at `0x3C8`.

No face-path declaration-order defect was found. The corrected object size and
the historical extra-argument hook typedef were the two face ABI defects.
Custom eye/material writes and managed face playback remain runtime-test-
required even though the hooks and vanilla pass-through path are ready.

### Reachable relocation audit

`tools/audit_face_relocations.py` requires the following exact supplied-
database records and verifies the four source selectors containing both IDs and
both independently recovered interior offsets:

| interface | exact 1.11.221 ID | RVA |
|---|---:|---:|
| `BSFaceGenNiNode::UpdateMorphing` | `2209487` | `0x6E8BF0` |
| `BSFaceGenAnimationData::Update` | `2209139` | `0x6D1310` |
| `BSFaceGenAnimationData::UpdateMorphsFromLip` | `2209124` | `0x6D0560` |

The normal unmanaged callback performs no additional relocation lookup. The
managed path reuses already-audited graph/string-pool and `Get3D` surfaces;
its remaining actor/process, node, geometry, shader and expression operations
are verified field/virtual access rather than hidden two-ID wrappers. No eager
face relocation was introduced. The face audit, 16-record graph audit, and
63-record HUD audit all pass with exact lookup enabled.

A clean Release x64/v143 build completed with zero compiler/linker errors.
`COPY_BUILD=OFF`; no deployment occurred. Output:
`f4se-plugin/build-1.11.221/Release/NAF.dll`. SHA-256:
`149E4CCB880C38616280DE1043E3016CA2B50C635F5E9EA7108015F60A6850AD`.

### Production enablement and runtime plan

Production initialization now installs both face/lip call-site hooks after the
known-good game-loop and graph hooks. Installation fails closed on an
unexpected runtime, missing exact ID, non-call opcode, or wrong decoded call
target. One-time logs identify the first natural face update and first natural
lip-morph update without creating a synthetic call or managed animation.

The production runtime test is:

1. Launch Fallout 4 1.11.221 through F4SE 0.7.8 and load an ordinary save.
2. Confirm the known-good HUD and graph installation/operation logs remain.
3. Confirm face hooks install at `2209487 + 0xBD` and `2209139 + 0x14B`.
4. Move, idle, and draw/holster normally to retain graph coverage.
5. Trigger ordinary vanilla NPC dialogue with visible facial/lip animation.
6. Confirm the one-time face and lip callback logs and observe normal
   expression/lip behavior.
7. Do not start a NAF scene or open gated NAF studio/scene interfaces.
8. Exit normally and preserve NAF/F4SE logs and any Addictol crash report.

This test validates installation and vanilla pass-through on the real
production subsystem. NAF-managed custom facial timelines will require a later
production consumer when scene/Papyrus integration is restored; they have not
been declared runtime-validated merely because vanilla dialogue passes.

Scenes, package hooks, camera, studio, Papyrus, serialization, full LooksMenu
morph behavior, and arbitrary scene playback remain gated. Package/process
hooks are the recommended next subsystem because managed actor lifecycle and
eventual scene execution depend on correct package/process observation before
camera or studio behavior.

## Package/process subsystem restoration

The production face/lip test passed: the game-loop and graph hooks and HUD
remained stable, both face hooks installed, natural vanilla lip-morph and face
updates reached their callbacks, and no Address Library or visible behavior
regression occurred. HUD, graph, and unmanaged vanilla face/lip pass-through
are therefore the current runtime-validated baseline.

### Dependency and behavior map

NAF's package override path has two complementary interceptors. The
`Actor::EvaluatePackage(bool,bool)` detour suppresses reset-AI evaluations for
actors present in the protected package map, while allowing command or
non-reset evaluation and all unprotected actors through. The second detour is
the historical ID `609985`, now identified exactly by the old PDB and
disassembly as `AIProcess::CheckforNewPackage(Actor*,bool)`. It returns false
for protected actors and otherwise invokes the original. This is behavior
substitution, not merely observation; no maintained package event can replace
the suppression semantics.

The production state transition remains:

1. Resolve the `ActorHandle` and require a live actor/current process.
2. Call `AIProcess::ClearCurrentPackage`.
3. Call `AIProcess::ClearCurrentDataForProcess`.
4. Install the created package through verified virtual slot `Actor::PutCreatedPackage`.
5. Request `Actor::EvaluatePackage(false,false)`.
6. Observe `TESPackageEvent::End`, consume the optional completion functor,
   and restore/clear through the same protected map during later scene cleanup.

Special-idle setup and playback reuse the previously verified
`AIProcess::SetupSpecialIdle` mapping and maintained `PlayIdle` adaptation.
`IsIdleLoading` and `RunActionOnActor` are not reachable from package hook
installation, ordinary evaluation, or the override manager itself; they remain
unresolved and gated with scene playback.

### Hook and interface recovery

| interface | 1.10.163 | 1.11.221 | evidence/status |
|---|---:|---:|---|
| `Actor::EvaluatePackage(bool,bool)` | ID `1395257` | ID `2229805`, RVA `0xC69240` | PDB plus old/new disassembly verified; production detour |
| `AIProcess::CheckforNewPackage(Actor*,bool)` | ID `609985`, RVA `0xE28F80` | ID `2231539`, RVA `0xCEFAD0` | PDB plus argument/data-flow and control-flow correspondence; production detour |
| `AIProcess::ClearCurrentPackage` | ID `241540` | ID `2231582`, RVA `0xCF1340` | previously disassembly verified |
| `AIProcess::ClearCurrentDataForProcess` | ID `577581` | ID `2232435`, RVA `0xD394F0` | previously disassembly verified |
| `AIProcess::SetupSpecialIdle` | ID `1446774` | ID `2231704`, RVA `0xCFAD40` | previously disassembly verified |
| `Actor::GetActorHandle` | ID `1145222`, RVA `0xD89690` | ID `2229894`, RVA `0xC73410` | PDB and handle-manager data flow verified |
| register `TESPackageEvent` sink | ID `1448133` | ID `2201672`, RVA `0x52D7B0` | wrapper body/callee semantics verified |
| unregister `TESPackageEvent` sink | ID `1182113` | ID `2201673`, RVA `0x52D7E0` | adjacent symmetric wrapper verified |

The historical `609985` body consumes `RCX=AIProcess*`, `RDX=Actor*`, and
`R8B=bool`, checks actor state at `+0x440`, excludes the player singleton, and
selects process-level package helpers before returning a Boolean. The AE body
at `0xCEFAD0` preserves those semantic signals. Both package detours now
require runtime 1.11.221 and validate their independently recorded entry
prologues before either patch is installed. Any mismatch fails plugin load.

### ABI and synchronization audit

The package restoration exposed one stale CommonLib declaration:
`Actor::GetActorHandle` still used the OG-only ID `1145222`. It now selects
the exact supplied-database AE ID `2229894`. IDs `313362/2231230` were
explicitly rejected: PDB and disassembly identify those as
`Actor::GetMountHandle`, not the actor's own handle.

The current `Actor` vtable is exact ID `1455516`, RVA `0x25602C0`; physical
slot `0xFA` points to PDB-named `Actor::PutCreatedPackage` at RVA `0xC81870`.
Selected and Dear declarations agree on this order and on
`Actor::currentProcess` at `0x300`. `TESPackageEvent` retains the corroborated
`0x10` layout (`NiPointer` at `0`, form ID at `8`, event type at `0xC`). No
package declaration-order or structure-layout defect was found.

The package-end listener previously moved and cleared `doneCallback` under a
shared lock. That is a source-level synchronization defect, not an engine ABI
change; it now takes the existing package-map mutex exclusively. Registration
is idempotent and performed directly on F4SE game-data-ready. NAF's broad
internal game-data event is intentionally not emitted yet because doing so
would transitively initialize still-gated scenes and menu listeners.

### Reachable relocation audit and build

`tools/audit_package_relocations.py` verifies 12 exact supplied-database
records: both detours, actor-handle access, three package/process operations,
package-event registration/unregistration, the three handle-manager entries,
and the Actor vtable. It also guards the five source selectors that must choose
the recovered AE mappings. No eager package global, OG-only invariant, or
1.10.984-as-AE selection remains on the enabled path. The package audit passes,
as do the existing 63-record HUD, 16-record graph, and 3-record face audits.

Release x64/v143 builds and links with zero errors and `COPY_BUILD=OFF`; no
deployment occurred. Output:
`f4se-plugin/build-1.11.221/Release/NAF.dll`. SHA-256:
`82DF18451C76B4871C3B7119BCE85E4310704F6A13FBFBA72E3E2E40A2753C13`.
The pinned ik timestamp generator still emits its known build-directory-only
warning; neither historical dependency tree is modified.

### Production runtime test plan

1. Launch Fallout 4 1.11.221 through F4SE 0.7.8 and load an ordinary save.
2. Confirm the game-loop, HUD, graph/Set3D, and face/lip baseline remains functional.
3. Confirm installation logs for package IDs `2229805` and `2231539` and
   package event-sink registration after game data becomes ready.
4. Remain near active NPCs, move locally, and wait briefly so ordinary package
   evaluation and reevaluation occur naturally.
5. Confirm the bounded first-observation logs for both package functions; do
   not start a NAF scene or invoke gated Papyrus/studio actions.
6. Exit normally and retain NAF/F4SE logs and any Addictol crash report.

This exercises real production interception and pass-through while the
override map is empty. Actual override/restoration behavior will gain a
production consumer when the public lifecycle layer is restored. Camera,
studio, Papyrus, serialization, full scenes/playback, and full LooksMenu morph
behavior remain gated. Papyrus/serialization is the recommended next coherent
stage because it provides the intended public and persistence lifecycle for
the now-restored package manager before camera/studio/full-scene activation.

### Package validator runtime regression

The first package-enabled runtime build failed closed after the already-known-
good game-loop, graph, and face/lip hooks installed. Its only package message
was `Package/process hook validation failed`; neither package target was
patched and F4SE correctly treated NAF as incompatible.

The failure was in the new `Actor::EvaluatePackage` validator, not its
relocation. Clean Fallout4.exe 1.11.221 at ID `2229805`, RVA `0xC69240`, begins:

```
40 53 56 57 48 83 EC 60 48 8B 01 0F B6 F2 B2 01
```

The validator incorrectly expected:

```
53 56 57 48 83 EC 60 48 8B 01 0F B6 F2 B2 01
```

It had been transcribed from mnemonic disassembly and omitted the otherwise
easy-to-overlook `0x40` REX prefix on `push rbx` and `0x48` REX.W prefix on
`mov rax,[rcx]`. Consequently byte zero necessarily failed on an unmodified
function and short-circuit evaluation meant the `CheckforNewPackage`
validator was never evaluated. The clean target remains the callable
PDB-named `Actor::EvaluatePackage(bool,bool)` entry with the previously
verified RCX/DL/R8B ABI and virtual/process control flow.

Clean `AIProcess::CheckforNewPackage` at ID `2231539`, RVA `0xCEFAD0`, begins:

```
48 89 6C 24 20 56 41 56 41 57 48 83 EC 20
```

and already agrees exactly with its validator. Its PDB identity, argument
consumption, actor-state reads, player exclusion, process-level dispatch and
Boolean return remain verified. Neither mapping changed.

Addictol's source contains no hook or patch for either `0xC69240` or
`0xCEFAD0`. Its package-related `Package Allocate Location` fix instead
patches the call at runtime-specific ID `2211931 + 0x144` to wrap
`ExtraDataList::GetPrimitive`. It does not overlap either NAF detour entry or
validated byte range. The failed build did not log live bytes, so that run
cannot independently prove no other plugin modified live memory; however, the
bad NAF signature alone fully explains the deterministic rejection. No hook
chaining is introduced or required by the available source evidence.

Validation now evaluates and reports both functions independently before any
detour installation. For each target it records the semantic name, exact ID,
resolved address, expected unmasked bytes, actual live bytes, every mismatch,
page protection/executability, and recognizes an original body, near/short
direct jump with destination, RIP-relative indirect jump, breakpoint/guard,
or another unexpected patch. A failure still prevents both package patches;
there is no fuzzy signature search, clean-byte restoration, or detour
overwrite. This also confirms NAF installation order cannot be responsible:
the preceding NAF hooks patch the game-loop queue call, graph entries, and two
face call sites, none of which overlaps either package entry.

The corrected Release x64/v143 build has zero compiler/linker errors with
`COPY_BUILD=OFF`. The 12-record package audit and the 63-record HUD, 16-record
graph, and 3-record face audits all pass. Output remains
`f4se-plugin/build-1.11.221/Release/NAF.dll`; corrected SHA-256:
`BF7A704FC316045325DA68CF436352C68FBDBE9A11B363DB8FFBA922EE8AAA68`.
It is safe to rerun the same production package test. If a prior plugin has in
fact patched either entry in the live process, the enhanced validator will
identify the exact target, bytes, branch shape/destination, and refuse to
install rather than blindly overwrite it.

## Papyrus and serialization lifecycle restoration

The corrected production package test passed. Both package hooks installed at
IDs `2229805` and `2231539`, both were reached naturally on a live actor/process,
the `TESPackageEvent` sink registered after game-data-ready, and HUD, graph,
Set3D and face/lip remained stable. Package/process interception and natural
pass-through are promoted to the known-good production baseline.

### Historical and current lifecycle

Historical NAF initialized in this order during `F4SEPlugin_Load`: F4SE/CommonLib,
all hooks, F4SE messaging, serialization unique ID/callbacks, Papyrus registration,
then filesystem/XML `Data::Global::Init`. The F4SE game-data-ready message later
registered menu/event sinks, linked game forms through `InitGameData`, set
`g_gameDataReady`, and broadcast NAF's internal game-data event. Save/load
callbacks stopped the timer thread, locked all framework states, serialized or
reconstructed nine records, and restarted the timer. Revert reset all nine
subsystems.

The 1.11.221 lifecycle preserves the externally observable F4SE ordering while
respecting subsystem gates:

1. Exact-runtime and hook validation/install.
2. F4SE messaging registration.
3. F4SE serialization interface acquisition, historical unique ID and three callbacks.
4. F4SE Papyrus interface acquisition and VM registration callback.
5. Return from plugin load with scene/data/studio/camera initialization still gated.
6. On game-data-ready, register the already-validated HUD and package event sinks.

F4SE 0.7.8 exposes Papyrus interface version 2 and serialization interface
version 1. NAF uses their public function tables directly. The verified callback
types are `bool F4SEAPI(IVirtualMachine*)` for native registration and
`void F4SEAPI(const SerializationInterface*)` for save/load/revert. Unique-ID,
record iteration/read/write, form/handle resolution and callback registration
therefore require no Fallout4.exe relocation or historical adapter.

### Native availability

Registration is intentionally production-real but bounded. Missing registrations
fail at the Papyrus VM boundary rather than calling a stub which reports success.

| classification | registered functions | reason |
|---|---|---|
| A: enabled production state | `SetDisableRescaler`, `GetDisableRescaler`, `SetPackageOverride`, `ClearPackageOverride` | local settings and validated package manager; package setter now rejects null actor/package |
| A: enabled HUD | `DrawRectangle`, `DrawText`, `DrawLine`, `SetElementPosition`, `TranslateElementTo`, `StopElementTranslation`, `RemoveElement`, `GetElementWidth`, `GetElementHeight`, `MoveElementToFront`, `MoveElementToBack`, `SetElementMask`, `AttachElementTo` | production HUD is runtime validated |
| C: scene lifecycle | `StartScene`, `WalkToAndStartScene`, `StopScene`, all scene query/property/speed/validation functions | would initialize or execute still-gated scenes, packages, actions and animation playback |
| C: playback/actor control | `PlayDynamicIdle`, `PlayNANIM`, `StopNANIM`, both IK functions, `ReEnablePlayerInput`, `SynchronizeAnimations` | arbitrary playback/actions or unresolved lifecycle dependencies |
| C: managed face | eye override and face play/stop functions | vanilla pass-through is validated; NAF-managed face timelines are not |
| C: other | `ToggleMenu`, actor-usable keyword functions, `GetLocalTransform` | full NAF menu/data forms or arbitrary-reference virtual access is not yet validated |

Seventeen natives are registered; twenty-eight remain deliberately absent.
No native is registered with a temporary successful no-op. Full Papyrus surface
registration is therefore not yet claimed.

### Papyrus ABI and relocation audit

`IVirtualMachine` remains size `0x10`; selected and Dear CommonLib agree on the
physical ordering through `BindNativeMethod` at slot `0x1B` and
`SetCallableFromTasklets` at slots `0x1C/0x1D`. The templated binder constructs
the maintained `NativeFunction`, transfers it to the VM, and marks the registered
functions tasklet-callable exactly as historical NAF did. No direct VM fields
are read. The earlier defaulted `IStackCallbackFunctor` destructor correction
remains intact; the scene event proxy which derives from it is not instantiated
in this stage. No Papyrus declaration-order, ownership, or refcount defect was
found in the enabled surface.

The F4SE registration path itself contains no Address Library lookup. Enabled
native calls reuse the already audited HUD and package relocations. A prospective
active package-record load exposed selected CommonLib's two-ID
`TESForm::GetAllForms` IDs `2689178/2689189` as 1.10.984-only: neither exists in
the supplied 1.11.221 database. Dear identifies separate AE candidates, but
their namespace was not imported. Package records are therefore preserved
pending focused recovery instead of making these stale globals reachable.
No new engine relocation was guessed or added.

`tools/audit_papyrus_serialization.py` verifies all 17 allowed and 28 gated
bindings, the five F4SE lifecycle registrations, callback ABI declarations,
and historical-v5/deferred-record guards. It passes together with the exact
12-record package, 63-record HUD, 16-record graph and 3-record face audits.

### Serialization schema and compatibility

The historical serialization unique ID remains the exact multicharacter value
`'NAF'`. The historical record set is unchanged:

| record | version | state |
|---|---:|---|
| `TASK` | 5 | deferred byte-for-byte |
| `SCNE` | 5 | deferred byte-for-byte |
| `EQPT` | 5 | deferred byte-for-byte |
| `UID` | 5 | deferred byte-for-byte |
| `FACE` | 5 | deferred byte-for-byte |
| `HUD` | 5 | actively deserialized and serialized |
| `PACK` | 5 | written in historical format for a fresh record; loaded records deferred byte-for-byte |
| `SHUD` | 5 | deferred byte-for-byte |
| `BODY` | 5 | deferred byte-for-byte |

Each active record retains the historical outer four-byte payload-size prefix
followed by the unchanged cereal binary archive, including existing integer and
enum widths. Loading validates the F4SE record length, the inner size prefix,
and cereal parse before replacing active HUD state. Malformed active records
are retained as deferred payloads rather than partially applied. Unknown record
types and versions are also preserved exactly.

Gated records are held with their original type, version and payload and written
back unchanged on the next save. They are not deserialized, so scene constructors,
managed graph/face fixups, timer tasks, equipment actions, studio state and
package restoration cannot execute. If a loaded co-save already contains a
deferred `PACK` record, it wins over current-session package state and is copied
unchanged; the log explicitly warns that the two are not yet merged. On a save
without a prior package record, current package state is emitted with the exact
historical version-5 schema, but a subsequent load keeps it pending until form
lookup and restoration are fully audited.

Revert resets only enabled HUD/package state and clears deferred payloads. It
does not instantiate/reset gated scene, timer, managed animation or studio
systems. HUD/package and broad internal game-data registration remain idempotent
and separated, preventing double registration.

Historical version-5 saves remain structurally readable: HUD is restored and
all other records survive a load/save cycle without schema conversion. New
saves remain readable by historical NAF because active records use its original
IDs, versions, prefix and cereal layout, and omitted empty gated records were
already legal. Behavioral restoration of deferred records is intentionally not
claimed yet.

### Build and production runtime plan

Release x64/v143 builds and links with zero errors and `COPY_BUILD=OFF`; no
deployment occurred. Output:
`f4se-plugin/build-1.11.221/Release/NAF.dll`. SHA-256:
`BF053780A40AFA1CE01D002F6D2F237700155DBE1D7FACB333E2BF8F924F8274`.

The production lifecycle test is:

1. Launch Fallout 4 1.11.221 through F4SE 0.7.8 and load an ordinary save.
2. Confirm the game-loop, HUD, graph/Set3D, face/lip and package baseline.
3. Confirm serialization interface/callback registration and the 17-native
   Papyrus registration log.
4. Exercise a safe existing settings, package-override, or HUD script call;
   do not invoke a gated scene/playback native.
5. Create a new manual save and confirm save entry/completion plus active and
   deferred record counts.
6. Load that new save and confirm load entry/completion, HUD restoration and
   deferred-record preservation without subsystem initialization.
7. If practical, return to the main menu or load another save and confirm
   revert entry/completion.
8. Exit normally and retain NAF, F4SE and Papyrus logs plus any crash report.

Camera, studio, full scenes, arbitrary ordered actions/playback and full
LooksMenu morph behavior remain gated. Full scene-execution prerequisites are
the recommended next stage: they are needed before deferred `SCNE`, `TASK`,
`EQPT`, `UID`, `BODY`, `FACE` and `SHUD` records or the remaining scene-facing
Papyrus natives can be safely activated. Camera should remain a separate later
stage unless scene prerequisite analysis proves it mandatory.

## Papyrus/serialization exact-ID regression audit

The first production lifecycle attempt failed closed on Address Library ID
`2689134`. Exact matching behaved correctly and remains mandatory; no nearest
record, fuzzy lookup, or raw-RVA fallback was introduced.

### Owner, timing, and runtime namespace

The sole owner in the effective build tree was selected CommonLibF4
`RE::GameVM::GetSingleton()` in
`CommonLibF4/include/RE/Bethesda/GameScript.h`. Its declaration was
`RelocationID(996227, 2689134)` with storage type `GameVM**`. The lookup occurs
inside the F4SE Papyrus registration callback, not while registering the F4SE
serialization callbacks and not while reading a co-save. The precise enabled
path is:

`F4SE Papyrus callback -> Papyrus::RegisterFunctions -> PAPYRUS_BIND(SetPackageOverride) -> IVirtualMachine::BindNativeMethod -> NativeFunction constructor -> GetTypeInfo<Actor/TESPackage> -> GameVM::GetSingleton`.

The first two settings natives have primitive types and do not require object
type information. `SetPackageOverride` is the first enabled native whose actor
and package arguments cause CommonLib's object-type lookup to acquire the game
VM, which explains why the stale singleton was reached during registration.

Dear CommonLib and Addictol explicitly model this singleton as the three-way
mapping `996227 / 2689134 / 4796420` for OG 1.10.163 / NG 1.10.984 / AE
1.11.221. Thus `2689134` is another 1.10.984-as-AE collapse in the selected
two-ID CommonLib model. This is not an OG-invariant error and not a
serialization-format failure.

The 1.11.221 Crash Logger PDB names `g_GameVM` at RVA `0x30E0288`. The supplied
1.11.221 Address Library independently maps that exact RVA to ID `4796420`.
Selected CommonLib now uses `RelocationID(996227, 4796420)` while retaining the
verified `GameVM**` indirection. This correction is in CommonLib commit
`1034e4b`. Dear's numeric AE record agrees here with the supplied namespace,
but the promotion rests on the PDB RVA and exact supplied-database record, not
on numeric copying alone.

### Expanded reachable audit

`tools/audit_papyrus_serialization.py` now requires the Address Library and
checks 15 exact ID/RVA pairs covering the bounded lifecycle's VM singleton,
native dispatch body, allocator/reallocator/scrap heap, BSStringPool operations,
actor-handle operations and `Actor::GetActorHandle`. It also source-guards the
corrected GameVM selector and the native-call selector. The known stale
`TESForm::GetAllForms` declarations `2689178/2689189` remain recorded as
1.10.984-only but unreachable.

The reachable-path review found no second bad ID beyond `2689134`. The active
17-native surface divides as follows: primitive settings calls are local;
package calls use the already verified package/handle surface; HUD calls use
the already verified UI/Scaleform surface; VM registration uses maintained
virtual slots plus the audited allocator/string/type-information paths. The
`NativeFunctionBase::Call` record ID `2314780` exists exactly at PDB-consistent
RVA `0x20F9870`. No enabled path uses the `vmobject`-only legacy single ID in
`BSScriptUtil`, scene event dispatch, or the unregistered structure-returning
scene natives.

There are no namespace-scope `REL::Relocation` objects in the bounded
Papyrus/serialization implementation. All audited CommonLib relocations are
function-local statics and resolve only when the owning operation is reached.
The `EventRegistrations` state allocation is ordinary local heap state; its
`GameVM` access is confined to the unused `Dispatch` template and is not
instantiated by the enabled lifecycle.

Deferred records remain literal passive preservation. `LoadCallback` reads the
F4SE record bytes and only deserializes a version-5 `HUD` record. Every other
type/version, including `PACK`, is moved directly into `DeferredRecord` without
calling `DeserializeForm`, `DeserializeHandle`, `TESForm::GetAllForms`, scene,
studio, camera, morph, or package fixups. Save writes those original bytes back.
The historical unique ID `'NAF'`, record version 5, four-byte size prefix,
cereal payload and serialized widths are unchanged.

All audits pass after the correction: 15 Papyrus/serialization, 63 HUD, 16
graph, 3 face/lip and 12 package exact-record checks (109 bounded assertions
across the subsystem scripts, with intentional overlap for shared primitives).
A clean Release x64/v143 build completes with zero compiler/linker errors and
`COPY_BUILD=OFF`. No deployment occurred. Output:
`f4se-plugin/build-1.11.221/Release/NAF.dll`; SHA-256:
`48A02A2D2858220AB80B3C89325B8B9CD56B9FB7E8095E98890E90824BA08A28`.

It is safe to rerun the unchanged production lifecycle test: load a save,
verify the HUD/graph/face/package baseline and 17-native registration, exercise
one enabled native, create and load a new save, verify HUD restoration and
passive deferred-record preservation, then exercise revert and retain all logs.

## Production camera restoration

The bounded Papyrus/serialization lifecycle test passed on 1.11.221. F4SE
registered both interfaces and all 17 enabled natives; HUD, graph/Set3D,
face/lip and package/process remained stable; save, load and revert callbacks
completed; version-5 HUD/package records were written; loaded deferred package
bytes were preserved on later saves; and no record was skipped. Papyrus
registration and the bounded serialization lifecycle are promoted to the
known-good production baseline. Individual native behavior remains tied to the
runtime status of its underlying subsystem rather than being inferred from
successful registration.

### Historical camera path and hook identity

Legacy Address Library ID `541110` resolves to old RVA `0x12453D0`. The
1.10.163 PDB names it exactly:
`FreeCameraState::Update(BSTSmartPointer<TESCameraState>&)`. NAF detours the
entry, calls the original first, then—under its recursive camera mutex—may:

1. apply studio-supplied local-axis translation to `cameraRoot`;
2. copy that translation back to `FreeCameraState::translation`;
3. apply external pitch/yaw or orbit-look rotation to both state and node;
4. apply scene look-at tracking and HUD Q/E vertical input;
5. update the camera node immediately to avoid one-update input lag.

`SetActive` owns the HUD input listener and clears retained look-at nodes on
shutdown. `LookAt` retains the actor/root and configured target node. Scene
begin enters free camera, sets the look-at target, disables player control and
later uses the hook for continuous tracking. Scene end disables the camera
controller, releases retained nodes/input listener, leaves free-camera mode,
restores third person through `Force3rdPerson`, and restores player controls.
Studio uses the same hook with externally supplied translation, rotation and
orbit inputs. First/third-person transitions themselves do not run through the
free-camera hook.

No maintained F4SE/CommonLib event supplies the post-`FreeCameraState::Update`
timing, the concrete `FreeCameraState*`, and same-tick camera-root access. The
maintained `PlayerCamera` methods can enter/leave states but cannot replace this
continuous post-update modification. The historical detour therefore remains
necessary.

### 1.11.221 recovery and ABI correction

The 1.11.221 PDB names the semantic equivalent at RVA `0x1022040`; the supplied
Address Library maps that exact entry to ID `2248280`. Old and new bodies have
the same settings/input tests, movement update, physical vslots `0x0C/0x0D`,
camera-root acquisition, transform propagation, node update and reference
balancing. The new compiler vectorizes several transform copies and adds an
early active-state check, but the function contract is unchanged.

The original NAF typedef incorrectly modeled the function as
`void(FreeCameraState*)`. The real virtual signature has always included
`BSTSmartPointer<TESCameraState>& nextState`; the old implementation happened
not to consume the incoming RDX value. The production detour and original call
now preserve both arguments explicitly. This is an ABI correction, not a new
behavioral assumption.

`TESCamera::Update` is PDB-named at RVA `0x827970` / exact ID `2214740`. It
loads `currentState`, checks state byte `+0x24`, and invokes physical vslot
`0x0B` with a stack `nextState` smart pointer. The 1.11.221
`FreeCameraState` vtable at ID `21725`, RVA `0x25A26F8`, independently resolves
slot `0x0B` to `0x1022040`. This proves the object type, slot, signature and
normal camera-update dispatch path.

Both selected and Dear headers omitted a real byte at `TESCameraState +0x24`.
Disassembly establishes its semantics: `FreeCameraState::Begin` sets it to
one, `End` clears it, and `Update` returns immediately when false. Selected
CommonLib now names this `stateActive`; the base remains size `0x28` and
`FreeCameraState` remains `0x58`. Existing offsets remain verified:
`camera +0x18`, state ID `+0x20`, translation `+0x28`, rotation `+0x34`, input
pairs through `+0x50`, direction `+0x54`, and flags `+0x56/+0x57`.

The selected `PlayerCamera::GetSingleton` also contained another two-runtime
collapse: ID `2688801` is 1.10.984-only. The 1.11.221 PDB names
`g_PlayerCamera` at RVA `0x30DBDD8`, exactly mapped by the supplied database to
ID `4796065`; CommonLib now uses `1171980 -> 4796065`. `Force3rdPerson` remains
the previously disassembly-verified `162201 -> 2248348` at RVA `0x1026A40`.
Its internal `SetState` target remains `2214742` at `0x827AC0`.
`ToggleFreeCameraMode` is exact ID `2248368`, RVA `0x1027590`.

The installed hook is exact-runtime gated, resolves
`RelocationID(541110, 2248280)`, and requires the verified 18-byte 1.11.221
prologue at an executable page before DetourXS may patch it. Failure blocks
plugin load; there is no fuzzy scan, alternate target, raw RVA or partial
camera enablement. The callback logs its first natural production invocation.

### Camera ABI and relocation audit

`tools/audit_camera_relocations.py` checks eight exact supplied-database
ID/RVA pairs, the corrected source selectors, and the clean executable's
physical FreeCameraState vslot `0x0B`. Selected and Dear declarations agree on
the BSInputEventUser slots `0x00-0x08`, followed by `Begin 0x09`, `End 0x0A`,
`Update 0x0B`, rotation/translation `0x0C/0x0D`, and save/load/revert through
`0x10`. No other NAF-used camera virtual order or signature defect was found.

The reachable production surface is:

| interface | 1.11.221 evidence | status |
|---|---|---|
| `FreeCameraState::Update` | ID `2248280`, PDB RVA `0x1022040`, cross-runtime disassembly, vslot `0x0B` | production hook enabled; runtime test required |
| `TESCamera::Update` | ID `2214740`, PDB RVA `0x827970` | statically verifies virtual scheduling |
| `PlayerCamera` singleton | ID `4796065`, PDB global RVA `0x30DBDD8` | corrected exact AE mapping |
| free-camera toggle | ID `2248368`, RVA `0x1027590` | exact maintained API; full scene behavior runtime-required |
| force third person | ID `2248348`, RVA `0x1026A40` | disassembly verified; cleanup behavior runtime-required |
| state transition | ID `2214742`, RVA `0x827AC0` | verified dependency of force-third-person |
| camera state/vtable | IDs `67299` / `21725`; sizes `0x28` / `0x58` | physical order and active byte statically verified |

No eager relocation was introduced. All camera relocations remain lazy except
the deliberately load-time hook entry, which is fully validated. The camera,
Papyrus, HUD, graph, face and package audits pass together: 8 camera, 15
Papyrus/serialization, 63 HUD, 16 graph, 3 face and 12 package checks.

Camera state has no independent version-5 NAF record. It is transient behavior
derived from an active scene/studio session. Deferred `SCNE`/studio records
remain passive and therefore cannot enter free camera during load. No
serialization schema or restoration behavior changed.

No Papyrus native becomes newly registerable from camera alone. Camera cleanup
is only one dependency of the gated scene/playback natives; player AI/input,
animation actions, ordered work and scene lifecycle must all be active before
those functions can truthfully succeed. `ToggleMenu` also remains gated by the
full NAF menu/data subsystem, not by camera availability.

### Build and production camera test

Production camera hook installation is enabled alongside the known-good
game-loop, HUD, graph/Set3D, face/lip, package/process, Papyrus and serialization
baseline. A clean Release x64/v143 build completes with zero compiler/linker
errors and `COPY_BUILD=OFF`; no deployment occurred. Output:
`f4se-plugin/build-1.11.221/Release/NAF.dll`; SHA-256:
`AB8D2DC8A53849C2415B03AE2F9FA153F32669B3AC488D3CAD5B89B5EDC89626`.

Production test procedure:

1. Launch 1.11.221 through F4SE 0.7.8 and load an ordinary save.
2. Confirm HUD, graph/Set3D, face/lip, package, Papyrus and serialization logs.
3. Confirm installation at camera ID `2248280`.
4. Switch first/third person and move normally to regression-test unaffected
   player-camera states.
5. Enter vanilla free camera with console `tfc`, move/rotate briefly, then run
   `tfc` again to leave it. This naturally exercises the production detour
   while NAF look-at/external-input controls remain inactive.
6. Require the first-interception log, normal camera behavior and no crash or
   Address Library failure. Do not start a NAF scene yet.
7. Create/load a save if desired to recheck the lifecycle baseline, exit
   normally, and retain NAF/F4SE/Papyrus logs and any crash report.

Passing this test validates hook installation and vanilla free-camera
pass-through. NAF-controlled look-at, external studio input, free-camera
entry/exit and cleanup remain scene/studio-runtime-required.

### Remaining scene/studio dependency graph

| remaining area | direct prerequisites | current blockers / evidence |
|---|---|---|
| scene initialization | data/forms initialization, actor eligibility, `InitializeActorInstant`, package takeover, AI/input transition, free-camera entry | `InitializeActorInstant` has strong PDB/ID candidate `2236393`; production data/form initialization and full transition symmetry still gated |
| animation playback | graph infrastructure, smart/dynamic idle, special idle, transition/idle-loading/action dispatch | graph and `SetupSpecialIdle` are verified; `IsActiveGraphInTransition` candidate `2214305` needs final semantic promotion; `IsIdleLoading` and `RunActionOnActor` remain unresolved |
| ordered actions | serialized task types, equipment/inventory, morph callbacks, arbitrary callbacks, game-loop ordering | delay bookkeeping only is runtime validated; engine-facing equipment/morph/arbitrary execution remains gated |
| actor control/cleanup | package manager, `SetAIControlled`, disable/reenable input, collision, transform/player-3D, third-person/camera cleanup | package and camera mappings are restored; compound player-control behavior is statically mapped but full symmetric scene runtime behavior is untested |
| LooksMenu body morphs | exact LooksMenu 1.7.0.3 checksum, body-morph singleton/function, ordered worker-thread use and cleanup | DLL offsets are disassembly verified; production scene ownership/thread/lifetime remains untested |
| active serialization restore | full scene/task/equipment/body/face/HUD-scene managers plus form/handle remapping | records remain byte-preserved; stale AE form-map path and subsystem fixup ordering must be resolved before active restore |
| studio | scene actor ownership, camera external input/orbit, UI, IK, transforms, collision, animation editing and previously unresolved studio globals | camera portion is ready, but studio is tightly coupled to nearly every remaining scene mutation path |
| gated Papyrus surface | corresponding scene/playback/body/face/menu systems | 28 natives remain absent until each complete dependency chain is enabled |

Studio should not be restored as the next isolated subsystem. Its useful paths
depend on scene actor ownership, ordered mutations, playback, IK/transforms,
collision, LooksMenu and the newly restored camera. The lower-risk next stage
is a coherent scene-execution prerequisite pass: finish the named animation
functions, enable data/form initialization, actor-control symmetry and bounded
ordered actions, then restore the basic scene lifecycle. Studio can follow on
that production foundation rather than duplicating or prematurely activating
the same unresolved paths.

## Basic production scene-execution prerequisite pass

The production camera test passed on runtime 1.11.221: the hook installed at
ID `2248280`, fired naturally, and did not regress HUD, graph, face/lip,
package/process, Papyrus or serialization. Camera integration is promoted to
the known-good production baseline. The work below begins the first coherent
scene pass; it is not evidence that scene behavior has passed runtime testing.

### Scene lifecycle dependency graph

| lifecycle stage | production work | classification before this pass | disposition |
|---|---|---|---|
| request/validation | resolve position, validate actors/cell/worldspace/alive/child/in-scene state | A: eligibility reads runtime validated; data linking gated | data parsing runs at plugin load and `InitGameData` runs at game-data-ready |
| attach/begin | scene keyword, stop interaction/combat/alarm, restrained/favor state, package override, transforms, headtracking, player third-person | package/camera A; compound actor mutation B | enabled for bounded basic positions; exact relocation audit required |
| post-start | `InitializeActorInstant`, player free camera/AI/input, graph foot-IK variable, control-system begin, rescale task | graph/camera/package A; initializer B | initializer promoted from PDB + exact ID; production scene runtime required |
| playback setup | apply actor slot, stop/reset old graph, play form/dynamic/NAF idle, face data, enter transition wait | graph/face A; action/idle-loading C | AE action dispatch and loading model adapted as described below |
| update/advance | transition/load wait, graph-time synchronization, animation loop, actor transform/collision/player-3D | game-loop serialization A; engine mutations B | production `UpdateScenes` enabled; scene runtime required |
| ordered work | position/equipment/morph delays and callbacks | delay A; equipment/morph/arbitrary B/C | delay actions enabled; equipment, morph and arbitrary callbacks explicitly fail closed |
| stop/cleanup | collision, face, player AI/input, NPC state/keywords/package, scale, graph reset, camera exit, detach | individual APIs A/B; whole symmetry B | complete historical order retained; production scene runtime required |
| persistence | HUD/package plus scene/task/body/equipment/UID records | HUD/package A; active scene restore C | package v5 records active; scene-related records remain byte-preserved/deferred |
| studio/advanced | group/tree control, continuous studio alignment, equipment choreography, body morphs, arbitrary callbacks | D | still gated |

Here A means already runtime-validated on the tested path, B means restored or
statically verified but not yet exercised as part of a scene, C means required
work that was unresolved before this pass, and D means optional/advanced for
the selected first scene.

### Animation action and loading architecture

Historical `BGSAnimationSystemUtils::RunActionOnActor` is ID `22408`, old RVA
`0xF92190`. Its overload builds graph/file state before dispatch. That public
wrapper no longer exists in the 1.11.221 PDB. The AE engine instead has
production callers that:

1. obtain `ActionInitializeToBaseState` from PDB-named RVA `0x806750`, exact
   supplied-database ID `2214309`;
2. construct `TESActionData` for priority `kTry`, the actor and action;
3. call PDB-named `TESActionData::DoIt()` at RVA `0xDBFEC0`, exact ID
   `2234273`.

The vtable at ID `780200`, RVA `0x245F6E8`, independently places `CreateCopy`
at physical slot `0x04` and `DoIt` at slot `0x05`. The local declaration is
now size `0x60`, with the AE field initialization observed in legitimate
callers: null target, zero action data, two fixed strings, fields at
`+0x38/+0x40/+0x48/+0x50/+0x58`. NAF's reset action uses this AE production
composition. This is a source-level semantic replacement backed by PDB,
vtable and caller disassembly, not a guessed successor to old ID `22408`.

`InitializeActorInstant` is exact ID `2236393`, PDB RVA `0xE13E40`;
`IsActiveGraphInTransition` is exact ID `2214305`, PDB RVA `0x806370`.
Both retain their previously established argument consumption and are promoted
to disassembly/PDB-verified scene dependencies.

The historical `IsIdleLoading` at old RVA `0xF924D0` queried the old
`AnimationFileManagerSingleton` loading-idle collection by requested idle name
and graph manager. AE removed that collection and public query. Its PDB exposes
the redesigned manager's dynamic-clip binding/check operations instead. There
is no one-to-one callable replacement. For form/HKX playback NAF now observes
the maintained active graph: absent or negative graph time remains loading,
and the separately verified graph-transition query must also clear before
synchronization. NAF `.nanim` continues to use its own graph-hook loading
state. This replacement avoids an invented relocation but remains explicitly
**runtime-test-required**; the first scene must demonstrate that it neither
advances early nor stalls.

### Form maps, events, transforms and package restoration

The selected CommonLib form maps contained 1.10.984 IDs. Dear's three-runtime
mapping, exact supplied-database records and executable xrefs converge on:

| interface | old / stale | verified 1.11.221 |
|---|---|---|
| all forms | `422985 / 2689178` | `4796465`, RVA `0x30E0E48` |
| all-forms lock | `691815 / 2689189` | `4796476`, RVA `0x30E0E98` |
| editor-ID forms | `642758 / 2689179` | `4796466`, RVA `0x30E0E50` |
| editor-ID lock | `910917 / 2689190` | `4796477`, RVA `0x30E0EA0` |

The globals remain function-local/lazy. Version-5 `PACK` records now use the
unchanged historical cereal schema and F4SE `ResolveFormID`, then the corrected
form map, to restore active package overrides. Unresolved/deleted actors or
forms continue to fail safely through the historical `PersistentState::load`
filter. Loaded package bytes are no longer kept passive once successfully
decoded; scene/task/body/equipment/UID records remain deferred unchanged.

Newly reachable transform/event declarations also contained OG/NG assumptions.
PDB and exact database records establish `SetAngleOnReference` ID `2201134`,
RVA `0x5137A0`; `SetScale` ID `2200893`, RVA `0x4FF5A0`; actor-location event
registration ID `2201562`, RVA `0x52B800`; and the `TESHitEvent` source ID
`2201886`, RVA `0x533C80`. The existing `TESHitEvent` size remains `0x110` and
agrees with Dear; no new direct `HitData` dependency was introduced.

### Bounded scene surface and feature gates

Only positions with the simple animation control system are accepted. A
position is rejected before mutation if it is a group/tree or declares
position-level equipment/morph sets, or if any animation slot declares morph,
equipment or action choreography. Managed face data and ordinary actor offsets
remain permitted because their production paths are already restored. The
ordered queue executes its historical delay ticks, but equipment, morph and
arbitrary callback insertion logs and rejects while their gates are false.

Four Papyrus natives are newly registered: `StartScene`, `StopScene`,
`GetSceneFromActor`, and `IsSceneRunning`. `WalkToAndStartScene` remains gated
because it reaches the still-unresolved `SetLinkedRef` AE relocation. Position
mutation, group/tree, speed/property extensions, direct dynamic-idle, body/IK,
managed face and transform natives remain absent. The production scene event
proxy and actor death/hit/location sinks initialize at game-data-ready, and the
timer thread receives menu pause events. This brings the active total to 21
natives while retaining 24 dependency-gated natives.

The player-control sequence is unchanged: begin forces third person, stops
movement, then post-start enters free camera, sets AI control and installs the
named `NAF_Scene` input layer. End first restores collision and face state,
then clears AI control and removes the input layer; post-stop resets animation,
disables NAF camera control, leaves free camera and detaches. NPC cleanup
reverses favor/restraint/combat-ignore/keywords, clears the reserved package
and reevaluates AI. Delayed post-start work is cancelled by `End`, preserving
early-stop symmetry. Save/load restoration of an active scene remains deferred,
so the first test must not save while a scene is active.

### Reachable audit, build and first production test

`tools/audit_scene_relocations.py` pins 34 exact 1.11.221 records across actor
control, input, transforms, animation initialization/action dispatch, form
maps, events and package operations. It additionally verifies physical
`TESActionData::DoIt` slot `0x05`, its prologue, source selectors and absence
of the retired idle/action/form-map IDs. All baseline audits pass together:
63 HUD, 16 graph, 3 face/lip, 12 package, 15 Papyrus/serialization, 8 camera
and 34 scene checks. Release x64/v143 builds and links with zero errors;
`COPY_BUILD=OFF`; no deployment occurred. Output:
`f4se-plugin/build-1.11.221/Release/NAF.dll`; SHA-256:
`A75340C8E359F1EC1BAB75166E4D3ACC03D57580D84016481CCC097544FC7777`.

First-scene runtime procedure:

1. Launch through F4SE 0.7.8, load an ordinary save, and verify all known-good
   subsystem registration logs.
2. Use the production `StartScene` native with two ordinary loaded, living
   actors and explicitly name a simple animation position that has no
   equipment sets, morphs, actions, animation group or position tree. Set
   `ignoreCombat=true`; do not include the player on the first attempt.
3. Require the active-scene log, reserved-package behavior, real animation
   start, transition into synchronized graph time, and several seconds of
   normal advancement.
4. Call production `StopScene` with the returned ID. Require the cleanup log;
   verify both actors resume packages, collision and normal graph/face state.
5. Repeat once with the player only after the NPC-only pass. Verify third/free
   camera entry, AI/input takeover, normal stop, camera exit and full player
   control restoration.
6. Do not save/load during either active scene, do not use studio, group/tree,
   equipment/morph choreography or walk-to-scene, and retain NAF/F4SE/Papyrus
   logs plus any crash report.

Passing the NPC scene promotes basic start/advance/NPC cleanup. The player
repeat is separately required to promote player AI/input/camera symmetry.
Advanced LooksMenu morphs, equipment actions, arbitrary callbacks, studio,
walk-to-scene, group/tree control, broader Papyrus APIs and active scene
serialization remain gated.

## Scene-entry relocation regression audit and production UI restoration

The first scene-enabled runtime build failed closed on ID `2688883` before a
scene could start. The owner is `TESDataHandler::GetSingleton()` in selected
CommonLib (`TESDataHandler.h`); the declaration was
`RelocationID(711558, 2688883)`. `2688883` is the 1.10.984 singleton ID and is
absent from the supplied 1.11.221 database. It became reachable when
`Data::Global::InitGameData()` began linking NAF forms/animation data through
`Forms::GetFormPointers()` and `TESDataHandler::LookupForm`; earlier bounded
builds did not execute that game-data linking path. Dear's OG/NG/AE tuple,
Addictol, the exact supplied record and the 1.11.221 PDB agree on ID `4796135`,
RVA `0x30DC080`, named `g_TESDataHandler`. The selected accessor now uses that
AE ID and preserves its original pointer indirection.

The expanded literal-selector sweep found 19 source IDs absent from the AE
database outside the verified surface: two are confined to the still-gated
Studio menu and seventeen are legacy NAF-local animation/Havok/geometry/menu
declarations whose call sites remain unreachable or have already been replaced
by verified production interfaces. None is reachable through basic scene data
linking, menu entry, start/stop, or cleanup. The scene audit was expanded from
34 to 40 exact records and now fails on recurrence of the stale
`TESDataHandler`, `ControlMap`, form-map, idle/action, or `SetLinkedRef`
selectors. The remaining subsystem audits continue to delimit the transitive
HUD, graph, face, package, Papyrus/serialization and camera surfaces.

### `SetLinkedRef` and walk-to-scene

Historical ID `192840` (RVA `0x481020`) and AE RVA `0x5645B0` were compared
instruction by instruction. Both consume `RCX=this`, `RDX=new linked
reference`, and `R8=keyword`; query the existing linked-reference extra data;
compare old/new targets; mark reference change flag `0x100`; update the extra
data; propagate old/new cell/world persistence state; and balance associated
reference notifications. Both are `void` and preserve the historical
`const TESObjectREFR*` source signature at the ABI boundary. The AE PDB names
the entry `TESObjectREFR::SetLinkedRef(TESObjectREFR*,BGSKeyword*)`, and the
supplied database maps ID `2202684` exactly to RVA `0x5645B0`. Evidence grade:
**disassembly-verified function**. CommonLib now selects
`RelocationID(192840, 2202684)`, so the production `WalkToAndStartScene` path
and native are enabled.

### Player-facing menu and Papyrus consumers

Original NAF's MCM is configuration only: it exposes settings and the
`NAFMenuHotkey` keybind. `keybinds.json` invokes global Papyrus native
`NAF.ToggleMenu`; that native opens the separate Scaleform `NAFMenu`. The HUD
is an overlay/status API and is not the scene-selection UI. `StartScene`,
`StopScene`, `GetSceneFromActor`, and `IsSceneRunning` are public Papyrus API
entry points for NAF/third-party scripts; the shipped normal player workflow
uses the C++ `NAFMenu` handlers, including `WalkToAndStartScene` for new scenes
and direct stop for active scenes.

The production `NAFMenu` is registered and `ToggleMenu` is restored. Its main
page is deliberately bounded to **New Scene**, **Active Scenes**, and
**Settings**. Creator/Studio and Item Explorer entries are removed from the
reachable menu because their animation-authoring, equipment and morph
dependencies remain gated. `SceneManager::IsBasicSceneCompatible()` remains
the final fail-closed check on a selected position, so group/tree positions or
equipment/morph/action choreography cannot be started through the menu.

Menu activation exposed another three-runtime collapse proactively:
`ControlMap::GetSingleton()` used 1.10.984 ID `2692014`. Dear/Addictol and the
exact supplied database establish AE ID `4799307`, RVA `0x31E5B08`.
`ControlMap::SetTextEntryMode` is independently instruction-matched from old
ID `1270079` to AE ID `4491359`, RVA `0x166F9A0`; both increment/decrement the
text-entry nesting byte at offset `0x140`. Existing exact AE entries
`2268335/2268336` remain the verified push/pop input-context calls.

Papyrus now registers 23 natives: settings (2), menu (1), package/process (2),
basic scene lifecycle including walk-to-scene (5), and HUD (13). Of these, the
menu toggle is directly user-facing through the shipped MCM hotkey; the scene
lifecycle functions are public API (the native menu invokes equivalent C++
paths); HUD drawing natives are API consumers of the visible overlay; settings
and package functions are script/API support. The remaining 22 natives stay
gated because they lead to dynamic idle, scene mutation/advanced queries,
NANIM/body/IK, managed face, direct transforms, or other dependencies outside
the bounded basic-scene surface.

### Build and next production scene test

Release x64/v143 builds and links with zero errors and `COPY_BUILD=OFF`.
All audits pass: 63 HUD, 16 graph, 3 face/lip, 12 package, 15
Papyrus/serialization, 8 camera, and 40 expanded scene records/guards. No
deployment occurred. Output is
`f4se-plugin/build-1.11.221/Release/NAF.dll`; SHA-256:
`6C8B50973A6A39976195B50EC9B0563E96AEC80C4DD41AC0B858B4DE1CFC79A6`.

The next test uses the shipped workflow: launch via F4SE, load a save, press
the MCM-configured **Menu Hotkey**, choose **New Scene**, select two ordinary
loaded NPCs, and choose the simplest position accepted by the menu. Avoid
equipment/morph choreography, groups, trees, Studio and Item Explorer. Let the
scene run briefly, then open **Active Scenes** and stop it. Verify both actors
resume ordinary AI/packages/collision and retain the logs. Only after that NPC
pass should the same bounded workflow be attempted with the player; do not save
while either scene is active.

## Production menu-entry regression audit (`1429302`)

The first hotkey test reached the registered Papyrus native but failed closed
on Address Library ID `1429302`. Its owner is selected CommonLib's
`BSScript::Stack::GetPageForFrame()` (`BSScript/Stack.h`), which incorrectly
treated the 1.10.163 ID as runtime-invariant. The precise path is MCM keybind
`NAFMenuHotkey` -> Papyrus global `NAF.ToggleMenu()` -> F4SE native-function
dispatch -> `BSScriptUtil` argument unpacking -> `Stack::GetPageForFrame()`.
The failure therefore precedes the `UIMessageQueue::AddMessage("NAFMenu",
kShow)` request and menu construction. The preceding audit covered native
registration and the UI body but not this lazy VM-stack helper, explaining the
miss.

Dear CommonLib identifies `1429302 / 2314680`; the supplied AE database maps
ID `2314680` exactly to RVA `0x20F32F0`. Instruction-level comparison confirms
the same `Stack*`, `StackFrame*` ABI and unsigned page-index return: both walk
the stack memory-page array, compare the frame address against each page's
payload bounds, and return the containing page index/count. The selected
declaration now uses `RelocationID(1429302, 2314680)`. Its adjacent argument
helper already used exact AE ID `2314681`, RVA `0x20F3360`.

The complete hotkey/open/navigation sweep proactively found one further
three-runtime collapse. `PlayerControls::GetSingleton()` is reached when
`NAFMenu::ProcessMessage(kShow)` blocks player input. Its old pair selected
1.10.984 ID `2692013`; Dear/Addictol, the exact supplied record, and PDB symbol
`g_PlayerControls` establish AE ID `4799306`, RVA `0x31E5B00`, with unchanged
`PlayerControls**` storage. This was corrected before another launch. Earlier
ControlMap corrections remain intact (`4799307` singleton and `4491359`
text-entry mode), as does disassembly-verified `SetLinkedRef` ID `2202684`, RVA
`0x5645B0`.

`tools/audit_menu_relocations.py` now independently pins 24 exact AE records
covering Papyrus stack argument access, UI/menu registration, message posting,
ControlMap input contexts/text entry, PlayerControls, Scaleform loading and HUD
filters, menu sounds/HUD modes, form enumeration/display names, and
`SetLinkedRef`. It also rejects recurrence of the four known stale selectors
and verifies the bounded menu source gates.

The menu ABI review found no new declaration defect. Selected and Dear
CommonLib agree on `IMenu` size `0x70`, `GameMenuBase` size `0xE0`,
`ControlMap` size `0x148`, and `PlayerControls` size `0x260`; the NAF-used
`IMenu`/`GameMenuBase` virtual declaration order and signatures match. The
already runtime-validated HUD exercises the shared GameMenuBase/Scaleform
construction and message interfaces. This remains static/corroborating
evidence for the newly opened NAFMenu, not a claim that its full navigation has
already run.

The reachable production UI was narrowed further. Active Scenes now exposes
status/current animation and **Stop Scene** only; speed mutation, position
changes, and inventory navigation remain compiled but have no reachable menu
binding. Hover highlighting is disabled because the optional historical
effect-shader entry remains OG-only; actor/location selection itself is not
blocked by the absence of that visual aid. Creator, Studio, Item Explorer,
equipment/morph actions, groups, and trees remain gated.

Release x64/v143 builds and links with zero errors, `COPY_BUILD=OFF`, and no
deployment. All baseline audits plus the new 24-entry production-menu audit
pass. Output: `f4se-plugin/build-1.11.221/Release/NAF.dll`; SHA-256:
`BDB8527422503C46D3D1FD674506CAFC990532750371AA6D1691CE2FAF4CD45A`.

The next runtime procedure remains production-level: load a save, press the
MCM Menu Hotkey, verify NAFMenu opens, select **New Scene**, choose two ordinary
loaded NPCs and a simple accepted position, let it run briefly, then use
**Active Scenes -> Stop Scene**. Verify AI, packages, collision and animation
state restore. Avoid Studio/Creator/Item Explorer, equipment, morphs,
groups/trees, and saving during an active scene. Test a player-involved scene
only after the NPC-only lifecycle succeeds.

## First active-scene playback relocation audit (`445184`)

The corrected CHAK and Dancecyclopedia animation XMLs now load and link in the
game. The first production scene reached `Active` at the one-actor position
`RBBelly1`, then failed closed on Address Library ID `445184`. No further XML
changes were made. `RBBelly1` resolves exactly to animation `RBBelly1` and its
single actor file
`radbeetle\Dances\loop\Belly_Dancing_Variation_One.hkx`; the one-actor count is
therefore expected.

ID `445184` belongs to selected CommonLib's
`BSScript::ArrayWrapper<Variable>::ReplaceArray`, not to the idle loader or
Havok playback itself. The exact call path is scene `Begin()` -> status becomes
`Active` -> `SCENE_START` -> `Scripts/Papyrus/EventProxy.h` ->
`GameUtil::SendPapyrusEvent()` -> `Papyrus::FunctionArgs` -> `ArrayWrapper`
construction -> `ReplaceArray`. The declaration incorrectly treated its
1.10.163 ID as runtime-invariant. Dear CommonLib supplies the candidate pair
`445184 / 2250435`; the supplied AE database resolves `2250435` to RVA
`0x10B6100`. Old/new disassembly independently verifies the same three
arguments, VM virtual call at physical offset `0xC8`, 16-byte `Variable`
element traversal, replacement ownership, and failure paths. The selected
CommonLib declaration now uses `RelocationID(445184, 2250435)`.

The expanded playback sweep found four additional stale invariant IDs before
they could fail at runtime. PDB names plus instruction-level old/new comparison
establish:

| Interface | 1.10.163 ID | 1.11.221 ID | AE RVA | Evidence |
|---|---:|---:|---:|---|
| `hkbContext(hkbCharacter*, ...)` | `1381136` | `2258328` | `0x13A9B10` | PDB + matching construction/refcount flow |
| `hkbContext::~hkbContext()` | `144578` | `2258329` | `0x13A9B90` | PDB + matching destruction adjacency/body |
| `hkbBehaviorGraph::setActiveGeneratorLocalTime` | `992878` | `2260451` | `0x147C170` | PDB + matching clone/internal-call/control flow |
| `hkbBehaviorGraph::getNodeClone` | `326555` | `2260454` | `0x147C5A0` | PDB + matching member access/tail-call/fallback |

These calls are reached after dynamic-idle playback while NAF polls readiness,
synchronizes actor graph times, and subsequently tracks animation time. Their
headers now select the exact AE IDs. The similarly named legacy helpers in
`RE/BSAnimationGraphManager.h` that serve the optional custom NANIM graph
traversal are outside the bounded HKX/Dancecyclopedia path and remain
unpromoted rather than being changed by name association.

`tools/audit_playback_relocations.py` pins 11 exact AE records/source guards:
the Papyrus array wrapper, actor initialization, graph-transition query,
reset-action lookup, `TESActionData::DoIt`, special-idle setup, graph-variable
setter, and the four Havok entries above. Together with the 40-entry scene
audit it covers the currently reachable basic solo start/play/stop surface.
Five stale mappings were corrected in this pass (`445184` plus four Havok
IDs); no additional bad exact selector was found.

### Dynamic-idle and animation-interface status

For `RBBelly1`, XML `actor@file` becomes `Data::Animation::Slot::idle[0]` with
`dynamicIdle=true`, then scene property `kDynIdle`, `SmartIdle::kHKX`, and
`Scene::DynamicIdle::Play`. That helper updates the locked NAF dynamic-idle
form with the HKX path/event/behavior graph and calls maintained
`AIProcess::PlayIdle`, which is the verified `SetupSpecialIdle` pair
`1446774 / 2231704` (AE RVA `0xCFAD40`). No custom NANIM loader or optional
`BShkbUtils::GraphTraverser` is used by this solo HKX path.

The four previously deferred animation interfaces now have these grades:

* `InitializeActorInstant`: disassembly/PDB verified ID `2236393`, RVA
  `0xE13E40`; reached by production post-start actor initialization.
* `IsActiveGraphInTransition`: disassembly/PDB verified ID `2214305`, RVA
  `0x806370`; reached during `SettingUp` and `WaitingForLoad`.
* `RunActionOnActor`: the AE wrapper was removed; the verified semantic
  replacement is `TESActionData::DoIt`, ID `2234273`, RVA `0xDBFEC0`, physical
  slot `0x05`. It preserves the constructed actor/action data path and is used
  for graph reset/stop, but remains scene-runtime-test-required.
* `IsIdleLoading`: no trustworthy one-to-one AE entry has been established.
  The bounded replacement polls the active graph clone/sync information and
  treats unavailable or negative graph time as not ready. Its component
  functions are now disassembly verified, but equivalence of the readiness
  policy is still runtime-test-required; it is not presented as the old
  function's relocation.

No new declaration-order defect was found in this bounded pass. The custom
`BSAnimationGraphManager`/Havok member offsets previously audited by graph
observation are now exercised more deeply by readiness/time synchronization;
their runtime behavior, including the readiness substitute and sync-info
layout, remains the principal playback uncertainty. The source adds lightweight
phase logs around actor control, scene-event dispatch, graph initialization,
assignment resolution, graph readiness, and dynamic-idle request so the next
production failure can be localized without synthetic diagnostics.

Release x64/v143 builds and links with zero errors and `COPY_BUILD=OFF`.
Playback, scene, HUD, graph, face, package, Papyrus/serialization, camera, and
menu audits all pass. Output is
`f4se-plugin/build-1.11.221/Release/NAF.dll`; SHA-256:
`4D6A6B173E408995BED3EEE60E518D1B2F8F0AEFA1E04BF677A55484D5F89B61`.

The next production test repeats the verified UI flow with the solo
Dancecyclopedia `RBBelly1` position. Confirm the new phase logs, actual
animation start, brief advancement, then stop through **Active Scenes** and
verify package/AI/collision/animation cleanup. Only after solo HKX playback and
cleanup succeed should a paired CHAK `_F.hkx`/`_M.hkx` position be tested, and
only after NPC-only pairing should the player be included. Do not save while a
scene is active.

## Scene-event Papyrus ABI audit (`69733`)

The next `RBBelly1` run again completed actor control/package/transform setup,
entered `Active`, and reached the new `SCENE_START` boundary log. Exact lookup
then failed on ID `69733` before the post-start task or animation assignment
ran. This confirms a Papyrus event-construction failure; no XML,
dynamic-idle, graph-time, or Havok playback code was involved in this runtime
failure.

ID `69733` is the historical 1.10.163 entry at RVA `0x13DDDB0` used by
`Papyrus::detail::CreateThreadScrapFunction`. It constructs the old
Microsoft-layout `BSTThreadScrapFunction<bool(BSScrapArray<Variable>&)>`
closure around NAF's `FunctionArgsBase`. Disassembly shows the 16-byte captured
`FunctionArgsBase`, old function-manager allocation/vtable setup, and the
closure body that copies the wrapped `ArrayWrapper<Variable>` into the output
scrap array. PDB evidence places the associated old lambda/function wrapper in
the same `0x13DDBB0` region. This is an OG ABI adapter, not an invariant engine
service and not a 1.10.984-as-AE ID. AE CommonLib intentionally dispatches with
`std::function`/`BSTThreadScrapFunction` lambdas over a packed
`BSScrapArray<Variable>`; a one-to-one relocation for `69733` is therefore the
wrong migration.

`GameUtil::SendPapyrusEventWithCallback` now uses that maintained AE path:
`PackVariables` creates the argument array, the F4SE external-registration
enumerator supplies each registered target, and the verified VM handle-based
`DispatchMethodCall` receives a lambda that copies the packed arguments. The
same source-level migration was applied to NAF's global and form-method call
helpers so they cannot re-enter the obsolete `FunctionArgs` adapter. The
historical ordering is preserved exactly: actor control is applied, status
becomes `Active`, `SCENE_START` is synchronously enumerated/dispatched, and only
then is the delayed post-start/graph/playback task scheduled. The event remains
constructed even when no external scripts are registered, matching the
historical call ordering rather than being skipped to reach playback.

The audit also found two declaration-order ABI defects in both
`BSScript::IVirtualMachine` and `BSScript::Internal::VirtualMachine`. Header
comments named the correct slots, but declarations placed the overloads in the
wrong physical order:

* `CreateArray(const TypeInfo&, ...)` is physical slot `0x19`, RVA `0x21123A0`;
  `CreateArray(RawType, ...)` is slot `0x1A`, RVA `0x2112220`.
* `DispatchMethodCall(Object, ...)` is physical slot `0x2D`, RVA `0x2115600`;
  `DispatchMethodCall(handle, ...)` is slot `0x2E`, PDB-named at RVA
  `0x2115480`.

Declaration order is now `0x19/0x1A` and `0x2D/0x2E` in both base and concrete
VM headers. This prevents the corrected event call from dispatching through
the object-overload slot with the handle-overload ABI, and prevents
`SCENE_END_DATA` actor-array packing from calling the raw-type array overload.
Neighboring physical slots were read directly from the AE VM vtable at RVA
`0x28F1D70`: `SendEvent` `0x2B -> 0x2114AD0`, `DispatchStaticCall`
`0x2C -> 0x21150F0`, and `DispatchUnboundMethodCall` `0x2F -> 0x2115630`.

`tools/audit_scene_event_relocations.py` now checks nine exact AE records and
seven physical VM slots/source guards. The exact records cover the VM vtable,
`g_GameVM`, string-pool get/release, scrap allocation/deallocation, and the
already corrected adjacent ArrayWrapper/Stack helpers. The audit found no
additional stale relocation ID beyond `69733`; it did proactively find and
correct the two overload-order defects above. `69733` remains only inside the
unused OG compatibility header and is no longer reachable from NAF's AE event
or call helpers.

Event-phase logging now distinguishes argument construction, packed arguments
ready/dispatch invocation, and dispatch return. On success the existing scene
post-start/graph/playback logs follow immediately. The playback implementation
and corrected animation XMLs are unchanged by this pass.

Release x64/v143 builds and links with zero errors and `COPY_BUILD=OFF`.
The new scene-event audit plus playback, scene, HUD, graph, face, package,
Papyrus/serialization, camera, and menu audits all pass. Output:
`f4se-plugin/build-1.11.221/Release/NAF.dll`; SHA-256:
`85F046AD663460F78D9F4F5C5316CE35F1D0BD3C7E2F697A766031DE4CB26C56`.

`SCENE_START` is statically ready for the next production `RBBelly1` run. The
expected next boundary is `external-event dispatch returned`, followed by
post-start graph initialization, animation assignment, dynamic-idle request,
and actual playback. Stop through **Active Scenes** and verify full cleanup;
do not save while the scene is active.

## `SceneEndedData` Actor-array ABI recovery

The first production `RBBelly1` scene subsequently completed dynamic-idle
assignment, entered `Active`, played the solo HKX, and completed StopScene's
actor/package/input/collision cleanup. `NAF::SceneStarted` and the simple
`NAF::SceneEnded` external events both built and returned. The remaining crash
occurred before any Actor array element was inserted, while constructing the
second argument for `NAF::SceneEndedData`. These milestones are therefore
runtime verified independently of the array-packing failure: solo playback,
dynamic-idle request, active scene lifetime, gameplay teardown,
`SceneStarted`, and simple `SceneEnded` dispatch.

The shipped `NAF.psc` contract and unchanged 1.0.14 C++ event path agree that
`NAF::SceneEndedData` takes `(NAF:SceneId akScene, Actor[] akActors)`. Original
NAF sends it synchronously after the simple end event from the same scene-end
notification; the port did not move it across cleanup. `Events::SceneData`
owns `NiPointer<Actor>` entries, so the actors remain strongly referenced while
the temporary `vector<Actor*>` is packed. The rebuilt DLL logs the retained
element count and null count before packing. The observed fault preceded the
element loop, so neither Actor lifetime nor lifecycle ordering caused this
crash.

Instruction-level reconstruction found the actual ABI defect. The AE VM vtable
at RVA `0x28F1D70` contains:

* physical slot `0x19` / byte offset `0xC8`, ID `2315148`, RVA `0x21123A0`:
  `bool CreateArray(const TypeInfo&, uint32_t, BSTSmartPointer<Array>&)`;
* physical slot `0x1A` / byte offset `0xD0`, ID `2315147`, RVA `0x2112220`:
  `bool CreateArray(RawType, const BSFixedString&, uint32_t,
  BSTSmartPointer<Array>&)`.

The TypeInfo entry receives `this` in RCX, a `TypeInfo*` in RDX, the 32-bit
count in R8D, and the output smart-pointer address in R9. It normalizes the
array tag (primitive array codes subtract `0xA`; complex object/struct arrays
clear pointer bit zero) before entering the common allocator. The RawType entry
uses RCX/RDX/R8/R9 for `this`, raw element type, type-name reference, and count;
its fifth output argument is on the stack. Return type is `bool` for both.

The previous header edit placed these overload declarations in apparent slot
order. Under the MSVC ABI, members of this overload set are emitted into the
vftable in reverse declaration order. The resulting NAF machine code called
`[vtable+0xD0]` with the three-argument TypeInfo register layout. The RawType
entry consequently read the TypeInfo pointer as a raw enum, the count as a
`BSFixedString` reference, the output address as the count, and a nonexistent
fifth argument as the output smart pointer. At RVA `0x21210D4` the shared array
allocator attempted to release that bogus output value. The crash logger's
nearest PDB label, `PostCachedErrorToLogger`, was misleading: the VM had not
reached an Actor-type lookup error path, and `0xFFFFFFFF` was the intrusive
reference decrement constant, not evidence of an invalid Actor RawType.

Both `IVirtualMachine` and `Internal::VirtualMachine` now deliberately declare
the RawType overload before the TypeInfo overload, with an MSVC-vftable note.
The rebuilt Actor-array packer emits `call [vtable+0xC8]`, verified directly in
`NAF.dll`; the scene-event audit rejects the old `+0xD0` call shape. The same
MSVC overload rule applies to `DispatchMethodCall`: the handle overload is
declared before the object overload so physical slots remain object `0x2D` and
handle `0x2E`. This corrects the broader external-event dispatch surface rather
than special-casing `SceneEndedData`.

`TypeInfo` remains eight bytes. AE encodes primitive arrays as element RawType
plus `0xA`, and complex object/struct arrays by setting bit zero on the aligned
type-info pointer. `GetTypeInfo<vector<Actor*>>()` correctly resolves the Actor
`ObjectTypeInfo` and applies the complex array bit; `CreateArray(TypeInfo)` is
the correct overload. A separate helper defect was found proactively:
`TypeInfo::SetArray(false)` subtracted `kArrayEnd` (`0x12`) for primitive arrays.
It now subtracts the verified array tag `kArrayStart` (`0xA`). Object packing,
`Variable` insertion, structure wrapping, and array ownership otherwise match
the Dear/CommonLib implementation and AE allocator semantics; no NAF-specific
array workaround or event suppression was introduced.

`tools/audit_scene_event_relocations.py` now verifies 11 exact AE records,
seven physical VM slots, both overload declaration-order guards, primitive
array decoding, and the compiled Actor-array call site. The complete HUD,
graph, face, package, Papyrus/serialization, camera, menu, scene, playback, and
scene-event audit suite passes. Release x64/v143 builds and links with zero
errors and `COPY_BUILD=OFF`. The same solo `RBBelly1` start/run/stop sequence is
ready for a full retest; success requires the retained-actor log, completed
`SceneEndedData` argument construction and dispatch, stable gameplay, and
normal post-scene AI/package restoration.

## Player-scene free-camera restoration

The next production run validated two consecutive single-actor scenes
(`RBBelly1` and `RBTwerk`) through start, dynamic-idle playback, normal stop,
actor/package/input/collision cleanup, both end events, and later
serialization. `SceneEndedData` retained one valid Actor, built its `Actor[]`,
and returned from external-event dispatch. Basic single-actor scene lifetime
and the corrected Papyrus array path are runtime validated on 1.11.221.

The player camera lock was not an omitted activation block. The 1.0.14 and
ported scene paths both hide the Pip-Boy, call `Force3rdPerson`, stop player
movement, choose player 3D as the optional NAF look-at target, call
`SetFlyCam(true)`, set the player AI controlled, and install the `NAF_Scene`
input layer. Scene end clears AI/input first; post-stop releases look-at and
calls `SetFlyCam(false)`. Runtime logging already proved that free-camera
activation succeeded and the production `FreeCameraState::Update` hook fired
during a scene.

`SetFlyCam` uses the engine's TFC/free-camera transition, not a console command:
`PlayerCamera::ToggleFreeCameraMode(false)`, old ID `224913`, AE ID `2248368`,
RVA `0x1027590`. Disassembly shows that the AE function compares current state
with camera-state index `3`, pushes and initializes free-camera state on entry,
and pops the saved state on exit. The relevant state indexes remain first
person `0`, free `3`, and third person `8`; the table starts at
`PlayerCamera+0xE0`, with `currentState` at `+0x28`. Because historical NAF
forces third person before pushing free camera, it restores third person rather
than preserving a pre-scene first-person mode. The port retains that behavior.

The historical scene mask disables Activate, Fighting, Jumping, MainFour,
Movement, POVSwitch, Sneaking, and VATS (`0xDE5`). Fallout 4 1.11.221 has a
distinct TFC input context, but free-camera translation is still categorized
as Movement. Installing the historical layer after entering TFC therefore
suppresses camera translation. The port now keeps `0xDE5` for package/walk
control and uses a player-free-camera-only mask of `0xDE4`, omitting only
`kMovement`. The animated player remains AI controlled while TFC consumes the
retained movement input. NPC-only scenes are unchanged. This is a source-level
AE semantic adaptation and actual free translation remains runtime-test-required.

`GameUtil::SetFlyCam` now validates PlayerCamera/current/free state, compares
state object identity (matching the verified engine implementation), logs the
before/after states, and returns transition success. Scene entry logs look-at,
mask, and activation; post-stop logs restoration. Normal stop, setup failures
routed through `End`, force-stop, and actor-loss teardown retain the original
symmetric cleanup ordering. Animation, playback, and Papyrus event code are
unchanged. The player-input Papyrus natives remain gated because the internal
fix does not establish their broader external API surface.

The extended camera audit passes eight exact IDs, physical FreeCameraState
slot `0x0B`, its AE prologue, and 15 source guards. The scene audit passes 40
exact IDs. HUD, graph, face, package, Papyrus/serialization, menu, playback,
and scene-event regression audits also pass. Release x64/v143 builds and links
with zero errors and `COPY_BUILD=OFF`.

Retest first with a solo NPC scene, then `RBBelly1` or `RBTwerk` on the player.
Verify free camera translation/look while the actor remains controlled, then
stop via Active Scenes and verify input, third-person camera, AI/package state,
collision, HUD, and both Papyrus end events. Historical behavior expects
third-person restoration even if the player began in first person.

## Common production scene-feature restoration

The player-camera build passed production runtime testing across solo
`RBBelly1`, `RBCharleston1`, `RBTwerk`, `RBBoogaloo`, and `RBMacarena`, plus
paired `(CHAK) Hug 01` and `(CHAK) Kissing 01`. The player Kissing scene entered
movable free camera with mask `0xDE4`, played distinct actor-specific HKXs,
stopped cleanly, packed a two-element `Actor[]`, dispatched both end events,
and restored normal camera/control. Solo and paired HKX playback, complete
scene lifetime/cleanup, player free camera, all three scene events, and one-/
two-Actor Papyrus arrays are now the known-good production baseline.

### Feature matrix

| Feature | Classification | Disposition |
| --- | --- | --- |
| slot/position equipment sets and equipment actions | A/B: common choreography | enabled through the production ordered queue |
| per-slot morphs and position start/stop morph sets | B: common optional behavior | enabled through guarded LooksMenu integration |
| delay actions | A | retained; runtime validated |
| HUD cleanup reached by equipment changes | B | enabled through the validated HUD path |
| animation request/reset and `AnimObjUnequip` | A | retained; production dispatch has executed in successful scenes |
| generic `InsertCustom` callbacks | C: unbounded dependencies | still rejected |
| animation groups, position trees, NANIM/Havok and body IK | C: advanced playback | gated |
| continuous transforms, Creator, Item Explorer and Studio | C/D: editor/Studio | gated |

Animation compatibility now accepts slot equipment, morph and equipment-action
metadata. Position compatibility accepts position-level equipment/morph sets
but still requires the ordinary single-animation control system, so group/tree
systems remain unreachable. Current Action objects contain only start/stop
equipment sets; they do not expose arbitrary callbacks.

### Equipment choreography

Original NAF executes unequip, re-equip, add and remove operations under the
ordered-action mutex from the serialized `NonRendersafeAI` callback. The port
restores that context. Removed slot forms and NAF-added inventory objects are
tracked by actor handle. Animation-slot stop sets run before transition/reset;
position stop sets run during post-stop. Normal and abort teardown share these
paths. Cleanup remains content-declared: malformed XML lacking matching
stop/re-equip actions is not given invented rollback behavior.

The reachable audit corrected three stale CommonLib mappings before enabling
equipment:

| Interface | Supplied-database AE mapping | Evidence |
| --- | --- | --- |
| `ActorEquipManager` singleton | ID `4798287`, RVA `0x31E33A8` | Dear three-runtime mapping plus resolved AE global; `2690994` was 1.10.984-only |
| `TESObjectREFR::GetInventoryObjectCount` | ID `2200939`, RVA `0x502290` | AE PDB name and exact record |
| `TESObjectREFR::AddInventoryItem` | ID `2200949`, RVA `0x502950` | AE PDB name and exact record |

Existing `ActorEquipManager::EquipObject` ID `2231392` / RVA `0xCE5A90` and
`UnequipObject` ID `2231395` / RVA `0xCE5F30` agree with Dear's declarations and
exact supplied records. Historical and Dear signatures agree. `RemoveItem`
remains the existing virtual call; no raw RVA or guessed slot was introduced.

### LooksMenu/body morphs

`LooksMenu::Init` is restored at the historical F4SE post-load point. The
verified LooksMenu 1.7.0.3 binary SHA-256 is
`B562D636D768620760AAD8C166CCFDA22B014F490ABACE0B21E84334B26C56F0`.
The local virtual declaration order exactly matches the supplied LooksMenu
source for all NAF-used morph methods. Runtime support now additionally fails
closed unless the known `ApplyMorphsToShapes` prologue at RVA `0x11400` and
the object at RVA `0x107E00` pointing to vtable RVA `0xC88B0` both match. No
fallback scan is performed.

Morph work retains original threading: slot and morph-set data enqueue
`SetMorph`/`UpdateMorphs` on the serialized `NonRendersafeAI` ordered queue.
Cleanup is represented by later values or an explicit stop morph set; the port
does not globally clear morphs owned by other mods. Same-session application
and declared cleanup remain runtime-test-required. Active-scene save/load stays
deferred because live equipment/morph queue state is not yet safely restored.

`AnimObjUnequip` remains a manual graph event delivered through
`BGSAnimationSystemUtils::NotifyGraphSources` on the F4SE task interface before
new playback. Its dispatch has naturally executed in successful solo/paired
scenes. Actual animation-object removal is content-dependent and remains
runtime-test-required.

### Ordered actions and Papyrus

Enabled ordered categories are delays, equipment unequip/re-equip/add/remove,
morph updates, existing animation reset/request, and validated HUD cleanup.
Generic callbacks remain compile-time gated.

Papyrus registration increases from 23 to 35 natives. Newly registered:
`IsActorUsable`, `SetActorUsable`, `PlayDynamicIdle`, `SetScenePosition`,
`GetSceneActors`, `GetSceneHKXs`, `GetSceneTags`, `SetSceneSpeed`,
`GetSceneSpeed`, `GetSceneProperty`, `ValidateSceneParams`, and the validated
emergency cleanup native `ReEnablePlayerInput`. NANIM/IK, managed-face,
transform/synchronization, and Studio/editor APIs remain unregistered. Each new
native remains behaviorally runtime-test-required when first exercised.

`tools/audit_scene_features.py` verifies five exact equipment/inventory AE
records, 15 source guards, and the exact LooksMenu SHA-256. All established
camera, scene/event, playback, HUD, graph, face, package, menu, and
Papyrus/serialization audits still pass. Release x64/v143 builds and links with
zero errors and `COPY_BUILD=OFF`.

Runtime progression: repeat solo/paired baseline scenes, then use real pack
content declaring start/stop equipment sets, followed by content declaring
morphs or start/stop morph sets. Verify cleanup after normal stop, then repeat a
player case and verify camera, package, AI, collision and input restoration.
The supplied CHAK/Dancecyclopedia conversions declare none of these features,
so they cannot validate them. Do not save during an active scene.

Remaining advanced systems: animation groups, position trees, NANIM/custom
Havok traversal, body IK, continuous alignment/transforms, full managed-face
API, Studio, Creator, Item Explorer, arbitrary callbacks, and active scene
serialization. Advanced playback/group systems are the recommended next stage
after the equipment/morph production test; Studio should follow those playback
and transform dependencies.

## LooksMenu detection and animation-group/position-tree restoration

### Runtime baseline and LooksMenu detection correction

The common scene-feature runtime retained the production scene baseline (solo,
paired and player scenes; camera; cleanup; events; HUD; graph; face; package;
Papyrus and serialization), but it did not exercise morphs: historical
`LooksMenu::Init` first queried F4SE plugin metadata using the logical name
`F4EE`. F4SE 0.7.8 did not resolve that historical metadata key even though the
exact supported `f4ee.dll` was loaded. This was a discovery failure, not a
LooksMenu binary or interface mismatch.

Detection now starts from the exact loaded module name `f4ee.dll`, obtains the
path of that loaded module, and computes SHA-256 using Windows CNG. Initialization
continues only for SHA-256
`B562D636D768620760AAD8C166CCFDA22B014F490ABACE0B21E84334B26C56F0`.
The existing RVA guards remain mandatory after the hash check:

- `ApplyMorphsToShapes`: RVA `0x11400` and its verified prologue;
- `g_bodyMorphInterface`: RVA `0x107E00`;
- `BodyMorphInterface` vtable: RVA `0xC88B0`.

There is no display-name fallback, fuzzy DLL match, signature scan, or support
for an unrecognized binary. `tools/audit_scene_features.py` now rejects a
regression to `GetPluginInfo("F4EE")`, verifies the loaded-module/SHA path and
still checks the supplied DLL hash. The new detection path is statically
validated and built; successful runtime detection/interface initialization and
actual morph application remain runtime-test-required. Equipment and morph
behavior likewise remain content-dependent because the supplied converted packs
do not declare suitable choreography.

### Animation-group and staged-position architecture

An animation group is a production control system layered over ordinary
animations. It selects the first stage, counts that stage's configured loops,
then calls the established scene animation transition path. Sequential groups
advance in definition order; non-sequential groups use their existing weighted
selection. Completion reports through the existing control-system contract.
Stop/cancel uses the same scene teardown already runtime-validated for ordinary
positions. A staged position is not a separate engine mechanism: it is a
position whose payload names an animation group.

Scene compatibility now admits a group only when it is nonempty and every stage
resolves to an animation compatible with the restored ordinary playback path.
This is checked before constructing the scene, so a missing or still-gated stage
fails closed instead of becoming a null transition during playback.

The installed CHAK conversion provides one complete initial production test:

- position `(CHAK) *Staged* Hugs` references group `CHAK_Hugs_Staged`;
- `CHAK_Hugs_Staged` is sequential;
- stage 1 is `CHAK_Hug_01`;
- stage 2 is `CHAK_Hug_02`;
- both animation definitions exist and already use the working paired-HKX path.

Other incomplete converted CHAK staged definitions are not accepted as evidence
against the framework.

### Position-tree architecture

Position trees remain a distinct production controller. Each node names an
ordinary position or animation-group position; a nested controller runs that
position. Tree progression uses the existing node timers and either automatic
advance or HUD directional input/branch selection, with local events and menu
sounds. Cleanup unregisters the tree listeners and tears down the nested
controller through the scene lifecycle. Nested position trees are not supported
by the historical `excludeTrees` construction path and are now rejected during
compatibility validation rather than failing after scene start.

The only newly reachable engine relocation in this bounded controller path is
`UIUtils::PlayMenuSound`, exact supplied-database ID `2249707`, RVA `0x1086220`.
Its mapping agrees semantically with the modern three-runtime reference. All
other reachable work uses the already restored HUD, graph/playback, timers,
events and scene mutation paths. No complete position-tree XML exists in the
supplied converted material, so the production implementation is statically
enabled but runtime behavior remains content-dependent.

`tools/audit_group_tree.py` verifies the exact menu-sound record, group/tree
source gates, continued rejection of arbitrary ordered callbacks, and the CHAK
group-to-position linkage. The complete established audit set passes: 63 HUD,
16 graph, 3 face, 12 package, 15 Papyrus/serialization, 8 camera, 40 scene, 24
menu, 11 playback, 11 scene-event, 5 equipment/inventory, and the bounded
group/tree record. Release x64/v143 builds and links with zero errors and
`COPY_BUILD=OFF`.

No additional Papyrus native is required for group/tree playback: the existing
production scene entry and `SetScenePosition` surface routes through the restored
control-system factory. The registered/gated count remains 35/10. Generic
callbacks, NANIM/custom Havok traversal, body IK, continuous alignment and
advanced transforms, managed-face APIs, Studio, Creator, Item Explorer, and
active-scene serialization remain gated.

Runtime plan: verify the explicit LooksMenu detection/hash/interface-success
log, regress one known solo or paired scene, then start `(CHAK) *Staged* Hugs`.
Observe `CHAK_Hug_01` followed by `CHAK_Hug_02`, actor synchronization, and
normal completion; also stop once during each stage and confirm package, AI,
collision, camera and input restoration. Do not save during the active staged
scene. Test position-tree navigation only when complete real tree content is
available.

## Active-scene persistence, Body IK, and continuous alignment

### Promoted runtime baseline

The 1.11.221 production test promoted loaded-module LooksMenu 1.7.0.3
detection, exact SHA-256 validation, body-morph interface initialization,
animation groups, staged positions, paired staged playback, player free camera,
staged cleanup, and both Papyrus end events. `(CHAK) *Staged* Hugs` cycled
`CHAK_Hug_01 -> CHAK_Hug_02 -> CHAK_Hug_01 -> CHAK_Hug_02` for both actors and
then restored camera, package, AI, input and collision state. Actual morph
application remains content-dependent.

### Dependency classification

| System | A: runtime validated | B: restored/not content-tested | C: required work completed here | D: remains gated |
| --- | --- | --- | --- | --- |
| active-scene persistence | F4SE save/load/revert callbacks, actor/form handles, packages, camera/input cleanup, scene playback and v5 record transport | active-scene records had previously only been byte-preserved | v5 scene/task/equipment/UID/face/scene-HUD activation and world-ready fixup | custom NANIM `BODY` record |
| Body IK | graph-update and Set3D hooks, `Get3D`, graph project-name access, actor 3D lifetime in the tested graph path | actual solver motion requires real GraphInfo IK chains/content | standalone node IK creation, target/enable API, graph cadence and cleanup | NANIM generator/recorder/Creator |
| continuous alignment | paired and staged scenes, worker update cadence, actor transforms, collision suppression, player-3D refresh | deliberate perturbation/drift correction remains content-dependent | bounded transform/synchronization Papyrus exposure | Studio forced-alignment/editor controls |

### Historical v5 active-scene records

The serialization schema and record version remain unchanged. Production now
writes and reads the same eight version-5 records as historical NAF:

| Record | Persistent contents | Restoration behavior |
| --- | --- | --- |
| `TASK` | serializable timed tasks, duration/repeat/time-scale state | timer thread is stopped while swapping state, then restarted |
| `SCNE` | scene IDs, actors/properties, control systems, group/tree state, speed, positions, sync state and scene task containers | loaded with updates inhibited; fixed up only after an active world exists |
| `EQPT` | unequipped and NAF-added item restoration bookkeeping | restored without serializing pending C++ callbacks |
| `UID` | next framework/UI identifiers | restored exactly |
| `FACE` | managed facial-animation persistent state | restored through the already runtime-validated face subsystem |
| `HUD` | production HUD state | unchanged |
| `PACK` | active package override state | actively restored using the verified form/handle path |
| `SHUD` | scene-tree HUD handles | restored with tree listener rebinding where applicable |

The serialized widths, cereal ordering, record identifiers and version remain
historical v5. Historical saves remain structurally readable, and new saves use
the historical schema. `BODY` is deliberately still preserved byte-for-byte:
that record serializes custom NANIM graph playback, not ordinary node IK. It is
not written from partially enabled state.

Raw graph managers, Havok objects, 3D pointers, locks, worker identity, free-
camera objects, detached C++ callbacks and dynamic-idle runtime objects are not
serialized. Stable form IDs/handles, animation/position data identifiers,
control-system state and timers are serialized. Equipment pending lambdas and
the ordered delay counter were not part of the historical schema and are not
invented now; persistent equipment rollback data is retained by `EQPT`.
LooksMenu owns its runtime morph data; NAF does not add a new morph record.

### Deferred scene fixup and failure policy

During `LoadCallback`, restored scenes are marked `noUpdate` before becoming
visible to the game-loop worker. A generation-guarded F4SE task waits until
`Main::gameActive`, preventing stale tasks from a revert or another load from
touching the new state. Fixup then:

1. resolves every serialized actor handle and requires a loaded parent cell;
2. restores the NAF in-scene marker, graph initialization and foot-IK disable;
3. reapplies collision suppression;
4. reapplies NPC restraint/combat/package override, or player AI/input/free
   camera ownership;
5. re-requests the current serialized actor animation assignments;
6. reenables scene updates.

The existing control system is not restarted. Thus an animation group's
`currentStage`/`loopsRemaining`, a position tree's `currentNode`, scene speed,
and serializable timers retain historical state. Position-tree transient parent
pointers and HUD key listeners are rebound through `OnSerializationLoad`
without calling `OnBegin` or resetting progression.

If any actor, control system, cell, or required runtime object is unavailable,
the scene is force-stopped through production teardown. Valid actors receive
normal package/input/collision/camera cleanup instead of leaving a partially
restored scene active. Revert invalidates pending fixup tasks and resets timer,
scene, equipment, UID, face, HUD, package and scene-HUD state.

### Body IK architecture and AE evidence

NAF Body IK is node-level IK, not direct traversal of Fallout 4 Havok layouts.
`NodeAnimationGraph` is created for an actor through the already verified graph
project-name path. `GraphInfo` supplies node names and chain topology; Set3D
registration refreshes `NiAVObject` node pointers. The existing graph detour
runs after the engine animation graph update and invokes `IKManager::Update`
while the graph's update mutex is held. The pinned historical `ik` library does
the solver math over NAF `NodeTransform`/verified `NiTransform` data. Chain
disable or graph deletion clears active solver state and 3D registrations.

The bounded reachable relocation set is unchanged and exact:

- `GetAnimationGraphProjectName`: ID `2214534`, RVA `0x818730`;
- graph update hook: ID `2214536`, RVA `0x818970`;
- `TESObjectREFR::Set3D`: ID `2201055`, RVA `0x50C5B0`.

No custom Havok structure, new virtual slot, or raw RVA was introduced. The
production `SetIKChainTarget` and `SetIKChainEnabled` natives are enabled;
`PlayNANIM`/`StopNANIM` remain gated. Solver movement is runtime-test-required
until real content with matching GraphInfo IK chains is available.

### Ordinary-scene alignment and bounded Papyrus surface

Continuous ordinary-scene alignment was already present in `Scene::Update` and
has run on the serialized `NonRendersafeAI` cadence throughout successful paired
and staged scenes. Each update derives actor-local offsets from the scene root,
corrects rotation, corrects position only outside the coordinate tolerance,
refreshes player 3D where required, and restores collision on teardown. This is
separate from `GameLoopHook::ForceAlignActor`, which remains used only by gated
Studio/editor code.

The path reuses exact ID `2229653` / RVA `0xC5CA70` for actor collision disable
and ID `2231882` / RVA `0xD02270` for player 3D refresh, plus the already audited
virtual transform calls and local math. No relocation or ABI correction was
needed. `GetLocalTransform` and `SynchronizeAnimations` are now registered
because they use the verified `Get3D`, `NiTransform`, and SmartIdle graph-time
paths. Registration is now 39 production natives with six gated: NANIM play/
stop and four managed-face calls. Studio/editor transform APIs remain absent.

### Audits and runtime scope

New audits:

- `tools/audit_active_scene_serialization.py` checks all eight v5 records,
  update inhibition, generation-gated world fixup, tree rebinding, cancellation,
  and continued `BODY` exclusion;
- `tools/audit_body_ik.py` checks three exact AE records, graph/IK update guards,
  two IK natives, and the NANIM gate;
- `tools/audit_continuous_alignment.py` checks ordinary-scene cadence, transform/
  collision/player-3D guards, two exact AE records and bounded native exposure.

All prior HUD, graph, face, package, Papyrus, camera, scene, menu, playback,
scene-event, equipment/LooksMenu, and group/tree audits continue to pass.

Runtime progression:

1. start a simple solo/paired scene, save while active, reload, confirm the
   persistence-fixup completion log, then stop and verify full cleanup;
2. repeat with `(CHAK) *Staged* Hugs`, checking that the saved group stage resumes
   rather than resetting to stage zero;
3. repeat with the player and verify free-camera/input ownership after load and
   restoration after stop;
4. use real IK-chain content when available; absence of such content does not
   block the persistence test;
5. observe paired alignment and cleanup; deliberate perturbation remains optional
   if no normal pack workflow exposes it.

This was the gate set at the start of the full-parity campaign. Phases A-C and
the Phase-D source restoration below have since removed every source gate.
Content-dependent NANIM, managed-face, IK/morph/equipment, and authoring
workflows remain explicitly on the runtime-coverage list.

## Full NAF 1.0.14 -> NAF AE parity matrix

### Preservation target and newly promoted evidence

The project target is full functional preservation of upstream NAF 1.0.14 on
Fallout 4 1.11.221. Every feature, public API, playback mode, persistence path,
menu and authoring workflow that functioned upstream remains in scope. Temporary
gates may exist only while an AE dependency, ABI, relocation, lifecycle, or
runtime test is unresolved. The final gate count must be zero unless binary and
source evidence proves that a feature cannot be reproduced on 1.11.221.

The latest production tests promote ordinary and staged/group NPC active-scene persistence to
**restored/runtime-validated**. An active two-actor scene wrote all eight v5
records, reloaded, reacquired both actors, restored its original control system
and position, restarted playback, reported `1 resumed, 0 cancelled`, then
stopped normally and dispatched both `SceneEnded` and `SceneEndedData`. The
staged CHAK Hugs scene wrote and restored all nine v5 records, reacquired both
actors, restored the staged controller, resumed the Hug01/Hug02 sequence, and
stopped normally. Historical v5 compatibility remains mandatory. A
player-involved active save/load is non-mandatory when the base game refuses to
save while the real PlayerCharacter is under NAF control; its restoration code
remains intact and NAF does not bypass that restriction.

The matrix below was produced by comparing `port/1.11.221` directly with the
clean `master` tree for NAF 1.0.14. Status counts refer to matrix rows (feature
families), not a claim that every row has equal implementation size.

| Original NAF feature | AE status | Dependency | Current blocker | Restoration plan |
| --- | --- | --- | --- | --- |
| Exact F4SE plugin load and lifecycle | restored/runtime-validated | F4SE 0.7.8, exact 1.11.221 gate | none on tested runtime | retain fail-closed metadata and lifecycle audits |
| Address Library lookup and hook validation | restored/runtime-validated | supplied 1.11.221 database | none on reachable baseline | retain exact-ID lookup, selector audits, prologue/site guards |
| XML/cache/game-data linking | restored/runtime-validated | pugi, NAF XML schemas | broader pack coverage | regress all upstream XML object types |
| Conditions, tags, race and furniture compatibility selection | restored/content-test-required | linked forms and condition evaluation | not every condition/furniture combination tested | exercise representative upstream packs |
| Core scene create/start/advance/stop | restored/runtime-validated | graph, package, camera, ordered actions | none on tested solo/paired paths | preserve as regression baseline |
| Solo dynamic-HKX playback | restored/runtime-validated | SmartIdle/dynamic idle | none on tested files | regression only |
| Paired actor-specific HKX playback | restored/runtime-validated | per-slot files, synchronization | none on tested CHAK scenes | regression only |
| Walk-to-and-start scene lifecycle | statically restored | actor movement, scene start and cancellation | production runtime coverage absent | test upstream Papyrus workflow and abort cleanup |
| Animation groups and staged positions | restored/runtime-validated | group controller, timers, scene transitions | none on tested CHAK stages | add staged save/load coverage |
| Weighted/non-sequential animation groups | restored/content-test-required | group RNG/selection | available test content is sequential | test real weighted group content |
| Non-nested position trees | statically restored | tree controller, HUD directions, menu sound | no complete real tree content tested | exercise navigation, auto-advance, mutation and persistence |
| Nested/recursive position trees | statically restored | recursive control-system creation and v5 controller graph | suitable real nested-tree content unavailable | runtime-test recursive navigation/persistence; cycle guard is fail-closed |
| Position-tree branching, conditions and transitions | restored/content-test-required | tree definitions and HUD events | real content coverage incomplete | test forced completion, duration, branches and conditions |
| Scene speed and position mutation | restored/content-test-required | control-system replacement and graph timing | not all menu/Papyrus mutation paths exercised | test live mutation across animation/group/tree controllers |
| Game-loop task/update cadence | restored/runtime-validated | `NonRendersafeAI` hook | no formal single-frame correspondence; historical cadence preserved | retain serialization/concurrency regression |
| Timed tasks and frame-delay bookkeeping | restored/runtime-validated | `TASK` v5 state and timer thread | generic callback payloads are not serializable | preserve historical persistent subset |
| Built-in ordered equipment actions | restored/content-test-required | inventory/equipment APIs | content-dependent choreography coverage | test unequip/add/remove/re-equip and abort rollback |
| Built-in ordered morph actions | restored/content-test-required | LooksMenu 1.7.0.3 exact binary | real morph-bearing content not yet exercised | test apply/reset and Set3D refresh with real pack |
| Generic ordered C++ callbacks | statically restored | serialized NonRendersafeAI worker callback | no upstream producer beyond the public queue helper and no real-content coverage | retain upstream transient `std::function<bool()>` FIFO semantics; test when an original producer/workflow exercises it |
| Animation-object unequip event behavior | restored/content-test-required | equipment events and actor animation objects | manual `AnimObjUnequip` dispatch still lacks runtime coverage | test object attach/detach content and cleanup |
| LooksMenu detection/hash/interface initialization | restored/runtime-validated | exact f4ee.dll 1.7.0.3 | morph application content-dependent | retain exact DLL guard and add real morph regression |
| Graph update and Set3D integration | restored/runtime-validated | IDs 2214536/2201055 | none on tested path | regression baseline for NANIM/IK |
| SmartIdle graph time and synchronization | restored/runtime-validated | graph variables and dynamic idle | broader synchronization native coverage | exercise multi-actor resynchronization |
| NANIM parsing/loading/playback/stop | statically restored | NAF node graph, NiAVObject transforms, graph update/Set3D hooks | production runtime/content coverage pending | test bundled BasePose and an authored NANIM; validate stop/repeat/3D reload |
| Exported C++ NANIM handle API | statically restored | NANIM loader/player and handle store | production plugin-consumer runtime coverage pending | test load/copy/move/queue/free semantics |
| NANIM graph recording and pose snapshots | statically restored | NodeAnimationCreator, Creator/Studio, NANIM writer | authoring workflow runtime coverage pending | create, bake/export, replay, and persist a moving NANIM through the production tools |
| `BODY` v5 serialization/restoration | statically restored | stable actor/resource/time descriptors plus world-ready graph reconstruction | NPC NANIM save/load runtime coverage pending | test active NPC NANIM save/reload/stop with exact v5 schema |
| Body IK solver/runtime graph integration | restored/content-test-required | graph/Set3D, GraphInfo chains, pinned IK math | no suitable real IK-chain content tested | retain production initialization and test real content |
| Body IK Papyrus target/enable API | statically restored | solver and actor node graph | registered but behavioral coverage pending | test chain targeting, disable/reset and scene teardown |
| Full IK/editor controls | statically restored | Studio/Creator node selection and graph editing | authoring workflow runtime coverage pending | exercise chain selection, target parenting, visibility, reset, bake and cleanup |
| Ordinary continuous scene alignment | restored/runtime-validated | actor transforms, collision, player 3D | deliberate drift perturbation not separately tested | regress paired/player scenes and add natural drift case |
| Advanced/editor forced alignment | statically restored | Studio actor list and transform editor | authoring workflow runtime coverage pending | exercise multi-actor Studio entry, edit, close, and restoration |
| Local-transform and animation-sync Papyrus helpers | statically restored | corrected Get3D ABI, NiTransform, SmartIdle | registered but dedicated behavior coverage incomplete | validate through original scripts/workflows |
| Vanilla and scene face/lip pass-through | restored/runtime-validated | two verified face hooks | none on tested dialogue/scene paths | regression baseline |
| Managed-face runtime state/persistence | restored/content-test-required | face hooks and `FACE` v5 record | external control functions gated | validate timelines, eye state, loop/body sync and cleanup in Phase C |
| Managed-face Papyrus API (4 natives) | restored/content-test-required | runtime-validated face hooks plus managed state/timeline engine | all four natives registered; visible eye/timeline and FACE-resume coverage pending suitable content/API invocation | production-test eye override, play/stop, and NPC save/load |
| Package override/evaluation/process integration | restored/runtime-validated | package hooks and v5 `PACK` | none on tested scenes/save cycle | regression baseline |
| Player AI/input transition symmetry | restored/runtime-validated | input manager, packages, free camera | player-scene persistence coverage pending | staged/player active-save regression |
| Free-camera and camera restoration | restored/runtime-validated | `FreeCameraState::Update`, input mask | none on tested player scenes | persistence regression after active reload |
| HUD menu, scene HUD and custom HUD elements | restored/runtime-validated | Scaleform, HUD events, v5 HUD/SHUD | every drawing native not individually exercised | test via original script consumers |
| Hover/effect highlighting | restored/content-test-required | AE effect shader play/finish lifetime | visual behavior across every selecting menu not yet tested | IDs 2205201/4796160 audited; test hover/add/remove/remove-all in production menus |
| Inventory/equipment navigation menu | restored/content-test-required | item enumeration, equipment state | broader inventory combinations not exercised | retain UI entry and test live scene equipment editing |
| Main NAF menu and active/new-scene pages | restored/content-test-required | Scaleform/data/scene manager | restored speed/position/inventory actions need expanded runtime coverage | regress all Active Scenes mutations and navigation |
| Studio menu registration and workflow | statically restored | NANIM, transforms, IK, camera, graph, Creator, compiled Studio SWF | production runtime workflow coverage pending | exercise actor selection, camera, gizmos, timeline, IK, bake/package, and fail-safe exit |
| Creator project/body/face authoring | statically restored | Studio, NANIM writer, managed face, file I/O | production runtime workflow coverage pending | create/load/save/reopen a project and validate exported assets |
| Body Animation Creator | statically restored | Studio/NANIM recording and graph editing | production runtime workflow coverage pending | edit, retime, preview, bake/package and replay generated NANIM |
| Face Animation Creator | statically restored | managed-face timelines and preview | production runtime workflow coverage pending | author, preview, save/reopen, then exercise managed-face API with its output |
| Item Explorer | statically restored | armor-form enumeration, ordered equipment, highlight | production runtime workflow coverage pending | browse plugins/forms, equip/remove, highlight, and exit cleanly |
| MCM hotkey integration | statically restored | shipped MCM config and ToggleMenu native | not independently regression-tested after menu expansion | test binding, modifiers and opening every restored page |
| Papyrus production surface (45 registered natives) | restored/content-test-required | VM registration and underlying restored subsystems | registration parity complete; behavior coverage remains dependency-specific | retain all bindings and validate through owning subsystems |
| Remaining Papyrus surface (4 formerly gated natives) | restored/content-test-required | managed face | Set/ClearEyeCoordOverride and Play/StopFaceAnimation are production-enabled | validate managed-face behavior and FACE restoration |
| External scene events and Actor-array packing | restored/runtime-validated | corrected VM/CreateArray ABI | none for one/two-actor tested events | regress advanced scene state/events |
| Nine active historical v5 records | restored/runtime-validated | TASK/SCNE/EQPT/UID/FACE/HUD/PACK/SHUD/BODY | NANIM BODY motion coverage pending | maintain exact schema and add content-dependent regressions |
| Staged/group active-scene persistence | restored/runtime-validated | serialized control system/stage/timers | none on tested CHAK Hugs path | retain as v5 regression baseline |
| Player-involved active-scene persistence | statically restored | input, camera, package and collision fixup | base game may prohibit saving during real-player control | retain implementation; test only if a legitimate engine-supported save path exists |
| Advanced scene persistence | statically restored | nested trees, managed face and editor state | nested-tree/managed-face/authoring content coverage pending | validate each original state owner with its subsystem |
| Public C++ scene/NANIM API exports | statically restored | NANIM plus validated object lifetime | runtime consumer coverage absent | test every export with original plugin-consumer pattern |

The four C++ helpers `PositionExists`, `GetPositionType`,
`GetPositionBaseAnimation`, and `AnimationExists` exist in upstream `NAF.h`, but
upstream `Papyrus.h` never registers them and the shipped `NAF.psc` does not
declare them. They are therefore source-local dead/unreachable helpers in NAF
1.0.14, not six additional omitted public natives. This finding is documented
rather than silently deleting their implementations.

### Central temporary-gate register

Every currently intentional behavior gate has one removal dependency:

| Gate in current AE source | Upstream behavior suppressed | Removal dependency |
| --- | --- | --- |
| none | no upstream behavior is intentionally source-gated | runtime validation and concrete fault repair only |

The broad `kGameplaySubsystemsEnabled` flag is currently `true`; it no longer
suppresses a subsystem. The equipment and morph ordered-action flags are also
`true`. Their old warning strings describe the disabled branch only and are not
active gates. Upstream's commented recording/file-browser entries in
`MainMenuHandler` were already commented in the clean 1.0.14 baseline and are
not counted as AE regressions.

### Dependency-ordered completion sequence

1. **Phase A - NANIM/custom graph runtime.** Verify the complete NANIM data ->
   node animation -> custom graph -> per-update traversal -> blend/stop/reset
   pipeline. Audit every used Havok/graph/character/root-generator layout,
   ownership edge, lock, virtual and refcount against the 1.10.163/1.11.221
   binaries, AE PDB and modern references. Restore Papyrus and exported C++
   playback APIs. Then activate `BODY` v5 load/save using stable identifiers and
   world-ready reconstruction, never runtime pointers.
2. **Phase B - complete scene action/control surface.** Remove the generic
   callback gate after producer/lifetime analysis; finish every action category,
   animation-object equipment behavior, advanced transforms/alignment,
   synchronization, weighted groups, recursive trees, mutation, UI navigation,
   and their persistent state.
3. **Phase C - complete managed face and IK.** Runtime-test Body IK with real
   chains; restore every editor/runtime IK control; restore eye override and face
   timeline APIs, cleanup and persistence; register all four face natives.
4. **Phase D - Studio and authoring.** Register the production Studio menu and
   restore Studio, Creator, Body Creator, Face Creator and Item Explorer in full,
   including selection/highlighting, transforms, IK, NANIM preview/recording,
   project load/save, pose snapshots and export.
5. **Phase E - public API/UI parity.** Restore every hidden main-menu entry,
   validate every one of the 45 upstream Papyrus bindings and every exported C++
   function, regress MCM/hotkeys/HUD pages, and remove the central gate register.

Each phase receives a reachable-path relocation/ABI audit with exact supplied
AE IDs, three-runtime namespace classification, eager-global checks, physical
virtual-order and structure-layout checks, PDB/binary corroboration, and
fail-closed hook validation. Runtime coverage remains open when suitable real
content is unavailable; static restoration is never relabeled as unnecessary.

### Remaining serialization and runtime coverage

- Ordinary two-actor active-scene persistence is runtime-validated.
- Staged/group NPC persistence is runtime-validated with CHAK Hugs: all nine v5
  records restored, the staged controller resumed, and teardown completed.
- Player-involved active save/load is not mandatory when Fallout 4 refuses to
  save while the real PlayerCharacter is AI-controlled and in free camera. The
  restoration implementation remains intact, but NAF will not bypass the engine
  restriction or substitute a doppelganger merely to force coverage. Player
  scene start/stop recovery is already runtime-validated. If a legitimate
  base-game-supported save path appears, it may be tested later.
- `BODY` now actively reads/writes the exact historical v5 descriptor and
  reconstructs transient NANIM state after world readiness.
- Nested-tree state is now active through the recursively serialized controller
  graph. Managed-face and authoring owners remain gated; NANIM/BODY remains
  enabled while visible motion coverage awaits suitable content. The v5 schema
  is not expanded or reordered.

The parity ledger is updated by each restoration phase; artifact hashes are
recorded in the corresponding implementation section below.

## NANIM runtime and active BODY restoration

### Persistence test correction and promoted baseline

Ordinary NPC, paired NPC, and staged/group NPC active-scene save/load are
runtime-validated. CHAK Hugs restored all nine v5 records, reacquired both
actors, resumed its staged controller and Hug01/Hug02 sequence, and stopped
cleanly.
Player-involved save/load is not a mandatory test if Fallout 4 disallows saving
while NAF controls the real PlayerCharacter. The implementation remains present
and fail-safe, but the port will not alter player-scene semantics or bypass the
engine restriction. Player start/stop AI, input, package, collision, free-camera
and camera restoration are already runtime-validated.

### Actual upstream NANIM architecture

Source tracing corrected an earlier imprecise description: NANIM does **not**
inject, clone, replace, or own a Havok generator. Its runtime pipeline is:

`PlayNANIM/API/scene SmartIdle` -> `NANIM::LoadFromFile` -> select an animation
ID -> convert timelines to `NodeAnimation` -> obtain `GraphInfo` node names and
IK chains -> create/reuse NAF's `NodeAnimationGraph` -> blend from the engine's
just-produced pose -> write generated `NodeTransform` values to each
`NiAVObject::local` -> apply Body IK -> continue on the verified post-engine
graph-update hook. `StopNANIM` blends the NAF output back to the current engine
pose, releases animation data at transition completion, and deletes a temporary
graph once it has neither animation nor active IK.

The file parser and animation generator are local, engine-independent code.
NANIM retains upstream paths and format semantics: Papyrus paths are relative to
`Data/NAF`, scenes use the same SmartIdle `kNAF` assignment, and the exported C
API retains load-handle, copy/move play, asynchronous queue, stop and free-handle
behavior. Missing files, unknown animation IDs, missing GraphInfo, actors or 3D
fail without installing a graph animation.

The graph update and Set3D hooks already validated in production provide the
thread/lifetime contract. `NodeAnimationGraph::updateLock` serializes generator,
transition, recorder and IK state. `GraphHook::stateLock` protects graph
ownership; `loadingAnimsLock` protects asynchronous load cancellation. Set3D
clears retained node smart pointers and reacquires the current skeleton nodes;
unloaded 3D sets `kUnloaded3D` and prevents stale node writes.

### Havok/graph ABI boundary

NANIM pose generation itself touches `NiAVObject` nodes, not custom Havok object
layouts. The adjacent Havok surface belongs to ordinary graph-time readiness and
multi-actor synchronization, which NANIM-aware SmartIdle also uses:

| Interface/layout | AE evidence/status |
| --- | --- |
| `hkbContext` constructor/destructor | IDs `2258328`/`2258329`, RVAs `0x13A9B10`/`0x13A9B90`, disassembly/PDB verified; size guard `0x68` |
| `setActiveGeneratorLocalTime` | ID `2260451`, RVA `0x147C170`, disassembly/PDB verified |
| `getNodeClone` | ID `2260454`, RVA `0x147C5A0`, disassembly/PDB verified |
| `hkbGeneratorSyncInfo` | size `0xA4`; duration/local-time fields exercised by successful synchronized HKX scenes |
| `hkbCharacter::behaviorGraph` | offset `0x80`; manager/graph chain exercised by graph-time synchronization |
| `hkbBehaviorGraph::rootGenerator` | offset `0xC0`; root clone/time path exercised by scene synchronization |

The duplicate `BShkbAnimationGraph::getNodeClone` wrapper now uses the verified
two-runtime mapping instead of stale invariant ID `326555`. Historical
`BShkbUtils::GraphTraverser` helpers remain unreachable from playback; they are
Studio/Creator recording-analysis dependencies and are not falsely promoted by
this pass.

### Active historical BODY record

The exact v5 schema remains the upstream cereal vector of:

- `SerializableRefHandle` actor/reference identity;
- NANIM `filePath` and animation `id` strings;
- generator `localTime` float.

No graph manager, node, 3D, Havok, lock, thread, animation object, or other raw
runtime pointer is serialized. Raw `LoadCallback` places descriptors in
`pendingRestores`. After `Main::gameActive`, BODY fixup resolves the reference,
requires loaded 3D and valid GraphInfo/NANIM data, creates the normal production
graph, restores local time, then allows SCNE fixup to re-request its SmartIdle.
A one-shot descriptor suppresses that duplicate asynchronous scene load. Failed
descriptors remain stable and serializable rather than leaving partially
installed graph state. Revert cancels loads and clears graphs, 3D registrations,
pending descriptors and suppression state.

NANIM is supported both independently through Papyrus/C exports and through
scene `.nanim` assignments. SCNE owns scene/control state; BODY owns the
reference's custom node-animation resource/time. Restoration order is BODY then
SCNE, avoiding competing ownership while preserving the historical two-record
model. Multi-actor synchronization continues through SmartIdle graph time; Body
IK runs after generated NANIM transforms, preserving upstream ordering.

`PlayNANIM` and `StopNANIM` are registered again. Papyrus coverage is now 41/45;
the remaining four are managed-face functions reserved for Phase C. The bundled
`Data/NAF/Animations/Human/BasePose.nanim` is a real format/load asset, but an
authored moving NANIM or Studio output is preferable for visible playback and
stop testing.

`tools/audit_nanim_runtime.py` verifies seven exact AE records, graph/Set3D and
Havok layout guards, transform-output/IK ordering, both Papyrus registrations,
active BODY save/load/fixup/reset, and absence of the four stale invariant IDs.
All established subsystem audits remain required.

Runtime test sequence:

1. regress one ordinary HKX scene;
2. invoke `PlayNANIM` on an NPC with a real NANIM file/animation ID;
3. verify visible playback, call `StopNANIM`, and confirm ordinary animation;
4. repeat play/stop, then exercise scene-side NANIM if matching position content
   is available;
5. save with NPC NANIM active, reload, require `BODY persistence fixup: 1 ...
   restored`, then stop and verify graph/AI health;
6. separately save `(CHAK) *Staged* Hugs` during stage 2 and verify group state;
7. do not force or bypass saving during PlayerCharacter control.

Production runtime execution and BODY restoration remain runtime-test-required;
no suitable moving NANIM sample was found locally beyond BasePose.

Release x64/v143 builds and links with zero errors and `COPY_BUILD=OFF`.
Development DLL SHA-256:
`64D9C7E9707E0245816539CF1B928926E62E8A3BFAF23B126FD2D8E3B39D9263`.

## Full runtime action, transform, and recursive-tree restoration

### Upstream-to-AE inventory and callback semantics

A direct diff against clean NAF 1.0.14 found three Phase-B behavior gates, not
a second hidden action framework: arbitrary `OrderedActionQueue` callbacks,
hover highlighting, and the Active Scenes speed/position/inventory bindings.
The remaining scene transform, synchronization, alignment, group transition,
IK helper, equipment, morph, and position-mutation implementations were already
the upstream production implementations with narrowly required AE API/math
adaptations.

The complete upstream ordered FIFO is:

| Action | Representation/ownership | Execution and completion | Persistence/cancellation |
| --- | --- | --- | --- |
| frame delay | value-captured lambda | adds callback ticks to `delayCounter`; later updates decrement one tick at a time | transient; cleared on revert/reset |
| custom callback | copied `std::function<bool()>` | runs inline on the serialized `NonRendersafeAI` worker callback; upstream ignores the bool and pops after one invocation | transient and intentionally not serialized; callable destruction occurs when popped/reset |
| unequip slot | `NiPointer<Actor>` plus slot | snapshots the equipped base form and unequips once | equipment snapshot persists in `EQPT`; queued callable does not |
| re-equip slot/all | `NiPointer<Actor>` plus optional slot | restores saved equipment once and prunes empty actor state | `EQPT` persists the rollback state |
| add equipment | `NiPointer<Actor>` plus form | adds if missing, records ownership, equips, clears player HUD messages | added-form ownership persists in `EQPT` |
| remove equipment | `NiPointer<Actor>` plus form | unequips and removes only NAF-added inventory | state pruned symmetrically |
| apply morph set | `NiPointer<Actor>` plus copied `Morphs` | invokes the restored LooksMenu path once | morph definitions are stable data; callable is transient |

`safe_mutex` is a recursive mutex. A callback can enqueue another action without
deadlocking; because `Update()` preserves the upstream loop, a newly appended
action may execute in the same callback tick unless a delay is inserted. Scene
stop does not invent callback serialization. F4SE revert calls `Reset()`, which
destroys all transient callables, resets the delay, and clears equipment state;
save/load persists only the historical `EQPT` state. Upstream has no other
`InsertCustom` producer, so restoring the queue helper is architectural parity,
not evidence that an arbitrary unknown callback has been runtime-exercised.

The execution context remains exactly historical: after verified engine
`ProcessQueues`, the serialized worker job runs scene updates, then the ordered
FIFO, then forced alignment. UI/Papyrus work must still use the task/event paths
owned by those subsystems; unrestricted callbacks are not reclassified as
universally thread-safe.

### Transform, synchronization, alignment, and menu parity

Runtime transforms retain upstream behavior: scene origin plus per-actor local
offset and yaw, repeated world position/angle correction, collision suppression
after movement, player 3D refresh, scale correction, and the shared
`ForceAlignActor` queue used later by Studio. The corrected AE math preserves
parent-space composition (`parent.rotate * child.rotate`, rotated/scaled child
translation) and world-to-local inversion (transpose parent rotation, subtract
translation, divide parent scale). Quaternion conversion/Slerp/intermediate
helpers remain the previously audited compatibility implementation.

Synchronization is complete for the upstream surface: scene setup waits for
graph transition and idle loading, starts every actor assignment, reads graph
time, applies the historical 10 ms player offset, moves all actors to the
minimum valid time, detects loops, drives sequential or weighted group
transitions, and exposes `SynchronizeAnimations` for an arbitrary validated
actor array. NANIM continues through the same SmartIdle graph-time abstraction.
No separate mixed-playback synchronizer exists upstream.

The Active Scenes menu again exposes upstream speed mutation (1-500%), live
position replacement, and actor inventory navigation. `SetPosition` retains
the existing symmetrical controller transition: queue the new controller,
invoke the old controller's `OnEnd`, apply old/new morph/equipment lifecycle,
and begin the replacement after its predecessor completes. The already bound
Papyrus transform, speed, position, IK-target and synchronization functions now
have their full upstream non-Studio dependencies. Papyrus remains 41/45 only
because the four managed-face bindings belong to Phase C.

### Recursive position trees and persistence

Upstream tree XML already permits a branch node to name any `Position`, while
the original controller passed `excludeTrees=true` when creating node
subsystems. The bounded AE port added a stronger start-time rejection. Phase B
removes both restrictions and permits a tree-position leaf to construct another
`PositionTreeControlSystem`.

Nested controllers retain their immediate parent, delegate actor/scene work
through `IControllable`, and report a completed nested leaf to the containing
tree rather than ending the outer scene. Serialization remains the historical
cereal controller graph: no new v5 field or record was added. World-ready
fixup recursively rebinds each active child controller to its parent before
playback resumes, preserving each tree's current node, queued subsystem,
selection, timer-completion flags and auto-advance state.

Maintenance hardening is deliberately narrow. Position validation and base-
animation discovery track active tree IDs and fail closed on a self-reference
or cycle; runtime `OnBegin` repeats the ancestor check before invoking a child.
Missing positions/trees also fail scene construction. Valid acyclic nesting has
no arbitrary depth limit; traversal is iterative within each XML tree and
recursive only across explicit tree references. Real nested-tree content was
not found locally, so runtime navigation and persistence remain content-test-
required rather than being mislabeled runtime-validated.

Sequential groups retain ordered stage/loop behavior. Non-sequential groups
retain upstream weighted random selection. Tree manual next/previous/left/right,
auto-advance timers, force-complete behavior, duration inheritance, HUD state,
tree-to-group/direct/tree transitions, live scene position replacement, speed
changes, and serialized controller state are all enabled. CHAK staged/group NPC
persistence is runtime-validated; weighted and nested tree behavior remains
content-test-required.

### Highlighting recovery and reachable relocation audit

Enabling the maintained highlighting replacement exposed two stale selectors
that could not be left to runtime popup discovery:

| Interface | Stale declaration | Verified supplied AE mapping | Evidence |
| --- | --- | --- | --- |
| `TESObjectREFR::ApplyEffectShader` | invariant OG ID `652173` | ID `2205201`, RVA `0x5E1E80` | Dear/Addictol convergence; exact supplied record; disassembly consumes reference/shader/time/facing/stack arguments and constructs/starts `ShaderReferenceEffect` |
| `ProcessLists` singleton | two-runtime second ID `2688869` (1.10.984) | ID `4796160`, RVA `0x30DC140` | Dear three-runtime declaration; exact supplied record; maintained singleton storage type |

Play uses the verified `ApplyEffectShader` entry. Stop uses maintained
`ProcessLists::ForEachShaderEffect`, which locks the magic-effect array, selects
the matching target handle and shader data, and marks the effect finished—the
previously disassembly-verified semantics of the historical finish wrapper.
`GetFullyLoaded3D` remains physical slot `0x8D`, directly neighboring the
corrected `Get3D` slots. Highlighting is production-enabled but visual coverage
for every hover owner remains runtime/content-test-required.

`tools/audit_runtime_action_surface.py` verifies both exact AE records, the
`ApplyEffectShader` entry bytes, CommonLib selectors, callback/reset ownership,
recursive tree construction/cycle/fixup guards, restored Active Scenes entries,
and highlighting composition. Existing group/tree, menu, scene-feature,
scene-relocation, serialization, NANIM, graph, face, package, camera, HUD and
Papyrus audits remain required. No additional stale ID, eager relocation, or
virtual/layout defect was found on the newly reachable Phase-B paths.

### Remaining central gates after Phase B

The generic callback, nested-tree, and highlighting gates are eliminated. The
central register now contains only:

- four managed-face Papyrus bindings, pending Phase C;
- Studio menu registration, pending the authoring phase;
- Creator/Body Creator/Face Creator main-menu entry, pending authoring;
- Item Explorer main-menu entry, pending authoring (its highlighting dependency
  is now restored).

NANIM playback, C API, scene integration and BODY restoration remain enabled;
visible motion/BODY runtime coverage awaits a suitable moving NANIM resource or
Studio/HKX conversion output and does not block subsequent parity work.

The 57-row parity ledger now totals: 22 restored/runtime-validated, 12
restored/content-test-required, 13 statically restored, 2 unresolved, and 8 not
yet ported. Thus 47/57 feature families have production or static
implementations; the remaining ten rows belong to managed-face/authoring or
their still-unrestored workflows. Four central source/UI gates remain.

Phase-B production regression plan:

1. run solo and paired HKX scenes and stop normally;
2. run staged CHAK Hugs, mutate speed and position through Active Scenes, open
   actor inventory, then stop;
3. save/reload staged NPC CHAK and require the established nine-record resume;
4. exercise tree content if available, including nested content when found;
5. hover actor/reference rows in restored menus and confirm shader application
   and removal;
6. verify cleanup, package/AI/collision/camera/input restoration and both end
   events. No artificial NANIM asset is required.

The recommended next phase is the complete managed-face API and runtime state,
followed by full Studio/Creator/Item Explorer and remaining authoring/UI parity.

All 18 subsystem audits pass, including the new 2-record/18-guard Phase-B
audit. A clean Release x64/v143 rebuild links successfully with `COPY_BUILD=OFF`.
Development DLL SHA-256:
`3DF82F0552AD82BA84A4D16194ACDDD655C785928E04A85D0240E37F4D097082`.

## Active Scenes inventory stale-ID closure

The first production exercise of the restored Actor Inventories entry failed
closed on Address Library ID `1105206`. The exact owner is NAF's local
`RE::OpenContainerMenu` wrapper in `src/RE/ContainerMenu.h`, reached by:

`Active Scenes -> Actor Inventories -> actor selection -> ShowActorInventory -> OpenContainerMenu`.

ID `1105206` is an OG 1.10.163 record (RVA `0x1264F40`) that had been declared
runtime-invariant. It is not a 1.11.221 record. The mapped historical function
constructs `OpenContainerMenuEventData`, consumes `(TESObjectREFR*, int, bool)`,
normalizes the menu mode when required, lazily obtains its event source, and
dispatches the event. Focused comparison located the equivalent 1.11.221 entry
at supplied-database ID `2248608`, RVA `0x103E1B0`. The AE implementation has
the same three-argument ABI, form/container validation, menu-mode normalization,
lazy event-source initialization and dispatch behavior. The wrapper now uses
`REL::RelocationID(1105206, 2248608)`; no signature, calling-convention, layout,
or ownership adaptation was required.

NAF does not enumerate or mutate individual inventory rows after this call.
The vanilla `ContainerMenu` owns item enumeration, names/metadata, selection,
equip/unequip actions and list refresh. NAF temporarily adds the established
`ShowWornItems` keyword when necessary, closes NAFMenu, dispatches the verified
open event, observes `MenuOpenCloseEvent`, removes its temporary keyword on
close, and reopens the prior Active Scenes submenu through the F4SE task/UI
queue. Bounded development logs now identify actor resolution, handoff to the
engine-owned inventory UI, event-dispatch return, menu open/close, temporary
keyword cleanup, and fail-closed handle expiry. They deliberately do not claim
to instrument engine-owned item selection internals.

`tools/audit_inventory_controls.py` provides a dedicated reachable-path audit.
It validates 12 exact supplied AE records spanning the container dispatcher,
UI/menu queue, equipment manager, inventory helpers and highlighting; validates
the known `OpenContainerMenu` entry bytes at the exact RVA; rejects the stale
invariant selector; and guards the Active Scenes bindings, keyword/menu-mode
composition, event handoff, cleanup logging and highlighting path. The existing
menu audit now also requires this mapping. A bounded sweep of speed mutation,
position replacement, group/tree navigation, inventory navigation, scene
equipment helpers and highlighting found no additional stale selector, eager
global, virtual-order defect or ABI mismatch on the presently reachable path.

All 19 subsystem audits pass (the established 18 plus the inventory audit).
A clean Release x64/v143 rebuild links with zero errors and `COPY_BUILD=OFF`.
Development DLL SHA-256:
`71EE134316098F51BA427B4E43AF52B089E8170F7FFDA2F5F546F8977CAF66C7`.

The Phase-B implementation gap is closed statically. Production runtime
validation should repeat Actor Inventories on a live scene, select/equip or
unequip an item through the vanilla menu, close it, and confirm NAFMenu returns
to the same scene with correct equipment state. No further Phase-B code gate is
known; successful runtime confirmation permits transition to Phase C, the full
managed-face API and runtime state.

## Phase C — complete managed-face API restoration

### Phase-B runtime closure

The production retest exercised the restored Actor Inventories entry end to
end: actor selection and resolution, verified ContainerMenu dispatch, engine-
owned enumeration/selection/actions, close notification, NAF submenu restore,
continued playback, and normal scene teardown all succeeded. Highlighting was
also visibly correct. These covered Phase-B paths are runtime-validated and are
no longer tracked as an open subsystem.

### Architecture and ownership

The upstream 1.0.14 architecture is preserved:

```text
Papyrus or scene animation selection
  -> stable ActorHandle-keyed FaceData
     -> optional eye override (user coordinates converted to UV offsets)
     -> optional asynchronously loaded face timeline
  -> BSFaceGenAnimationData* reverse index for the current actor 3D
  -> engine BSFaceGenAnimationData::Update
     -> NAF timeline post-update under state + engine instance locks
     -> expression[54], eye UV, and optional emissive timeline writes
  -> managed lip hook suppresses vanilla lip overwrite
  -> stop/end/revert removes the appropriate managed state
```

`PersistentState::managedAnims` is keyed by `SerializableActorHandle`, not raw
Actor or face-data pointers. `managedDatas` is a transient reverse index from
the current `BSFaceGenAnimationData*`; it is never serialized. Eye geometry is
held by `NiPointer<BSGeometry>` only as a per-3D cache. The verified production
Set3D hook now notifies managed face after the engine call: cached eye geometry
and the old reverse-index entry are discarded and the current face data is
reacquired. The same rebuilding occurs during world-ready FACE fixup. A missing
actor or temporarily missing face data leaves stable handle state intact but
does not create a null reverse-index key.

### Public semantics and face format

All four upstream signatures match the shipped `NAF.psc` exactly and are
registered:

- `SetEyeCoordOverride(Actor, Float x, Float y)` stores an absolute user-space
  coordinate where each axis is documented as `[-1,1]`; upstream conversion is
  `u = x * -0.25`, `v = y * 0.2`. It persists and overrides eye timeline keys.
- `ClearEyeCoordOverride(Actor)` removes only the eye override. Repeated or
  missing-state calls are harmless; a separately playing face animation stays
  managed.
- `PlayFaceAnimation(Actor, String id, Bool loop, Bool bodySync)` resolves an
  XML-defined face animation, loads its binary cache asynchronously, replaces
  ordinary managed playback, and returns false only for a null actor or unknown
  ID. Body sync reads the actor graph time; ordinary playback advances by the
  engine face-update delta.
- `StopFaceAnimation(Actor)` cancels an outstanding load and removes managed
  playback while deliberately retaining an independent eye override. Once no
  managed property remains, vanilla lip/expression behavior resumes.

The source format is the upstream XML `<faceAnim id="..." frames="...">` with
`<key frame="...">` children containing `<morph id="..." value="..."/>` and/or
`<eyes x="..." y="..."/>`. Frames use the upstream 30 fps runtime conversion;
key interpolation uses the stored easing function. XML parsing builds the
unchanged cereal binary `AnimationData` cache (normalized timeline keys plus
duration). No separate file extension or NANIM format is involved. No usable
moving face-animation sample was found in the local content tree, so visible
timeline playback remains content-test-required rather than gated.

### Threading, lifecycle, and v5 FACE persistence

Papyrus/native writes take the managed-state writer lock. Engine face updates
take its reader lock, then the verified `instanceData.lock` at offset `0x2B4`;
individual timeline mutation uses the animation mutex. Async resource loading
uses `loadingAnimsLock` before publishing under `stateLock`, matching stop and
serialization lock order. The hooks continue running in their original engine
face-update contexts; no work was moved onto the serialized AI worker callback.

The historical v5 `FACE` payload is unchanged. It stores the handle-keyed map;
for each actor it writes `animationId`, optional `EyeVector`, `hasAnim`, and—if
active—`timeElapsed`, `loop`, and `havokSync`, in that order. Runtime reverse
indexes, face/geometry pointers, graph-time cache, locks, async loads, and
Creator preview backup are transient. Load reconstructs timeline data from the
stable ID, then the existing deferred world-ready fixup resolves actors and
face data. Revert atomically clears persistent state, pending loads, reverse
indexes, and geometry cache. Scene-owned playback retains upstream teardown
calls; independent public eye state is not incorrectly cleared by
`StopFaceAnimation`.

The AE face layout remains the previously verified size `0x2E8`, with expression
arrays at `0x18/0xF0/0x1C8`, instance data at `0x2A0`, and spin lock at `0x2B4`.
Managed APIs touch no additional unknown engine fields. Papyrus packing is
bounded to the already-audited Actor object, string, float and bool paths; no new
array, structure-wrapper, or callback-functor ABI is activated.

### Relocation and parity audit

No new stale ID or ABI defect was found. Managed execution reaches only the
already verified exact AE records: `2209487` (`0x6E8BF0`), `2209139`
(`0x6D1310`), and `2209124` (`0x6D0560`). `tools/audit_managed_face.py` verifies
those records plus the physical layout guards, validated hook selectors,
handle ownership, Set3D rebinding, eye conversion, parser/cache path, FACE v5
activation/fixup/reset, exact script signatures, and all four registrations.
The Papyrus audit now requires 45/45 bindings, and the full-parity audit treats
managed face as restored rather than gated.

Phase-C production test plan:

1. regress an ordinary scene and the known-good face/lip pass-through;
2. invoke eye override on a loaded NPC, verify hold, clear it, and verify vanilla
   eye behavior returns;
3. if a real face-animation definition is available, play, replace/loop, stop,
   and verify vanilla facial behavior resumes;
4. save with NPC managed face state active, reload, require FACE world-ready
   rebinding/resume, then stop/clear normally;
5. force a legitimate actor 3D rebuild where practical and verify the managed
   state rebinds without retaining the old face-data or eye-geometry pointer.

At Phase-C closure, the parity ledger totaled 22 restored/runtime-validated, 14
restored/content-test-required, 13 statically restored, 0 unresolved, and 8 not
yet ported (49/57 with production or static implementations). The central gates at that point were
the upstream authoring/UI workflows: Studio; Creator including Body Creator and
Face Creator; and Item Explorer. Phase D should begin by mapping their shared
menu/state dependencies, then restore the main-menu entries and Studio/Creator/
Item Explorer workflows against the already restored camera, transforms, IK,
NANIM, managed face, highlighting, graph, scene, and persistence foundations.

Release x64/v143 builds and links with zero errors and `COPY_BUILD=OFF`. Phase-C
development DLL SHA-256:
`6BB09D7A225F746AD7AEB53C739FDC5C72D14F5B5EA7F5731DDCB63DB3887AB9`.

## Phase D - Studio, Creator and authoring UI restoration

### Dependency and gate audit

Direct comparison with clean upstream NAF 1.0.14 established that the authoring
implementations had not been reduced during the AE port. Face Creator and Item
Explorer are source-identical; Creator, Body Creator and Studio differ only at
the already-audited AE API adaptation points. The effective development gates
were narrower than the older ledger suggested:

```text
NAF Menu
  -> Creator (main-menu binding was removed)
     -> project state (CreatorData singleton)
        -> Face Animation Creator (NAFMenu state)
        -> Body Animation Creator (NAFMenu state)
           -> NAFStudioMenu (menu registration was omitted)
              -> NodeAnimationGraph / NodeAnimationCreator
              -> graph hook + NANIM + IK + transforms + package control
              -> free-camera hook + external camera input/orbit
              -> bake/package NANIM output
  -> Item Explorer (main-menu binding was removed)
     -> TESDataHandler armor forms
     -> ordered equipment actions + highlighting
```

The Creator and Item Explorer bindings are restored, and the game-data-ready
menu path now registers `NAFStudioMenu` together with `NAFMenu`. No constructor,
Scaleform handler, Creator callback, preview controller, or project data source
was otherwise gated. All 45 Papyrus registrations remain enabled; the authoring
pages communicate primarily through C++/Scaleform state handlers rather than a
second Papyrus registration layer.

The compiled authoring asset remains the upstream `Interface/NAFStudioMenu.swf`.
Its ActionScript/XFL source is present under `as3`, but the repository does not
contain the Adobe-Animate-produced SWF. Runtime testing therefore requires the
unchanged upstream NAF interface asset to be installed alongside the new DLL;
the port does not synthesize or substitute a reduced SWF.

### Creator, file model and authoring output

Creator preserves the upstream workflow and file formats:

- `NAF Menu -> Creator -> New Project/Load Project/Save Project` owns the
  singleton project workspace;
- project metadata and face timelines are written to
  `Data/NAF/<name>_project.xml`;
- body animations are written to the referenced
  `Data/NAF/<name>_project.nanim`;
- Face Creator edits 30-fps morph/eye/emissive timelines, previews through the
  production managed-face state, optionally synchronizes to graph time, and is
  saved through the project XML;
- Body Creator opens the production Studio, edits NodeAnimationCreator keys,
  chain state/targets/interpolation, and can bake per-actor NANIM files or one
  packaged multi-character NANIM;
- pose snapshots remain `Data/NAF/pose_snapshot.nanim`.

Filesystem behavior received bounded maintenance hardening without a schema or
path change. `Data/NAF` is created when necessary, project enumeration uses the
non-throwing `error_code` iterator path and reports access failures, and project
save now honors pugixml's boolean failure result instead of falsely reporting a
successful XML write. Body NANIM is still written before its XML reference, as
upstream. No raw pointer or runtime graph object is persisted.

`Data/NAF/Animations/Human/BasePose.nanim` is authoring infrastructure, not a
visible moving test animation. `NAF_graphInfo.xml` associates it with
`RaiderProject`; Creator resolves the actor graph project, loads animation
`pose`, and uses the graph's node list to seed a new body animation. If the file
is unavailable it falls back to the GraphInfo XML skeleton pose and finally to
an empty pose. Its role is base-pose initialization/reset for authoring and
preview.

### Studio, Body Creator, transforms and cleanup

Studio uses the already-restored production graph, NANIM, IK, transform,
package, collision and camera implementations. It supports upstream actor and
target selection, node markers, translation/rotation gizmos, timeline keys,
copy/delete/undo/redo, playback, node visibility, IK enable/reset/target
parenting, interpolation, multi-actor time synchronization, save, bake and
package export. Opening Studio pushes its three input contexts, HUD special
mode, player-input block, free camera and external camera input. Closing saves
the current creator graphs, destroys markers, stops forced alignment, clears
package overrides, restores scale/collision, disables IK, transitions graphs
back, destroys Creator instances, pops input/HUD state, disables fly camera and
clears orbit state.

Two dormant authoring bounds defects were corrected: managed-actor and gizmo
marker indices now require `index < size`, not `index <= size`. These are
fail-safe maintenance corrections for invalid UI selections and do not change
valid upstream behavior.

The old Studio screen-width/height relocations `1302430` and `644641` are OG
1.10.163-only globals and are absent from the supplied AE database. They have
not been remapped speculatively. Studio now uses maintained
`RE::GetINISetting("iSize W:Display")` and `RE::GetINISetting("iSize H:Display")`;
if a setting is unavailable it logs and uses the authored 1920x1080 stage
fallback. This removes both stale IDs from the reachable authoring path.

### Graph traversal finding

The legacy declarations named `BShkbUtils::GraphTraverser`,
`BShkbVisitor::TraverseAnimationGraphNodes`, and
`BShkbAnimationGraph::VisitGraph` were audited rather than presumed to be a
Studio dependency. Neither upstream 1.0.14 nor the AE authoring source calls
them. Studio traversal is through NAF's `NodeAnimationGraph`, its explicit node
map, and `GraphHook::VisitGraph`. The unused declarations remain fail-closed and
cannot resolve merely by loading the plugin.

For future reference only, old/new binary/PDB comparison gives strong entry
candidates for the unused helper constructor/next pair at AE IDs `2258080`
(RVA `0x139AC00`) and `2258082` (RVA `0x139AD20`), and identifies
`BShkbVisitor::TraverseAnimationGraphNodes` at ID `2257415`, RVA `0x1367110`.
They were deliberately not incorporated because no live upstream workflow
requires them and the remaining legacy destructor/VisitGraph set is not fully
verified. This is not a reduced Studio path: it records that GraphTraverser is
dead source in the shipped upstream workflow.

### Item Explorer and LookAt Cam

Item Explorer is its own upstream armor browser, not an alias for ContainerMenu.
It groups playable, player-race-compatible armor by owning plugin, presents
selection state, and adds/removes selections through the production ordered
equipment queue. Fileless/dynamic armor forms are now skipped safely and
unnamed armor receives a display fallback; valid forms retain upstream behavior.

Source comparison confirms `LookAt Cam` is already behaviorally faithful. Scene
entry passes the player's actor root to `CamHook::LookAt`. The configurable
target is a child skeleton node, default `HEAD` (alternatives `Chest` and `COM`).
Each free-camera update first calls the engine original, then rotates from the
camera root's current local translation toward the target node's **world**
translation. It does not translate/recenter the camera onto the head and has no
framing offset. Consequently a view that remains off-center while continuously
oriented toward the head is expected upstream behavior. Normal free-camera
translation remains available; look-at overwrites rotation, while Q/E apply the
upstream vertical translation adjustment. Studio's separate orbit mode targets
the selected gizmo/node and is not the ordinary-scene LookAt path.

The target root/node are intrusive `NiPointer`s and are cleared when LookAt is
disabled. Upstream reacquires the named node when LookAt starts or its setting
changes; it does not implement a distinct Set3D observer for an already-running
LookAt session. The AE port preserves that behavior rather than silently
redesigning target semantics.

### Audits, parity and runtime plan

Eight Phase-D audits were added and integrated with the central source ledger:
Studio, Creator, Face Creator, Body Creator, Item Explorer, authoring file I/O,
LookAt semantics, and the legacy GraphTraverser reachability check. Together
they verify 71 Phase-D source/lifecycle guards plus the traversal reachability
invariant. All established relocation and
ABI audits also pass: 63 HUD records, 40 scene records, 25 menu records, 16
graph records, 15 Papyrus/serialization records, 12 package records, 12
inventory records, 11 playback records, 11 scene-event records, 8 camera
records, 7 NANIM records, 5 feature records, and the face/IK/alignment/layout
guards. No new virtual-order, structure-layout or eager-relocation defect was
found.

The 57-row parity ledger now contains 22 restored/runtime-validated, 14
restored/content-test-required, and 21 statically restored families: 57/57 have
production or static implementations, with zero `unresolved` and zero `not yet
ported` rows. The central source gate list is empty. This is not a claim of
Phase-D runtime validation: every authoring workflow remains production-test-
required until exercised with the installed upstream SWF.

Recommended first Phase-D production workflow:

1. regress NAF menu and enter Item Explorer; browse a plugin, select/remove an
   armor item, verify highlight/equipment state, then exit;
2. enter Creator, create and name a project, create a face animation, open Face
   Animator, add morph/eye keys, preview, save, close and reopen the project;
3. play the Creator-generated face animation through the normal managed-face
   API, test replacement/stop and eye clear, and verify vanilla face recovery;
4. create a body animation, select a loaded actor, enter Studio, exercise camera
   and LookAt, node selection, translate/rotate, IK visibility/target/reset,
   timeline playback, save and bake/package;
5. replay the generated moving NANIM through normal runtime and scene paths,
   then exercise BODY active persistence;
6. exit every page and verify camera, input, package, collision, graph, IK,
   highlight and temporary authoring state are restored.

No deployment or live-game modification was performed by this build stage.
Release x64/v143 builds and links with zero errors and `COPY_BUILD=OFF`.
Phase-D development DLL SHA-256:
`AAD3EDFB5140458C1691EA2989D27403D2758FD7E958E581F391225B588DC5B9`.

### Phase-D production follow-up: Item Explorer and LookAt Cam

The first production authoring workflow reached Item Explorer successfully but
failed closed only after selecting an armor form, on missing exact ID `1095748`.
The owner is selected CommonLibF4's `RE::BGSObjectInstance` constructor in
`TESObjectREFRs.h`, not form enumeration or menu construction. The precise path
is:

```text
NAF Menu -> Item Explorer
  -> TESDataHandler armor enumeration
  -> plugin/form list and selection
  -> OrderedActionQueue::AddEquipment/RemoveEquipment
  -> DoEquip/DoUnequip
  -> BGSObjectInstance(TESForm*, TBO_InstanceData*)
  -> stale invariant ID 1095748
```

ID `1095748` is the OG 1.10.163 constructor at RVA `0x2F7B50`; it is absent from
the supplied 1.11.221 database. Dear CommonLib/Addictol independently identify
AE ID `2197563`, and that exact supplied-database record resolves to RVA
`0x436830`. The clean AE body takes `this`/form/instance-data in RCX/RDX/R8,
writes the two `BGSObjectInstance` fields, retains non-null instance data, and
derives/retains the form's default instance data when the caller supplies null.
That establishes the historical signature, layout and intrusive-pointer
ownership semantics. CommonLib now selects
`REL::RelocationID(1095748, 2197563)`; this is another OG-invariant declaration,
not a 1.10.984-as-AE pair.

The full reachable Item Explorer sweep covers eleven exact AE records: data and
player singletons, `TESForm::GetFile` (`2193103`, RVA `0x3120B0`), the new
object-instance constructor, inventory count/add, equip-manager singleton and
equip/unequip operations, and UI message queue access. Form-array access,
`TESRaceForm::GetFormRace`, and `TESFile::GetFilename` are direct layout reads.
Selected and Dear declarations independently agree on `TESForm::GetPlayable`
at physical slot `0x1D` and `GetFilledSlots` at slot `0x47`; display-name access
uses the established `TESFullName` component. Removal uses the existing
`TESObjectREFR::RemoveItem` virtual surface. No additional stale ID, virtual
order defect or layout discrepancy was found on this path.

Bounded production logs now record Explorer open, enumeration start/result,
plugin/form list population, plugin selection, selected form/action, ordered
equip/remove submission, and close. Fileless dynamic forms and unnamed-form
hardening remain intact. Upstream Item Explorer does **not** provide a hover
callback or world-reference preview: its rows are armor forms, not references.
Accordingly it cannot apply the scene/reference highlight shader merely by
hovering a row; adding that behavior would not preserve upstream semantics.

The LookAt Cam source audit compared the complete upstream and AE hook bodies.
Apart from the already-verified `FreeCameraState` ABI adaptation and detour
guard, their behavior is identical:

- the scene passes the real player's current 3D root;
- the default target is exactly skeleton node `HEAD` (user alternatives are
  `Chest` and `COM`);
- the target uses the node's world translation;
- every free-camera update calls the engine original first, preserving ordinary
  camera input, then computes yaw/pitch from the camera root's current local
  translation to the target world position;
- the hook overwrites rotation continuously but does not move the camera toward
  the target, change distance, recenter framing, or follow the actor root;
- Q/E retain the original one-unit vertical adjustment;
- no upstream code hides/shows the player, changes near-clip distance, adds a
  collision/safety offset, or prevents the camera intersecting the actor.

Thus the apparent translation "lock" is upstream's static-pivot behavior in the
sense that enabling LookAt never attaches camera translation to the actor. It
does not explicitly suppress manual free-camera translation: the original
engine update still executes first. A player leaving the frame as the animated
target moves is therefore possible, and an extremely close camera may also
produce vanilla near-plane/interior-model culling. The current evidence rules
out an NAF hide-player transition, but the reported run alone cannot distinguish
out-of-frame orientation from near-camera clipping or prove that AE's resulting
rotation was visually applied.

To close that distinction without redesigning the feature, bounded diagnostics
now log target lookup and LookAt updates 1, 2, 3 and 10: actor-root world
position, `HEAD` world position, camera position, direction vector, distance,
yaw and pitch. Lookup failure logs and disables the operation; there is no root
fallback. Studio's external-orbit target remains a separate selected-gizmo path
and is not conflated with ordinary-scene LookAt. No camera transform or behavior
was changed in this follow-up.

Recommended retest:

1. browse several Item Explorer plugin/form lists, select/equip and remove an
   item, close, and verify NAFMenu restoration;
2. regress Active Scenes inventory and an ordinary scene;
3. in a player scene, place free camera several feet away, enable LookAt, allow
   target motion, disable it, then repeat from very close range;
4. compare the bounded camera/HEAD/direction logs to the view. Changing yaw or
   pitch with a valid direction proves target computation; a distant player
   still outside the view would then identify a camera-matrix/application issue,
   while only close-range disappearance supports vanilla clipping.

### Phase-D production follow-up: Face Creator and AE LookAt application

The next production run closed the Item Explorer issue but failed exact lookup
on ID `1306546` after entering Face Animator.  The ID belongs to
`TESNPC::GetAlternateHeadPartListMap`, declared in selected CommonLib's
`TESBoundAnimObjects.h`.  Face Animator initializes a default player preview in
`InitSubmenu`; `FaceUpdateHook::StartAnimation` binds the managed face, and its
eye-geometry acquisition calls `GameUtil::GetEyeGeometry` ->
`TESNPC::GetHeadParts(true)`.  For a player using an alternate head-part list,
that reaches the global map before the first editor interaction.  This is the
exact failing stage: Face Animator preview/eye-data initialization, not Creator
project or filesystem creation.

`1306546` is the 1.10.163 Address Library anchor (old RVA `0x38B0EC`) and had
incorrectly been treated as invariant.  Dear CommonLib and Addictol identify
the same map as `1306546/2662368`; importantly, ID `2662368` exists exactly in
the supplied 1.11.221 database at RVA `0x2EFE278`.  CommonLib intentionally
applies `-0x8`, yielding the map object at `0x2EFE270`.  Six clean-AE RIP-relative
xrefs at RVAs `0x65EBF8`, `0x65ECA4`, `0x65ED6E`, `0x65EED8`, `0x66181A`, and
`0x661893` reference the anchor, corroborating its live global identity.  The
declaration is now `REL::RelocationID(1306546, 2662368)` with the historical
`-0x8` layout adaptation preserved.  This was an OG-only invariant error, not a
1.10.984-as-AE collapse.

The expanded Face Creator audit checks five exact records: the alternate
head-part map, verified player singleton, and the three production face-update
entries.  It also checks twenty source/ABI/lifecycle guards spanning project
entry, default preview binding, eye geometry, timeline controls, runtime preview
and XML output.  A dormant-source literal sweep across Face Creator, Creator,
Studio, face/body animation and shared `GameUtil` found zero additional missing
AE selectors.  Bounded logs now identify Face Animator entry, preview-target
replacement and preview initialization.

The same run disproved the earlier conclusion that LookAt was runtime-faithful:
node lookup and the vector-to-angle calculation succeeded, but the resulting
view pointed away and the player disappeared.  The historical formula remains:

```
direction = targetWorld - cameraPosition
yaw       = atan2(direction.x, direction.y)
pitch     = -atan2(direction.z, horizontalDistance)
state.pitch = pitch
state.yaw   = yaw
rotation.FromEulerAnglesZXY(yaw, pitch, 0)
```

The 1.11.221 layout is still correct: translation is `+0x28`, absolute pitch is
`+0x34`, and absolute yaw is `+0x38`.  `FreeCameraState::Update` modifies these
as accumulated absolute angles, `GetRotation` loads `+0x38` as Z/yaw and
`+0x34` as X/pitch, and calls AE `FromEulerAnglesZXY` ID `2269825`.  The
historical and AE matrix routines have the same sign/order convention.  Hook
timing also remains post-original, matching upstream.

The AE-specific defect was the second, immediate transform write.  On 1.10.163
upstream wrote both state angles and `TESCamera::cameraRoot` to avoid one-frame
input lag.  Clean 1.11.221 disassembly shows that vanilla Update instead obtains
a separate render-camera object through ID `2214738`, converts the state
quaternion via ID `2195654`, and writes that object's matrix/translation.  The
port was rotating and recursively updating `cameraRoot` as though it were still
the render target.  That wrong hierarchy-layer mutation accounts for the
orientation/culling symptom; enabling LookAt performs no explicit player hide,
camera-target assignment, fade or geometry-flag change.

The compatibility adaptation now writes only authoritative
`FreeCameraState::rotation` (and the original Q/E state translation adjustment).
The next vanilla update applies it to the correct AE render-camera object.
Ordinary free-camera translation remains processed by the original update and
is neither disabled nor recentered; LookAt continuously changes orientation
only.  The historical Euler formula and HEAD/Chest/COM targets are unchanged.

Bounded updates 1, 2, 3 and 10 now record authoritative state translation,
camera-root local/world translations, normalized desired direction, the forward
vector reconstructed from the state Euler angles, and their dot product.  A
value approaching `+1` verifies angle construction; runtime visual tracking is
still required to verify AE application and HEAD/Chest/COM switching.  The
LookAt audit now rejects reintroduction of the historical direct camera-root
write.

Production retest:

1. enter Creator -> project -> new face animation -> Face Animator; select an
   actor, edit a minimal key, preview, save, close and reopen;
2. start a player solo scene in an open area and position free camera in front
   and to one side;
3. enable HEAD and require the player to remain visible and the bounded dot to
   approach `+1`; switch Chest, COM and HEAD and observe the expected descending
   pitch targets;
4. move while LookAt remains active, then disable it and verify ordinary free
   camera and scene cleanup.
