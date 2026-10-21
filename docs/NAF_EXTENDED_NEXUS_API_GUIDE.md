# NAF Extended — Native API Guide for Mod Authors

NAF Extended provides an optional native C API for independent F4SE plugins.
Consumers can observe scenes or request managed facial behavior without linking
against NAF internals.

The public header is
[`f4se-plugin/include/NAFExtendedAPI.h`](../f4se-plugin/include/NAFExtendedAPI.h).
It uses only standard C integer and size types, so consumers do not need NAF's
modified CommonLibF4RD or the NAF source tree.

## Available services

### Scene API v1

Subscribe to scene start, update, ending, and ended events; enumerate scenes
that were already active when your plugin connected; and copy resolved scene
snapshots. Snapshots include stable scene/revision identity, position/group/tree
state, speed and stage, actor FormIDs/handles, animation IDs, tags, and semantic
metadata. This supports overlays, reactions, telemetry, and other scene-aware
plugins without reading NAF's private classes.

### Facial API v1

Request an authored NAF face animation, stop managed playback, explicitly reset
an actor's expression, or query the current NAF-managed facial state.

### Dynamic expression API v2

Request a reusable expression preset at a chosen intensity, query its state,
and release it. Dynamic expressions may be used independently of a NAF scene.

## Discovery and negotiation

Acquire the service after F4SE plugins have loaded—normally on or after post-load,
and defer actor operations until a save/new game is active. Resolve the exported
getter from the already loaded `NAF.dll`, or listen for the F4SE discovery message
`NAFEXT_DISCOVERY_MESSAGE_TYPE` (`NAFX`). Do not load another private copy of NAF.

```cpp
#include "NAFExtendedAPI.h"
#include <Windows.h>

const NAFExtAPIv1* AcquireNAFSceneAPI()
{
    const auto module = GetModuleHandleW(L"NAF.dll");
    if (!module) return nullptr;

    const auto getAPI = reinterpret_cast<NAFExtGetAPIFn>(
        GetProcAddress(module, "NAFExt_GetAPI"));
    if (!getAPI) return nullptr;

    const auto api = getAPI(NAFEXT_ABI_VERSION_1, sizeof(NAFExtAPIv1));
    const auto required = NAFEXT_CAP_SCENE_EVENTS |
                          NAFEXT_CAP_SCENE_QUERY |
                          NAFEXT_CAP_ACTIVE_ENUMERATION;
    if (!api || api->struct_size < sizeof(NAFExtAPIv1) ||
        api->abi_version != NAFEXT_ABI_VERSION_1 ||
        (api->capabilities & required) != required) return nullptr;
    return api;
}
```

Always validate the returned table size/version, required capability bits, and
every function pointer you plan to call. `get_provider_info` can additionally
verify `LZB7.NAF.Extended`. Treat absence, an older service, or a missing
capability as a supported configuration—not a fatal plugin error.

## Scene callbacks and retained data

```cpp
void NAFEXT_CALL OnScene(const NAFExtSceneEvent* event, void*)
{
    if (!event || event->struct_size < sizeof(NAFExtSceneEvent)) return;
    // event->snapshot and all nested views are borrowed until this returns.
    // Copy required strings now, or use get_scene_snapshot_size/copy_scene_snapshot.
}

NAFExtSubscription subscription{};
if (api->subscribe_scene_events(OnScene, nullptr, &subscription) != NAFEXT_OK) {
    subscription = 0;
}
// On shutdown: api->unsubscribe_scene_events(subscription);
```

Callbacks run from NAF's game-loop processing context. Keep them short, avoid
blocking, and marshal work to the execution context your plugin requires.
Unregister before unloading and keep callback `userData` alive until any
currently executing callback returns.

Callback snapshots are borrowed. For durable data, first call
`get_scene_snapshot_size(scene, revision, &bytes)`, allocate that many bytes,
then call `copy_scene_snapshot`. The result is one caller-owned block containing
offsets rather than provider pointers. `NAFEXT_E_BUSY` means that a requested
nonzero revision changed between calls; enumerate/query again. Use
`enumerate_active_scenes` after acquisition so late-loading consumers reconcile
scenes that began before subscription.

## Facial requests

Actor identities are load-order-resolved FormIDs for the current game session.
Facial play/reset/expression requests are asynchronous: `NAFEXT_OK` means the
operation was accepted, not that engine loading or application has completed.
Query the corresponding state when completion matters.

```cpp
const auto getFace = reinterpret_cast<NAFExtGetFaceAPIv2Fn>(
    GetProcAddress(GetModuleHandleW(L"NAF.dll"), "NAFExt_GetFaceAPIv2"));
const auto face = getFace ? getFace(NAFEXT_FACE_API_VERSION_2,
                                    sizeof(NAFExtFaceAPIv2)) : nullptr;
if (face && (face->capabilities & NAFEXT_CAP_EXPRESSION_PRESETS)) {
    const char id[] = "dev.smile";
    const NAFExtStringView preset{ id, sizeof(id) - 1 };
    face->request_expression(actorFormID, preset, 0.75f, 0);
}
```

String views are byte spans and need not be null-terminated. Query output
buffers use explicit required sizes. Pass zero for options in the current facial
service versions.

## Compatibility boundaries

- The original NAF preservation DLL does not automatically expose this API.
- Negotiate services and capabilities; never assume Extended is installed.
- NAF Bridge is a separate customized NAF implementation, not an API consumer.
- Do not include or access NAF's private C++ structures from another plugin.
- Service versions are independent: a plugin may use scene v1, face v1, face v2,
  or any supported combination.

Complete examples are in [`docs/api`](api), with lifecycle testing guidance in
[`NAF_EXTENDED_API_HARNESS_TEST.md`](NAF_EXTENDED_API_HARNESS_TEST.md). The
detailed ABI and metadata contract is documented in
[`NAF_EXTENDED_NATIVE_API_PROPOSAL.md`](NAF_EXTENDED_NATIVE_API_PROPOSAL.md).
