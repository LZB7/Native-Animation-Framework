#ifndef NAF_EXTENDED_API_H
#define NAF_EXTENDED_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#  define NAFEXT_CALL __cdecl
#  ifdef NAFEXT_PROVIDER
#    define NAFEXT_EXPORT __declspec(dllexport)
#  else
#    define NAFEXT_EXPORT __declspec(dllimport)
#  endif
#else
#  define NAFEXT_CALL
#  define NAFEXT_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NAFEXT_ABI_VERSION_1 1u
#define NAFEXT_SNAPSHOT_VERSION_1 1u
#define NAFEXT_FACE_API_VERSION_1 1u
#define NAFEXT_FACE_API_VERSION_2 2u
#define NAFEXT_FACE_STATE_VERSION_1 1u
#define NAFEXT_EXPRESSION_STATE_VERSION_1 1u
#define NAFEXT_DISCOVERY_MESSAGE_TYPE UINT32_C(0x5846414E) /* 'NAFX' */
#define NAFEXT_PROVIDER_ID "LZB7.NAF.Extended"

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
    NAFEXT_CAP_RESOLVED_ANIMATION_METADATA = UINT64_C(1) << 4,
    NAFEXT_CAP_RESOLVED_MORPHS = UINT64_C(1) << 5,
    NAFEXT_CAP_SEMANTIC_METADATA = UINT64_C(1) << 6,
    NAFEXT_CAP_FACE_PLAYBACK = UINT64_C(1) << 7,
    NAFEXT_CAP_FACE_STOP = UINT64_C(1) << 8,
    NAFEXT_CAP_FACE_RESET = UINT64_C(1) << 9,
    NAFEXT_CAP_FACE_QUERY = UINT64_C(1) << 10,
    NAFEXT_CAP_EXPRESSION_PRESETS = UINT64_C(1) << 11,
    NAFEXT_CAP_EXPRESSION_INTENSITY = UINT64_C(1) << 12,
    NAFEXT_CAP_EXPRESSION_RELEASE = UINT64_C(1) << 13,
    NAFEXT_CAP_EXPRESSION_QUERY = UINT64_C(1) << 14
} NAFExtCapability;

typedef enum NAFExtFaceStatus {
    NAFEXT_FACE_IDLE = 0,
    NAFEXT_FACE_PLAY_PENDING = 1,
    NAFEXT_FACE_ACTIVE = 2,
    NAFEXT_FACE_RESET_PENDING = 3,
    NAFEXT_FACE_STOPPED = 4,
    NAFEXT_FACE_COMPLETED = 5,
    NAFEXT_FACE_CANCELED = 6,
    NAFEXT_FACE_FAILED = 7
} NAFExtFaceStatus;

typedef enum NAFExtFaceStateFlag {
    NAFEXT_FACE_FLAG_MANAGED = 1u << 0,
    NAFEXT_FACE_FLAG_FACE_DATA_AVAILABLE = 1u << 1,
    NAFEXT_FACE_FLAG_LOOP = 1u << 2,
    NAFEXT_FACE_FLAG_HAVOK_SYNC = 1u << 3,
    NAFEXT_FACE_FLAG_AUTHORED_SOURCE = 1u << 4
} NAFExtFaceStateFlag;

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
    const char* data;
    uint32_t length_bytes;
} NAFExtStringView;

typedef struct NAFExtActorSlotView {
    uint32_t struct_size;
    uint32_t slot_index;
    uint32_t actor_form_id;
    uint32_t actor_handle;
    NAFExtStringView display_name;
    NAFExtStringView animation_id;
    const NAFExtStringView* tags;
    uint32_t tag_count;
    uint32_t reserved;
} NAFExtActorSlotView;

typedef struct NAFExtMetadataView {
    uint32_t struct_size;
    uint32_t value_type; /* 1 = UTF-8 JSON payload */
    NAFExtStringView name_space;
    NAFExtStringView key;
    const void* data;
    uint32_t data_size;
    uint32_t reserved;
} NAFExtMetadataView;

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
    const NAFExtMetadataView* metadata;
    uint32_t metadata_count;
    uint32_t reserved[4];
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
    const NAFExtSceneSnapshotView* snapshot;
} NAFExtSceneEvent;

typedef void (NAFEXT_CALL *NAFExtSceneCallback)(const NAFExtSceneEvent*, void*);

typedef struct NAFExtProviderInfo {
    uint32_t struct_size;
    uint32_t abi_version;
    uint32_t provider_version;
    uint32_t snapshot_version;
    uint64_t capabilities;
    NAFExtStringView provider_id;
    NAFExtStringView build_id;
} NAFExtProviderInfo;

/* Header followed by packed actor/string/metadata data. All offsets are from
   the start of this header. No provider pointers occur in the copied form. */
typedef struct NAFExtSceneSnapshotCopy {
    uint32_t struct_size;
    uint32_t snapshot_version;
    NAFExtSceneID scene_id;
    uint64_t revision;
    uint64_t registration_epoch;
    uint32_t phase;
    uint32_t actor_count;
    uint32_t actors_offset;
    uint32_t position_offset;
    uint32_t position_length;
    uint32_t group_offset;
    uint32_t group_length;
    uint32_t tree_offset;
    uint32_t tree_length;
    uint32_t stage_index;
    float scene_speed;
    uint32_t metadata_offset;
    uint32_t metadata_count;
} NAFExtSceneSnapshotCopy;

typedef struct NAFExtActorSlotCopy {
    uint32_t struct_size;
    uint32_t slot_index;
    uint32_t actor_form_id;
    uint32_t actor_handle;
    uint32_t display_name_offset;
    uint32_t display_name_length;
    uint32_t animation_id_offset;
    uint32_t animation_id_length;
    uint32_t tags_offset; /* NAFExtStringCopy[tag_count] */
    uint32_t tag_count;
} NAFExtActorSlotCopy;

typedef struct NAFExtStringCopy {
    uint32_t data_offset;
    uint32_t length_bytes;
} NAFExtStringCopy;

typedef struct NAFExtMetadataCopy {
    uint32_t struct_size;
    uint32_t value_type;
    uint32_t namespace_offset;
    uint32_t namespace_length;
    uint32_t key_offset;
    uint32_t key_length;
    uint32_t data_offset;
    uint32_t data_size;
} NAFExtMetadataCopy;

typedef struct NAFExtAPIv1 {
    uint32_t struct_size;
    uint32_t abi_version;
    uint64_t capabilities;
    NAFExtResult (NAFEXT_CALL *get_provider_info)(NAFExtProviderInfo*);
    NAFExtResult (NAFEXT_CALL *subscribe_scene_events)(NAFExtSceneCallback, void*, NAFExtSubscription*);
    NAFExtResult (NAFEXT_CALL *unsubscribe_scene_events)(NAFExtSubscription);
    NAFExtResult (NAFEXT_CALL *enumerate_active_scenes)(NAFExtSceneID*, uint32_t, uint32_t*, uint64_t*);
    NAFExtResult (NAFEXT_CALL *get_scene_snapshot_size)(NAFExtSceneID, uint64_t, size_t*);
    NAFExtResult (NAFEXT_CALL *copy_scene_snapshot)(NAFExtSceneID, uint64_t, void*, size_t, size_t*);
} NAFExtAPIv1;

/* Actor identities are load-order-resolved FormIDs in the current game session.
   Calls enqueue work onto NAF's engine-safe task path. NAFEXT_OK means accepted,
   not that asynchronous loading or playback has already completed. */
typedef struct NAFExtFaceState {
    uint32_t struct_size;
    uint32_t state_version;
    uint32_t actor_form_id;
    uint32_t status;
    uint64_t request_generation;
    uint32_t flags;
    uint32_t animation_id_length;
} NAFExtFaceState;

typedef struct NAFExtFaceAPIv1 {
    uint32_t struct_size;
    uint32_t service_version;
    uint64_t capabilities;
    NAFExtResult (NAFEXT_CALL *play_face_animation)(uint32_t, NAFExtStringView, uint32_t, uint32_t);
    NAFExtResult (NAFEXT_CALL *stop_face_animation)(uint32_t);
    NAFExtResult (NAFEXT_CALL *reset_face_expression)(uint32_t);
    NAFExtResult (NAFEXT_CALL *get_face_animation_state)(uint32_t, NAFExtFaceState*, char*, uint32_t, uint32_t*);
} NAFExtFaceAPIv1;

typedef struct NAFExtExpressionState {
    uint32_t struct_size;
    uint32_t state_version;
    uint32_t actor_form_id;
    uint32_t status; /* NAFExtFaceStatus */
    uint64_t request_generation;
    float requested_intensity;
    uint32_t flags; /* NAFExtFaceStateFlag */
    uint32_t preset_id_length;
    uint32_t reserved;
} NAFExtExpressionState;

/* v2 repeats the v1 prefix so consumers can validate it independently without
   relying on C structure inheritance. options must be zero in this version. */
typedef struct NAFExtFaceAPIv2 {
    uint32_t struct_size;
    uint32_t service_version;
    uint64_t capabilities;
    NAFExtResult (NAFEXT_CALL *play_face_animation)(uint32_t, NAFExtStringView, uint32_t, uint32_t);
    NAFExtResult (NAFEXT_CALL *stop_face_animation)(uint32_t);
    NAFExtResult (NAFEXT_CALL *reset_face_expression)(uint32_t);
    NAFExtResult (NAFEXT_CALL *get_face_animation_state)(uint32_t, NAFExtFaceState*, char*, uint32_t, uint32_t*);
    NAFExtResult (NAFEXT_CALL *request_expression)(uint32_t, NAFExtStringView, float, uint32_t);
    NAFExtResult (NAFEXT_CALL *release_expression)(uint32_t, uint32_t);
    NAFExtResult (NAFEXT_CALL *get_expression_state)(uint32_t, NAFExtExpressionState*, char*, uint32_t, uint32_t*);
} NAFExtFaceAPIv2;

/* Snapshot retention contract:
   - callback NAFExtSceneSnapshotView pointers expire when the callback returns;
   - call get_scene_snapshot_size(id, revision), allocate that many bytes, then
     call copy_scene_snapshot(id, revision, ...);
   - NAFEXT_E_BUSY means the requested nonzero revision is no longer current;
     enumerate/query again rather than retaining provider pointers;
   - a successful NAFExtSceneSnapshotCopy is entirely caller-owned and contains
     offsets only, so it remains valid after callbacks and later scene changes. */

typedef struct NAFExtAnnouncement {
    uint32_t struct_size;
    uint32_t abi_version;
    const NAFExtAPIv1* api;
} NAFExtAnnouncement;

typedef const NAFExtAPIv1* (NAFEXT_CALL *NAFExtGetAPIFn)(uint32_t requested_abi, uint32_t caller_table_size);
typedef const NAFExtFaceAPIv1* (NAFEXT_CALL *NAFExtGetFaceAPIFn)(uint32_t requested_version, uint32_t caller_table_size);
typedef const NAFExtFaceAPIv2* (NAFEXT_CALL *NAFExtGetFaceAPIv2Fn)(uint32_t requested_version, uint32_t caller_table_size);
NAFEXT_EXPORT const NAFExtAPIv1* NAFEXT_CALL NAFExt_GetAPI(uint32_t requested_abi, uint32_t caller_table_size);
NAFEXT_EXPORT const NAFExtFaceAPIv1* NAFEXT_CALL NAFExt_GetFaceAPI(uint32_t requested_version, uint32_t caller_table_size);
NAFEXT_EXPORT const NAFExtFaceAPIv2* NAFEXT_CALL NAFExt_GetFaceAPIv2(uint32_t requested_version, uint32_t caller_table_size);

#ifdef __cplusplus
}
#endif
#endif
