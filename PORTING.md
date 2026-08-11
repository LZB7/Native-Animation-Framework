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
