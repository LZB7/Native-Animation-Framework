# NAF Extended managed facial-animation API v1

NAF Extended exposes its authored FaceGen timeline player to independent F4SE
plugins through a C ABI. Consumers need only `NAFExtendedAPI.h`; they must not
link CommonLibF4RD or include NAF private headers.

## Discovery and negotiation

After a save or new game has entered, find `NAF.dll`, resolve
`NAFExt_GetFaceAPI`, and request `NAFEXT_FACE_API_VERSION_1` with
`sizeof(NAFExtFaceAPIv1)`. A null return means the requested version/table size
is unsupported. Validate the returned size, service version, required capability
bits, and each function pointer before use.

The original `NAFExt_GetAPI` scene table is unchanged. Its capability mask now
also advertises `NAFEXT_CAP_FACE_PLAYBACK`, `NAFEXT_CAP_FACE_STOP`,
`NAFEXT_CAP_FACE_RESET`, and `NAFEXT_CAP_FACE_QUERY`. Facial consumers do not
need to subscribe to scene events, and scene-only ABI-v1 consumers do not need
the facial table.

## Actor identity

Every operation takes a load-order-resolved actor FormID for the current game
session. Do not pass pointers. FormIDs/handles must be reacquired after game
load; a client must not persist them as universal identities. PlayerCharacter
is FormID `0x14`. Invalid, deleted, or unavailable actors fail safely and are
reported as `NAFEXT_FACE_FAILED` when the queued operation is processed.

## Operations

```c
NAFExtResult play_face_animation(
    uint32_t actor_form_id,
    NAFExtStringView animation_id,
    uint32_t loop,
    uint32_t havok_sync);

NAFExtResult stop_face_animation(uint32_t actor_form_id);
NAFExtResult reset_face_expression(uint32_t actor_form_id);

NAFExtResult get_face_animation_state(
    uint32_t actor_form_id,
    NAFExtFaceState* state,
    char* animation_id_buffer,
    uint32_t animation_id_buffer_size,
    uint32_t* required_size);
```

`animation_id` is UTF-8 and is copied before the call returns. It names an
existing NAF face-animation registry entry. Boolean parameters use zero/one.
Play preserves the existing Papyrus semantics: one managed animation per actor,
last request wins, optional loop, and optional body/Havok time synchronization.

Stop cancels a pending load and releases managed playback according to the
legacy `StopFaceAnimation` behavior. It does not neutralize the composed
expression. Reset calls the same stronger Phase-3A service used by Papyrus and
NAF Settings: it invalidates pending loads, removes NAF-managed face/eye state,
and queues safe neutralization of the composed output.

## Threading and asynchronous results

The four service functions may be called by an external plugin thread after
game entry. Mutation functions only validate/copy arguments and enqueue work on
NAF's F4SE task path. `NAFEXT_OK` means accepted, not completed. Authored face
data loading remains asynchronous. Query state to observe progress.

`get_face_animation_state` is synchronized and does not dereference a client
pointer or return provider-owned storage. On the first call, pass a null string
buffer; when an ID exists it returns `NAFEXT_E_BUFFER_TOO_SMALL` and writes the
exact byte count to `required_size`. Allocate that many bytes and call again.
The returned bytes are UTF-8 and are not NUL-terminated. `NAFExtFaceState` is
copied into caller storage.

Statuses are `IDLE`, `PLAY_PENDING`, `ACTIVE`, `RESET_PENDING`, `STOPPED`,
`COMPLETED`, `CANCELED`, and `FAILED`. The generation changes with each actor-
level request. A late load only changes state when its generation is still
current, so animation A cannot overwrite later animation B or a later reset.
Flags report NAF ownership, known face-data availability, looping, and Havok
synchronization. They describe NAF-managed state—not every facial effect from
Fallout or another mod and not guaranteed rendered pixels.

## Lifecycle and ownership

Control is actor-level and shared: there is no per-plugin ownership namespace.
A later valid request replaces the earlier request. Revert clears transient
request/query state. `Set3D` rebinds managed FaceGen data and refreshes the
availability flag. NAF never invokes client callbacks while holding face locks;
this service has no callbacks.

The historical `FACE` v5 record continues to restore active managed face state.
The independent Extended `FRST` v1 reset marker remains provisional pending a
controlled no-HFE A/B runtime comparison. A newer valid face request removes a
retained reset marker so it cannot unexpectedly override that request.

## Minimal consumer

See `docs/api/NAFExtendedFaceAPI_consumer_example.cpp`. Production consumers
should acquire after F4SE reports a successful post-load/new-game transition,
validate every optional service field, and treat `NAFEXT_E_PROVIDER_UNAVAILABLE`
as a lifecycle condition rather than retrying from loader initialization.

