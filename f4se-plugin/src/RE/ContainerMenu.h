
namespace RE
{
	inline void OpenContainerMenu(const TESObjectREFR* a_refr, int a_menuMode, bool a_ownedNoCrime)
	{
		using func_t = decltype(&OpenContainerMenu);
		// 1.11.221 disassembly-verified OpenContainerMenuEventData dispatcher.
		// The historical invariant ID is 1.10.163-only.
		REL::Relocation<func_t> func{ REL::ID(1105206, 2248608) };
		func(a_refr, a_menuMode, a_ownedNoCrime);
	}
}
