#ifndef NAF_EXTENDED_API_DRAFT_H
#define NAF_EXTENDED_API_DRAFT_H

/* Design draft only. This header is not implemented or shipped by NAF yet. */
#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#  define NAFEXT_CALL __cdecl
#else
#  define NAFEXT_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NAFEXT_ABI_VERSION_1 1u
#define NAFEXT_DISCOVERY_MESSAGE_TYPE 0x5846414Eu /* 'NAFX' little-endian */

typedef uint64_t NAFExtSceneID;
typedef uint64_t NAFExtSubscription;

typedef enum NAFExtResult {
    NAFEXT_OK = 0,
    NAFEXT_E_INVALID_ARGUMENT = 1,
    NAFEXT_E_BUFFER_TOO_SMALL = 2,
    NAFEXT_E_NOT_FOUND = 3,
    NAFEXT_E_UNSUPPORTED = 4,
    NAFEXT_E_VERSION_MISMATCH = 5,
    NAFEXT_E_BUSY = 6,
    NAFEXT_E_PROVIDER_UNAVAILABLE = 7
} NAFExtResult;

typedef enum NAFExtCapability {
    NAFEXT_CAP_SCENE_EVENTS = UINT64_C(1) << 0,
    NAFEXT_CAP_SCENE_QUERY = UINT64_C(1) << 1,
    NAFEXT_CAP_ACTIVE_ENUMERATION = UINT64_C(1) << 2,
    NAFEXT_CAP_ACTOR_SLOT_METADATA = UINT64_C(1) << 3,
    NAFEXT_CAP_RESOLVED_MORPHS = UINT64_C(1) << 4,
    NAFEXT_CAP_SEMANTIC_METADATA = UINT64_C(1) << 5
} NAFExtCapability;

typedef enum NAFExtScenePhase {
    NAFEXT_SCENE_ACTIVE = 1,
    NAFEXT_SCENE_ENDING = 2,
    NAFEXT_SCENE_ENDED = 3
} NAFExtScenePhase;

typedef enum NAFExtUpdateReason {
    NAFEXT_UPDATE_POSITION = 1u << 0,
    NAFEXT_UPDATE_STAGE = 1u << 1,
    NAFEXT_UPDATE_GROUP_TREE = 1u << 2,
    NAFEXT_UPDATE_ANIMATION = 1u << 3,
    NAFEXT_UPDATE_SYNCHRONIZATION = 1u << 4,
    NAFEXT_UPDATE_SPEED = 1u << 5,
    NAFEXT_UPDATE_ACTOR_3D = 1u << 6,
    NAFEXT_UPDATE_RESTORED = 1u << 7
} NAFExtUpdateReason;

typedef struct NAFExtStringView {
    const char* data;       /* UTF-8; callback-lifetime only */
    uint32_t length_bytes;  /* excludes any terminator */
} NAFExtStringView;

typedef struct NAFExtActorSlotView {
    uint32_t struct_size;
    uint32_t slot_index;
    uint32_t actor_form_id;
    uint32_t actor_handle;
    NAFExtStringView animation_id;
    const NAFExtStringView* tags;
    uint32_t tag_count;
    uint32_t reserved;
} NAFExtActorSlotView;

typedef struct NAFExtMorphView {
    uint32_t struct_size;
    uint32_t slot_index;
    NAFExtStringView name;
    float value;
    uint32_t reserved;
} NAFExtMorphView;

typedef struct NAFExtSceneSnapshotView {
    uint32_t struct_size;
    uint32_t snapshot_version;
    NAFExtSceneID scene_id;
    uint64_t revision;
    uint64_t registration_epoch;
    uint32_t phase;
    uint32_t actor_count;
    NAFExtStringView position_id;
    NAFExtStringView group_id;
    NAFExtStringView tree_id;
    uint32_t stage_index;
    float scene_speed;
    const NAFExtActorSlotView* actors;
    const NAFExtMorphView* morphs;
    uint32_t morph_count;
    uint32_t reserved[5];
} NAFExtSceneSnapshotView;

typedef enum NAFExtSceneEventKind {
    NAFEXT_EVENT_SCENE_STARTED = 1,
    NAFEXT_EVENT_SCENE_UPDATED = 2,
    NAFEXT_EVENT_SCENE_ENDING = 3,
    NAFEXT_EVENT_SCENE_ENDED = 4
} NAFExtSceneEventKind;

typedef struct NAFExtSceneEvent {
    uint32_t struct_size;
    uint32_t event_version;
    uint32_t kind;
    uint32_t update_reasons;
    NAFExtSceneID scene_id;
    uint64_t revision;
    const NAFExtSceneSnapshotView* snapshot; /* null only for SceneEnded */
} NAFExtSceneEvent;

typedef void (NAFEXT_CALL *NAFExtSceneCallback)(const NAFExtSceneEvent* event, void* user_data);

typedef struct NAFExtProviderInfo {
    uint32_t struct_size;
    uint32_t abi_version;
    uint32_t provider_version;
    uint32_t reserved0;
    uint64_t capabilities;
    NAFExtStringView provider_id;    /* e.g. LZB7.NAF.Extended */
    NAFExtStringView build_id;
} NAFExtProviderInfo;

typedef struct NAFExtAPIv1 {
    uint32_t struct_size;
    uint32_t abi_version;
    uint64_t capabilities;
    NAFExtResult (NAFEXT_CALL *get_provider_info)(NAFExtProviderInfo* out_info);
    NAFExtResult (NAFEXT_CALL *subscribe_scene_events)(NAFExtSceneCallback callback, void* user_data, NAFExtSubscription* out_subscription);
    NAFExtResult (NAFEXT_CALL *unsubscribe_scene_events)(NAFExtSubscription subscription);
    NAFExtResult (NAFEXT_CALL *enumerate_active_scenes)(NAFExtSceneID* ids, uint32_t capacity, uint32_t* inout_count, uint64_t* out_epoch);
    NAFExtResult (NAFEXT_CALL *get_scene_snapshot_size)(NAFExtSceneID scene_id, uint64_t expected_revision, size_t* out_size);
    NAFExtResult (NAFEXT_CALL *copy_scene_snapshot)(NAFExtSceneID scene_id, uint64_t expected_revision, void* buffer, size_t buffer_size, size_t* out_size);
} NAFExtAPIv1;

typedef struct NAFExtAcquireRequest {
    uint32_t struct_size;
    uint32_t requested_abi;
    uint64_t required_capabilities;
    const NAFExtAPIv1* out_api;
    NAFExtResult result;
} NAFExtAcquireRequest;

#ifdef __cplusplus
}
#endif
#endif
