#pragma once

namespace JSONAnimation
{
	// Extended JSON is normalized into the original Animation/Position model so
	// XML and JSON content share one playback path. The cache stores normalized,
	// pointer-free JSON only; engine forms and runtime objects are resolved after
	// registration just as they are for XML content.
	struct MetadataRecord { std::string nameSpace; std::string key; std::string jsonValue; };
	struct SourceRecord { std::string path; std::uint64_t size{}; std::int64_t modified{}; std::uint64_t fingerprint{}; };

	inline constexpr std::uint32_t kCacheVersion = 2;
	inline constexpr std::uint32_t kSchemaVersion = 1;
	inline constexpr std::uint32_t kPackSchemaVersion = 2;
	inline constexpr std::uint64_t kMaxCacheBytes = 512ull * 1024ull * 1024ull;
	inline constexpr std::size_t kMaxDefinitions = 100000;
	inline constexpr std::size_t kMaxTreeNodes = 100000;
	inline const std::filesystem::path cachePath{ USERDATA_DIR + "NAFPackCache.bin" };
	inline std::shared_mutex metadataLock;
	inline std::unordered_map<std::string, std::vector<MetadataRecord>, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> metadata;
	inline std::unordered_map<std::string, std::vector<MetadataRecord>, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> animationMetadata;
	inline std::unordered_map<std::string, std::vector<MetadataRecord>, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> groupMetadata;
	inline std::unordered_map<std::string, std::vector<std::vector<MetadataRecord>>, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> stageMetadata;
	inline std::unordered_map<std::string, std::vector<MetadataRecord>, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> treeMetadata;
	inline std::unordered_map<std::string, std::vector<MetadataRecord>, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> treeNodeMetadata;
	inline std::unordered_map<std::string, std::string, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> displayNames;

	inline void Clear() { std::unique_lock lock{ metadataLock }; metadata.clear(); animationMetadata.clear(); groupMetadata.clear(); stageMetadata.clear(); treeMetadata.clear(); treeNodeMetadata.clear(); displayNames.clear(); }
	inline std::vector<MetadataRecord> GetMetadata(const std::string& a_positionID, const std::string& a_animationID, const std::string& a_groupID, std::uint32_t a_stageIndex, const std::string& a_treeID, const std::string& a_treeNodeID) {
		std::shared_lock lock{ metadataLock }; std::vector<MetadataRecord> result;
		auto append = [&](const auto& map, const std::string& id) { if (!id.empty()) if (const auto it = map.find(id); it != map.end()) result.insert(result.end(), it->second.begin(), it->second.end()); };
		append(metadata, a_positionID); append(animationMetadata, a_animationID); append(groupMetadata, a_groupID);
		if (!a_groupID.empty()) if (const auto it = stageMetadata.find(a_groupID); it != stageMetadata.end() && a_stageIndex < it->second.size()) result.insert(result.end(), it->second[a_stageIndex].begin(), it->second[a_stageIndex].end());
		append(treeMetadata, a_treeID); append(treeNodeMetadata, a_treeID.empty() || a_treeNodeID.empty() ? std::string{} : a_treeID + "\x1F" + a_treeNodeID);
		return result;
	}
	inline std::string GetDisplayName(const std::string& a_id) {
		std::shared_lock lock{ metadataLock }; if (const auto it = displayNames.find(a_id); it != displayNames.end()) return it->second; return a_id;
	}

	inline std::optional<ActorGender> ParseGender(const std::string& a_value) {
		if (_stricmp(a_value.c_str(), "any") == 0) return ActorGender::Any;
		if (_stricmp(a_value.c_str(), "male") == 0) return ActorGender::Male;
		if (_stricmp(a_value.c_str(), "female") == 0) return ActorGender::Female;
		return std::nullopt;
	}
	inline std::optional<std::string> ParseExpectedBodyType(const std::string& a_value) {
		if (_stricmp(a_value.c_str(), "any") == 0) return "Any";
		if (_stricmp(a_value.c_str(), "masc") == 0) return "Masc";
		if (_stricmp(a_value.c_str(), "femme") == 0) return "Femme";
		return std::nullopt;
	}
	inline std::optional<std::string> ParseRole(const std::string& a_value) {
		if (_stricmp(a_value.c_str(), "giver") == 0) return "giver";
		if (_stricmp(a_value.c_str(), "receiver") == 0) return "receiver";
		return std::nullopt;
	}

	inline std::uint64_t HashBytes(const void* a_data, std::size_t a_size) {
		const auto* bytes = static_cast<const std::uint8_t*>(a_data); std::uint64_t hash = 14695981039346656037ull;
		for (std::size_t i = 0; i < a_size; ++i) { hash ^= bytes[i]; hash *= 1099511628211ull; } return hash;
	}
	inline std::optional<std::uint64_t> HashFile(const std::filesystem::path& a_path) {
		std::ifstream stream{ a_path, std::ios::binary }; if (!stream) return std::nullopt;
		std::uint64_t hash = 14695981039346656037ull; std::array<char, 64 * 1024> buffer{};
		while (stream) { stream.read(buffer.data(), buffer.size()); const auto count = static_cast<std::size_t>(stream.gcount());
			for (std::size_t i = 0; i < count; ++i) { hash ^= static_cast<std::uint8_t>(buffer[i]); hash *= 1099511628211ull; } }
		return hash;
	}
	inline bool BuildManifest(const std::vector<std::filesystem::path>& a_files, std::vector<SourceRecord>& a_result) {
		// Size/time make the common path cheap to diagnose, while the fingerprint
		// prevents a same-size/same-timestamp edit from reusing stale content.
		a_result.clear(); a_result.reserve(a_files.size());
		try { for (const auto& path : a_files) { const auto fingerprint = HashFile(path); if (!fingerprint) return false;
			a_result.push_back({ std::filesystem::relative(path, USERDATA_DIR).lexically_normal().generic_string(), std::filesystem::file_size(path),
				static_cast<std::int64_t>(std::filesystem::last_write_time(path).time_since_epoch().count()), *fingerprint }); } }
		catch (...) { return false; } return true;
	}

	inline void AddExtensions(std::vector<MetadataRecord>& a_records, const nlohmann::json& a_extensions) {
		if (!a_extensions.is_object()) return;
		for (const auto& [nameSpace, value] : a_extensions.items()) {
			if (value.is_object()) for (const auto& [key, item] : value.items()) a_records.push_back({ nameSpace, key, item.dump() });
			else a_records.push_back({ nameSpace, "value", value.dump() });
		}
	}

	inline nlohmann::json ExtensionObject(const nlohmann::json& a_node) {
		return a_node.contains("extensions") && a_node["extensions"].is_object() ? a_node["extensions"] : nlohmann::json::object();
	}

	inline bool NormalizeActor(const nlohmann::json& a_actorNode, const nlohmann::json& a_defaults, nlohmann::json& a_actor, std::string& a_error) {
		if (!a_actorNode.is_object()) { a_error = "contains a non-object actor slot"; return false; }
		const auto defaultReq = a_defaults.contains("requirements") && a_defaults["requirements"].is_object() ? a_defaults["requirements"] : nlohmann::json::object();
		const auto requirements = a_actorNode.contains("requirements") && a_actorNode["requirements"].is_object() ? a_actorNode["requirements"] : nlohmann::json::object();
		auto inheritedString = [&](std::string_view key, std::string fallback) {
			if (requirements.contains(key) && requirements[key].is_string()) return requirements[key].get<std::string>();
			if (a_actorNode.contains(key) && a_actorNode[key].is_string()) return a_actorNode[key].get<std::string>();
			if (defaultReq.contains(key) && defaultReq[key].is_string()) return defaultReq[key].get<std::string>();
			return fallback;
		};
		const auto file = a_actorNode.value("file", std::string{});
		const auto skeleton = inheritedString("skeleton", "Human");
		std::string genderText = inheritedString("legacyGameSex", "Any");
		if (!a_actorNode.contains("legacyGameSex") && a_actorNode.contains("gender") && a_actorNode["gender"].is_string()) genderText = a_actorNode["gender"].get<std::string>();
		std::string expectedText = a_defaults.value("expectedBodyType", std::string{ "Any" });
		if (a_actorNode.contains("expectedBodyType")) { if (!a_actorNode["expectedBodyType"].is_string()) { a_error = "contains a non-string expectedBodyType"; return false; } expectedText = a_actorNode["expectedBodyType"].get<std::string>(); }
		const auto expected = ParseExpectedBodyType(expectedText);
		if (file.empty() || skeleton.empty() || !ParseGender(genderText)) { a_error = "contains an invalid actor file, skeleton, or legacy game-sex constraint"; return false; }
		if (!expected) { a_error = "contains an invalid expectedBodyType (expected Masc, Femme, or Any)"; return false; }
		a_actor = { { "file", file }, { "skeleton", skeleton }, { "legacyGameSex", genderText }, { "expectedBodyType", *expected }, { "extensions", ExtensionObject(a_actorNode) } };
		if (a_actorNode.contains("role")) { if (!a_actorNode["role"].is_string()) { a_error = "contains a non-string role"; return false; } const auto role = ParseRole(a_actorNode["role"].get<std::string>()); if (!role) { a_error = "contains an invalid role"; return false; } a_actor["role"] = *role; }
		if (a_actorNode.contains("erection")) { if (!a_actorNode["erection"].is_boolean()) { a_error = "contains a non-boolean erection instruction"; return false; } a_actor["erection"] = a_actorNode["erection"]; }
		return true;
	}

	inline bool NormalizeV1(const nlohmann::json& a_root, const std::string& a_source, nlohmann::json& a_output, bool a_verbose) {
		auto fail = [&](std::string_view reason) { if (a_verbose) logger::warn("JSON animation definition '{}' {}", a_source, reason); return false; };
		if (!a_root.is_object() || a_root.value("format", std::string{}) != "NAFExtendedAnimation") return fail("has an unsupported format");
		if (a_root.value("schemaVersion", 0u) != kSchemaVersion) return fail("has an unsupported schemaVersion");
		if (!a_root.contains("animation") || !a_root["animation"].is_object() || !a_root.contains("position") || !a_root["position"].is_object()) return fail("requires animation and position objects");
		const auto& animationNode = a_root["animation"]; const auto& positionNode = a_root["position"];
		const auto animationID = animationNode.value("id", std::string{}); const auto positionID = positionNode.value("id", animationID);
		if (animationID.empty() || positionID.empty() || !animationNode.contains("actors") || !animationNode["actors"].is_array() || animationNode["actors"].empty()) return fail("has no usable ID or actor slots");
		nlohmann::json actors = nlohmann::json::array();
		for (const auto& actorNode : animationNode["actors"]) {
			nlohmann::json actor; std::string error;
			if (!NormalizeActor(actorNode, nlohmann::json::object(), actor, error)) return fail(error);
			actors.push_back(std::move(actor));
		}
		nlohmann::json tags = nlohmann::json::array();
		if (positionNode.contains("tags")) { if (!positionNode["tags"].is_array()) return fail("contains non-array position tags"); for (const auto& tag : positionNode["tags"]) if (tag.is_string()) tags.push_back(tag); }
		const auto displayName = positionNode.value("displayName", positionID); if (displayName.empty()) return fail("contains an empty position displayName");
		a_output = { { "normalizedKind", "v1" }, { "source", a_source }, { "animationID", animationID }, { "positionID", positionID }, { "loadPriority", a_root.value("loadPriority", 0) },
			{ "actors", std::move(actors) }, { "hidden", positionNode.value("hidden", false) }, { "tags", std::move(tags) }, { "displayName", displayName },
			{ "extensions", a_root.contains("extensions") && a_root["extensions"].is_object() ? a_root["extensions"] : nlohmann::json::object() } };
		return true;
	}

	inline nlohmann::json NormalizeExtensions(const nlohmann::json& a_node) { return ExtensionObject(a_node); }

	inline bool NormalizeV2(const nlohmann::json& a_root, const std::string& a_source, nlohmann::json& a_output, bool a_verbose) {
		auto documentFail = [&](std::string_view reason) { if (a_verbose) logger::warn("JSON animation package '{}' {}", a_source, reason); return false; };
		if (!a_root.is_object() || a_root.value("format", std::string{}) != "NAFExtendedAnimationPack") return documentFail("has an unsupported format");
		if (a_root.value("schemaVersion", 0u) != kPackSchemaVersion) return documentFail("has an unsupported schemaVersion");
		if (!a_root.contains("pack") || !a_root["pack"].is_object()) return documentFail("requires a pack object");
		const auto packID = a_root["pack"].value("id", std::string{}); if (packID.empty()) return documentFail("requires a non-empty pack.id");
		const auto loadPriority = a_root.value("loadPriority", 0);
		const auto defaults = a_root.contains("defaults") && a_root["defaults"].is_object() ? a_root["defaults"] : nlohmann::json::object();
		a_output = { { "normalizedKind", "v2" }, { "source", a_source }, { "loadPriority", loadPriority },
			{ "pack", { { "id", packID }, { "displayName", a_root["pack"].value("displayName", packID) }, { "extensions", NormalizeExtensions(a_root["pack"]) } } },
			{ "animations", nlohmann::json::array() }, { "animationGroups", nlohmann::json::array() }, { "positionTrees", nlohmann::json::array() }, { "positions", nlohmann::json::array() } };
		std::unordered_set<std::string, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> animationIDs, groupIDs, treeIDs, positionIDs;
		auto definitionError = [&](std::string_view kind, const std::string& id, std::string_view reason) { if (a_verbose) logger::warn("JSON package '{}' {} '{}' {}", a_source, kind, id.empty() ? "<unknown>" : id, reason); };

		if (a_root.contains("animations") && !a_root["animations"].is_array()) return documentFail("contains a non-array animations field");
		for (const auto& node : a_root.value("animations", nlohmann::json::array())) {
			const auto id = node.is_object() ? node.value("id", std::string{}) : std::string{};
			if (!node.is_object() || id.empty() || !node.contains("actors") || !node["actors"].is_array() || node["actors"].empty()) { definitionError("animation", id, "has no usable ID or actor slots"); continue; }
			if (!animationIDs.insert(id).second) { definitionError("animation", id, "duplicates an earlier animation ID in the same package"); continue; }
			nlohmann::json actors = nlohmann::json::array(); bool valid = true; std::string error;
			for (const auto& actorNode : node["actors"]) { nlohmann::json actor; if (!NormalizeActor(actorNode, defaults, actor, error)) { valid = false; break; } actors.push_back(std::move(actor)); }
			if (!valid) { definitionError("animation", id, error); continue; }
			nlohmann::json tags = nlohmann::json::array();
			if (defaults.contains("tags")) { if (!defaults["tags"].is_array()) return documentFail("contains non-array defaults.tags"); for (const auto& tag : defaults["tags"]) if (tag.is_string()) tags.push_back(tag); }
			if (node.contains("tags")) { if (!node["tags"].is_array()) { definitionError("animation", id, "contains non-array tags"); continue; } for (const auto& tag : node["tags"]) if (tag.is_string()) tags.push_back(tag); }
			a_output["animations"].push_back({ { "id", id }, { "loadPriority", node.value("loadPriority", loadPriority) }, { "actors", std::move(actors) }, { "tags", std::move(tags) }, { "extensions", NormalizeExtensions(node) } });
		}

		if (a_root.contains("animationGroups") && !a_root["animationGroups"].is_array()) return documentFail("contains a non-array animationGroups field");
		for (const auto& node : a_root.value("animationGroups", nlohmann::json::array())) {
			const auto id = node.is_object() ? node.value("id", std::string{}) : std::string{};
			if (!node.is_object() || id.empty() || !node.contains("stages") || !node["stages"].is_array() || node["stages"].empty()) { definitionError("animationGroup", id, "has no usable ID or stages"); continue; }
			if (!groupIDs.insert(id).second) { definitionError("animationGroup", id, "duplicates an earlier group ID in the same package"); continue; }
			nlohmann::json stages = nlohmann::json::array(); bool valid = true; std::size_t stageIndex = 0;
			for (const auto& stage : node["stages"]) {
				if (!stage.is_object() || stage.value("animation", std::string{}).empty()) { definitionError("animationGroup", id, std::format("contains an invalid stage at index {}", stageIndex)); valid = false; break; }
				std::uint32_t loopMin = 1, loopMax = 1;
				if (stage.contains("loops")) {
					if (stage["loops"].is_number_unsigned()) loopMin = loopMax = stage["loops"].get<std::uint32_t>();
					else if (stage["loops"].is_object()) { loopMin = stage["loops"].value("min", 1u); loopMax = stage["loops"].value("max", loopMin); }
					else { definitionError("animationGroup", id, std::format("contains invalid loops at stage {}", stageIndex)); valid = false; break; }
				}
				if (loopMin < 1) loopMin = 1; if (loopMax < loopMin) loopMax = loopMin;
				stages.push_back({ { "animation", stage.value("animation", std::string{}) }, { "loopMin", loopMin }, { "loopMax", loopMax }, { "weight", std::max(1u, stage.value("weight", 1u)) }, { "extensions", NormalizeExtensions(stage) } }); ++stageIndex;
			}
			if (valid) a_output["animationGroups"].push_back({ { "id", id }, { "loadPriority", node.value("loadPriority", loadPriority) }, { "sequential", node.value("sequential", false) }, { "stages", std::move(stages) }, { "extensions", NormalizeExtensions(node) } });
		}

		std::function<std::optional<nlohmann::json>(const nlohmann::json&, std::size_t&, std::size_t)> normalizeTreeNode;
		normalizeTreeNode = [&](const nlohmann::json& node, std::size_t& count, std::size_t depth) -> std::optional<nlohmann::json> {
			if (!node.is_object() || depth > 256 || ++count > kMaxTreeNodes) return std::nullopt;
			const auto position = node.value("position", std::string{}); if (position.empty()) return std::nullopt;
			nlohmann::json children = nlohmann::json::array();
			if (node.contains("children")) { if (!node["children"].is_array()) return std::nullopt; for (const auto& child : node["children"]) { auto normalized = normalizeTreeNode(child, count, depth + 1); if (!normalized) return std::nullopt; children.push_back(std::move(*normalized)); } }
			return nlohmann::json{ { "id", node.value("id", std::string{ "Unknown" }) }, { "position", position }, { "duration", node.value("duration", 0.0f) }, { "forceComplete", node.value("forceComplete", false) }, { "children", std::move(children) }, { "extensions", NormalizeExtensions(node) } };
		};
		if (a_root.contains("positionTrees") && !a_root["positionTrees"].is_array()) return documentFail("contains a non-array positionTrees field");
		for (const auto& node : a_root.value("positionTrees", nlohmann::json::array())) {
			const auto id = node.is_object() ? node.value("id", std::string{}) : std::string{}; std::size_t count = 0;
			if (!node.is_object() || id.empty() || !node.contains("root")) { definitionError("positionTree", id, "has no usable ID or root"); continue; }
			if (!treeIDs.insert(id).second) { definitionError("positionTree", id, "duplicates an earlier tree ID in the same package"); continue; }
			auto root = normalizeTreeNode(node["root"], count, 0); if (!root) { definitionError("positionTree", id, "contains an invalid, over-deep, or oversized node hierarchy"); continue; }
			a_output["positionTrees"].push_back({ { "id", id }, { "loadPriority", node.value("loadPriority", loadPriority) }, { "root", std::move(*root) }, { "extensions", NormalizeExtensions(node) } });
		}

		if (a_root.contains("positions") && !a_root["positions"].is_array()) return documentFail("contains a non-array positions field");
		for (const auto& node : a_root.value("positions", nlohmann::json::array())) {
			const auto id = node.is_object() ? node.value("id", std::string{}) : std::string{};
			if (!node.is_object() || id.empty()) { definitionError("position", id, "has no usable ID"); continue; }
			if (!positionIDs.insert(id).second) { definitionError("position", id, "duplicates an earlier position ID in the same package"); continue; }
			std::string targetType, targetID; std::size_t targets = 0;
			for (const auto key : { "animation", "animationGroup", "positionTree" }) if (node.contains(key) && node[key].is_string() && !node[key].get<std::string>().empty()) { targetType = key; targetID = node[key].get<std::string>(); ++targets; }
			if (targets != 1) { definitionError("position", id, "must reference exactly one animation, animationGroup, or positionTree"); continue; }
			nlohmann::json tags = nlohmann::json::array();
			if (defaults.contains("tags")) for (const auto& tag : defaults["tags"]) if (tag.is_string()) tags.push_back(tag);
			if (node.contains("tags")) { if (!node["tags"].is_array()) { definitionError("position", id, "contains non-array tags"); continue; } for (const auto& tag : node["tags"]) if (tag.is_string()) tags.push_back(tag); }
			nlohmann::json locations = nlohmann::json::array(); if (node.contains("locations")) { if (!node["locations"].is_array()) { definitionError("position", id, "contains non-array locations"); continue; } for (const auto& location : node["locations"]) if (location.is_string()) locations.push_back(location); }
			a_output["positions"].push_back({ { "id", id }, { "loadPriority", node.value("loadPriority", loadPriority) }, { "targetType", targetType }, { "targetID", targetID },
				{ "displayName", node.value("displayName", id) }, { "hidden", node.value("hidden", false) }, { "tags", std::move(tags) }, { "locations", std::move(locations) },
				{ "startMorphSet", node.value("startMorphSet", std::string{}) }, { "stopMorphSet", node.value("stopMorphSet", std::string{}) },
				{ "startEquipmentSet", node.value("startEquipmentSet", std::string{}) }, { "stopEquipmentSet", node.value("stopEquipmentSet", std::string{}) }, { "extensions", NormalizeExtensions(node) } });
		}
		return true;
	}

	inline bool RegisterV1(const nlohmann::json& a_definition, bool a_verbose) {
		const auto source = a_definition.value("source", std::string{ "<cache>" }); const auto animationID = a_definition.value("animationID", std::string{}); const auto positionID = a_definition.value("positionID", std::string{});
		if (animationID.empty() || positionID.empty() || !a_definition.contains("actors") || !a_definition["actors"].is_array()) return false;
		// JSON never overrides an existing XML or JSON identity. This keeps mixed
		// installations deterministic and avoids partially replacing an animation
		// while retaining a position (or vice versa).
		if (Global::Animations.get_ptr(animationID) || Global::Positions.get_ptr(positionID)) { if (a_verbose) logger::warn("JSON animation definition '{}' conflicts with existing animation/position ID '{}'/'{}'; keeping the first loaded definition", source, animationID, positionID); return false; }
		auto animation = std::make_shared<Animation>(); animation->id = animationID; animation->loadPriority = a_definition.value("loadPriority", 0);
		std::vector<MetadataRecord> records; std::uint32_t slotIndex = 0;
		for (const auto& actor : a_definition["actors"]) {
			const auto gender = ParseGender(actor.value("legacyGameSex", std::string{ "Any" })); if (!gender) return false;
			auto& slot = animation->slots.emplace_back(); slot.dynamicIdle = true; slot.behaviorRequiresConvert = true; slot.idleRequiresConvert = false;
			slot.rootBehavior = actor.value("skeleton", std::string{ "Human" }); slot.gender = *gender; slot.idle[0] = actor.value("file", std::string{}); slot.idle[1] = "default";
			const auto prefix = std::format("slot.{}.", slotIndex++);
			records.push_back({ "naf.extended.actor", prefix + "requirements.skeleton", nlohmann::json(slot.rootBehavior).dump() });
			records.push_back({ "naf.extended.actor", prefix + "expectedBodyType", nlohmann::json(actor.value("expectedBodyType", std::string{ "Any" })).dump() });
			if (actor.contains("role")) records.push_back({ "naf.extended.actor", prefix + "role", actor["role"].dump() });
			if (actor.contains("erection")) records.push_back({ "naf.extended.actor", prefix + "erection", actor["erection"].dump() });
			if (actor.contains("extensions")) { std::vector<MetadataRecord> actorExtensions; AddExtensions(actorExtensions, actor["extensions"]); for (auto& record : actorExtensions) { record.key = prefix + "extensions." + record.key; records.push_back(std::move(record)); } }
		}
		auto position = std::make_shared<Position>(); position->id = positionID; position->idForType = animationID; position->loadPriority = animation->loadPriority; position->posType = Position::kAnimation; position->hidden = a_definition.value("hidden", false);
		for (const auto& tag : a_definition.value("tags", nlohmann::json::array())) { if (!tag.is_string()) continue; auto value = tag.get<std::string>(); Utility::TransformStringToLower(value); position->tags.emplace(std::move(value)); }
		const auto displayName = a_definition.value("displayName", positionID); if (displayName != positionID) records.push_back({ "naf.extended", "position.displayName", nlohmann::json(displayName).dump() });
		if (const auto it = a_definition.find("extensions"); it != a_definition.end()) AddExtensions(records, *it);
		Global::Animations.priority_insert(animation); Global::Positions.priority_insert(position);
		{ std::unique_lock lock{ metadataLock }; metadata[positionID] = std::move(records); displayNames[positionID] = displayName; }
		return true;
	}

	inline std::vector<MetadataRecord> PackageRecords(const nlohmann::json& a_document) {
		std::vector<MetadataRecord> records;
		if (!a_document.contains("pack") || !a_document["pack"].is_object()) return records;
		const auto& pack = a_document["pack"];
		records.push_back({ "naf.extended.package", "id", nlohmann::json(pack.value("id", std::string{})).dump() });
		records.push_back({ "naf.extended.package", "displayName", nlohmann::json(pack.value("displayName", std::string{})).dump() });
		if (pack.contains("extensions")) AddExtensions(records, pack["extensions"]);
		return records;
	}

	inline std::vector<MetadataRecord> AnimationRecords(const nlohmann::json& a_definition, const nlohmann::json* a_document = nullptr) {
		auto records = a_document ? PackageRecords(*a_document) : std::vector<MetadataRecord>{};
		std::uint32_t slotIndex = 0;
		for (const auto& actor : a_definition.value("actors", nlohmann::json::array())) {
			const auto prefix = std::format("slot.{}.", slotIndex++);
			records.push_back({ "naf.extended.actor", prefix + "requirements.skeleton", nlohmann::json(actor.value("skeleton", std::string{ "Human" })).dump() });
			records.push_back({ "naf.extended.actor", prefix + "expectedBodyType", nlohmann::json(actor.value("expectedBodyType", std::string{ "Any" })).dump() });
			if (actor.contains("role")) records.push_back({ "naf.extended.actor", prefix + "role", actor["role"].dump() });
			if (actor.contains("erection")) records.push_back({ "naf.extended.actor", prefix + "erection", actor["erection"].dump() });
			if (actor.contains("extensions")) { std::vector<MetadataRecord> extra; AddExtensions(extra, actor["extensions"]); for (auto& record : extra) { record.key = prefix + "extensions." + record.key; records.push_back(std::move(record)); } }
		}
		if (a_definition.contains("extensions")) AddExtensions(records, a_definition["extensions"]);
		return records;
	}

	inline bool RegisterAnimationDefinition(const nlohmann::json& a_definition, const nlohmann::json* a_document, bool a_verbose) {
		const auto id = a_definition.value("id", a_definition.value("animationID", std::string{})); const auto source = a_document ? a_document->value("source", std::string{ "<cache>" }) : a_definition.value("source", std::string{ "<cache>" });
		if (id.empty() || Global::Animations.get_ptr(id)) { if (a_verbose) logger::warn("JSON animation '{}' from '{}' conflicts with an existing animation ID; keeping the first loaded definition", id, source); return false; }
		auto animation = std::make_shared<Animation>(); animation->id = id; animation->loadPriority = a_definition.value("loadPriority", 0);
		for (const auto& actor : a_definition.value("actors", nlohmann::json::array())) {
			const auto gender = ParseGender(actor.value("legacyGameSex", std::string{ "Any" })); if (!gender) return false;
			auto& slot = animation->slots.emplace_back(); slot.dynamicIdle = true; slot.behaviorRequiresConvert = true; slot.idleRequiresConvert = false;
			slot.rootBehavior = actor.value("skeleton", std::string{ "Human" }); slot.gender = *gender; slot.idle[0] = actor.value("file", std::string{}); slot.idle[1] = "default";
		}
		for (const auto& tag : a_definition.value("tags", nlohmann::json::array())) if (tag.is_string()) { auto value = tag.get<std::string>(); Utility::TransformStringToLower(value); animation->tags.emplace(std::move(value)); }
		Global::Animations.priority_insert(animation);
		{ std::unique_lock lock{ metadataLock }; animationMetadata[id] = AnimationRecords(a_definition, a_document); }
		return true;
	}

	inline bool RegisterGroupDefinition(const nlohmann::json& a_definition, const nlohmann::json& a_document, bool a_verbose) {
		const auto id = a_definition.value("id", std::string{}); const auto source = a_document.value("source", std::string{ "<cache>" });
		if (id.empty() || Global::AnimationGroups.get_ptr(id)) { if (a_verbose) logger::warn("JSON animation group '{}' from '{}' conflicts with an existing group ID; keeping the first loaded definition", id, source); return false; }
		auto group = std::make_shared<AnimationGroup>(); group->id = id; group->loadPriority = a_definition.value("loadPriority", 0); group->sequential = a_definition.value("sequential", false);
		std::vector<std::vector<MetadataRecord>> stagesMetadata;
		for (const auto& stageNode : a_definition.value("stages", nlohmann::json::array())) {
			const auto animationID = stageNode.value("animation", std::string{}); if (!Global::Animations.get_ptr(animationID)) { if (a_verbose) logger::warn("JSON animation group '{}' from '{}' references missing animation '{}'; discarding the group", id, source, animationID); return false; }
			auto& stage = group->stages.emplace_back(); stage.animation = animationID; stage.loopMin = stageNode.value("loopMin", 1u); stage.loopMax = stageNode.value("loopMax", stage.loopMin); stage.weight = stageNode.value("weight", 1u);
			std::vector<MetadataRecord> records; records.push_back({ "naf.extended.stage", "animation", nlohmann::json(animationID).dump() }); if (stageNode.contains("extensions")) AddExtensions(records, stageNode["extensions"]); stagesMetadata.push_back(std::move(records));
		}
		Global::AnimationGroups.priority_insert(group);
		{ std::unique_lock lock{ metadataLock }; auto records = PackageRecords(a_document); records.push_back({ "naf.extended.group", "sequential", nlohmann::json(group->sequential).dump() }); if (a_definition.contains("extensions")) AddExtensions(records, a_definition["extensions"]); groupMetadata[id] = std::move(records); stageMetadata[id] = std::move(stagesMetadata); }
		return true;
	}

	inline std::shared_ptr<PositionTree::Node> BuildTreeNode(const nlohmann::json& a_node, const std::shared_ptr<PositionTree::Node>& a_parent = nullptr) {
		auto node = std::make_shared<PositionTree::Node>(); node->id = a_node.value("id", std::string{ "Unknown" }); node->position = a_node.value("position", std::string{}); node->duration = a_node.value("duration", 0.0f); node->forceComplete = a_node.value("forceComplete", false); node->parent = a_parent;
		for (const auto& child : a_node.value("children", nlohmann::json::array())) node->children.push_back(BuildTreeNode(child, node)); return node;
	}

	inline void CollectTreePositionIDs(const nlohmann::json& a_node, std::vector<std::string>& a_ids) { a_ids.push_back(a_node.value("position", std::string{})); for (const auto& child : a_node.value("children", nlohmann::json::array())) CollectTreePositionIDs(child, a_ids); }

	inline bool RegisterTreeDefinition(const nlohmann::json& a_definition, const nlohmann::json& a_document, const std::unordered_set<std::string, CaseInsensitiveStringHash, CaseInsensitiveStringEqual>& a_candidatePositions, bool a_verbose) {
		const auto id = a_definition.value("id", std::string{}); const auto source = a_document.value("source", std::string{ "<cache>" });
		if (id.empty() || Global::PositionTrees.get_ptr(id)) { if (a_verbose) logger::warn("JSON position tree '{}' from '{}' conflicts with an existing tree ID; keeping the first loaded definition", id, source); return false; }
		std::vector<std::string> positions; CollectTreePositionIDs(a_definition["root"], positions);
		for (const auto& position : positions) if (!Global::Positions.get_ptr(position) && !a_candidatePositions.contains(position)) { if (a_verbose) logger::warn("JSON position tree '{}' from '{}' references missing position '{}'; discarding the tree", id, source, position); return false; }
		auto tree = std::make_shared<PositionTree>(); tree->id = id; tree->loadPriority = a_definition.value("loadPriority", 0); tree->tree = BuildTreeNode(a_definition["root"]); Global::PositionTrees.priority_insert(tree);
		{ std::unique_lock lock{ metadataLock }; auto records = PackageRecords(a_document); if (a_definition.contains("extensions")) AddExtensions(records, a_definition["extensions"]); treeMetadata[id] = std::move(records);
			std::function<void(const nlohmann::json&)> storeNode = [&](const nlohmann::json& node) { std::vector<MetadataRecord> nodeRecords; nodeRecords.push_back({ "naf.extended.tree", "position", nlohmann::json(node.value("position", std::string{})).dump() }); if (node.contains("extensions")) AddExtensions(nodeRecords, node["extensions"]); treeNodeMetadata[id + "\x1F" + node.value("id", std::string{ "Unknown" })] = std::move(nodeRecords); for (const auto& child : node.value("children", nlohmann::json::array())) storeNode(child); }; storeNode(a_definition["root"]); }
		return true;
	}

	inline bool RegisterPositionDefinition(const nlohmann::json& a_definition, const nlohmann::json* a_document, bool a_verbose) {
		const auto id = a_definition.value("id", a_definition.value("positionID", std::string{})); const auto source = a_document ? a_document->value("source", std::string{ "<cache>" }) : a_definition.value("source", std::string{ "<cache>" });
		if (id.empty() || Global::Positions.get_ptr(id)) { if (a_verbose) logger::warn("JSON position '{}' from '{}' conflicts with an existing position ID; keeping the first loaded definition", id, source); return false; }
		const auto type = a_definition.value("targetType", std::string{ "animation" }); const auto target = a_definition.value("targetID", a_definition.value("animationID", std::string{}));
		Position::Type positionType = Position::kAnimation; bool found = false;
		if (type == "animation") found = Global::Animations.get_ptr(target) != nullptr;
		else if (type == "animationGroup") { positionType = Position::kAnimationGroup; found = Global::AnimationGroups.get_ptr(target) != nullptr; }
		else if (type == "positionTree") { positionType = Position::kPositionTree; found = Global::PositionTrees.get_ptr(target) != nullptr; }
		if (!found) { if (a_verbose) logger::warn("JSON position '{}' from '{}' references missing {} '{}'; discarding the position", id, source, type, target); return false; }
		auto position = std::make_shared<Position>(); position->id = id; position->idForType = target; position->posType = positionType; position->loadPriority = a_definition.value("loadPriority", 0); position->hidden = a_definition.value("hidden", false);
		position->startMorphSet = a_definition.value("startMorphSet", std::string{}); position->stopMorphSet = a_definition.value("stopMorphSet", std::string{}); position->startEquipSet = a_definition.value("startEquipmentSet", std::string{}); position->stopEquipSet = a_definition.value("stopEquipmentSet", std::string{});
		for (const auto& location : a_definition.value("locations", nlohmann::json::array())) if (location.is_string()) position->locations.push_back(location.get<std::string>());
		for (const auto& tag : a_definition.value("tags", nlohmann::json::array())) if (tag.is_string()) { auto value = tag.get<std::string>(); Utility::TransformStringToLower(value); position->tags.emplace(std::move(value)); }
		Global::Positions.priority_insert(position); std::vector<MetadataRecord> records = a_document ? PackageRecords(*a_document) : std::vector<MetadataRecord>{};
		const auto displayName = a_definition.value("displayName", id); if (displayName != id) records.push_back({ "naf.extended", "position.displayName", nlohmann::json(displayName).dump() }); if (a_definition.contains("extensions")) AddExtensions(records, a_definition["extensions"]);
		{ std::unique_lock lock{ metadataLock }; metadata[id] = std::move(records); displayNames[id] = displayName; }
		return true;
	}

	inline void RegisterDocuments(std::vector<nlohmann::json>& a_documents, bool a_verbose) {
		// Registration is phased across all lexically ordered sources so forward
		// references do not depend on array or file order. Each registry still uses
		// first-definition-wins collision semantics after XML has loaded.
		std::unordered_set<std::string, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> reservedAnimations, reservedPositions, reservedGroups, reservedTrees;
		for (auto& document : a_documents) {
			if (document.value("normalizedKind", std::string{}) == "v1") {
				const auto animationID = document.value("animationID", std::string{}); const auto positionID = document.value("positionID", std::string{});
				const bool accepted = !Global::Animations.get_ptr(animationID) && !Global::Positions.get_ptr(positionID) && !reservedAnimations.contains(animationID) && !reservedPositions.contains(positionID);
				document["_accepted"] = accepted; if (accepted) { reservedAnimations.insert(animationID); reservedPositions.insert(positionID); }
			} else {
				for (auto& animation : document["animations"]) { const auto id = animation.value("id", std::string{}); const bool accepted = !Global::Animations.get_ptr(id) && reservedAnimations.insert(id).second; animation["_accepted"] = accepted; }
				for (auto& group : document["animationGroups"]) { const auto id = group.value("id", std::string{}); const bool accepted = !Global::AnimationGroups.get_ptr(id) && reservedGroups.insert(id).second; group["_accepted"] = accepted; }
				for (auto& tree : document["positionTrees"]) { const auto id = tree.value("id", std::string{}); const bool accepted = !Global::PositionTrees.get_ptr(id) && reservedTrees.insert(id).second; tree["_accepted"] = accepted; }
				for (auto& position : document["positions"]) { const auto id = position.value("id", std::string{}); const bool accepted = !Global::Positions.get_ptr(id) && reservedPositions.insert(id).second; position["_accepted"] = accepted; }
			}
		}
		for (auto& document : a_documents) {
			if (document.value("normalizedKind", std::string{}) == "v1") {
				const auto animationID = document.value("animationID", std::string{}); const auto positionID = document.value("positionID", std::string{});
				if (!document.value("_accepted", false)) { if (a_verbose) logger::warn("JSON animation definition '{}' conflicts with existing animation/position ID '{}'/'{}'; keeping the first loaded definition", document.value("source", std::string{ "<cache>" }), animationID, positionID); document["_animationRegistered"] = false; continue; }
				nlohmann::json animation{ { "id", document.value("animationID", std::string{}) }, { "loadPriority", document.value("loadPriority", 0) }, { "actors", document.value("actors", nlohmann::json::array()) }, { "tags", nlohmann::json::array() }, { "extensions", document.value("extensions", nlohmann::json::object()) } };
				document["_animationRegistered"] = RegisterAnimationDefinition(animation, nullptr, a_verbose);
			} else for (const auto& animation : document.value("animations", nlohmann::json::array())) if (animation.value("_accepted", false)) RegisterAnimationDefinition(animation, &document, a_verbose); else if (a_verbose) logger::warn("JSON animation '{}' from '{}' conflicts with an earlier definition; keeping the first", animation.value("id", std::string{}), document.value("source", std::string{}));
		}
		for (auto& document : a_documents) if (document.value("normalizedKind", std::string{}) == "v2") for (const auto& group : document.value("animationGroups", nlohmann::json::array())) if (group.value("_accepted", false)) RegisterGroupDefinition(group, document, a_verbose); else if (a_verbose) logger::warn("JSON animation group '{}' from '{}' conflicts with an earlier definition; keeping the first", group.value("id", std::string{}), document.value("source", std::string{}));
		std::unordered_set<std::string, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> candidatePositions;
		for (const auto& document : a_documents) {
			if (document.value("normalizedKind", std::string{}) == "v1") candidatePositions.insert(document.value("positionID", std::string{}));
			else for (const auto& position : document.value("positions", nlohmann::json::array())) if (position.value("_accepted", false)) candidatePositions.insert(position.value("id", std::string{}));
		}
		for (auto& document : a_documents) if (document.value("normalizedKind", std::string{}) == "v2") for (const auto& tree : document.value("positionTrees", nlohmann::json::array())) if (tree.value("_accepted", false)) RegisterTreeDefinition(tree, document, candidatePositions, a_verbose); else if (a_verbose) logger::warn("JSON position tree '{}' from '{}' conflicts with an earlier definition; keeping the first", tree.value("id", std::string{}), document.value("source", std::string{}));
		for (auto& document : a_documents) {
			if (document.value("normalizedKind", std::string{}) == "v1") {
				if (!document.value("_animationRegistered", false)) continue;
				nlohmann::json position{ { "id", document.value("positionID", std::string{}) }, { "targetType", "animation" }, { "targetID", document.value("animationID", std::string{}) }, { "loadPriority", document.value("loadPriority", 0) }, { "hidden", document.value("hidden", false) }, { "tags", document.value("tags", nlohmann::json::array()) }, { "displayName", document.value("displayName", document.value("positionID", std::string{})) }, { "extensions", document.value("extensions", nlohmann::json::object()) } };
				RegisterPositionDefinition(position, nullptr, a_verbose);
			} else for (const auto& position : document.value("positions", nlohmann::json::array())) if (position.value("_accepted", false)) RegisterPositionDefinition(position, &document, a_verbose); else if (a_verbose) logger::warn("JSON position '{}' from '{}' conflicts with an earlier definition; keeping the first", position.value("id", std::string{}), document.value("source", std::string{}));
		}
	}

	inline bool ParseAndNormalize(const std::filesystem::path& a_path, nlohmann::json& a_output, bool a_verbose) {
		std::ifstream stream{ a_path, std::ios::binary }; if (!stream) { if (a_verbose) logger::warn("Failed to open JSON animation definition '{}'", a_path.generic_string()); return false; }
		try { nlohmann::json root; stream >> root; const auto format = root.value("format", std::string{}); const auto version = root.value("schemaVersion", 0u);
			if (format == "NAFExtendedAnimation" && version == kSchemaVersion) return NormalizeV1(root, a_path.generic_string(), a_output, a_verbose);
			if (format == "NAFExtendedAnimationPack" && version == kPackSchemaVersion) return NormalizeV2(root, a_path.generic_string(), a_output, a_verbose);
			if (a_verbose) logger::warn("JSON animation document '{}' has unsupported format '{}' or schemaVersion {}", a_path.generic_string(), format, version); return false; }
		catch (const std::exception& e) { if (a_verbose) logger::warn("Failed to parse JSON animation definition '{}': {}", a_path.generic_string(), e.what()); return false; }
	}

	struct CacheHeader { char magic[8]; std::uint32_t cacheVersion; std::uint32_t schemaVersion; std::uint64_t payloadSize; std::uint64_t checksum; };
	static_assert(sizeof(CacheHeader) == 32);
	inline nlohmann::json ManifestJSON(const std::vector<SourceRecord>& a_manifest) {
		nlohmann::json result = nlohmann::json::array(); for (const auto& source : a_manifest) result.push_back({ { "path", source.path }, { "size", source.size }, { "modified", source.modified }, { "fingerprint", source.fingerprint } }); return result;
	}
	inline bool LoadCache(const std::vector<SourceRecord>& a_manifest, std::vector<nlohmann::json>& a_definitions) {
		// Any header, schema, manifest, size, checksum, or CBOR error rejects the
		// entire cache. LoadFiles then rebuilds it from authoritative source files.
		try { if (!std::filesystem::exists(cachePath)) return false; const auto fileSize = std::filesystem::file_size(cachePath); if (fileSize < sizeof(CacheHeader) || fileSize > kMaxCacheBytes) return false;
			std::ifstream stream{ cachePath, std::ios::binary }; CacheHeader header{}; stream.read(reinterpret_cast<char*>(&header), sizeof(header));
			if (!stream || std::memcmp(header.magic, "NAFPACK", 7) != 0 || header.cacheVersion != kCacheVersion || header.schemaVersion != kSchemaVersion || header.payloadSize != fileSize - sizeof(CacheHeader)) return false;
			std::vector<std::uint8_t> payload(static_cast<std::size_t>(header.payloadSize)); stream.read(reinterpret_cast<char*>(payload.data()), payload.size()); if (!stream || HashBytes(payload.data(), payload.size()) != header.checksum) return false;
			const auto root = nlohmann::json::from_cbor(payload, true, true); if (!root.is_object() || root.value("cacheVersion", 0u) != kCacheVersion || root.value("schemaVersion", 0u) != kSchemaVersion ||
				!root.contains("manifest") || root["manifest"] != ManifestJSON(a_manifest) || !root.contains("definitions") || !root["definitions"].is_array() || root["definitions"].size() > kMaxDefinitions) return false;
			a_definitions.assign(root["definitions"].begin(), root["definitions"].end()); return true;
		} catch (const std::exception& e) { logger::warn("NAFPackCache.bin is invalid and will be rebuilt: {}", e.what()); return false; }
	}
	inline void SaveCache(const std::vector<SourceRecord>& a_manifest, const std::vector<nlohmann::json>& a_definitions) {
		try { nlohmann::json root{ { "cacheVersion", kCacheVersion }, { "schemaVersion", kSchemaVersion }, { "manifest", ManifestJSON(a_manifest) }, { "definitions", a_definitions } }; const auto payload = nlohmann::json::to_cbor(root);
			if (payload.size() > kMaxCacheBytes - sizeof(CacheHeader)) throw std::runtime_error("normalized payload exceeds cache size limit"); CacheHeader header{ { 'N', 'A', 'F', 'P', 'A', 'C', 'K', '\0' }, kCacheVersion, kSchemaVersion, static_cast<std::uint64_t>(payload.size()), HashBytes(payload.data(), payload.size()) };
			const auto temporary = cachePath.wstring() + L".tmp"; { std::ofstream stream{ temporary, std::ios::binary | std::ios::trunc }; if (!stream) throw std::runtime_error("temporary cache could not be opened"); stream.write(reinterpret_cast<const char*>(&header), sizeof(header)); stream.write(reinterpret_cast<const char*>(payload.data()), payload.size()); stream.flush(); if (!stream) throw std::runtime_error("temporary cache write was incomplete"); }
			if (!MoveFileExW(temporary.c_str(), cachePath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) { DeleteFileW(temporary.c_str()); throw std::runtime_error("atomic cache replacement failed"); }
		} catch (const std::exception& e) { logger::warn("Unable to write NAFPackCache.bin; loaded JSON definitions remain usable: {}", e.what()); }
	}
	inline void LoadFiles(const std::vector<std::filesystem::path>& a_files, bool a_verbose = true) {
		const auto started = std::chrono::steady_clock::now();
		const bool cacheExisted = std::filesystem::exists(cachePath);
		std::vector<SourceRecord> manifest; std::vector<nlohmann::json> definitions; const bool manifestReady = BuildManifest(a_files, manifest); const bool warm = manifestReady && LoadCache(manifest, definitions);
		if (!warm && cacheExisted && a_verbose) logger::info("NAFPackCache.bin is stale or invalid; rebuilding from JSON sources");
		if (!warm) { definitions.clear(); definitions.reserve(a_files.size()); for (const auto& path : a_files) { nlohmann::json normalized; if (ParseAndNormalize(path, normalized, a_verbose)) definitions.push_back(std::move(normalized)); } if (manifestReady) SaveCache(manifest, definitions); }
		RegisterDocuments(definitions, a_verbose);
		std::size_t loaded = 0; for (const auto& definition : definitions) { if (definition.value("normalizedKind", std::string{}) == "v1") ++loaded; else loaded += definition.value("animations", nlohmann::json::array()).size(); }
		if (a_verbose) logger::info("JSON animation packs: {} animation definitions processed from {} document(s) via {} in {:.3f}s", loaded, definitions.size(), warm ? "NAFPackCache.bin" : "source files",
			std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
	}
	inline bool ParseFile(const std::filesystem::path& a_path, bool a_verbose = true) { nlohmann::json normalized; if (!ParseAndNormalize(a_path, normalized, a_verbose)) return false; std::vector<nlohmann::json> documents{ std::move(normalized) }; RegisterDocuments(documents, a_verbose); return true; }
}
