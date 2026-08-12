#pragma once
#include "General.h"

#define LOAD_PERSISTENT_STATE(_recordId, _recordName, _typePath) \
case _recordId: \
{ \
	auto state = std::make_unique<_typePath::PersistentState>(); \
	if (Serialization::General::LoadRecord(_recordName, state)) { \
		_typePath::state.reset(state.release()); \
	} \
	break; \
}

#define LOAD_PERSISTENT_STATE_CL(_recordId, _recordName, _typePath, _statePath) \
	case _recordId:                                                          \
		{                                                                    \
			auto state = std::make_unique<_typePath::PersistentState>();     \
			if (Serialization::General::LoadRecord(_recordName, state)) {    \
				_statePath.reset(state.release());                           \
			}                                                                \
			break;                                                           \
		}

#define LOAD_PERSISTENT_STATE_NP(_recordId, _recordName, _typePath, _statePath) \
	case _recordId:                                                          \
		{                                                                    \
			_typePath state;     \
			if (Serialization::General::LoadRecord(_recordName, state)) {    \
				_statePath = std::move(state);                                \
			}                                                                \
			break;                                                           \
		}

#define SAVE_PERSISTENT_STATE(_recordId, _ver, _recordName, _statePath) \
if (!a_intfc->OpenRecord(_recordId, _ver)) { \
logger::error("Failed to open co-save {} record. Some data will not be saved!", _recordName); \
} \
else \
{ \
	Serialization::General::SaveRecord(_recordName, _statePath); \
} \
	

namespace Serialization
{
	constexpr auto DISABLE_SERIALIZATION{ false };

	struct DeferredRecord
	{
		std::uint32_t type;
		std::uint32_t version;
		std::vector<std::uint8_t> payload;
	};

	inline std::vector<DeferredRecord> deferredRecords;
	inline std::mutex deferredRecordsLock;
	inline std::atomic<std::uint32_t> sceneFixupGeneration{ 0 };

	[[nodiscard]] constexpr bool IsActivelyRestoredRecord(std::uint32_t a_type)
	{
		switch (a_type) {
		case 'TASK':
		case 'SCNE':
		case 'EQPT':
		case 'UID':
		case 'FACE':
		case 'HUD':
		case 'PACK':
		case 'SHUD':
			return true;
		default:
			return false;
		}
	}

	[[nodiscard]] inline bool ReadRecordPayload(const F4SE::SerializationInterface* a_intfc,
		std::uint32_t a_length, std::vector<std::uint8_t>& a_payload)
	{
		a_payload.resize(a_length);
		return a_length == 0 || a_intfc->ReadRecordData(a_payload.data(), a_length) == a_length;
	}

	template <class T>
	[[nodiscard]] bool LoadPayload(std::string_view a_name, const std::vector<std::uint8_t>& a_payload, T& a_output)
	{
		if (a_payload.size() < sizeof(std::uint32_t)) {
			logger::warn("{} record is shorter than its historical size prefix", a_name);
			return false;
		}

		std::uint32_t serializedSize = 0;
		std::memcpy(&serializedSize, a_payload.data(), sizeof(serializedSize));
		if (serializedSize != a_payload.size() - sizeof(serializedSize)) {
			logger::warn("{} record size prefix {} does not match payload size {}", a_name,
				serializedSize, a_payload.size() - sizeof(serializedSize));
			return false;
		}

		std::istringstream buffer(
			std::string(reinterpret_cast<const char*>(a_payload.data() + sizeof(serializedSize)), serializedSize),
			std::ios::binary);
		try {
			cereal::BinaryInputArchive archive(buffer);
			archive(a_output);
		} catch (const std::exception& e) {
			logger::warn("Failed to read {} record data: {}", a_name, e.what());
			return false;
		}
		return true;
	}

	inline bool WriteDeferredRecord(const F4SE::SerializationInterface* a_intfc, const DeferredRecord& a_record)
	{
		if (!a_intfc->OpenRecord(a_record.type, a_record.version)) {
			return false;
		}
		return a_record.payload.empty() ||
			a_intfc->WriteRecordData(a_record.payload.data(), static_cast<std::uint32_t>(a_record.payload.size()));
	}

	void RunRestoredSceneFixup(std::uint32_t a_generation)
	{
		if (sceneFixupGeneration.load(std::memory_order_acquire) != a_generation) {
			return;
		}
		auto main = RE::Main::GetSingleton();
		if (!main || !main->gameActive) {
			F4SE::GetTaskInterface()->AddTask([a_generation]() { RunRestoredSceneFixup(a_generation); });
			return;
		}
		FaceAnimation::FaceUpdateHook::FixupAfterSerialization();
		Scene::SceneManager::FixupRestoredScenes();
		Tasks::TimerThread::GetSingleton()->Start();
	}

	void SaveCallback(const F4SE::SerializationInterface* a_intfc)
	{
		if (DISABLE_SERIALIZATION) {
			return;
		}

		Utility::StartPerformanceCounter();
		logger::info("Serialization save callback entered");
		auto timer = Tasks::TimerThread::GetSingleton();
		timer->Stop();

		{
			std::scoped_lock l{
				Scene::SceneManager::scenesMapLock,
				Scene::SceneManager::actorsWalkingLock,
				timer->timerLock,
				FaceAnimation::FaceUpdateHook::loadingAnimsLock,
				FaceAnimation::FaceUpdateHook::stateLock,
				Data::Uid::lock,
				PackageOverride::lock,
				Menu::HUDManager::elementsLock,
				Menu::HUDManager::translationsLock,
				Scene::OrderedActionQueue::lock,
				Menu::SceneHUD::lock,
				deferredRecordsLock
			};

			Serialization::General::s_intfc = a_intfc;

			SAVE_PERSISTENT_STATE('TASK', 5, "task", timer->state);
			SAVE_PERSISTENT_STATE('SCNE', 5, "scene", Scene::SceneManager::state);
			SAVE_PERSISTENT_STATE('EQPT', 5, "equipment", Scene::OrderedActionQueue::state);
			SAVE_PERSISTENT_STATE('UID', 5, "UID", Data::Uid::state);
			SAVE_PERSISTENT_STATE('FACE', 5, "face animation", FaceAnimation::FaceUpdateHook::state);
			SAVE_PERSISTENT_STATE('HUD', 5, "HUD", Menu::HUDManager::state);
			SAVE_PERSISTENT_STATE('PACK', 5, "package override", PackageOverride::state);
			SAVE_PERSISTENT_STATE('SHUD', 5, "scene HUD", Menu::SceneHUD::state);

			std::size_t deferredWritten = 0;
			for (const auto& record : deferredRecords) {
				if (IsActivelyRestoredRecord(record.type)) {
					continue;
				}
				if (!WriteDeferredRecord(a_intfc, record)) {
					logger::warn("Failed to preserve deferred co-save record 0x{:08X}", record.type);
				} else {
					++deferredWritten;
				}
			}

			Serialization::General::s_intfc = nullptr;
			logger::info("Serialization save completed: eight historical v5 production records written, {} advanced records preserved", deferredWritten);
		}

		logger::info("Finished serialization in {:.3f}ms", Utility::GetPerformanceCounterMS());
		timer->Start();

	}

	void LoadCallback(const F4SE::SerializationInterface* a_intfc)
	{
		if (DISABLE_SERIALIZATION) {
			return;
		}

		Utility::StartPerformanceCounter();
		logger::info("Serialization load callback entered");
		auto timer = Tasks::TimerThread::GetSingleton();
		timer->Stop();

		{
			std::scoped_lock l{
				Scene::SceneManager::scenesMapLock,
				Scene::SceneManager::actorsWalkingLock,
				timer->timerLock,
				FaceAnimation::FaceUpdateHook::loadingAnimsLock,
				FaceAnimation::FaceUpdateHook::stateLock,
				Data::Uid::lock,
				PackageOverride::lock,
				Menu::HUDManager::elementsLock,
				Menu::HUDManager::translationsLock,
				Scene::OrderedActionQueue::lock,
				Menu::SceneHUD::lock,
				deferredRecordsLock
			};

			Serialization::General::s_intfc = a_intfc;
			deferredRecords.clear();

			uint32_t type;
			uint32_t length;
			uint32_t version;

			std::size_t activeLoaded = 0;
			std::size_t deferredLoaded = 0;
			std::size_t skipped = 0;
			while (a_intfc->GetNextRecordInfo(type, version, length)) {
				std::vector<std::uint8_t> payload;
				if (!ReadRecordPayload(a_intfc, length, payload)) {
					logger::warn("Failed to read co-save record 0x{:08X}; record skipped", type);
					++skipped;
					continue;
				}

				bool loaded = false;
				if (version >= 5) {
					switch (type) {
					case 'TASK': {
						auto value = std::make_unique<Tasks::TimerThread::PersistentState>();
						loaded = LoadPayload("task", payload, value);
						if (loaded) timer->state = std::move(value);
						break;
					}
					case 'SCNE': {
						auto value = std::make_unique<Scene::SceneManager::PersistentState>();
						loaded = LoadPayload("scene", payload, value);
						if (loaded) {
							for (auto iter = value->scenes.begin(); iter != value->scenes.end();) {
								if (!iter->second) {
									logger::warn("Discarding null scene {} from the co-save", iter->first);
									iter = value->scenes.erase(iter);
									continue;
								}
								iter->second->noUpdate = true;
								++iter;
							}
							Scene::SceneManager::state = std::move(value);
						}
						break;
					}
					case 'EQPT': {
						auto value = std::make_unique<Scene::OrderedActionQueue::PersistentState>();
						loaded = LoadPayload("equipment", payload, value);
						if (loaded) Scene::OrderedActionQueue::state = std::move(value);
						break;
					}
					case 'UID': {
						auto value = std::make_unique<Data::Uid::PersistentState>();
						loaded = LoadPayload("UID", payload, value);
						if (loaded) Data::Uid::state = std::move(value);
						break;
					}
					case 'FACE': {
						auto value = std::make_unique<FaceAnimation::FaceUpdateHook::PersistentState>();
						loaded = LoadPayload("face animation", payload, value);
						if (loaded) FaceAnimation::FaceUpdateHook::state = std::move(value);
						break;
					}
					case 'HUD': {
						auto value = std::make_unique<Menu::HUDManager::PersistentState>();
						loaded = LoadPayload("HUD", payload, value);
						if (loaded) Menu::HUDManager::state = std::move(value);
						break;
					}
					case 'PACK': {
						auto value = std::make_unique<PackageOverride::PersistentState>();
						loaded = LoadPayload("package override", payload, value);
						if (loaded) PackageOverride::state = std::move(value);
						break;
					}
					case 'SHUD': {
						auto value = std::make_unique<Menu::SceneHUD::PersistentState>();
						loaded = LoadPayload("scene HUD", payload, value);
						if (loaded) Menu::SceneHUD::state = std::move(value);
						break;
					}
					}
				}

				if (loaded) {
					++activeLoaded;
				} else {
					deferredRecords.push_back({ type, version, std::move(payload) });
					++deferredLoaded;
				}
			}

			Serialization::General::s_intfc = nullptr;
			logger::info("Serialization load completed: {} active records, {} deferred records preserved, {} records skipped",
				activeLoaded, deferredLoaded, skipped);
		}

		logger::info("Finished deserialization in {:.3f}ms", Utility::GetPerformanceCounterMS());
		const auto fixupGeneration = sceneFixupGeneration.fetch_add(1, std::memory_order_acq_rel) + 1;
		F4SE::GetTaskInterface()->AddTask([fixupGeneration]() { RunRestoredSceneFixup(fixupGeneration); });

	}

	void RevertCallback(const F4SE::SerializationInterface*)
	{
		logger::info("Serialization revert callback entered");
		sceneFixupGeneration.fetch_add(1, std::memory_order_acq_rel);
		Tasks::TimerThread::GetSingleton()->Reset();
		PackageOverride::Reset();
		FaceAnimation::FaceUpdateHook::Reset();
		Scene::SceneManager::Reset();
		Scene::OrderedActionQueue::Reset();
		Data::Uid::Reset();
		Menu::HUDManager::Reset();
		Menu::SceneHUD::Reset();
		{
			std::scoped_lock l{ deferredRecordsLock };
			deferredRecords.clear();
		}
		logger::info("Serialization revert completed: active scene/task/equipment/face/HUD/package state reset and advanced records cleared");
	}
}
