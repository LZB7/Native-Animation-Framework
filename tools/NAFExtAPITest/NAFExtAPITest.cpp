#include "F4SE/F4SE.h"
#include "NAFExtendedAPI.h"

#include <Windows.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <atomic>
#include <cstring>
#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace logger = F4SE::log;
using namespace std::literals;

namespace
{
	const NAFExtAPIv1* api{};
	NAFExtSubscription subscription{};
	std::mutex stateLock;
	std::mutex retainedLock;
	std::vector<std::byte> retainedSnapshot;
	std::atomic_bool reconciledDuringActiveScene{};
	std::atomic_bool deliberatelyUnregistered{};

	std::string ToString(NAFExtStringView value)
	{
		return value.data && value.length_bytes ? std::string(value.data, value.length_bytes) : std::string{};
	}

	const char* EventName(std::uint32_t kind)
	{
		switch (kind) {
		case NAFEXT_EVENT_SCENE_STARTED: return "SceneStarted";
		case NAFEXT_EVENT_SCENE_UPDATED: return "SceneUpdated";
		case NAFEXT_EVENT_SCENE_ENDING: return "SceneEnding";
		case NAFEXT_EVENT_SCENE_ENDED: return "SceneEnded";
		default: return "Unknown";
		}
	}

	bool Validate(const NAFExtAPIv1* candidate, bool strictProvider)
	{
		constexpr auto required = NAFEXT_CAP_SCENE_EVENTS | NAFEXT_CAP_SCENE_QUERY | NAFEXT_CAP_ACTIVE_ENUMERATION;
		if (!candidate || candidate->struct_size < sizeof(NAFExtAPIv1) || candidate->abi_version != NAFEXT_ABI_VERSION_1 ||
			(candidate->capabilities & required) != required || !candidate->get_provider_info || !candidate->subscribe_scene_events ||
			!candidate->unsubscribe_scene_events || !candidate->enumerate_active_scenes || !candidate->get_scene_snapshot_size ||
			!candidate->copy_scene_snapshot) {
			return false;
		}
		NAFExtProviderInfo info{ sizeof(info) };
		if (candidate->get_provider_info(&info) != NAFEXT_OK || info.abi_version != NAFEXT_ABI_VERSION_1) return false;
		return !strictProvider || ToString(info.provider_id) == NAFEXT_PROVIDER_ID;
	}

	bool ValidatePacked(const std::vector<std::byte>& bytes)
	{
		if (bytes.size() < sizeof(NAFExtSceneSnapshotCopy)) return false;
		const auto* snapshot = reinterpret_cast<const NAFExtSceneSnapshotCopy*>(bytes.data());
		auto rangeValid = [&](std::uint32_t offset, std::size_t length) {
			return offset <= bytes.size() && length <= bytes.size() - offset;
		};
		if (!rangeValid(snapshot->actors_offset, snapshot->actor_count * sizeof(NAFExtActorSlotCopy)) ||
			!rangeValid(snapshot->position_offset, snapshot->position_length) ||
			!rangeValid(snapshot->group_offset, snapshot->group_length) || !rangeValid(snapshot->tree_offset, snapshot->tree_length) ||
			!rangeValid(snapshot->metadata_offset, snapshot->metadata_count * sizeof(NAFExtMetadataCopy))) return false;

		const auto* actors = reinterpret_cast<const NAFExtActorSlotCopy*>(bytes.data() + snapshot->actors_offset);
		for (std::uint32_t i = 0; i < snapshot->actor_count; ++i) {
			if (!rangeValid(actors[i].display_name_offset, actors[i].display_name_length) ||
				!rangeValid(actors[i].animation_id_offset, actors[i].animation_id_length) ||
				!rangeValid(actors[i].tags_offset, actors[i].tag_count * sizeof(NAFExtStringCopy))) return false;
			const auto* tags = reinterpret_cast<const NAFExtStringCopy*>(bytes.data() + actors[i].tags_offset);
			for (std::uint32_t j = 0; j < actors[i].tag_count; ++j) if (!rangeValid(tags[j].data_offset, tags[j].length_bytes)) return false;
		}
		return true;
	}

	bool CopyAndLog(NAFExtSceneID id, std::uint64_t revision)
	{
		for (unsigned attempt = 0; attempt < 3; ++attempt) {
			std::size_t required{};
			auto result = api->get_scene_snapshot_size(id, revision, &required);
			if (result == NAFEXT_E_BUSY) continue;
			if (result != NAFEXT_OK) return false;
			std::vector<std::byte> bytes(required);
			result = api->copy_scene_snapshot(id, revision, bytes.data(), bytes.size(), &required);
			if (result == NAFEXT_E_BUSY || result == NAFEXT_E_BUFFER_TOO_SMALL) continue;
			if (result != NAFEXT_OK || !ValidatePacked(bytes)) return false;
			const auto* copy = reinterpret_cast<const NAFExtSceneSnapshotCopy*>(bytes.data());
			const std::string position(reinterpret_cast<const char*>(bytes.data() + copy->position_offset), copy->position_length);
			logger::info("Copied snapshot scene={} revision={} actors={} position='{}' bytes={} (self-contained validation passed)",
				copy->scene_id, copy->revision, copy->actor_count, position, bytes.size());
			{
				std::scoped_lock lock{ retainedLock };
				retainedSnapshot = std::move(bytes);
			}
			return true;
		}
		logger::warn("Snapshot scene={} changed repeatedly during sizing/copy; retry budget exhausted", id);
		return false;
	}

	void ReconcileLateScenes()
	{
		std::uint32_t count{};
		std::uint64_t epoch{};
		auto result = api->enumerate_active_scenes(nullptr, 0, &count, &epoch);
		if (result != NAFEXT_OK && result != NAFEXT_E_BUFFER_TOO_SMALL) {
			logger::error("Late enumeration sizing failed: {}", static_cast<unsigned>(result));
			return;
		}
		std::vector<NAFExtSceneID> ids(count);
		result = api->enumerate_active_scenes(ids.data(), count, &count, &epoch);
		if (result != NAFEXT_OK) {
			logger::error("Late enumeration copy failed: {}", static_cast<unsigned>(result));
			return;
		}
		logger::info("Late reconciliation epoch={} activeScenes={}", epoch, count);
		for (const auto id : ids) CopyAndLog(id, 0);
	}

	void NAFEXT_CALL OnSceneEvent(const NAFExtSceneEvent* event, void*)
	{
		if (!event || event->struct_size < sizeof(NAFExtSceneEvent)) return;
		{
			std::scoped_lock lock{ retainedLock };
			if (!retainedSnapshot.empty()) {
				logger::info("Previously copied snapshot remains valid after callback return: {}", ValidatePacked(retainedSnapshot));
			}
		}
		const auto* snapshot = event->snapshot;
		logger::info("{} scene={} revision={} phase={} reasons=0x{:X} actors={} position='{}'",
			EventName(event->kind), event->scene_id, event->revision, snapshot ? snapshot->phase : 0, event->update_reasons,
			snapshot ? snapshot->actor_count : 0, snapshot ? ToString(snapshot->position_id) : "");
		if (snapshot) {
			for (std::uint32_t i = 0; i < snapshot->actor_count; ++i) {
				const auto& actor = snapshot->actors[i];
				logger::info("  slot={} form={:08X} handle={:08X} name='{}' animation='{}' tags={}", actor.slot_index,
					actor.actor_form_id, actor.actor_handle, ToString(actor.display_name), ToString(actor.animation_id), actor.tag_count);
			}
			for (std::uint32_t i = 0; i < snapshot->metadata_count; ++i) {
				const auto& item = snapshot->metadata[i];
				logger::info("  metadata {}:{} bytes={}", ToString(item.name_space), ToString(item.key), item.data_size);
			}
			CopyAndLog(event->scene_id, event->revision);
		}

		// Deterministic subscribe-first late-enumeration and self-unregistration test.
		if (event->kind == NAFEXT_EVENT_SCENE_STARTED && !reconciledDuringActiveScene.exchange(true)) {
			const auto old = subscription;
			if (api->unsubscribe_scene_events(old) == NAFEXT_OK) {
				logger::info("Self-unregistration succeeded; resubscribing before active-scene enumeration");
				subscription = 0;
				if (api->subscribe_scene_events(OnSceneEvent, nullptr, &subscription) == NAFEXT_OK) ReconcileLateScenes();
			}
		}
		// The first completed scene deliberately disables further callbacks. Starting
		// another scene proves that successful unregistration suppresses delivery.
		if (event->kind == NAFEXT_EVENT_SCENE_ENDED && !deliberatelyUnregistered.exchange(true)) {
			if (api->unsubscribe_scene_events(subscription) == NAFEXT_OK) {
				logger::info("Final test unregistration succeeded; no later scene callbacks are expected");
				subscription = 0;
			}
		}
	}

	bool Acquire(const NAFExtAPIv1* candidate, std::string_view method)
	{
		std::scoped_lock lock{ stateLock };
		if (api) {
			logger::info("Duplicate API discovery via {} ignored", method);
			return true;
		}
		if (!Validate(candidate, false) || !Validate(candidate, true)) {
			logger::error("Rejected incompatible API provider discovered via {}", method);
			return false;
		}
		NAFExtProviderInfo info{ sizeof(info) };
		candidate->get_provider_info(&info);
		auto incompatible = *candidate;
		const bool nullRejected = !Validate(nullptr, false);
		incompatible.abi_version = NAFEXT_ABI_VERSION_1 + 1;
		const bool abiRejected = !Validate(&incompatible, false);
		incompatible = *candidate;
		incompatible.struct_size = sizeof(NAFExtAPIv1) - 1;
		const bool sizeRejected = !Validate(&incompatible, false);
		incompatible = *candidate;
		incompatible.capabilities &= ~NAFEXT_CAP_SCENE_QUERY;
		const bool capabilityRejected = !Validate(&incompatible, false);
		const bool identityMismatchRejected = std::string_view("Example.Other.Provider") != NAFEXT_PROVIDER_ID;
		logger::info("Controlled negotiation tests: unavailable={} unsupportedABI={} undersized={} missingCapability={} strictIdentityMismatch={}",
			nullRejected, abiRejected, sizeRejected, capabilityRejected, identityMismatchRejected);
		api = candidate;
		if (api->subscribe_scene_events(OnSceneEvent, nullptr, &subscription) != NAFEXT_OK) {
			api = nullptr;
			return false;
		}
		logger::info("Acquired API via {}: provider='{}' providerVersion={} ABI={} tableSize={} snapshotVersion={} capabilities=0x{:X}",
			method, ToString(info.provider_id), info.provider_version, info.abi_version, api->struct_size, info.snapshot_version, info.capabilities);
		return true;
	}

	void TryDirectAcquisition()
	{
		const auto module = GetModuleHandleW(L"NAF.dll");
		if (!module) {
			logger::info("Direct acquisition: NAF.dll is not loaded yet");
			return;
		}
		const auto getAPI = reinterpret_cast<NAFExtGetAPIFn>(GetProcAddress(module, "NAFExt_GetAPI"));
		if (!getAPI) {
			logger::warn("Direct acquisition: NAF.dll does not export NAFExt_GetAPI");
			return;
		}
		Acquire(getAPI(NAFEXT_ABI_VERSION_1, sizeof(NAFExtAPIv1)), "NAFExt_GetAPI export");
	}

	void OnNAFMessage(F4SE::MessagingInterface::Message* message)
	{
		if (!message || message->type != NAFEXT_DISCOVERY_MESSAGE_TYPE || message->dataLen < sizeof(NAFExtAnnouncement)) return;
		const auto* announcement = static_cast<const NAFExtAnnouncement*>(message->data);
		if (!announcement || announcement->struct_size < sizeof(NAFExtAnnouncement) || announcement->abi_version != NAFEXT_ABI_VERSION_1) return;
		Acquire(announcement->api, "F4SE announcement");
	}

	void OnF4SEMessage(F4SE::MessagingInterface::Message* message)
	{
		if (message && message->type == F4SE::MessagingInterface::kPostLoad) {
			TryDirectAcquisition();
			if (!api) logger::warn("NAF Extended provider unavailable at PostLoad; announcement listener remains active");
		}
	}
}

F4SE_EXPORT constinit F4SE::PluginVersionData F4SEPlugin_Version = []() constexpr {
	F4SE::PluginVersionData data{};
	data.PluginVersion({ 1, 0, 0 });
	data.PluginName("NAFExtAPITest");
	data.AuthorName("LZB7 / NAF Extended development");
	data.UsesAddressLibrary(false);
	data.UsesSigScanning(false);
	data.HasNoStructUse(true);
	data.CompatibleVersions({ F4SE::RUNTIME_1_11_240 });
	data.MinimumRequiredXSEVersion({ 0, 7, 9 });
	return data;
}();

extern "C" __declspec(dllexport) bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* f4se)
{
	F4SE::Init(f4se);
	auto path = logger::log_directory();
	if (!path) return false;
	*path /= "NAFExtAPITest.log";
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
	auto logger = std::make_shared<spdlog::logger>("NAFExtAPITest", std::move(sink));
	logger->set_level(spdlog::level::info);
	logger->flush_on(spdlog::level::info);
	spdlog::set_default_logger(std::move(logger));
	spdlog::set_pattern("[%m/%d/%Y - %T] [%^%l%$] %v");

	auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(OnF4SEMessage) || !messaging->RegisterListener(OnNAFMessage, "NAF"sv)) {
		logger::critical("Unable to register F4SE message listeners");
		return false;
	}
	logger::info("Harness loaded; waiting for direct acquisition or F4SE announcement");
	TryDirectAcquisition();
	return true;
}

extern "C" __declspec(dllexport) void NAFEXT_CALL NAFExtAPITest_Shutdown()
{
	std::scoped_lock lock{ stateLock };
	if (api && subscription) api->unsubscribe_scene_events(subscription);
	subscription = 0;
	api = nullptr;
}
