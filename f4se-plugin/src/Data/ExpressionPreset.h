#pragma once
#include <bitset>

namespace Data::ExpressionPreset
{
	struct Channel
	{
		std::uint8_t morph{};
		float value{};
	};

	struct Preset
	{
		std::string id;
		std::string displayName;
		std::vector<Channel> channels;
	};

	inline constexpr std::uint32_t kSchemaVersion = 1;
	inline std::shared_mutex presetLock;
	inline std::unordered_map<std::string, Preset, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> presets;

	inline std::optional<Preset> Get(std::string_view a_id)
	{
		std::shared_lock lock{ presetLock };
		if (const auto found = presets.find(std::string{ a_id }); found != presets.end()) return found->second;
		return std::nullopt;
	}

	inline bool ParseFile(const std::filesystem::path& a_path, bool a_verbose = true)
	{
		auto fail = [&](std::string_view a_reason) {
			if (a_verbose) logger::warn("Expression preset '{}' {}", a_path.generic_string(), a_reason);
			return false;
		};
		try {
			std::ifstream stream{ a_path, std::ios::binary };
			if (!stream) return fail("could not be opened");
			nlohmann::json root;
			stream >> root;
			if (!root.is_object() || root.value("format", std::string{}) != "NAFExtendedExpressionPreset") return fail("has an unsupported format");
			if (root.value("schemaVersion", 0u) != kSchemaVersion) return fail("has an unsupported schemaVersion");
			Preset preset;
			preset.id = root.value("id", std::string{});
			preset.displayName = root.value("displayName", preset.id);
			if (preset.id.empty() || preset.id.size() > 256 || preset.displayName.size() > 512) return fail("has an invalid ID or display name");
			if (!root.contains("channels") || !root["channels"].is_array() || root["channels"].empty() || root["channels"].size() > 54) return fail("requires 1-54 channels");
			std::bitset<54> used;
			for (const auto& item : root["channels"]) {
				if (!item.is_object() || !item.contains("morph") || !item["morph"].is_number_unsigned() || !item.contains("value") || !item["value"].is_number()) return fail("contains an invalid channel");
				const auto morph = item["morph"].get<std::uint32_t>();
				const auto value = item["value"].get<float>();
				if (morph >= 54 || !std::isfinite(value) || value < 0.0f || value > 1.0f || used.test(morph)) return fail("contains an out-of-range or duplicate channel");
				used.set(morph);
				preset.channels.push_back({ static_cast<std::uint8_t>(morph), value });
			}
			std::unique_lock lock{ presetLock };
			if (presets.contains(preset.id)) return fail("duplicates an existing preset ID; keeping the first definition");
			presets.emplace(preset.id, std::move(preset));
			return true;
		} catch (const std::exception& e) {
			if (a_verbose) logger::warn("Failed to parse expression preset '{}': {}", a_path.generic_string(), e.what());
			return false;
		}
	}

	inline void LoadFiles(const std::vector<std::filesystem::path>& a_files, bool a_verbose = true)
	{
		std::unique_lock lock{ presetLock };
		presets.clear();
		lock.unlock();
		std::size_t loaded = 0;
		for (const auto& file : a_files) if (ParseFile(file, a_verbose)) ++loaded;
		if (a_verbose) logger::info("Dynamic expression presets: {} definitions registered", loaded);
	}
}
