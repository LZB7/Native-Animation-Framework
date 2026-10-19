#pragma once
#include <bcrypt.h>
#include <fstream>
#pragma comment(lib, "bcrypt.lib")
#pragma warning(push)
#pragma warning(disable: 4100)

namespace LooksMenu
{
	enum LMVersion
	{
		k1_6_20,
		k1_6_18,
		k1_7_0_3,
		k1_7_0_4
	};

	class BodyMorphInterface
	{
	public:
		virtual void Save(const F4SE::SerializationInterface*, uint32_t) {}
		virtual bool Load(const F4SE::SerializationInterface*, bool, uint32_t, const std::unordered_map<uint32_t, std::string>&) { return false; }
		virtual void Revert() {}

		virtual void LoadBodyGenSliderMods() {}
		virtual void ClearBodyGenSliders() {}

		virtual bool LoadBodyGenSliders(const std::string& filePath) { return false; }

		virtual void ForEachSlider(uint8_t gender, std::function<void(const std::shared_ptr<uint8_t>& slider)> func) {}

		virtual std::shared_ptr<uint8_t> GetTrishapeMap(const char* relativePath) { return nullptr; }
		virtual std::shared_ptr<uint8_t> GetMorphMap(RE::Actor* actor, bool isFemale) { return nullptr; }

		virtual void SetMorph(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, RE::BGSKeyword* keyword, float value) {}
		virtual float GetMorph(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, RE::BGSKeyword* keyword) { return 0.0f; }

		virtual void GetKeywords(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, std::vector<RE::BGSKeyword*>& keywords) {}
		virtual void GetMorphs(RE::Actor* actor, bool isFemale, std::vector<RE::BSFixedString>& morphs) {}
		virtual void RemoveMorphsByName(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph) {}
		virtual void RemoveMorphsByKeyword(RE::Actor* actor, bool isFemale, RE::BGSKeyword* keyword) {}
		virtual void ClearMorphs(RE::Actor* actor, bool isFemale) {}
		virtual void CloneMorphs(RE::Actor* source, RE::Actor* target) {}

		virtual void GetMorphableShapes(RE::NiAVObject* node, std::vector<std::shared_ptr<uint8_t>>& shapes) {}
		virtual bool ApplyMorphsToShapes(RE::Actor* actor, RE::NiAVObject* slotNode) { return false; }
		virtual bool ApplyMorphsToShape(RE::Actor* actor, const std::shared_ptr<uint8_t>& morphableShape) { return false; }
		virtual bool UpdateMorphs(RE::Actor* actor) { return false; }
	};

	std::optional<std::array<std::uint8_t, 32>> sha256(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::binary);
		if (!file) {
			logger::warn("Error opening file: {}", filename);
			return std::nullopt;
		}

		std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
		BCRYPT_ALG_HANDLE algorithm = nullptr;
		BCRYPT_HASH_HANDLE hash = nullptr;
		DWORD objectSize = 0;
		DWORD bytesWritten = 0;
		std::vector<std::uint8_t> hashObject;
		std::array<std::uint8_t, 32> result{};
		bool successful = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0;
		if (successful) {
			successful = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &bytesWritten, 0) >= 0;
		}
		if (successful) {
			hashObject.resize(objectSize);
			successful = BCryptCreateHash(algorithm, &hash, hashObject.data(), objectSize, nullptr, 0, 0) >= 0;
		}
		if (successful && !buffer.empty()) {
			successful = BCryptHashData(hash, buffer.data(), static_cast<ULONG>(buffer.size()), 0) >= 0;
		}
		if (successful) {
			successful = BCryptFinishHash(hash, result.data(), static_cast<ULONG>(result.size()), 0) >= 0;
		}
		if (hash) {
			BCryptDestroyHash(hash);
		}
		if (algorithm) {
			BCryptCloseAlgorithmProvider(algorithm, 0);
		}
		return successful ? std::optional{ result } : std::nullopt;
	}

	bool isInstalled = false;
	LMVersion version;

	namespace detail
	{
		typedef bool(ApplyMorphsToShapes)(BodyMorphInterface*, RE::Actor*, RE::NiAVObject*);
		typedef void(LoadBodyGenSliderMods)(BodyMorphInterface*);
		DetourXS applyMorphsHook;
		ApplyMorphsToShapes* originalApply;

		BodyMorphInterface* g_bodyMorphInterface = nullptr;

		uint64_t GetVersionOffset(uint64_t off1_6_18, uint64_t off1_6_20, uint64_t off1_7_0_3, uint64_t off1_7_0_4) {
			switch (version) {
			case k1_6_18:
				return off1_6_18;
			case k1_6_20:
				return off1_6_20;
			case k1_7_0_3:
				return off1_7_0_3;
			case k1_7_0_4:
				return off1_7_0_4;
			default:
				return 0;
			}
		}

		bool HookedApplyMorphs(BodyMorphInterface* a1, RE::Actor* a2, RE::NiAVObject* a3) {
			std::this_thread::sleep_for(std::chrono::microseconds(1));
			return originalApply(a1, a2, a3);
		}

		void RegisterHook() {
			uintptr_t baseAddr = reinterpret_cast<uintptr_t>(GetModuleHandleA("f4ee.dll"));
			if (baseAddr == 0) {
				logger::warn("LooksMenu reported installed but f4ee.dll is not loaded; body morph support disabled");
				isInstalled = false;
				return;
			}

			const auto applyAddress = baseAddr + GetVersionOffset(0x10040, 0xFFCC, 0x11400, 0x11780);
			const auto interfaceAddress = baseAddr + GetVersionOffset(0xFB440, 0xF4DF0, 0x107E00, 0x10DD30);
			if (version == k1_7_0_3 || version == k1_7_0_4) {
				constexpr std::array<std::uint8_t, 16> expectedApplyPrologue1703{
					0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74,
					0x24, 0x18, 0x48, 0x89, 0x54, 0x24, 0x10, 0x55
				};
				constexpr std::array<std::uint8_t, 16> expectedApplyPrologue1704{
					0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x54,
					0x24, 0x10, 0x55, 0x56, 0x57, 0x41, 0x56, 0x41
				};
				const auto& expectedApplyPrologue = version == k1_7_0_4 ? expectedApplyPrologue1704 : expectedApplyPrologue1703;
				const auto expectedVtable = baseAddr + (version == k1_7_0_4 ? 0xCD840 : 0xC88B0);
				const auto interfaceVtable = *reinterpret_cast<const std::uintptr_t*>(interfaceAddress);
				if (std::memcmp(reinterpret_cast<const void*>(applyAddress), expectedApplyPrologue.data(), expectedApplyPrologue.size()) != 0 ||
					interfaceVtable != expectedVtable) {
					logger::warn("LooksMenu body-morph ABI guard failed; support disabled");
					isInstalled = false;
					return;
				}
			}

			if (!applyMorphsHook.Create(reinterpret_cast<LPVOID>(applyAddress), &HookedApplyMorphs)) {
				logger::warn("Failed to create ApplyMorphsToShapes hook!");
				isInstalled = false;
			} else {
				originalApply = reinterpret_cast<ApplyMorphsToShapes*>(applyMorphsHook.GetTrampoline());
			}

			g_bodyMorphInterface = reinterpret_cast<BodyMorphInterface*>(interfaceAddress);
		}
	}

	void Init() {
		const auto module = GetModuleHandleA("f4ee.dll");
		if (!module) {
			logger::info("Loaded module f4ee.dll was not found; LooksMenu support disabled.");
			return;
		}

		std::array<char, MAX_PATH> modulePath{};
		if (GetModuleFileNameA(module, modulePath.data(), static_cast<DWORD>(modulePath.size())) == 0) {
			logger::warn("Failed to resolve the loaded f4ee.dll path; LooksMenu support disabled.");
			return;
		}

		constexpr std::array<std::uint8_t, 32> expectedHash1703{
			0xB5, 0x62, 0xD6, 0x36, 0xD7, 0x68, 0x62, 0x07,
			0x60, 0xAA, 0xD8, 0xC1, 0x66, 0xCC, 0xFD, 0xA2,
			0x2B, 0x01, 0x4F, 0x49, 0x0A, 0xBA, 0xCE, 0x0B,
			0x21, 0xE8, 0x43, 0x34, 0xB2, 0x6C, 0x56, 0xF0
		};
		constexpr std::array<std::uint8_t, 32> expectedHash1704{
			0x2E, 0xB7, 0x6A, 0x87, 0x8B, 0xAD, 0x00, 0xED,
			0xEE, 0x64, 0xD1, 0x94, 0x31, 0xD1, 0x9D, 0x07,
			0x2F, 0xFF, 0xA0, 0x96, 0x21, 0x5D, 0xEB, 0x04,
			0xDD, 0x31, 0x80, 0xAB, 0xE3, 0x9F, 0xDB, 0x0F
		};
		const auto actualHash = sha256(modulePath.data());
		if (!actualHash || (*actualHash != expectedHash1703 && *actualHash != expectedHash1704)) {
			logger::warn("Loaded f4ee.dll does not match a supported exact LooksMenu SHA-256; support disabled.");
			return;
		}

		version = *actualHash == expectedHash1704 ? k1_7_0_4 : k1_7_0_3;
		isInstalled = true;
		detail::RegisterHook();
		if (isInstalled) {
			logger::info("LooksMenu {} detected by loaded module, SHA-256 validated, and body-morph interface initialized.",
				version == k1_7_0_4 ? "1.7.0.4" : "1.7.0.3");
		}
	}

	void SetMorph(RE::Actor* actor, const RE::BSFixedString& morph, RE::BGSKeyword* keyword, float value)
	{
		if (isInstalled && actor && detail::g_bodyMorphInterface) {
			bool isFemale = (actor->GetNPC()->GetSex() == RE::SEX::kFemale);
			detail::g_bodyMorphInterface->SetMorph(actor, isFemale, morph, keyword, value);
		}
	}

	void UpdateMorphs(RE::Actor* actor)
	{
		if (isInstalled && actor && detail::g_bodyMorphInterface) {
			detail::g_bodyMorphInterface->UpdateMorphs(actor);
		}
	}

	void RemoveMorphsByName(RE::Actor* actor, const RE::BSFixedString& morph) {
		if (isInstalled && actor && detail::g_bodyMorphInterface) {
			bool isFemale = (actor->GetNPC()->GetSex() == RE::SEX::kFemale);
			detail::g_bodyMorphInterface->RemoveMorphsByName(actor, isFemale, morph);
		}
	}
}

#pragma warning(pop)
