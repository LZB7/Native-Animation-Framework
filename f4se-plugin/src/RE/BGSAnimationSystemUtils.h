#pragma once

namespace RE
{
	namespace BGSAnimationSystemUtils
	{
		inline bool WillEventChangeState(const TESObjectREFR& ref, const BSFixedString& evn)
		{
			using func_t = decltype(&WillEventChangeState);
			REL::Relocation<func_t> func{ REL::ID(35074) };
			return func(ref, evn);
		}

		inline float GetAnimationMaxDuration(TESObjectREFR* a_refr) {
			using func_t = decltype(&GetAnimationMaxDuration);
			REL::Relocation<func_t> func{ REL::ID(177755) };
			return func(a_refr);
		}

		inline bool GetEventSourcePointersFromGraph(const TESObjectREFR* a_refr, BSScrapArray<BSTEventSource<RE::BSAnimationGraphEvent>*>& a_sourcesOut) {
			using func_t = decltype(&GetEventSourcePointersFromGraph);
			REL::Relocation<func_t> func{ REL::RelocationID(897074, 2249847) };
			return func(a_refr, a_sourcesOut);
		}

		inline void NotifyGraphSources(const TESObjectREFR* a_refr, RE::BSAnimationGraphEvent& a_event) {
			RE::BSScrapArray<RE::BSTEventSource<RE::BSAnimationGraphEvent>*> sources;
			if (GetEventSourcePointersFromGraph(a_refr, sources)) {
				for (const auto& s : sources) {
					if (s != nullptr)
						s->Notify(a_event);
				}
			}
		}

		inline BGSAction* GetDefaultObjectForActionInitializeToBaseState() {
			using func_t = decltype(&GetDefaultObjectForActionInitializeToBaseState);
			REL::Relocation<func_t> func{ REL::RelocationID(639576, 2214309) };
			return func();
		}

		inline bool RunActionOnActor(Actor* a_actor, TESActionData& a_action, bool a_unk = false) {
			if (!a_actor || a_unk) {
				return false;
			}

			// 1.11.221 removed the historical BGSAnimationSystemUtils wrapper.
			// Its production callers construct TESActionData for the actor/action
			// and invoke the PDB-named TESActionData::DoIt entry directly.
			using func_t = bool(TESActionData*);
			REL::Relocation<func_t> func{ REL::RelocationID(390914, 2234273) };
			return func(std::addressof(a_action));
		}
	};
}
