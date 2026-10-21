#include "NAFExtendedAPI.h"

#include <Windows.h>
#include <ShlObj.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace std::literals;

namespace F4SERaw
{
	constexpr std::uint32_t kMessaging = 1;
	constexpr std::uint32_t kPostLoadGame = 3;
	constexpr std::uint32_t kNewGame = 8;
	using PluginHandle = std::uint32_t;

	struct Interface
	{
		std::uint32_t f4seVersion;
		std::uint32_t runtimeVersion;
		std::uint32_t editorVersion;
		std::uint32_t isEditor;
		void* (__cdecl* QueryInterface)(std::uint32_t);
		PluginHandle(__cdecl* GetPluginHandle)();
		std::uint32_t(__cdecl* GetReleaseIndex)();
		const void* (__cdecl* GetPluginInfo)(const char*);
		const char* (__cdecl* GetSaveFolderName)();
	};

	struct Message
	{
		const char* sender;
		std::uint32_t type;
		std::uint32_t dataLen;
		void* data;
	};

	using MessageCallback = void(__cdecl*)(Message*);

	struct Messaging
	{
		std::uint32_t interfaceVersion;
		bool(__cdecl* RegisterListener)(PluginHandle, const char*, void*);
		bool(__cdecl* Dispatch)(PluginHandle, std::uint32_t, void*, std::uint32_t, const char*);
		void* (__cdecl* GetEventDispatcher)(std::uint32_t);
	};

	struct PluginVersionData
	{
		const std::uint32_t dataVersion{ 1 };
		std::uint32_t pluginVersion{};
		char pluginName[256]{};
		char author[256]{};
		std::uint32_t addressIndependence{};
		std::uint32_t structureIndependence{};
		std::uint32_t compatibleVersions[16]{};
		std::uint32_t xseMinimum{};
		const std::uint32_t reservedNonBreaking{};
		const std::uint32_t reservedBreaking{};
		const std::uint8_t reserved[512]{};
	};

	constexpr std::uint32_t PackVersion(std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d = 0)
	{
		return ((a & 0xFF) << 24) | ((b & 0xFF) << 16) | ((c & 0xFFF) << 4) | (d & 0xF);
	}

	static_assert(sizeof(PluginVersionData) == 0x45C);
}

namespace logger
{
	inline SRWLOCK lock = SRWLOCK_INIT;
	inline HANDLE file = INVALID_HANDLE_VALUE;

	void WriteRaw(std::string_view level, std::string_view message)
	{
		if (file == INVALID_HANDLE_VALUE) return;
		std::string line;
		line.reserve(level.size() + message.size() + 5);
		line.append("[").append(level).append("] ").append(message).append("\r\n");
		AcquireSRWLockExclusive(&lock);
		DWORD written{};
		WriteFile(file, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
		FlushFileBuffers(file);
		ReleaseSRWLockExclusive(&lock);
	}

	template <class... Args>
	void Write(std::string_view level, std::format_string<Args...> format, Args&&... args)
	{
		WriteRaw(level, std::format(format, std::forward<Args>(args)...));
	}

	template <class... Args> void info(std::format_string<Args...> f, Args&&... a) { Write("INFO", f, std::forward<Args>(a)...); }
	template <class... Args> void warn(std::format_string<Args...> f, Args&&... a) { Write("WARN", f, std::forward<Args>(a)...); }
	template <class... Args> void error(std::format_string<Args...> f, Args&&... a) { Write("ERROR", f, std::forward<Args>(a)...); }
	template <class... Args> void critical(std::format_string<Args...> f, Args&&... a) { Write("CRITICAL", f, std::forward<Args>(a)...); }

	bool Initialize()
	{
		wchar_t documents[MAX_PATH]{};
		if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documents))) return false;
		std::wstring directory = documents;
		directory += L"\\My Games\\Fallout4\\F4SE";
		CreateDirectoryW(directory.c_str(), nullptr);
		const auto path = directory + L"\\NAFExtAPITest.log";
		file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) return false;
		WriteRaw("INFO", "Logger initialized");
		return true;
	}
}

namespace
{
	const NAFExtAPIv1* api{};
	const NAFExtFaceAPIv1* faceAPI{};
	NAFExtSubscription subscription{};
	std::mutex stateLock;
	std::mutex retainedLock;
	std::vector<std::byte> retainedSnapshot;
	std::atomic_bool deliberatelyUnregistered{};
	std::atomic_bool controlsStarted{};
	std::atomic_bool controlsStop{};
	std::string faceAnimationA;
	std::string faceAnimationB;
	constexpr std::uint32_t kPlayerFormID = 0x14;

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

	bool ValidateFace(const NAFExtFaceAPIv1* candidate)
	{
		constexpr auto required = NAFEXT_CAP_FACE_PLAYBACK | NAFEXT_CAP_FACE_STOP | NAFEXT_CAP_FACE_RESET | NAFEXT_CAP_FACE_QUERY;
		return candidate && candidate->struct_size >= sizeof(NAFExtFaceAPIv1) &&
			candidate->service_version == NAFEXT_FACE_API_VERSION_1 && (candidate->capabilities & required) == required &&
			candidate->play_face_animation && candidate->stop_face_animation && candidate->reset_face_expression &&
			candidate->get_face_animation_state;
	}

	const char* FaceStatusName(std::uint32_t status)
	{
		switch (status) {
		case NAFEXT_FACE_IDLE: return "Idle";
		case NAFEXT_FACE_PLAY_PENDING: return "PlayPending";
		case NAFEXT_FACE_ACTIVE: return "Active";
		case NAFEXT_FACE_RESET_PENDING: return "ResetPending";
		case NAFEXT_FACE_STOPPED: return "Stopped";
		case NAFEXT_FACE_COMPLETED: return "Completed";
		case NAFEXT_FACE_CANCELED: return "Canceled";
		case NAFEXT_FACE_FAILED: return "Failed";
		default: return "Unknown";
		}
	}

	void QueryFaceState()
	{
		if (!faceAPI) return;
		NAFExtFaceState state{ sizeof(state) };
		std::uint32_t required{};
		auto result = faceAPI->get_face_animation_state(kPlayerFormID, &state, nullptr, 0, &required);
		std::vector<char> animation(required);
		if (result == NAFEXT_E_BUFFER_TOO_SMALL) {
			state.struct_size = sizeof(state);
			result = faceAPI->get_face_animation_state(kPlayerFormID, &state, animation.data(), required, &required);
		}
		const std::string id(animation.begin(), animation.end());
		logger::info("Face query result={} status={} generation={} flags=0x{:X} animation='{}'",
			static_cast<unsigned>(result), FaceStatusName(state.status), state.request_generation, state.flags, id);
	}

	void PlayFace(const std::string& id)
	{
		if (!faceAPI || id.empty()) {
			logger::warn("Face play skipped: facial service or configured animation ID is unavailable");
			return;
		}
		const NAFExtStringView view{ id.data(), static_cast<std::uint32_t>(id.size()) };
		logger::info("Face play '{}' accepted result={}", id,
			static_cast<unsigned>(faceAPI->play_face_animation(kPlayerFormID, view, 1, 0)));
	}

	void LoadFaceFixture()
	{
		std::ifstream input("Data/F4SE/Plugins/NAFExtAPITest.face.txt");
		auto nextID = [&]() {
			std::string value;
			while (std::getline(input, value)) if (!value.empty() && value.front() != '#') return value;
			return std::string{};
		};
		faceAnimationA = nextID();
		faceAnimationB = nextID();
		logger::info("Facial test fixture loaded: animationA='{}' animationB='{}'", faceAnimationA, faceAnimationB);
	}

	void FacialControlLoop()
	{
		LoadFaceFixture();
		logger::WriteRaw("INFO", "Facial controls ready: F6=play A, F7=play B, F8=query, F9=stop, F10=reset, F11=A then B, Insert=A then reset");
		while (!controlsStop.load(std::memory_order_acquire)) {
			if (GetAsyncKeyState(VK_F6) & 1) PlayFace(faceAnimationA);
			if (GetAsyncKeyState(VK_F7) & 1) PlayFace(faceAnimationB);
			if (GetAsyncKeyState(VK_F8) & 1) QueryFaceState();
			if (GetAsyncKeyState(VK_F9) & 1) logger::info("Face stop accepted result={}", static_cast<unsigned>(faceAPI->stop_face_animation(kPlayerFormID)));
			if (GetAsyncKeyState(VK_F10) & 1) logger::info("Face reset accepted result={}", static_cast<unsigned>(faceAPI->reset_face_expression(kPlayerFormID)));
			if (GetAsyncKeyState(VK_F11) & 1) { PlayFace(faceAnimationA); PlayFace(faceAnimationB); }
			if (GetAsyncKeyState(VK_INSERT) & 1) { PlayFace(faceAnimationA); faceAPI->reset_face_expression(kPlayerFormID); }
			Sleep(50);
		}
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
		ReconcileLateScenes();
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
		const auto getFaceAPI = reinterpret_cast<NAFExtGetFaceAPIFn>(GetProcAddress(module, "NAFExt_GetFaceAPI"));
		if (!getFaceAPI || !ValidateFace(getFaceAPI(NAFEXT_FACE_API_VERSION_1, sizeof(NAFExtFaceAPIv1)))) {
			logger::warn("Optional facial service is unavailable or incompatible");
			return;
		}
		faceAPI = getFaceAPI(NAFEXT_FACE_API_VERSION_1, sizeof(NAFExtFaceAPIv1));
		auto incompatibleFace = *faceAPI;
		incompatibleFace.service_version = NAFEXT_FACE_API_VERSION_1 + 1;
		const bool faceVersionRejected = !ValidateFace(&incompatibleFace);
		incompatibleFace = *faceAPI;
		incompatibleFace.struct_size = sizeof(NAFExtFaceAPIv1) - 1;
		const bool faceSizeRejected = !ValidateFace(&incompatibleFace);
		incompatibleFace = *faceAPI;
		incompatibleFace.capabilities &= ~NAFEXT_CAP_FACE_RESET;
		const bool faceCapabilityRejected = !ValidateFace(&incompatibleFace);
		logger::info("Facial negotiation tests: unavailable={} unsupportedVersion={} undersized={} missingCapability={}",
			!ValidateFace(nullptr), faceVersionRejected, faceSizeRejected, faceCapabilityRejected);
		logger::info("Acquired facial service v{} tableSize={} capabilities=0x{:X}", faceAPI->service_version,
			faceAPI->struct_size, faceAPI->capabilities);
		if (!controlsStarted.exchange(true)) std::thread(FacialControlLoop).detach();
	}

	void __cdecl OnF4SEMessage(F4SERaw::Message* message)
	{
		if (message && (message->type == F4SERaw::kNewGame ||
			(message->type == F4SERaw::kPostLoadGame && static_cast<bool>(message->data)))) {
			logger::WriteRaw("INFO", "Game entered; acquiring NAF Extended API");
			TryDirectAcquisition();
			if (!api) logger::warn("NAF Extended provider unavailable after entering the game");
		}
	}
}

extern "C" __declspec(dllexport) constinit F4SERaw::PluginVersionData F4SEPlugin_Version = []() constexpr {
	F4SERaw::PluginVersionData data{};
	data.pluginVersion = F4SERaw::PackVersion(1, 0, 0);
	constexpr std::string_view name{ "NAFExtAPITest" };
	constexpr std::string_view author{ "LZB7 / NAF Extended development" };
	for (std::size_t i = 0; i < name.size(); ++i) data.pluginName[i] = name[i];
	for (std::size_t i = 0; i < author.size(); ++i) data.author[i] = author[i];
	data.structureIndependence = 1;  // no game structures
	data.compatibleVersions[0] = F4SERaw::PackVersion(1, 11, 240);
	data.xseMinimum = F4SERaw::PackVersion(0, 7, 9);
	return data;
}();

extern "C" __declspec(dllexport) bool __cdecl F4SEPlugin_Load(const F4SERaw::Interface* f4se)
{
	if (!f4se || !logger::Initialize() || f4se->isEditor || !f4se->QueryInterface || !f4se->GetPluginHandle) return false;
	logger::WriteRaw("INFO", "F4SEPlugin_Load entered");
	const auto messaging = static_cast<F4SERaw::Messaging*>(f4se->QueryInterface(F4SERaw::kMessaging));
	const auto handle = f4se->GetPluginHandle();
	logger::WriteRaw("INFO", "Registering F4SE listener");
	if (!messaging || !messaging->RegisterListener ||
		!messaging->RegisterListener(handle, "F4SE", reinterpret_cast<void*>(OnF4SEMessage))) {
		logger::WriteRaw("CRITICAL", "Unable to register F4SE listener");
		return false;
	}
	logger::WriteRaw("INFO", "Harness load complete; API acquisition deferred until a save or new game is entered");
	return true;
}

extern "C" __declspec(dllexport) void NAFEXT_CALL NAFExtAPITest_Shutdown()
{
	controlsStop.store(true, std::memory_order_release);
	std::scoped_lock lock{ stateLock };
	if (api && subscription) api->unsubscribe_scene_events(subscription);
	subscription = 0;
	api = nullptr;
}
