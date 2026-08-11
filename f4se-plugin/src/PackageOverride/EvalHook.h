#pragma once
#include "detourxs/DetourXS.h"
#include "Manager.h"

namespace PackageOverride::EvalHook
{
	typedef void(EvaluatePackageSig)(RE::Actor*, bool, bool);
	typedef bool(CheckPackageSig)(RE::AIProcess*, RE::Actor*, bool);

	REL::Relocation<EvaluatePackageSig> OriginalEvaluatePackage;
	REL::Relocation<CheckPackageSig> OriginalCheckPackage;
	DetourXS evalHook;
	DetourXS checkHook;
	std::atomic_bool loggedEvaluatePackage{ false };
	std::atomic_bool loggedCheckPackage{ false };
	
	void HookedEvaluatePackage(RE::Actor* a_actor, bool a_isCommand, bool a_resetAI) {
		if (!loggedEvaluatePackage.exchange(true, std::memory_order_relaxed)) {
			logger::info("Production Actor::EvaluatePackage interception observed: actor={}, command={}, resetAI={}",
				fmt::ptr(a_actor), a_isCommand, a_resetAI);
		}
		if (!a_resetAI || IsEvalAllowed(a_actor))
			OriginalEvaluatePackage(a_actor, a_isCommand, a_resetAI);
	}

	bool HookedCheckPackage(RE::AIProcess* a_process, RE::Actor* a_actor, bool a_unk) {
		if (!loggedCheckPackage.exchange(true, std::memory_order_relaxed)) {
			logger::info("Production AIProcess::CheckforNewPackage interception observed: process={}, actor={}, argument={}",
				fmt::ptr(a_process), fmt::ptr(a_actor), a_unk);
		}
		return IsEvalAllowed(a_actor) ? OriginalCheckPackage(a_process, a_actor, a_unk) : false;
	}

	[[nodiscard]] bool RegisterHook() {
		if (REL::Module::get().version() != F4SE::RUNTIME_1_11_221) {
			logger::critical("Package/process hooks require Fallout 4 runtime 1.11.221.0");
			return false;
		}

		// Actor::EvaluatePackage(bool, bool) and
		// AIProcess::CheckforNewPackage(Actor*, bool), respectively.
		REL::Relocation<EvaluatePackageSig> EvalPkgLoc{ REL::RelocationID(1395257, 2229805) };
		REL::Relocation<CheckPackageSig> CheckPkgLoc{ REL::RelocationID(609985, 2231539) };
		constexpr std::array<std::uint8_t, 15> evaluatePackagePrologue{
			0x53, 0x56, 0x57, 0x48, 0x83, 0xEC, 0x60, 0x48,
			0x8B, 0x01, 0x0F, 0xB6, 0xF2, 0xB2, 0x01
		};
		constexpr std::array<std::uint8_t, 14> checkPackagePrologue{
			0x48, 0x89, 0x6C, 0x24, 0x20, 0x56, 0x41,
			0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x20
		};

		if (std::memcmp(reinterpret_cast<const void*>(EvalPkgLoc.address()), evaluatePackagePrologue.data(), evaluatePackagePrologue.size()) != 0 ||
			std::memcmp(reinterpret_cast<const void*>(CheckPkgLoc.address()), checkPackagePrologue.data(), checkPackagePrologue.size()) != 0) {
			logger::critical("Package/process hook validation failed; refusing to patch the runtime");
			return false;
		}

		if (!evalHook.Create(reinterpret_cast<LPVOID>(EvalPkgLoc.address()), &HookedEvaluatePackage)) {
			logger::critical("Failed to create Actor::EvaluatePackage hook");
			return false;
		} else {
			OriginalEvaluatePackage = reinterpret_cast<uintptr_t>(evalHook.GetTrampoline());
		}

		if (!checkHook.Create(reinterpret_cast<LPVOID>(CheckPkgLoc.address()), &HookedCheckPackage)) {
			logger::critical("Failed to create AIProcess::CheckforNewPackage hook");
			return false;
		} else {
			OriginalCheckPackage = reinterpret_cast<uintptr_t>(checkHook.GetTrampoline());
		}
		logger::info("Installed production package/process hooks at IDs 2229805 and 2231539");
		return true;
	}
}
