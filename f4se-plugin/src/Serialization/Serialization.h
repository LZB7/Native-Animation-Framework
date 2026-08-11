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

	void SaveCallback(const F4SE::SerializationInterface* a_intfc)
	{
		if (DISABLE_SERIALIZATION) {
			return;
		}

		Utility::StartPerformanceCounter();
		logger::info("Serialization save callback entered");

		{
			std::scoped_lock l{
				PackageOverride::lock,
				Menu::HUDManager::elementsLock,
				Menu::HUDManager::translationsLock,
				deferredRecordsLock
			};

			Serialization::General::s_intfc = a_intfc;

			SAVE_PERSISTENT_STATE('HUD', 5, "HUD", Menu::HUDManager::state);
			const bool hasDeferredPackage = std::ranges::any_of(deferredRecords, [](const DeferredRecord& a_record) {
				return a_record.type == 'PACK';
			});
			if (!hasDeferredPackage) {
				SAVE_PERSISTENT_STATE('PACK', 5, "package override", PackageOverride::state);
			} else {
				logger::warn("Preserving deferred package record unchanged; current-session package changes are not merged yet");
			}

			std::size_t deferredWritten = 0;
			for (const auto& record : deferredRecords) {
				if (!WriteDeferredRecord(a_intfc, record)) {
					logger::warn("Failed to preserve deferred co-save record 0x{:08X}", record.type);
				} else {
					++deferredWritten;
				}
			}

			Serialization::General::s_intfc = nullptr;
			logger::info("Serialization save completed: HUD active, package record {}, {} deferred records preserved",
				hasDeferredPackage ? "preserved pending restoration" : "written in historical v5 format", deferredWritten);
		}

		logger::info("Finished serialization in {:.3f}ms", Utility::GetPerformanceCounterMS());

	}

	void LoadCallback(const F4SE::SerializationInterface* a_intfc)
	{
		if (DISABLE_SERIALIZATION) {
			return;
		}

		Utility::StartPerformanceCounter();
		logger::info("Serialization load callback entered");

		{
			std::scoped_lock l{
				PackageOverride::lock,
				Menu::HUDManager::elementsLock,
				Menu::HUDManager::translationsLock,
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
				if (version >= 5 && type == 'HUD') {
					auto loadedState = std::make_unique<Menu::HUDManager::PersistentState>();
					loaded = LoadPayload("HUD", payload, loadedState);
					if (loaded) {
						Menu::HUDManager::state = std::move(loadedState);
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

	}

	void RevertCallback(const F4SE::SerializationInterface*)
	{
		logger::info("Serialization revert callback entered");
		PackageOverride::Reset();
		Menu::HUDManager::Reset();
		{
			std::scoped_lock l{ deferredRecordsLock };
			deferredRecords.clear();
		}
		logger::info("Serialization revert completed: active HUD/package state reset and deferred records cleared");
	}
}
