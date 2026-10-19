#pragma once

namespace EngineCompat
{
	namespace ExactRuntime
	{
		[[nodiscard]] inline bool ValidateFreeUpAllMarkers()
		{
			if (REL::Module::get().version() != F4SE::RUNTIME_1_11_240) {
				logger::critical("FreeUpAllMarkers validation requires Fallout 4 1.11.240.0");
				return false;
			}

			// Mask the two relative branch displacements while checking the verified
			// entry prologue, RCX object use, base-form load and FURN/TERM tests.
			static constexpr std::array<std::uint8_t, 43> signature{
				0x40, 0x55, 0x56, 0x48, 0x83, 0xEC, 0x28, 0x48, 0x8B, 0xB1, 0xE0, 0x00, 0x00, 0x00,
				0x48, 0x8B, 0xE9, 0x48, 0x85, 0xF6, 0x0F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xB6,
				0x46, 0x1A, 0x3C, 0x2A, 0x74, 0x08, 0x3C, 0x37, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00, 0x48
			};
			static constexpr std::array<std::uint8_t, signature.size()> mask{
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF
			};

			// The official 1.11.240 Address Library resolves exact ID 2202575 to
			// PDB-named TESObjectREFR::FreeUpAllMarkers at RVA 0x55F7B0.
			static REL::Relocation<std::uintptr_t> target{ REL::RelocationID(1324833, 2202575) };
			const auto* bytes = reinterpret_cast<const std::uint8_t*>(target.address());
			for (std::size_t i = 0; i < signature.size(); ++i) {
				if ((bytes[i] & mask[i]) != (signature[i] & mask[i])) {
					logger::critical("FreeUpAllMarkers Address Library validation failed for ID 2202575; refusing to call mismatched runtime code");
					return false;
				}
			}
			return true;
		}
	}

	[[nodiscard]] inline bool FreeUpAllMarkers(RE::TESObjectREFR* a_refr)
	{
		static const bool validated = ExactRuntime::ValidateFreeUpAllMarkers();
		if (!validated) {
			return false;
		}

		using func_t = void(RE::TESObjectREFR*);
		static REL::Relocation<func_t> func{ REL::RelocationID(1324833, 2202575) };
		func(a_refr);
		return true;
	}

	inline void ModifyKeyword(RE::TESObjectREFR* a_refr, RE::BGSKeyword* a_keyword, bool a_add)
	{
		if (a_add) {
			a_refr->AddKeyword(a_keyword);
		} else {
			a_refr->RemoveKeyword(a_keyword);
		}
	}

	[[nodiscard]] inline float GetScale(RE::TESObjectREFR* a_refr)
	{
		float result = static_cast<float>(a_refr->refScale) * 0.01f;
		if (auto npc = a_refr->GetObjectReference()->As<RE::TESNPC>()) {
			result *= npc->GetHeight(a_refr, a_refr->GetVisualsRace());
		}
		return result;
	}

	inline void SetNoCollision(RE::TESObjectREFR* a_refr, bool a_noCollision)
	{
		if (a_refr->loadedData) {
			constexpr std::uint16_t noCollision = 0x0800;
			if (a_noCollision) {
				a_refr->loadedData->flags |= noCollision;
			} else {
				a_refr->loadedData->flags &= static_cast<std::uint16_t>(~noCollision);
			}
		}
	}

	[[nodiscard]] inline RE::TESWorldSpace* GetWorldSpace(RE::TESObjectCELL* a_cell)
	{
		return a_cell && a_cell->IsExterior() ? a_cell->worldSpace : nullptr;
	}

	[[nodiscard]] inline bool IsFurniture(RE::TESObjectREFR* a_refr)
	{
		const auto base = a_refr->GetObjectReference();
		return base && (base->GetFormType() == RE::ENUM_FORM_ID::kFURN ||
			base->GetFormType() == RE::ENUM_FORM_ID::kTERM);
	}

	[[nodiscard]] inline std::string GetBaseFullName(RE::TESObjectREFR* a_refr)
	{
		const auto base = a_refr->GetObjectReference();
		return base ? std::string(RE::TESFullName::GetFullName(*base)) : std::string{};
	}

	inline void DisableCollision(RE::Actor* a_actor)
	{
		using func_t = void(RE::Actor*);
		static REL::Relocation<func_t> func{ REL::RelocationID(475952, 2229653) };
		func(a_actor);
	}

	inline void EnableCollision(RE::Actor* a_actor)
	{
		using func_t = void(RE::Actor*);
		static REL::Relocation<func_t> func{ REL::RelocationID(1274117, 2229652) };
		func(a_actor);
	}

	[[nodiscard]] inline bool GetCanDoFavor(RE::Actor* a_actor, bool a_ignoreState = false)
	{
		using func_t = bool(RE::Actor*, bool);
		static REL::Relocation<func_t> func{ REL::RelocationID(46113, 2230148) };
		return func(a_actor, a_ignoreState);
	}

	inline void SetCanDoFavor(RE::Actor* a_actor, bool a_canDoFavor)
	{
		using func_t = void(RE::Actor*, bool);
		static REL::Relocation<func_t> func{ REL::RelocationID(391670, 2230149) };
		func(a_actor, a_canDoFavor);
	}

	inline void SetRestrained(RE::Actor* a_actor, bool a_restrained)
	{
		using func_t = void(RE::Actor*, bool);
		static REL::Relocation<func_t> func{ REL::RelocationID(316742, 2229910) };
		func(a_actor, a_restrained);
	}

	inline void StopAlarmOnActor(RE::Actor* a_actor)
	{
		using func_t = void(RE::Actor*);
		static REL::Relocation<func_t> func{ REL::RelocationID(1564830, 2229667) };
		func(a_actor);
	}

	inline void StopInteractingQuick(RE::Actor* a_actor, bool a_flag1 = true, bool a_flag2 = true, bool a_initializeNodes = false)
	{
		using func_t = void(RE::Actor*, bool, bool, bool);
		static REL::Relocation<func_t> func{ REL::RelocationID(129904, 2231227) };
		func(a_actor, a_flag1, a_flag2, a_initializeNodes);
	}

	inline void StopMoving(RE::Actor* a_actor, float a_decelerationMult = 1.0f)
	{
		using func_t = void(RE::Actor*, float);
		static REL::Relocation<func_t> func{ REL::RelocationID(940952, 2230262) };
		func(a_actor, a_decelerationMult);
	}

	inline void ClearLookAtTarget(RE::Actor* a_actor)
	{
		using func_t = void(RE::Actor*);
		static REL::Relocation<func_t> func{ REL::RelocationID(1345971, 2230291) };
		func(a_actor);
	}

	inline void TurnOffHeadtracking(RE::Actor* a_actor)
	{
		using func_t = void(RE::Actor*);
		static REL::Relocation<func_t> func{ REL::RelocationID(149104, 2230240) };
		func(a_actor);
	}

	inline void WarpToRef(RE::Actor* a_actor, const RE::TESObjectREFR* a_destination)
	{
		using func_t = void(RE::Actor*, const RE::NiPoint3&, float, float, RE::TESObjectCELL*, RE::TESWorldSpace*, bool, bool, bool);
		static REL::Relocation<func_t> func{ REL::RelocationID(728864, 2229838) };
		func(a_actor,
			a_destination->data.location,
			a_destination->data.angle.z,
			0.0f,
			a_destination->parentCell,
			GetWorldSpace(a_destination->parentCell),
			false,
			false,
			false);
	}

	inline void UpdatePlayer3D(RE::PlayerCharacter* a_player)
	{
		using func_t = void(RE::AIProcess*, RE::Actor*, bool);
		static REL::Relocation<func_t> func{ REL::RelocationID(986782, 2231882) };
		func(a_player->currentProcess, a_player, true);
	}

	inline void GetAnimationGraphProjectName(RE::IAnimationGraphManagerHolder* a_holder, RE::BSFixedString& a_name)
	{
		using func_t = void(RE::IAnimationGraphManagerHolder*, RE::BSFixedString&);
		static REL::Relocation<func_t> func{ REL::RelocationID(190119, 2214534) };
		func(a_holder, a_name);
	}

	inline void UpdateMenuMode(RE::UI* a_ui, const RE::BSFixedString& a_menuName, bool a_enterMenuMode)
	{
		RE::BSAutoWriteLock lock{ RE::UI::GetMenuMapRWLock() };
		const auto it = a_ui->menuMap.find(a_menuName);
		if (it == a_ui->menuMap.end() || !it->second.menu) {
			return;
		}

		auto* menu = it->second.menu.get();
		if (menu->menuFlags.all(RE::UI_MENU_FLAGS::kPausesGame) == a_enterMenuMode) {
			return;
		}

		const auto oldMenuMode = a_ui->menuMode;
		menu->UpdateFlag(RE::UI_MENU_FLAGS::kPausesGame, a_enterMenuMode);
		if (a_enterMenuMode) {
			++a_ui->menuMode;
		} else {
			--a_ui->menuMode;
		}

		const RE::MenuModeCounterChangedEvent counterEvent{ a_menuName, a_enterMenuMode };
		a_ui->GetEventSource<RE::MenuModeCounterChangedEvent>()->Notify(counterEvent);
		if ((oldMenuMode == 0) != (a_ui->menuMode == 0)) {
			const RE::MenuModeChangeEvent modeEvent{ a_menuName, a_ui->menuMode != 0 };
			a_ui->GetEventSource<RE::MenuModeChangeEvent>()->Notify(modeEvent);
		}
	}

	inline void Force3rdPerson(RE::PlayerCamera* a_camera)
	{
		using func_t = void(RE::PlayerCamera*);
		static REL::Relocation<func_t> func{ REL::RelocationID(162201, 2248348) };
		func(a_camera);
	}

	inline void DisableInputForPlayer(RE::PlayerCharacter* a_player, const char* a_layerName, std::uint32_t a_inputMask)
	{
		using func_t = void(RE::TESObjectREFR*, const char*, std::uint32_t);
		static REL::Relocation<func_t> func{ REL::RelocationID(53346, 2200872) };
		func(a_player, a_layerName, a_inputMask);
	}

	inline void ReenableInputForPlayer(RE::PlayerCharacter* a_player)
	{
		using func_t = void(RE::TESObjectREFR*);
		static REL::Relocation<func_t> func{ REL::RelocationID(478350, 2200873) };
		func(a_player);
	}

	inline void SetAIControlled(RE::PlayerCharacter* a_player, bool a_aiControlled)
	{
		using func_t = void(RE::PlayerCharacter*, bool);
		static REL::Relocation<func_t> func{ REL::RelocationID(1431972, 2233087) };
		func(a_player, a_aiControlled);
	}
}
