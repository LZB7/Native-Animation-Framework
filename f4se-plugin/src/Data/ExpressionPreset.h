#pragma once
#include <array>
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
	inline constexpr std::array<std::string_view, 54> kMorphNames{
		"Brow Squeeze",
		"Jaw Forward",
		"Jaw Open",
		"Left Outer Brow Up",
		"Left Cheek Up",
		"Left Frown",
		"Jaw Left",
		"Left Corner In",
		"Left Corner Out",
		"Left Lower Lid Down",
		"Left Lower Lid Up",
		"Left Lower Lip Down",
		"Left Lower Lip Up",
		"Left Middle Brow Down",
		"Left Middle Brow Up",
		"Left Nose Up",
		"Left Outer Brow Down",
		"Left Smile",
		"Left Upper Lid Down",
		"Left Upper Lid Up",
		"Left Upper Lip Down",
		"Left Upper Lip Up",
		"Lower Funnel",
		"Lower Roll In",
		"Lower Roll Out",
		"Pucker",
		"Right Outer Brow Up",
		"Right Cheek Up",
		"Right Frown",
		"Jaw Right",
		"Right Corner In",
		"Right Corner Out",
		"Right Lower Lid Down",
		"Right Lower Lid Up",
		"Right Lower Lip Down",
		"Right Lower Lip Up",
		"Right Middle Brow Down",
		"Right Middle Brow Up",
		"Right Nose Up",
		"Right Outer Brow Down",
		"Right Smile",
		"Right Upper Lid Down",
		"Right Upper Lid Up",
		"Right Upper Lip Down",
		"Right Upper Lip Up",
		"Sticky",
		"Upper Funnel",
		"Upper Roll In",
		"Upper Roll Out",
		"Tongue",
		"Extra 1",
		"Extra 2",
		"Extra 3",
		"Extra 4"
	};
	inline std::shared_mutex presetLock;
	inline std::unordered_map<std::string, Preset, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> presets;

	inline std::optional<Preset> Get(std::string_view a_id)
	{
		std::shared_lock lock{ presetLock };
		if (const auto found = presets.find(std::string{ a_id }); found != presets.end()) return found->second;
		return std::nullopt;
	}

	inline std::optional<std::uint8_t> ResolveMorph(const nlohmann::json& a_identifier)
	{
		if (a_identifier.is_number_unsigned()) {
			const auto value = a_identifier.get<std::uint32_t>();
			if (value < kMorphNames.size()) return static_cast<std::uint8_t>(value);
			return std::nullopt;
		}
		if (a_identifier.is_string()) {
			const auto name = a_identifier.get<std::string>();
			for (std::size_t i = 0; i < kMorphNames.size(); ++i) {
				if (name == kMorphNames[i]) return static_cast<std::uint8_t>(i);
			}
		}
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
				if (!item.is_object() || !item.contains("morph") || !item.contains("value") || !item["value"].is_number()) return fail(std::format("preset '{}' contains an invalid channel", preset.id));
				const auto morph = ResolveMorph(item["morph"]);
				if (!morph) {
					if (item["morph"].is_string()) return fail(std::format("preset '{}' contains unknown FaceGen morph '{}'", preset.id, item["morph"].get<std::string>()));
					return fail(std::format("preset '{}' contains an invalid FaceGen morph identifier", preset.id));
				}
				const auto value = item["value"].get<float>();
				if (!std::isfinite(value) || value < 0.0f || value > 1.0f) return fail(std::format("preset '{}' contains an invalid channel value", preset.id));
				if (used.test(*morph)) return fail(std::format("preset '{}' duplicates FaceGen morph {} ('{}')", preset.id, *morph, kMorphNames[*morph]));
				used.set(*morph);
				preset.channels.push_back({ *morph, value });
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
