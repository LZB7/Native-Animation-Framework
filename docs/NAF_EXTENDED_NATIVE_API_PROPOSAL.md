# NAF Extended native API proposal

Status: design only; not implemented. The draft ABI is in `docs/api/NAFExtendedAPI_draft.h`.

## Boundary and discovery

The API is a versioned C ABI acquired through F4SE messaging. A client registers a listener for `NAF`, broadcasts a query containing `NAFExtAcquireRequest`, and accepts a response only after checking the result, ABI version, table size, required function pointers, and capability bits. NAF Extended also announces availability after load. Clients must support both paths: listen before the announcement and actively query later. Repeated responses are deduplicated by API-table address plus provider identity. Save/revert does not invalidate the function table, but does invalidate scene snapshots and advances the registration epoch.

Capability-based integration is the default: any compatible provider may implement the ABI. Authors who require this implementation can additionally check `provider_id == "LZB7.NAF.Extended"`. The identity check is optional and is not a security boundary.

If NAF is absent, the request receives no compatible response and the client disables only its NAF integration. An undersized table, unsupported ABI, missing function, or absent required capability is a clean incompatibility, never permission to call through a guessed layout.

## Lifecycle delivery points

- `SceneStarted`: emitted after the scene is committed Active and a complete snapshot exists.
- `SceneUpdated`: emitted after a committed change. Its reason mask covers position, stage, group/tree, animation, synchronization, speed, actor-3D rebind, and persistence fixup.
- `SceneEnding`: emitted before actor slots and scene-owned state are destroyed; the final snapshot is valid.
- `SceneEnded`: emitted after removal as an idempotent tombstone with no live snapshot.

NAF copies event work out of its internal event handlers and releases scene/global locks before invoking external callbacks. Events preserve commit order per scene. Callbacks are observational and must not synchronously mutate NAF; future commands must be queued through a separately designed command API. The implementation must document the actual worker/game/UI context and clients must marshal engine work to their own safe context. No main-thread guarantee is implied.

Registration returns an opaque token. Unregistration waits only for dispatch bookkeeping, not arbitrary client work. A callback may unregister itself; the provider marks it inactive and completes removal after the current delivery. Plugin unload requires prior unregistration. NAF never calls a removed subscription after unregistration returns. Client exceptions must not cross the C boundary.

## Snapshots and ownership

Callback snapshots are immutable NAF-owned views valid only until the callback returns. Strings are UTF-8 views with explicit byte lengths. Actors are represented by form ID plus engine handle value, not `Actor*`. Scene IDs and revisions are monotonic opaque integers. No STL, CommonLib, RTTI, XML node, allocator, or private NAF type crosses the boundary.

Queries use a two-call sizing/copy model. `get_scene_snapshot_size` returns the required byte count for a specific revision; `copy_scene_snapshot` returns `BUSY` if that revision changed before copying. The copied buffer is self-contained and caller-owned. This avoids cross-CRT allocation and permits late consumers to retain data.

Late recovery is: enumerate IDs and epoch, copy every current snapshot, then subscribe. If the epoch or any revision changes during recovery, repeat enumeration. Implementations may optimize this with subscribe-first plus revision de-duplication, but the observable rule remains that `(scene_id, revision)` uniquely identifies committed state.

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

NAF Bridge can consume this boundary instead of private C++ internals. A future official NAF 2.0 can implement the same ABI under its own provider ID. Facial reset and furniture merging are deliberately absent from this proposal.
