#pragma once

namespace JSONAnimation
{
	struct MetadataRecord
	{
		std::string nameSpace;
		std::string key;
		std::string jsonValue;
	};

	inline std::shared_mutex metadataLock;
	inline std::unordered_map<std::string, std::vector<MetadataRecord>, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> metadata;

	inline void Clear()
	{
		std::unique_lock lock{ metadataLock };
		metadata.clear();
	}

	inline std::vector<MetadataRecord> GetMetadata(const std::string& a_positionID)
	{
		std::shared_lock lock{ metadataLock };
		if (const auto it = metadata.find(a_positionID); it != metadata.end()) {
			return it->second;
		}
		return {};
	}

	inline std::optional<ActorGender> ParseGender(const std::string& a_value)
	{
		if (_stricmp(a_value.c_str(), "any") == 0) {
			return ActorGender::Any;
		}
		if (_stricmp(a_value.c_str(), "male") == 0) {
			return ActorGender::Male;
		}
		if (_stricmp(a_value.c_str(), "female") == 0) {
			return ActorGender::Female;
		}
		return std::nullopt;
	}

	inline bool ParseFile(const std::filesystem::path& a_path, bool a_verbose = true)
	{
		std::ifstream stream{ a_path, std::ios::binary };
		if (!stream) {
			if (a_verbose) logger::warn("Failed to open JSON animation definition '{}'", a_path.generic_string());
			return false;
		}

		nlohmann::json root;
		try {
			stream >> root;
		} catch (const std::exception& e) {
			if (a_verbose) logger::warn("Failed to parse JSON animation definition '{}': {}", a_path.generic_string(), e.what());
			return false;
		}

		if (!root.is_object() || root.value("format", std::string{}) != "NAFExtendedAnimation") {
			if (a_verbose) logger::warn("JSON animation definition '{}' has an unsupported format", a_path.generic_string());
			return false;
		}
		if (root.value("schemaVersion", 0u) != 1u) {
			if (a_verbose) logger::warn("JSON animation definition '{}' has unsupported schemaVersion {}", a_path.generic_string(), root.value("schemaVersion", 0u));
			return false;
		}
		if (!root.contains("animation") || !root["animation"].is_object() || !root.contains("position") || !root["position"].is_object()) {
			if (a_verbose) logger::warn("JSON animation definition '{}' requires animation and position objects", a_path.generic_string());
			return false;
		}

		const auto& animationNode = root["animation"];
		const auto& positionNode = root["position"];
		const auto animationID = animationNode.value("id", std::string{});
		const auto positionID = positionNode.value("id", animationID);
		if (animationID.empty() || positionID.empty() || !animationNode.contains("actors") || !animationNode["actors"].is_array() || animationNode["actors"].empty()) {
			if (a_verbose) logger::warn("JSON animation definition '{}' has no usable ID or actor slots", a_path.generic_string());
			return false;
		}
		if (Global::Animations.get_ptr(animationID) || Global::Positions.get_ptr(positionID)) {
			if (a_verbose) logger::warn("JSON animation definition '{}' conflicts with existing animation/position ID '{}'/'{}'; keeping the first loaded definition", a_path.generic_string(), animationID, positionID);
			return false;
		}

		auto animation = std::make_shared<Animation>();
		animation->id = animationID;
		animation->loadPriority = root.value("loadPriority", 0);
		for (const auto& actorNode : animationNode["actors"]) {
			if (!actorNode.is_object()) {
				return false;
			}
			const auto file = actorNode.value("file", std::string{});
			const auto skeleton = actorNode.value("skeleton", std::string{ "Human" });
			const auto genderText = actorNode.value("gender", std::string{ "Any" });
			const auto gender = ParseGender(genderText);
			if (file.empty() || skeleton.empty() || !gender) {
				if (a_verbose) logger::warn("JSON animation definition '{}' contains an invalid actor slot", a_path.generic_string());
				return false;
			}
			auto& slot = animation->slots.emplace_back();
			slot.dynamicIdle = true;
			slot.behaviorRequiresConvert = true;
			slot.idleRequiresConvert = false;
			slot.rootBehavior = skeleton;
			slot.gender = *gender;
			slot.idle[0] = file;
			slot.idle[1] = "default";
		}

		auto position = std::make_shared<Position>();
		position->id = positionID;
		position->idForType = animationID;
		position->loadPriority = animation->loadPriority;
		position->posType = Position::kAnimation;
		position->hidden = positionNode.value("hidden", false);
		if (positionNode.contains("tags") && positionNode["tags"].is_array()) {
			for (const auto& tag : positionNode["tags"]) {
				if (tag.is_string()) {
					auto value = tag.get<std::string>();
					Utility::TransformStringToLower(value);
					position->tags.emplace(std::move(value));
				}
			}
		}

		std::vector<MetadataRecord> records;
		if (const auto extensionIt = root.find("extensions"); extensionIt != root.end() && extensionIt->is_object()) {
			for (const auto& [nameSpace, value] : extensionIt->items()) {
				if (value.is_object()) {
					for (const auto& [key, item] : value.items()) {
						records.push_back({ nameSpace, key, item.dump() });
					}
				} else {
					records.push_back({ nameSpace, "value", value.dump() });
				}
			}
		}

		Global::Animations.priority_insert(animation);
		Global::Positions.priority_insert(position);
		{
			std::unique_lock lock{ metadataLock };
			metadata[positionID] = std::move(records);
		}
		return true;
	}
}
