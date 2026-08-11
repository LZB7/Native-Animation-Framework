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
			REL::Relocation<func_t> func{ REL::RelocationID(1448133, 2201672) };
			return func(a_sink);
		}

		void UnregisterForPackage(BSTEventSink<TESPackageEvent>* a_sink) {
			using func_t = decltype(&UnregisterForPackage);
			REL::Relocation<func_t> func{ REL::RelocationID(1182113, 2201673) };
			return func(a_sink);
		}

		void RegisterForActorLocationChange(BSTEventSink<TESActorLocationChangeEvent>* a_sink)
		{
			using func_t = decltype(&RegisterForActorLocationChange);
			REL::Relocation<func_t> func{ REL::RelocationID(78267, 2201562) };
			return func(a_sink);
		}
	}
}
