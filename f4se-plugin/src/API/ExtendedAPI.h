#pragma once

#define NAFEXT_PROVIDER
#include "NAFExtendedAPI.h"
#include "Data/JSONAnimation.h"

namespace NAFExtended::NativeAPI
{
	constexpr std::uint64_t kCapabilities =
		NAFEXT_CAP_SCENE_EVENTS |
		NAFEXT_CAP_SCENE_QUERY |
		NAFEXT_CAP_ACTIVE_ENUMERATION |
		NAFEXT_CAP_ACTOR_SLOT_METADATA |
		NAFEXT_CAP_RESOLVED_ANIMATION_METADATA |
		NAFEXT_CAP_SEMANTIC_METADATA;

	struct OwnedActor
	{
		std::uint32_t slot{};
		std::uint32_t formID{};
		std::uint32_t handle{};
		std::string displayName;
		std::string animationID;
		std::vector<std::string> tags;
		std::vector<NAFExtStringView> tagViews;
		NAFExtActorSlotView view{};
	};

	struct OwnedMetadata
	{
		std::string nameSpace;
		std::string key;
		std::string value;
		NAFExtMetadataView view{};
	};

	struct OwnedSnapshot
	{
		std::uint64_t sceneID{};
		std::uint64_t revision{};
		std::uint64_t epoch{};
		std::uint32_t phase{ NAFEXT_SCENE_ACTIVE };
		std::string positionID;
		std::string groupID;
		std::string treeID;
		std::uint32_t stageIndex{};
		float speed{};
		std::vector<OwnedActor> actors;
		std::vector<OwnedMetadata> metadata;
		std::vector<NAFExtActorSlotView> actorViews;
		std::vector<NAFExtMetadataView> metadataViews;
		NAFExtSceneSnapshotView view{};

		static NAFExtStringView StringView(const std::string& value)
		{
			return { value.data(), static_cast<std::uint32_t>(value.size()) };
		}

		void FinalizeViews()
		{
			actorViews.clear();
			actorViews.reserve(actors.size());
			for (auto& actor : actors) {
				actor.tagViews.clear();
				actor.tagViews.reserve(actor.tags.size());
				for (const auto& tag : actor.tags) actor.tagViews.push_back(StringView(tag));
				actor.view = {
					sizeof(NAFExtActorSlotView), actor.slot, actor.formID, actor.handle,
					StringView(actor.displayName), StringView(actor.animationID), actor.tagViews.data(), static_cast<std::uint32_t>(actor.tagViews.size()), 0
				};
				actorViews.push_back(actor.view);
			}
			metadataViews.clear();
			metadataViews.reserve(metadata.size());
			for (auto& entry : metadata) {
				entry.view = {
					sizeof(NAFExtMetadataView), 1, StringView(entry.nameSpace), StringView(entry.key),
					entry.value.data(), static_cast<std::uint32_t>(entry.value.size()), 0
				};
				metadataViews.push_back(entry.view);
			}
			view = {
				sizeof(NAFExtSceneSnapshotView), NAFEXT_SNAPSHOT_VERSION_1, sceneID, revision, epoch,
				phase, static_cast<std::uint32_t>(actorViews.size()), StringView(positionID), StringView(groupID),
				StringView(treeID), stageIndex, speed, actorViews.data(), metadataViews.data(),
				static_cast<std::uint32_t>(metadataViews.size()), {}
			};
		}
	};

	struct Subscription
	{
		NAFExtSubscription id{};
		NAFExtSceneCallback callback{};
		void* userData{};
		std::atomic_bool active{ true };
	};

	struct PendingEvent
	{
		std::uint64_t sceneID{};
		std::uint32_t kind{};
		std::uint32_t reasons{};
	};

	inline std::mutex stateLock;
	inline std::unordered_map<std::uint64_t, std::shared_ptr<OwnedSnapshot>> snapshots;
	inline std::unordered_map<std::uint64_t, std::uint64_t> revisions;
	inline std::unordered_map<NAFExtSubscription, std::shared_ptr<Subscription>> subscriptions;
	inline std::vector<PendingEvent> pending;
	inline std::vector<std::shared_ptr<OwnedSnapshot>> pendingEnded;
	inline bool resyncRequested{};
	inline constexpr std::size_t kMaxPendingEvents = 4096;
	inline std::atomic_uint64_t registrationEpoch{ 1 };
	inline std::atomic_uint64_t nextSubscription{ 1 };
	inline std::vector<std::pair<Data::Events::event_type, Data::Events::EventRegistration>> internalRegistrations;

	inline std::uint64_t EventSceneID(Data::Events::event_type type, Data::Events::EventData& data)
	{
		try {
			switch (type) {
			case Data::Events::SCENE_START:
			case Data::Events::SCENE_ANIM_LOOP:
				return std::any_cast<std::uint64_t>(data);
			case Data::Events::SCENE_END:
				return std::any_cast<Data::Events::SceneData>(data).id;
			case Data::Events::SCENE_POS_CHANGE:
				return std::any_cast<Data::Events::ScenePositionData>(data).id;
			case Data::Events::TREE_POS_CHANGE:
				return std::any_cast<Data::Events::TreePositionData>(data).id;
			case Data::Events::SCENE_ANIM_CHANGE:
				return std::any_cast<std::pair<std::uint64_t, std::string>>(data).first;
			case Data::Events::SCENE_SPEED_CHANGE:
				return std::any_cast<std::pair<std::uint64_t, float>>(data).first;
			case Data::Events::SCENE_SYNC_STATUS_CHANGE:
				return std::any_cast<std::pair<std::uint64_t, Scene::SyncState>>(data).first;
			default:
				return 0;
			}
		} catch (const std::bad_any_cast&) {
			return 0;
		}
	}

	inline void QueueInternalEvent(Data::Events::event_type type, Data::Events::EventData& data)
	{
		const auto id = EventSceneID(type, data);
		if (!id) return;
		PendingEvent event{ id, NAFEXT_EVENT_SCENE_UPDATED, 0 };
		switch (type) {
		case Data::Events::SCENE_START: event.kind = NAFEXT_EVENT_SCENE_STARTED; break;
		case Data::Events::SCENE_END: event.kind = NAFEXT_EVENT_SCENE_ENDING; break;
		case Data::Events::SCENE_POS_CHANGE: event.reasons = NAFEXT_UPDATE_POSITION; break;
		case Data::Events::TREE_POS_CHANGE: event.reasons = NAFEXT_UPDATE_POSITION | NAFEXT_UPDATE_GROUP_TREE; break;
		case Data::Events::SCENE_ANIM_CHANGE: event.reasons = NAFEXT_UPDATE_ANIMATION | NAFEXT_UPDATE_STAGE; break;
		case Data::Events::SCENE_SPEED_CHANGE: event.reasons = NAFEXT_UPDATE_SPEED; break;
		case Data::Events::SCENE_SYNC_STATUS_CHANGE: event.reasons = NAFEXT_UPDATE_SYNCHRONIZATION; break;
		case Data::Events::SCENE_ANIM_LOOP: event.reasons = NAFEXT_UPDATE_STAGE; break;
		default: break;
		}
		std::scoped_lock lock{ stateLock };
		if (pending.size() < kMaxPendingEvents) {
			pending.push_back(event);
		} else {
			pending.clear();
			resyncRequested = true;
		}
	}

	inline std::shared_ptr<OwnedSnapshot> BuildSnapshot(const std::shared_ptr<Scene::IScene>& scene, std::uint32_t phase)
	{
		if (!scene) return nullptr;
		auto result = std::make_shared<OwnedSnapshot>();
		result->sceneID = scene->uid;
		result->epoch = registrationEpoch.load(std::memory_order_acquire);
		result->phase = phase;
		result->speed = scene->animMult;
		if (scene->controlSystem) {
			result->positionID = scene->controlSystem->QSystemID();
			const auto animationID = scene->controlSystem->QAnimationID();
			std::vector<std::string> resolvedTags;
			if (const auto position = Data::GetPosition(result->positionID)) {
				resolvedTags.assign(position->tags.begin(), position->tags.end());
			}
			if (const auto* group = dynamic_cast<Scene::AnimationGroupControlSystem*>(scene->controlSystem.get())) {
				result->groupID = group->id;
				result->stageIndex = static_cast<std::uint32_t>(group->currentStage);
			}
			if (const auto* tree = dynamic_cast<Scene::PositionTreeControlSystem*>(scene->controlSystem.get())) {
				result->treeID = tree->id;
			}
			const auto orderedActors = Scene::GetActorsInOrder(scene->actors);
			result->actors.reserve(orderedActors.size());
			for (std::uint32_t index = 0; index < orderedActors.size(); ++index) {
				const auto& actor = orderedActors[index];
				if (!actor) continue;
				result->actors.push_back({ index, actor->formID, actor->GetActorHandle().native_handle(), GameUtil::GetActorName(actor.get()), animationID, resolvedTags, {}, {} });
			}
		}
		for (const auto& record : Data::JSONAnimation::GetMetadata(result->positionID)) {
			result->metadata.push_back({ record.nameSpace, record.key, record.jsonValue, {} });
		}
		return result;
	}

	inline void Dispatch(std::uint32_t kind, std::uint32_t reasons, const std::shared_ptr<OwnedSnapshot>& snapshot)
	{
		std::vector<std::shared_ptr<Subscription>> targets;
		{
			std::scoped_lock lock{ stateLock };
			for (const auto& [id, subscription] : subscriptions) {
				(void)id;
				if (subscription->active.load(std::memory_order_acquire)) targets.push_back(subscription);
			}
		}
		NAFExtSceneEvent event{
			sizeof(NAFExtSceneEvent), 1, kind, reasons, snapshot->sceneID, snapshot->revision,
			kind == NAFEXT_EVENT_SCENE_ENDED ? nullptr : &snapshot->view
		};
		for (const auto& target : targets) {
			if (!target->active.load(std::memory_order_acquire)) continue;
			try {
				target->callback(&event, target->userData);
			} catch (...) {
				logger::error("NAF Extended API consumer callback raised a C++ exception; callback ignored");
			}
		}
	}

	inline void ProcessPending()
	{
		std::vector<PendingEvent> work;
		std::vector<std::shared_ptr<OwnedSnapshot>> endedWork;
		bool resync = false;
		{
			std::scoped_lock lock{ stateLock };
			work.swap(pending);
			endedWork.swap(pendingEnded);
			resync = std::exchange(resyncRequested, false);
		}
		for (const auto& tombstone : endedWork) Dispatch(NAFEXT_EVENT_SCENE_ENDED, 0, tombstone);

		// Internal position/animation notifications can be queued in the same game-loop
		// interval as SCENE_START. Present a deterministic external lifecycle: the first
		// notification for a scene is Started, followed by any updates from that interval.
		std::stable_partition(work.begin(), work.end(), [](const auto& event) { return event.kind == NAFEXT_EVENT_SCENE_STARTED; });
		std::unordered_set<std::uint64_t> queuedStarts;
		std::erase_if(work, [&](const auto& event) {
			return event.kind == NAFEXT_EVENT_SCENE_STARTED && !queuedStarts.insert(event.sceneID).second;
		});

		// Reconcile unknown active scenes for late registration and persistence fixups.
		for (const auto& scene : Scene::SceneManager::GetSceneMapSnapshot()) {
			bool known = false;
			{
				std::scoped_lock lock{ stateLock };
				known = snapshots.contains(scene->uid);
			}
			if ((!known || resync) && scene->status == Scene::SceneState::Active && !queuedStarts.contains(scene->uid)) {
				work.push_back({ scene->uid, NAFEXT_EVENT_SCENE_STARTED, NAFEXT_UPDATE_RESTORED });
				if (known) work.back().kind = NAFEXT_EVENT_SCENE_UPDATED;
			}
		}

		for (const auto& item : work) {
			std::shared_ptr<Scene::IScene> scene;
			Scene::SceneManager::GetScene(item.sceneID, scene);
			std::shared_ptr<OwnedSnapshot> snapshot;
			std::shared_ptr<OwnedSnapshot> previous;
			{
				std::scoped_lock lock{ stateLock };
				if (const auto it = snapshots.find(item.sceneID); it != snapshots.end()) previous = it->second;
			}
			if (scene) {
				std::unique_lock sceneGuard{ scene->lock };
				snapshot = BuildSnapshot(scene, item.kind == NAFEXT_EVENT_SCENE_ENDING ? NAFEXT_SCENE_ENDING : NAFEXT_SCENE_ACTIVE);
			}
			if (previous && snapshot && previous->positionID.size() && snapshot->positionID.empty()) {
				snapshot = std::make_shared<OwnedSnapshot>(*previous);
				snapshot->phase = item.kind == NAFEXT_EVENT_SCENE_ENDING ? NAFEXT_SCENE_ENDING : NAFEXT_SCENE_ACTIVE;
			}
			// Scene teardown can clear the control system before SCENE_END is drained.
			// Preserve the last committed resolved metadata for SceneEnding cleanup.
			if (item.kind == NAFEXT_EVENT_SCENE_ENDING && previous && (!snapshot || snapshot->positionID.empty())) {
				snapshot = std::make_shared<OwnedSnapshot>(*previous);
				snapshot->phase = NAFEXT_SCENE_ENDING;
			}
			if (!snapshot) snapshot = previous;
			if (!snapshot) continue;
			{
				std::scoped_lock lock{ stateLock };
				snapshot->revision = ++revisions[item.sceneID];
				snapshot->FinalizeViews();
				snapshots[item.sceneID] = snapshot;
			}
			Dispatch(item.kind, item.reasons, snapshot);
			if (item.kind == NAFEXT_EVENT_SCENE_ENDING) {
				auto tombstone = std::make_shared<OwnedSnapshot>(*snapshot);
				tombstone->phase = NAFEXT_SCENE_ENDED;
				{
					std::scoped_lock lock{ stateLock };
					tombstone->revision = ++revisions[item.sceneID];
					tombstone->FinalizeViews();
					snapshots.erase(item.sceneID);
				}
				Dispatch(NAFEXT_EVENT_SCENE_ENDED, 0, tombstone);
			}
		}
	}

	inline void ResetEpoch()
	{
		std::scoped_lock lock{ stateLock };
		pending.clear();
		resyncRequested = false;
		for (const auto& [id, snapshot] : snapshots) {
			auto tombstone = std::make_shared<OwnedSnapshot>(*snapshot);
			tombstone->phase = NAFEXT_SCENE_ENDED;
			tombstone->revision = ++revisions[id];
			tombstone->FinalizeViews();
			pendingEnded.push_back(std::move(tombstone));
		}
		snapshots.clear();
		revisions.clear();
		registrationEpoch.fetch_add(1, std::memory_order_acq_rel);
	}

	inline NAFExtResult NAFEXT_CALL GetProviderInfo(NAFExtProviderInfo* output)
	{
		if (!output || output->struct_size < sizeof(NAFExtProviderInfo)) return NAFEXT_E_INVALID_ARGUMENT;
		static constexpr std::string_view provider{ NAFEXT_PROVIDER_ID };
		static constexpr std::string_view build{ "1.11.240-phase2c" };
		*output = { sizeof(NAFExtProviderInfo), NAFEXT_ABI_VERSION_1, 1, NAFEXT_SNAPSHOT_VERSION_1, kCapabilities,
			{ provider.data(), static_cast<std::uint32_t>(provider.size()) }, { build.data(), static_cast<std::uint32_t>(build.size()) } };
		return NAFEXT_OK;
	}

	inline NAFExtResult NAFEXT_CALL Subscribe(NAFExtSceneCallback callback, void* userData, NAFExtSubscription* output)
	{
		if (!callback || !output) return NAFEXT_E_INVALID_ARGUMENT;
		auto subscription = std::make_shared<Subscription>();
		subscription->id = nextSubscription.fetch_add(1, std::memory_order_relaxed);
		subscription->callback = callback;
		subscription->userData = userData;
		{
			std::scoped_lock lock{ stateLock };
			subscriptions[subscription->id] = subscription;
		}
		*output = subscription->id;
		return NAFEXT_OK;
	}

	inline NAFExtResult NAFEXT_CALL Unsubscribe(NAFExtSubscription id)
	{
		std::scoped_lock lock{ stateLock };
		const auto it = subscriptions.find(id);
		if (it == subscriptions.end()) return NAFEXT_E_NOT_FOUND;
		it->second->active.store(false, std::memory_order_release);
		subscriptions.erase(it);
		return NAFEXT_OK;
	}

	inline NAFExtResult NAFEXT_CALL Enumerate(NAFExtSceneID* ids, std::uint32_t capacity, std::uint32_t* count, std::uint64_t* epoch)
	{
		if (!count || !epoch) return NAFEXT_E_INVALID_ARGUMENT;
		std::scoped_lock lock{ stateLock };
		const auto required = static_cast<std::uint32_t>(snapshots.size());
		*count = required;
		*epoch = registrationEpoch.load(std::memory_order_acquire);
		if (!ids || capacity < required) return required == 0 ? NAFEXT_OK : NAFEXT_E_BUFFER_TOO_SMALL;
		std::uint32_t index = 0;
		for (const auto& [id, snapshot] : snapshots) {
			(void)snapshot;
			ids[index++] = id;
		}
		return NAFEXT_OK;
	}

	inline std::size_t PackedSize(const OwnedSnapshot& snapshot)
	{
		std::size_t size = sizeof(NAFExtSceneSnapshotCopy) + snapshot.actors.size() * sizeof(NAFExtActorSlotCopy) + snapshot.metadata.size() * sizeof(NAFExtMetadataCopy);
		size += snapshot.positionID.size() + snapshot.groupID.size() + snapshot.treeID.size();
		for (const auto& actor : snapshot.actors) {
			size += actor.displayName.size() + actor.animationID.size() + actor.tags.size() * sizeof(NAFExtStringCopy);
			for (const auto& tag : actor.tags) size += tag.size();
		}
		for (const auto& entry : snapshot.metadata) size += entry.nameSpace.size() + entry.key.size() + entry.value.size();
		return size;
	}

	inline std::shared_ptr<OwnedSnapshot> FindSnapshot(NAFExtSceneID id, std::uint64_t expected)
	{
		std::scoped_lock lock{ stateLock };
		const auto it = snapshots.find(id);
		if (it == snapshots.end() || (expected && it->second->revision != expected)) return nullptr;
		return it->second;
	}

	inline NAFExtResult NAFEXT_CALL GetSnapshotSize(NAFExtSceneID id, std::uint64_t expected, std::size_t* output)
	{
		if (!output) return NAFEXT_E_INVALID_ARGUMENT;
		const auto snapshot = FindSnapshot(id, expected);
		if (!snapshot) return expected ? NAFEXT_E_BUSY : NAFEXT_E_NOT_FOUND;
		*output = PackedSize(*snapshot);
		return NAFEXT_OK;
	}

	inline NAFExtResult NAFEXT_CALL CopySnapshot(NAFExtSceneID id, std::uint64_t expected, void* buffer, std::size_t bufferSize, std::size_t* outputSize)
	{
		if (!outputSize) return NAFEXT_E_INVALID_ARGUMENT;
		const auto snapshot = FindSnapshot(id, expected);
		if (!snapshot) return expected ? NAFEXT_E_BUSY : NAFEXT_E_NOT_FOUND;
		const auto required = PackedSize(*snapshot);
		*outputSize = required;
		if (!buffer || bufferSize < required) return NAFEXT_E_BUFFER_TOO_SMALL;
		auto* bytes = static_cast<std::uint8_t*>(buffer);
		std::memset(bytes, 0, required);
		auto* header = reinterpret_cast<NAFExtSceneSnapshotCopy*>(bytes);
		const auto metadataOffset = sizeof(NAFExtSceneSnapshotCopy) + snapshot->actors.size() * sizeof(NAFExtActorSlotCopy);
		std::size_t tagDescriptorOffset = metadataOffset + snapshot->metadata.size() * sizeof(NAFExtMetadataCopy);
		std::size_t tagDescriptorCount = 0;
		for (const auto& actor : snapshot->actors) tagDescriptorCount += actor.tags.size();
		auto cursor = tagDescriptorOffset + tagDescriptorCount * sizeof(NAFExtStringCopy);
		auto append = [&](const std::string& value) {
			const auto offset = static_cast<std::uint32_t>(cursor);
			std::memcpy(bytes + cursor, value.data(), value.size());
			cursor += value.size();
			return offset;
		};
		*header = { sizeof(*header), NAFEXT_SNAPSHOT_VERSION_1, snapshot->sceneID, snapshot->revision, snapshot->epoch, snapshot->phase,
			static_cast<std::uint32_t>(snapshot->actors.size()), sizeof(*header), 0, static_cast<std::uint32_t>(snapshot->positionID.size()),
			0, static_cast<std::uint32_t>(snapshot->groupID.size()), 0, static_cast<std::uint32_t>(snapshot->treeID.size()), snapshot->stageIndex,
			snapshot->speed, static_cast<std::uint32_t>(metadataOffset),
			static_cast<std::uint32_t>(snapshot->metadata.size()) };
		header->position_offset = append(snapshot->positionID);
		header->group_offset = append(snapshot->groupID);
		header->tree_offset = append(snapshot->treeID);
		auto* actors = reinterpret_cast<NAFExtActorSlotCopy*>(bytes + header->actors_offset);
		for (std::size_t i = 0; i < snapshot->actors.size(); ++i) {
			const auto& source = snapshot->actors[i];
			actors[i] = { sizeof(NAFExtActorSlotCopy), source.slot, source.formID, source.handle, append(source.displayName),
				static_cast<std::uint32_t>(source.displayName.size()), append(source.animationID), static_cast<std::uint32_t>(source.animationID.size()),
				static_cast<std::uint32_t>(tagDescriptorOffset), static_cast<std::uint32_t>(source.tags.size()) };
			auto* tags = reinterpret_cast<NAFExtStringCopy*>(bytes + tagDescriptorOffset);
			for (std::size_t tagIndex = 0; tagIndex < source.tags.size(); ++tagIndex) {
				tags[tagIndex] = { append(source.tags[tagIndex]), static_cast<std::uint32_t>(source.tags[tagIndex].size()) };
			}
			tagDescriptorOffset += source.tags.size() * sizeof(NAFExtStringCopy);
		}
		auto* metadataCopy = reinterpret_cast<NAFExtMetadataCopy*>(bytes + header->metadata_offset);
		for (std::size_t i = 0; i < snapshot->metadata.size(); ++i) {
			const auto& source = snapshot->metadata[i];
			metadataCopy[i] = { sizeof(NAFExtMetadataCopy), 1, append(source.nameSpace), static_cast<std::uint32_t>(source.nameSpace.size()),
				append(source.key), static_cast<std::uint32_t>(source.key.size()), append(source.value), static_cast<std::uint32_t>(source.value.size()) };
		}
		return NAFEXT_OK;
	}

	inline const NAFExtAPIv1 api{
		sizeof(NAFExtAPIv1), NAFEXT_ABI_VERSION_1, kCapabilities, &GetProviderInfo, &Subscribe, &Unsubscribe, &Enumerate, &GetSnapshotSize, &CopySnapshot
	};

	inline void Announce()
	{
		if (const auto messaging = F4SE::GetMessagingInterface()) {
			static const NAFExtAnnouncement announcement{ sizeof(NAFExtAnnouncement), NAFEXT_ABI_VERSION_1, &api };
			messaging->Dispatch(NAFEXT_DISCOVERY_MESSAGE_TYPE, const_cast<NAFExtAnnouncement*>(&announcement), sizeof(announcement), nullptr);
		}
	}

	inline void Initialize()
	{
		const std::array<Data::Events::event_type, 8> eventTypes{
			Data::Events::SCENE_START, Data::Events::SCENE_END, Data::Events::SCENE_POS_CHANGE, Data::Events::TREE_POS_CHANGE,
			Data::Events::SCENE_ANIM_CHANGE, Data::Events::SCENE_SPEED_CHANGE, Data::Events::SCENE_SYNC_STATUS_CHANGE, Data::Events::SCENE_ANIM_LOOP
		};
		for (const auto type : eventTypes) {
			internalRegistrations.emplace_back(type, Data::Events::Subscribe(type, QueueInternalEvent));
		}
		Announce();
		logger::info("NAF Extended native API v1 initialized with capabilities 0x{:X}", kCapabilities);
	}
}

extern "C" __declspec(dllexport) const NAFExtAPIv1* NAFEXT_CALL NAFExt_GetAPI(std::uint32_t requestedABI, std::uint32_t callerTableSize)
{
	if (requestedABI != NAFEXT_ABI_VERSION_1 || callerTableSize < sizeof(NAFExtAPIv1)) return nullptr;
	return &NAFExtended::NativeAPI::api;
}
