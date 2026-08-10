#pragma once

namespace EngineCompat
{
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
}
