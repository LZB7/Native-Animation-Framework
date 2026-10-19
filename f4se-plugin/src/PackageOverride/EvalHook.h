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

	namespace detail
	{
		template <std::size_t N>
		std::string BytesToString(const std::array<std::uint8_t, N>& a_bytes)
		{
			std::string result;
			for (std::size_t i = 0; i < N; ++i) {
				if (i != 0) {
					result.push_back(' ');
				}
				result += fmt::format("{:02X}", a_bytes[i]);
			}
			return result;
		}

		template <std::size_t N>
		[[nodiscard]] bool ValidateEntry(std::string_view a_name, std::uint64_t a_id,
			std::uintptr_t a_address, const std::array<std::uint8_t, N>& a_expected)
		{
			std::array<std::uint8_t, N> actual{};
			std::memcpy(actual.data(), reinterpret_cast<const void*>(a_address), actual.size());

			MEMORY_BASIC_INFORMATION memoryInfo{};
			const auto queried = ::VirtualQuery(reinterpret_cast<const void*>(a_address), &memoryInfo, sizeof(memoryInfo));
			const auto protection = queried != 0 ? memoryInfo.Protect : 0;
			const bool executable = queried != 0 &&
				(protection & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0 &&
				(protection & (PAGE_GUARD | PAGE_NOACCESS)) == 0;

			if (actual == a_expected && executable) {
				logger::debug("{} validation passed: ID {}, address=0x{:X}, bytes={}",
					a_name, a_id, a_address, BytesToString(actual));
				return true;
			}

			std::string mismatches;
			for (std::size_t i = 0; i < N; ++i) {
				if (actual[i] != a_expected[i]) {
					if (!mismatches.empty()) {
						mismatches += ", ";
					}
					mismatches += fmt::format("+0x{:X} expected {:02X}/actual {:02X}", i, a_expected[i], actual[i]);
				}
			}

			std::string entryKind = actual == a_expected ? "the original function body" : "another patch or unexpected function body";
			std::optional<std::uintptr_t> branchTarget;
			if (actual[0] == 0xE9 && N >= 5) {
				std::int32_t displacement = 0;
				std::memcpy(&displacement, actual.data() + 1, sizeof(displacement));
				branchTarget = a_address + 5 + displacement;
				entryKind = "direct near jump";
			} else if (actual[0] == 0xEB && N >= 2) {
				branchTarget = a_address + 2 + static_cast<std::int8_t>(actual[1]);
				entryKind = "direct short jump";
			} else if (actual[0] == 0xFF && actual[1] == 0x25) {
				entryKind = "RIP-relative indirect jump";
			} else if (actual[0] == 0xCC) {
				entryKind = "breakpoint/guard";
			}

			logger::critical(
				"{} validation failed: ID {}, address=0x{:X}, executable={}, protection=0x{:X}; expected unmasked bytes [{}]; actual [{}]; mismatches [{}]; entry appears {}{}",
				a_name, a_id, a_address, executable, protection, BytesToString(a_expected), BytesToString(actual),
				mismatches.empty() ? "none (memory is not executable)" : mismatches, entryKind,
				branchTarget ? fmt::format(" to 0x{:X}", *branchTarget) : "");
			return false;
		}
	}
	
	void HookedEvaluatePackage(RE::Actor* a_actor, bool a_isCommand, bool a_resetAI) {
		if (!a_resetAI || IsEvalAllowed(a_actor))
			OriginalEvaluatePackage(a_actor, a_isCommand, a_resetAI);
	}

	bool HookedCheckPackage(RE::AIProcess* a_process, RE::Actor* a_actor, bool a_unk) {
		return IsEvalAllowed(a_actor) ? OriginalCheckPackage(a_process, a_actor, a_unk) : false;
	}

	[[nodiscard]] bool RegisterHook() {
		if (REL::Module::get().version() != F4SE::RUNTIME_1_11_240) {
			logger::critical("Package/process hooks require Fallout 4 runtime 1.11.240.0");
			return false;
		}

		// Actor::EvaluatePackage(bool, bool) and
		// AIProcess::CheckforNewPackage(Actor*, bool), respectively.
		REL::Relocation<EvaluatePackageSig> EvalPkgLoc{ REL::RelocationID(1395257, 2229805) };
		REL::Relocation<CheckPackageSig> CheckPkgLoc{ REL::RelocationID(609985, 2231539) };
		constexpr std::array<std::uint8_t, 16> evaluatePackagePrologue{
			0x40, 0x53, 0x56, 0x57, 0x48, 0x83, 0xEC, 0x60,
			0x48, 0x8B, 0x01, 0x0F, 0xB6, 0xF2, 0xB2, 0x01
		};
		constexpr std::array<std::uint8_t, 14> checkPackagePrologue{
			0x48, 0x89, 0x6C, 0x24, 0x20, 0x56, 0x41,
			0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x20
		};

		const bool evaluateValid = detail::ValidateEntry(
			"Actor::EvaluatePackage", 2229805, EvalPkgLoc.address(), evaluatePackagePrologue);
		const bool checkValid = detail::ValidateEntry(
			"AIProcess::CheckforNewPackage", 2231539, CheckPkgLoc.address(), checkPackagePrologue);
		if (!evaluateValid || !checkValid) {
			logger::critical("Package/process hook validation failed; neither package target was patched");
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
