#pragma once

namespace RE
{
	struct TESActorLocationChangeEvent
	{
		NiPointer<RE::TESObjectREFR> refr;
		BGSLocation* oldLocation;
		BGSLocation* newLocation;
	};

	struct TESPackageEvent
	{
		enum PackageEventType
		{
			Begin,
			End,
			Change
		};

		NiPointer<TESObjectREFR> refr;
		uint32_t formId;
		PackageEventType type;
	};

	namespace TESObjectREFR_Events
	{
		void RegisterForPackage(BSTEventSink<TESPackageEvent>* a_sink) {
			using func_t = decltype(&RegisterForPackage);
			REL::Relocation<func_t> func{ REL::ID(1448133) };
			return func(a_sink);
		}

		void UnregisterForPackage(BSTEventSink<TESPackageEvent>* a_sink) {
			using func_t = decltype(&UnregisterForPackage);
			REL::Relocation<func_t> func{ REL::ID(1182113) };
			return func(a_sink);
		}

		void RegisterForActorLocationChange(BSTEventSink<TESActorLocationChangeEvent>* a_sink)
		{
			using func_t = decltype(&RegisterForActorLocationChange);
			REL::Relocation<func_t> func{ REL::ID(78267) };
			return func(a_sink);
		}
	}
}
