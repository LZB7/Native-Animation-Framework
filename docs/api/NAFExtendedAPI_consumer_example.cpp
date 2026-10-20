#include "NAFExtendedAPI.h"

#include <cstddef>
#include <cstring>
#include <vector>

namespace ExampleConsumer
{
	const NAFExtAPIv1* api{};
	NAFExtSubscription subscription{};

	bool Validate(const NAFExtAPIv1* candidate, bool strictProvider)
	{
		if (!candidate || candidate->struct_size < sizeof(NAFExtAPIv1) || candidate->abi_version != NAFEXT_ABI_VERSION_1 ||
			(candidate->capabilities & (NAFEXT_CAP_SCENE_EVENTS | NAFEXT_CAP_SCENE_QUERY | NAFEXT_CAP_ACTIVE_ENUMERATION)) !=
				(NAFEXT_CAP_SCENE_EVENTS | NAFEXT_CAP_SCENE_QUERY | NAFEXT_CAP_ACTIVE_ENUMERATION) ||
			!candidate->get_provider_info || !candidate->subscribe_scene_events || !candidate->unsubscribe_scene_events ||
			!candidate->enumerate_active_scenes || !candidate->get_scene_snapshot_size || !candidate->copy_scene_snapshot) {
			return false;
		}
		NAFExtProviderInfo info{ sizeof(info) };
		if (candidate->get_provider_info(&info) != NAFEXT_OK || info.abi_version != NAFEXT_ABI_VERSION_1) return false;
		if (strictProvider && (info.provider_id.length_bytes != std::strlen(NAFEXT_PROVIDER_ID) ||
			std::memcmp(info.provider_id.data, NAFEXT_PROVIDER_ID, info.provider_id.length_bytes) != 0)) return false;
		return true;
	}

	void NAFEXT_CALL OnSceneEvent(const NAFExtSceneEvent* event, void*)
	{
		if (!event || event->struct_size < sizeof(NAFExtSceneEvent)) return;
		// Copy IDs/strings before returning. event->snapshot is callback-lifetime only.
	}

	bool Acquire(const NAFExtAPIv1* candidate)
	{
		if (!Validate(candidate, false)) return false;
		api = candidate;
		return api->subscribe_scene_events(OnSceneEvent, nullptr, &subscription) == NAFEXT_OK;
	}

	void ReconcileLateScenes()
	{
		if (!api) return;
		std::uint32_t count{};
		std::uint64_t epoch{};
		if (api->enumerate_active_scenes(nullptr, 0, &count, &epoch) != NAFEXT_E_BUFFER_TOO_SMALL && count != 0) return;
		std::vector<NAFExtSceneID> ids(count);
		if (api->enumerate_active_scenes(ids.data(), count, &count, &epoch) != NAFEXT_OK) return;
		for (const auto id : ids) {
			std::size_t size{};
			if (api->get_scene_snapshot_size(id, 0, &size) != NAFEXT_OK) continue;
			std::vector<std::byte> storage(size);
			if (api->copy_scene_snapshot(id, 0, storage.data(), storage.size(), &size) != NAFEXT_OK) continue;
		}
	}

	void Shutdown()
	{
		if (api && subscription) api->unsubscribe_scene_events(subscription);
		subscription = 0;
		api = nullptr;
	}
}
