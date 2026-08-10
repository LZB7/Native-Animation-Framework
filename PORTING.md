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

The NG cleanup is a distinct unaddressed function beginning at RVA 0x55F4B0;
RVA 0x55F4E0 is its marker-loop body. It is adjacent to, but not an interior
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
