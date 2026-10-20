# NAF Extended native API proposal

Status: ABI v1 implemented; an independent loadable Phase 2B harness is included
for runtime validation. The canonical standalone header is
`f4se-plugin/include/NAFExtendedAPI.h`; the older draft remains design history.

## Boundary and discovery

The API is a versioned C ABI. NAF Extended broadcasts an F4SE message of type
`NAFEXT_DISCOVERY_MESSAGE_TYPE` containing `NAFExtAnnouncement` after the API is
initialized. A consumer loaded later obtains the same table from the exported
`NAFExt_GetAPI` function. DLL presence alone is not compatibility: every client
must check ABI version, table size, required functions and capabilities. Save or
revert does not invalidate the table, but does invalidate snapshots and advance
the registration epoch.

Capability-based integration is the default: any compatible provider may implement the ABI. Authors who require this implementation can additionally check `provider_id == "LZB7.NAF.Extended"`. The identity check is optional and is not a security boundary.

If NAF is absent, the request receives no compatible response and the client disables only its NAF integration. An undersized table, unsupported ABI, missing function, or absent required capability is a clean incompatibility, never permission to call through a guessed layout.

## Lifecycle delivery points

- `SceneStarted`: emitted once after the scene is committed Active and a complete snapshot exists. Notifications queued in the same game-loop interval are ordered so Started precedes Updated. A late-registration or persistence reconciliation may synthesize Started with `NAFEXT_UPDATE_RESTORED`, but it does not duplicate an already queued start.
- `SceneUpdated`: emitted after a committed change. Its reason mask covers position, stage, group/tree, animation, synchronization, speed, actor-3D rebind, and persistence fixup.
- `SceneEnding`: emitted during teardown with actor identity and the last committed resolved position, animation, tags, and semantic metadata retained for deterministic consumer cleanup.
- `SceneEnded`: emitted after removal as an idempotent tombstone with no live snapshot.

NAF copies event work out of its internal event handlers and releases scene/global locks before invoking external callbacks. Events preserve commit order per scene. Callbacks are observational and must not synchronously mutate NAF; future commands must be queued through a separately designed command API. The implementation must document the actual worker/game/UI context and clients must marshal engine work to their own safe context. No main-thread guarantee is implied.

Registration returns an opaque token. Unregistration waits only for dispatch bookkeeping, not arbitrary client work. A callback may unregister itself; the provider marks it inactive and completes removal after the current delivery. Plugin unload requires prior unregistration. NAF never calls a removed subscription after unregistration returns. Client exceptions must not cross the C boundary.

## Snapshots and ownership

Callback snapshots are immutable NAF-owned views valid only until the callback returns. Strings are UTF-8 views with explicit byte lengths. Actors are represented by form ID plus engine handle value, not `Actor*`. Scene IDs and revisions are monotonic opaque integers. No STL, CommonLib, RTTI, XML node, allocator, or private NAF type crosses the boundary.

Queries use a two-call sizing/copy model. `get_scene_snapshot_size` returns the required byte count for a specific revision; `copy_scene_snapshot` returns `BUSY` if that revision changed before copying. The copied buffer is self-contained and caller-owned. This avoids cross-CRT allocation and permits late consumers to retain data.

Late recovery is subscribe first, enumerate IDs and epoch, then copy every current
snapshot. A client reconciles callback and enumeration results by `(scene_id,
revision)` and repeats if the epoch changes or a copy reports `BUSY`. This avoids
the enumeration-before-subscription race.

Resolved metadata belongs in snapshots: ordered actor slots, selected position/group/tree/stage, resolved per-slot animation IDs, tags, and morph directives. Consumers do not repeat XML priority, conditions, tree selection, or actor assignment. Future semantic metadata is added through new capability bits and appended size-versioned structures.

## Atten-Hut example

1. Register for F4SE messages and issue an acquire request for ABI 1 plus scene events, query, enumeration, actor-slot metadata, and morph metadata.
2. Optionally require provider ID `LZB7.NAF.Extended`; normally accept any compatible provider.
3. Subscribe, enumerate active scenes, and copy snapshots by revision.
4. On Started/Updated, derive Atten-Hut-owned requests from resolved slot/morph metadata.
5. On Ending, release scene-scoped requests while the final actor identity is available.
6. On Ended, perform idempotent cleanup keyed only by scene ID.
7. On revert or provider loss, clear integration state without affecting unrelated Atten-Hut features.

NAF does not become an erection controller. It exposes resolved scene truth; Atten-Hut owns semantic state, arbitration, body profiles, and physical application.

## Required NAF source changes for implementation

1. Add a private snapshot builder at the committed scene-state boundary.
2. Add a lock-free/out-of-lock dispatch queue with per-scene revision assignment.
3. Emit Ending before actor cleanup and Ended after scene removal.
4. Maintain a read-only active-scene registry suitable for copy-out queries.
5. Invalidate epochs and drain subscriptions safely on serialization revert/shutdown.
6. Add F4SE query/announcement handling and a static ABI table.
7. Add C-ABI conformance tests, hostile size/version tests, late-load race tests, self-unregistration tests, and callback reentrancy tests.

The existing NAF Bridge cannot consume this boundary: it distributes a customized
replacement `NAF.dll` and is mutually exclusive with NAF Extended. A future,
independent Bridge adapter could consume ABI v1. A future official NAF can also
implement the ABI under its own provider identity. Facial reset and furniture
merging remain deliberately absent from Phase 2.

## Implemented execution and ownership contract

Internal NAF events enqueue only scene IDs and reason bits. Snapshot construction
and external callbacks occur after `SceneManager::UpdateScenes` and the ordered
action update return to the serialized game-loop hook. NAF scene/event locks are
not held across client code. No main-thread guarantee is made; clients must marshal
engine mutations to a proven context.

Callback views are NAF-owned and valid only for the call. Query copies contain no
provider pointers and use offsets from `NAFExtSceneSnapshotCopy`. Subscriptions
are token-based; self-unregistration marks the shared registration inactive and
does not wait on itself. C++ exceptions are contained at the callback boundary.
Plugin unload still requires cooperative unregistration because F4SE does not
provide a safe general DLL-unload notification.

Implemented capabilities are lifecycle events, snapshots, active enumeration,
actor-slot metadata, resolved animation metadata, and namespaced semantic metadata.
Resolved morph metadata is not advertised yet because the current scene runtime
applies legacy morphs without retaining a complete resolved immutable list.
