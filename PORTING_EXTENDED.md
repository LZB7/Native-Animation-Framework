# NAF Extended port notes

## Phase 3C: reusable dynamic expressions

Phase 3C adds cached `.nafexpr.json` expression presets and an additive
`NAFExtFaceAPIv2`. Presets contain validated Face Creator morph indices and
bounded authored weights; callers select a preset and a normalized intensity.
The existing FaceGen hook applies only those channels after vanilla composition,
while vanilla lip updates remain enabled. Dynamic expressions, authored facial
animations, release, and hard reset share the Phase 3B actor ownership and
generation mechanism, so the last valid request wins and stale work cannot
reappear.

Release removes only the dynamic contribution; Phase 3A reset remains the
strong neutralization operation. Requests are transient across load/revert and
add no serialization record. `NAFExtFaceAPIv1` and the scene API are unchanged.
Immediate replacement is implemented; timed blending is deliberately deferred.
See `docs/NAF_EXTENDED_DYNAMIC_EXPRESSIONS.md`. Compilation and static audits do
not by themselves constitute visual validation. The October 21 production run
loaded all three development presets and visibly exercised the expression API
both outside a scene and during JSON-only HKX playback. Full and 0.35 intensity
requests queried `Active`, replacement requests were accepted, release queried
`Completed`, and the body scene completed normally. The matching MFG appearance
comes from both tools addressing Fallout 4's underlying FaceGen morph channel
indices; NAF Face Creator has no source dependency on MFG. A full exit/reload
with an active expression remains the outstanding transient-lifecycle check.

## Phase 3B: independent native managed-face service

The established `NAFExtAPIv1` scene table remains byte-for-byte additive-safe:
no field or offset changed. Four new capability bits advertise a separately
negotiated `NAFExtFaceAPIv1`, acquired through `NAFExt_GetFaceAPI`. The service
uses only fixed-width C types, `NAFExtStringView`, caller-owned query buffers,
and current-session actor FormIDs. It exposes authored face play, legacy stop,
the shared Phase-3A reset, and synchronized managed-state queries without
requiring scene subscriptions, Papyrus, CommonLibF4RD linkage, or private NAF
headers.

Mutations copy arguments and enter through the F4SE task interface; authored
data loading remains asynchronous. Actor-level generation tracking prevents a
late A load from overwriting B or a later reset. Query results distinguish
pending play, active ownership, pending reset, stop, completion, cancellation,
and detectable failure, and report the selected ID and known face-data state.
This remains a one-controller-per-actor, last-valid-request-wins model rather
than pretending that external plugins have independent ownership. Revert clears
the transient query/request ledger, and Set3D refreshes face-data availability.

The controlled no-HFE A/B run closed the `FRST` question. The retained build
wrote/read ten records and stayed neutral; the comparison build safely
discarded an existing `FRST`, then wrote/read only the nine historical v5
records and also stayed neutral through full exit/reload. Cross-save reset
persistence is therefore unnecessary. Production no longer writes or restores
`FRST`; old development saves remain readable and the obsolete payload is
consumed without being carried forward. Transient reset deferral, Set3D retry,
and generation invalidation remain because they protect temporarily unavailable
FaceGen data and asynchronous ordering within the current session.

The independent harness negotiates both services only after entering a game.
Its F6-F11 plus Insert controls deliberately exercise play A/B, state query, stop, reset,
A-to-B supersession, and A-to-reset supersession. It still has no CommonLib or
Runtime Database dependency and performs no automatic facial mutation on load.
The October 20 production run runtime-validated the complete service contract:
controlled negotiation failures were rejected, stop queried as `Stopped`, reset
queried as `Completed`, repeated A-to-B replacement left B active, and the Insert
A-to-reset sequence completed with no animation ID. The final generation state
proves that an older queued play did not reassert itself after reset. Provider and
harness remained stable, and the provider continued to write/load only the nine
historical v5 records with no `FRST` record.

## Phase 3A: compiled Papyrus extension and facial-reset persistence

The Extended `NAF.psc` preserves the 45 upstream native declarations exactly
and adds one global native, `Bool Function ResetFaceExpression(Actor akActor)`.
It compiles with Bethesda Papyrus Compiler 2.8.0.4 using the Fallout 4 vanilla
source set and `Institute_Papyrus_Flags.flg`; the F4SE `modified` directory is a
set of patch fragments and is intentionally not an import directory. The
release/final optimized compiler invocation and disassembly verification are
recorded with the Phase 3A production artifacts. The resulting PEX identifies
script `NAF`, extends `ScriptObject`, retains the three upstream structures and
all non-native helper functions, and exposes 46 native globals. Existing
callers of the original 45 functions require no recompilation.

Papyrus registration now observes the real return from the VM's native-binding
operation. The log separately reports the 45 upstream registrations and the
Extended reset registration; it no longer claims 46 successes after the VM
rejects a method absent from an installed PEX. This required changing the
CommonLibF4RD convenience template to return the underlying virtual call's
existing boolean result without changing the engine ABI.

The October 20 controlled reproduction verified the complete `FRST` lifecycle:
the save contained one player marker, load resolved that actor and its face
data, the world-ready task cleared `finalExp`, and the one-load marker was
consumed. The first post-reset FaceGen samples were effectively neutral. About
2.4 seconds after fixup, Hit Facial Expression explicitly requested a new
`PlayFaceAnimation` (`DEP01` in the first recorded cycle and `DEP03` in the
second). This matches HFE's `LoadGame()` script sequence: wait 2.0 seconds,
call `NAF.StopFaceAnimation(PlayerRef)`, wait 0.5 seconds, then replay a damage
expression when its stored `Cripple` Boolean is true. The same explicit replay
also occurred on a load without an `FRST` record.

The former persistence mechanism was functioning as designed; the apparent
post-load reset failure is caused by HFE retaining its own `Cripple` script
state and issuing a fresh request, not by stale NAF ownership, premature marker
consumption, actor readiness, or asynchronous reinstallation of an old NAF
request. NAF must accept a new explicit `PlayFaceAnimation` call, so suppressing
it after reset would break public API semantics and other consumers. HFE should
revalidate the player's current cripple/damage condition (or clear its stored
flag) before the delayed replay. Nonzero `finalExp` values produced by later
ordinary engine FaceGen composition are not by themselves evidence that the
pain animation was restored; engine-owned `baseExp` and `modExp` remain
untouched intentionally. The bounded diagnostic logging used to establish this
ordering has been removed from the production source. A later controlled
no-HFE comparison established that cross-save reset persistence was unnecessary;
Phase 3B therefore removed `FRST` while retaining safe legacy-record discard.
The nine historical v5 records remain unchanged.

## Phase 2C: normalized pack cache, actor semantics, and lifecycle ordering

JSON animation sources now use the independent `Data/NAF/NAFPackCache.bin`.
The established XML cache remains untouched. Cache version 1 stores validated,
normalized definitions as CBOR; it contains no engine pointers or parser-owned
objects. Its exact lexical source manifest records normalized relative path,
size, modification time, and FNV-1a fingerprint. Header version/schema, bounded
payload length, and payload checksum reject stale, truncated, or corrupt data.
Cold rebuild writes a temporary file and atomically replaces the old cache.
Failure to write the cache does not prevent source definitions from loading.

Warm reconstruction still runs ordinary registry insertion after XML loading,
so XML-first precedence, JSON lexical first-wins behavior, independent IDs that
share an HKX, display names, extension metadata, and runtime linking semantics
are unchanged. Hot reload clears registries and validates the complete manifest;
deleted sources cannot survive through the cache. Fingerprinting closes the
same-size/preserved-timestamp edit gap at the cost of reading JSON bytes on a
warm load; it avoids JSON text parsing and normalization but cannot eliminate
filesystem enumeration or improve HKX playback/loading itself.

Actor slots accept descriptive `expectedBodyType` (`Masc`, `Femme`, `Any`,
default `Any`), optional `role` (`giver`, `receiver`), and tri-state `erection`
(`true`, `false`, omitted). They never alter eligibility or engine state. The
deprecated JSON `gender` and preferred `requirements.legacyGameSex` retain the
experimental hard game-record-sex behavior independently. ABI v1 remains
unchanged: namespace `naf.extended.actor` publishes slot-indexed skeleton,
expected body type, optional role, and optional erection JSON values. Absence of
the erection key remains distinguishable from explicit `false`.

The Phase 2B harness showed Updated revision 1, Started revision 2, and a second
synthetic Started revision 3 because same-tick internal events were drained in
queue order and active-scene reconciliation ran before that queue established a
known snapshot. Processing now orders and deduplicates queued starts before
updates and suppresses reconciliation when an explicit start is pending. The
empty revision 13 snapshot came from control-system cleanup preceding event
drain. The API now preserves the last committed resolved snapshot through that
window and `SceneEnding`; `SceneEnded` remains a snapshot-free tombstone.

Static/build validation is complete. Cache cold/warm timing and the new metadata
records still require focused in-game evidence before they are described as
runtime-validated.

The standalone `tools/benchmark_pack_cache.cpp` model uses the same nlohmann
text parse, normalized CBOR, and full-source fingerprint strategy without NAF
registry or engine-linking cost. On the development machine it measured:

| Definitions/files | Cold parse + normalize | Warm hash + CBOR | Invalidated hash + parse | Cache size | Working set |
|---:|---:|---:|---:|---:|---:|
| 1,000 | 30.1 ms | 22.6 ms | 40.6 ms | 166,715 B | 8,908,800 B |
| 5,000 | 148.4 ms | 113.4 ms | 204.4 ms | 846,715 B | 25,894,912 B |
| 20,000 | 775.0 ms | 695.6 ms | 1,282.7 ms | 3,426,715 B | 89,026,560 B |

These are development pipeline measurements, not in-game startup claims. Many
small-file opens and correctness-oriented full fingerprints dominate warm time;
the cache removes JSON text parsing/normalization but not source enumeration,
hashing, runtime registry insertion, engine linking, or HKX loading.

## Phase 2B: independent API harness, JSON requirements and cache correctness

`tools/NAFExtAPITest` is a development-only F4SE consumer built as
`NAFExtAPITest.dll`. It includes only the standalone C header plus the documented
raw F4SE interface ABI; it does not link CommonLib, Runtime Database, spdlog, or
NAF implementation code. Plugin load registers only an F4SE lifecycle listener.
After a save or new game is entered it acquires exported `NAFExt_GetAPI`,
validates ABI/table/capabilities/provider identity, subscribes, enumerates active
scenes, logs all lifecycle callbacks, and validates caller-owned packed
snapshots. It permanently unregisters after the first scene ends, so a second
scene verifies that delivery stopped. The harness is not a release artifact.

Experimental JSON actor `gender` was already translated only into the legacy
`Animation::Slot::gender` matcher; it introduced no additional restriction.
It is now explicitly optional and defaults to `ActorGender::Any`. The clearer
`requirements.skeleton` and optional `requirements.legacyGameSex` spellings are
accepted while the original top-level `skeleton`/`gender` spellings remain
compatible. This preserves XML behavior and creates room for future anatomy,
rig and role capabilities without conflating those concepts with Fallout's NPC
sex field.

JSON position `displayName` is optional. Machine IDs remain authoritative and
fall back as the label. The New Scene and Active Scene position menus show the
display name while retaining the ID for lookup/mutation; API consumers receive
it as `naf.extended:position.displayName` metadata. IDs require only a non-empty
unique string; `NAFExtJSON_` was solely a fixture convention. Distinct IDs may
legitimately share one HKX path.

`_XMLCache.bin` stores the full text and filenames of XML inputs plus FaceAnim
cache metadata. It does not cache JSON. Its historical validity check rejected
newer XML but did not compare the current file manifest, so deleting an XML (or
adding one whose timestamp predated the cache) could preserve stale cached
definitions. Cache validation now compares the exact normalized current XML
path set as well as timestamps. Phase 2C supersedes the earlier uncached JSON
behavior with the separate pack cache described above. No XML cache or
historical serialization format changed.

Runtime procedure: install NAF Extended and Runtime Database, copy only the
development harness DLL to `Data/F4SE/Plugins`, launch through F4SE 0.7.9, and
inspect `Documents/My Games/Fallout4/F4SE/NAFExtAPITest.log`. Start the JSON `J1`
scene, mutate it once, then stop it. The log must contain Started/Updated/
Ending/Ended, actor and copied-snapshot details, active enumeration and final
unregistration. Start a second scene; no further harness
callback lines should appear. Remove the harness after testing.

## Phase 2: native API and experimental JSON animation definitions

ABI v1 is implemented in `f4se-plugin/include/NAFExtendedAPI.h`. It exposes a size/version-negotiated C function table, optional provider identity `LZB7.NAF.Extended`, capability bits, token-based subscriptions, immutable scene views, active-scene enumeration, and self-contained caller-buffer snapshot copies. NAF announces the table through F4SE messaging during load and again at PostLoad; late clients can acquire the same verified table through `NAFExt_GetAPI`.

Internal scene events enqueue only stable IDs/reason flags. Snapshot construction and external callback execution occur after scene and ordered-action updates return to the serialized game-loop hook, outside NAF scene/event locks. The bounded queue holds 4096 transition records; overflow requests a full active-scene resynchronization. Subscribe-first enumeration uses scene revisions and a reset epoch. Revert queues tombstone events for later safe delivery and invalidates all old snapshots. Callback views are call-lifetime only; retained data must use the two-call packed-copy API. Actor identity is FormID plus the engine's opaque 32-bit handle, with an optional copied display name. No CommonLib, STL, RTTI, or private NAF type crosses the public header.

The implemented capabilities are lifecycle events, scene query/enumeration, actor-slot metadata, resolved animation IDs/tags, and namespaced semantic metadata. Resolved morph metadata is deliberately not advertised until NAF retains a complete post-resolution morph list rather than only applying it as an action.

Experimental schema v1 discovers `Data/NAF/**/*.naf.json`, validates the document, and constructs the same `Data::Animation` and `Data::Position` objects used by XML. It supports ordered HKX actor slots, skeleton/gender assignment, a basic position, tags, and opaque namespaced extension values. XML loads first and wins ID conflicts; JSON is processed in lexical path order and the first JSON definition wins. Reload clears the parallel metadata registry. Stages, groups, trees, conditions and the final Passion vocabulary remain deferred rather than being speculatively frozen. The development fixture is `tests/fixtures/phase2_solo.naf.json` and requires the existing Dancecyclopedia HKX asset to receive the intended in-game playback test.

The Atten-Hut research corrects the earlier Bridge assumption: the existing NAF Bridge ships a customized replacement `NAF.dll`, so it cannot coexist with or consume this API. A future independent Bridge adapter can be written against ABI v1. The Runtime Database dependency remains mandatory and fail-closed; the loader dialog shown when only Address Library is installed is expected behavior.

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
