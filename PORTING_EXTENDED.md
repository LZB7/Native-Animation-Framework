# NAF Extended port notes

## Phase 2B: independent API harness, JSON requirements and cache correctness

`tools/NAFExtAPITest` is a development-only F4SE consumer built as
`NAFExtAPITest.dll`. It includes only the standalone C header plus the documented
raw F4SE interface ABI; it does not link CommonLib, Runtime Database, spdlog, or
NAF implementation code. It listens for the NAF announcement and attempts the
exported `NAFExt_GetAPI`, validates ABI/table/capabilities/provider identity,
logs all lifecycle callbacks, validates caller-owned packed snapshots, and on
the first started scene deliberately self-unregisters, resubscribes, then
enumerates the still-active scene. It permanently unregisters after that scene
ends, so starting a second scene verifies that delivery stopped. The harness is
not a release artifact.

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
path set as well as timestamps. JSON is reparsed on every initialization and
reload; reload clears all NAF registries and the parallel JSON metadata/name
registry. No cache format or historical serialization format changed.

Runtime procedure: install NAF Extended and Runtime Database, copy only the
development harness DLL to `Data/F4SE/Plugins`, launch through F4SE 0.7.9, and
inspect `Documents/My Games/Fallout4/F4SE/NAFExtAPITest.log`. Start the JSON `J1`
scene, mutate it once, then stop it. The log must contain Started/Updated/
Ending/Ended, actor and copied-snapshot details, successful self-unregistration,
resubscription and active enumeration. Start a second scene; no further harness
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
