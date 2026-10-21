#pragma once

namespace JSONAnimation
{
	// Extended JSON is normalized into the original Animation/Position model so
	// XML and JSON content share one playback path. The cache stores normalized,
	// pointer-free JSON only; engine forms and runtime objects are resolved after
	// registration just as they are for XML content.
	struct MetadataRecord { std::string nameSpace; std::string key; std::string jsonValue; };
	struct SourceRecord { std::string path; std::uint64_t size{}; std::int64_t modified{}; std::uint64_t fingerprint{}; };

	inline constexpr std::uint32_t kCacheVersion = 1;
	inline constexpr std::uint32_t kSchemaVersion = 1;
	inline constexpr std::uint64_t kMaxCacheBytes = 512ull * 1024ull * 1024ull;
	inline constexpr std::size_t kMaxDefinitions = 100000;
	inline const std::filesystem::path cachePath{ USERDATA_DIR + "NAFPackCache.bin" };
	inline std::shared_mutex metadataLock;
	inline std::unordered_map<std::string, std::vector<MetadataRecord>, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> metadata;
	inline std::unordered_map<std::string, std::string, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> displayNames;

	inline void Clear() { std::unique_lock lock{ metadataLock }; metadata.clear(); displayNames.clear(); }
	inline std::vector<MetadataRecord> GetMetadata(const std::string& a_id) {
		std::shared_lock lock{ metadataLock }; if (const auto it = metadata.find(a_id); it != metadata.end()) return it->second; return {};
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

	inline bool Normalize(const nlohmann::json& a_root, const std::string& a_source, nlohmann::json& a_output, bool a_verbose) {
		auto fail = [&](std::string_view reason) { if (a_verbose) logger::warn("JSON animation definition '{}' {}", a_source, reason); return false; };
		if (!a_root.is_object() || a_root.value("format", std::string{}) != "NAFExtendedAnimation") return fail("has an unsupported format");
		if (a_root.value("schemaVersion", 0u) != kSchemaVersion) return fail("has an unsupported schemaVersion");
		if (!a_root.contains("animation") || !a_root["animation"].is_object() || !a_root.contains("position") || !a_root["position"].is_object()) return fail("requires animation and position objects");
		const auto& animationNode = a_root["animation"]; const auto& positionNode = a_root["position"];
		const auto animationID = animationNode.value("id", std::string{}); const auto positionID = positionNode.value("id", animationID);
		if (animationID.empty() || positionID.empty() || !animationNode.contains("actors") || !animationNode["actors"].is_array() || animationNode["actors"].empty()) return fail("has no usable ID or actor slots");
		nlohmann::json actors = nlohmann::json::array();
		for (const auto& actorNode : animationNode["actors"]) {
			if (!actorNode.is_object()) return fail("contains a non-object actor slot");
			const auto file = actorNode.value("file", std::string{}); const auto requirements = actorNode.contains("requirements") && actorNode["requirements"].is_object() ? &actorNode["requirements"] : nullptr;
			const auto skeleton = requirements ? requirements->value("skeleton", actorNode.value("skeleton", std::string{ "Human" })) : actorNode.value("skeleton", std::string{ "Human" });
			const auto genderText = requirements ? requirements->value("legacyGameSex", actorNode.value("gender", std::string{ "Any" })) : actorNode.value("gender", std::string{ "Any" });
			if (file.empty() || skeleton.empty() || !ParseGender(genderText)) return fail("contains an invalid actor file, skeleton, or legacy game-sex constraint");
			std::string expectedText{ "Any" }; if (actorNode.contains("expectedBodyType")) { if (!actorNode["expectedBodyType"].is_string()) return fail("contains a non-string expectedBodyType"); expectedText = actorNode["expectedBodyType"].get<std::string>(); }
			const auto expected = ParseExpectedBodyType(expectedText); if (!expected) return fail("contains an invalid expectedBodyType (expected Masc, Femme, or Any)");
			nlohmann::json actor{ { "file", file }, { "skeleton", skeleton }, { "legacyGameSex", genderText }, { "expectedBodyType", *expected } };
			if (actorNode.contains("role")) { if (!actorNode["role"].is_string()) return fail("contains a non-string role"); const auto role = ParseRole(actorNode["role"].get<std::string>()); if (!role) return fail("contains an invalid role (expected giver or receiver)"); actor["role"] = *role; }
			if (actorNode.contains("erection")) { if (!actorNode["erection"].is_boolean()) return fail("contains a non-boolean erection instruction"); actor["erection"] = actorNode["erection"]; }
			actors.push_back(std::move(actor));
		}
		nlohmann::json tags = nlohmann::json::array();
		if (positionNode.contains("tags")) { if (!positionNode["tags"].is_array()) return fail("contains non-array position tags"); for (const auto& tag : positionNode["tags"]) if (tag.is_string()) tags.push_back(tag); }
		const auto displayName = positionNode.value("displayName", positionID); if (displayName.empty()) return fail("contains an empty position displayName");
		a_output = { { "source", a_source }, { "animationID", animationID }, { "positionID", positionID }, { "loadPriority", a_root.value("loadPriority", 0) },
			{ "actors", std::move(actors) }, { "hidden", positionNode.value("hidden", false) }, { "tags", std::move(tags) }, { "displayName", displayName },
			{ "extensions", a_root.contains("extensions") && a_root["extensions"].is_object() ? a_root["extensions"] : nlohmann::json::object() } };
		return true;
	}

	inline bool Register(const nlohmann::json& a_definition, bool a_verbose) {
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
		}
		auto position = std::make_shared<Position>(); position->id = positionID; position->idForType = animationID; position->loadPriority = animation->loadPriority; position->posType = Position::kAnimation; position->hidden = a_definition.value("hidden", false);
		for (const auto& tag : a_definition.value("tags", nlohmann::json::array())) { if (!tag.is_string()) continue; auto value = tag.get<std::string>(); Utility::TransformStringToLower(value); position->tags.emplace(std::move(value)); }
		const auto displayName = a_definition.value("displayName", positionID); if (displayName != positionID) records.push_back({ "naf.extended", "position.displayName", nlohmann::json(displayName).dump() });
		if (const auto it = a_definition.find("extensions"); it != a_definition.end() && it->is_object()) for (const auto& [nameSpace, value] : it->items()) {
			if (value.is_object()) for (const auto& [key, item] : value.items()) records.push_back({ nameSpace, key, item.dump() }); else records.push_back({ nameSpace, "value", value.dump() }); }
		Global::Animations.priority_insert(animation); Global::Positions.priority_insert(position);
		{ std::unique_lock lock{ metadataLock }; metadata[positionID] = std::move(records); displayNames[positionID] = displayName; }
		return true;
	}

	inline bool ParseAndNormalize(const std::filesystem::path& a_path, nlohmann::json& a_output, bool a_verbose) {
		std::ifstream stream{ a_path, std::ios::binary }; if (!stream) { if (a_verbose) logger::warn("Failed to open JSON animation definition '{}'", a_path.generic_string()); return false; }
		try { nlohmann::json root; stream >> root; return Normalize(root, a_path.generic_string(), a_output, a_verbose); }
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
		std::size_t loaded = 0; for (const auto& definition : definitions) if (Register(definition, a_verbose)) { ++loaded; if (a_verbose) logger::info("Loaded experimental JSON animation definition {}", definition.value("source", std::string{ "<cache>" })); }
		if (a_verbose) logger::info("JSON animation packs: {} definitions registered from {} in {:.3f}s", loaded, warm ? "NAFPackCache.bin" : "source files",
			std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
	}
	inline bool ParseFile(const std::filesystem::path& a_path, bool a_verbose = true) { nlohmann::json normalized; return ParseAndNormalize(a_path, normalized, a_verbose) && Register(normalized, a_verbose); }
}
