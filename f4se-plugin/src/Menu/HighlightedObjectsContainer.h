#pragma once
#include "Serialization/General.h"
#include <unordered_set>

namespace Menu
{
	using namespace Serialization::General;

	class HighlightedObjectsContainer
	{
	public:
		// The historical shader calls still use OG-only engine entries. Keep the
		// production scene menu usable without reaching that optional visual aid.
		inline static constexpr bool kHighlightingEnabled = false;
		RE::TESEffectShader* hoverEffect = nullptr;
		std::unordered_set<SerializableRefHandle> refs;

		HighlightedObjectsContainer()
		{
			GetEffect();
		}

		~HighlightedObjectsContainer()
		{
			RemoveAll();
		}

		bool HasEffect() {
			return hoverEffect != nullptr;
		}

		bool GetEffect() {
			if constexpr (!kHighlightingEnabled) {
				return false;
			}
			//NAF_HoverFXS
			auto hoverForm = RE::TESDataHandler::GetSingleton()->LookupForm(0x1734, "NAF.esp");
			if (hoverForm) {
				hoverEffect = hoverForm->As<RE::TESEffectShader>();
				if (hoverEffect) {
					return true;
				}
			}
			return false;
		}

		void Add(RE::TESObjectREFR* ref) {
			if constexpr (!kHighlightingEnabled) {
				return;
			}
			if (ref && HasEffect()) {
				EngineCompat::PlayEffectShader(hoverEffect, ref);
				refs.insert(ref->GetHandle());
			}
		}

		void AddSingle(RE::TESObjectREFR* ref)
		{
			RemoveAll();
			Add(ref);
		}

		void Remove(RE::TESObjectREFR* ref)
		{
			if constexpr (!kHighlightingEnabled) {
				return;
			}
			if (ref && refs.contains(ref->GetHandle())) {
				EngineCompat::StopEffectShader(hoverEffect, ref);
			}
		}

		void RemoveAll() {
			if constexpr (!kHighlightingEnabled) {
				return;
			}
			if (HasEffect()) {
				for (auto& h : refs) {
					auto r = h.get().get();
					if (r) {
						EngineCompat::StopEffectShader(hoverEffect, r);
					}
				}
			}
		}
	};
}
