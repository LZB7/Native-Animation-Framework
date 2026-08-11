#pragma once

namespace Tasks
{
	namespace GameLoopHook
	{
		struct ActorAlignInfo
		{
			RE::NiPointer<RE::Actor> actor;
			RE::NiPoint3 location;
			RE::NiPoint3 angle;
		};

		typedef bool(ProcessQueues)(void*, float, uint32_t);

		REL::Relocation<ProcessQueues> OriginalProcessQueues;
		inline std::atomic<std::uint64_t> diagnosticCallbackCount{ 0 };
		inline std::atomic<std::uint32_t> diagnosticThreadID{ 0 };
		inline std::atomic<std::uint32_t> diagnosticInFlight{ 0 };
		inline std::atomic<std::uint32_t> diagnosticMaxInFlight{ 0 };
		inline std::atomic<std::int64_t> diagnosticLastEntryQPC{ 0 };
		inline std::atomic<bool> diagnosticOverlapReported{ false };
		inline std::atomic<std::uint32_t> diagnosticBookkeepingTicks{ 0 };
		inline std::atomic<bool> diagnosticEligibilityAttempted{ false };
		inline bool diagnosticOnly = false;
		inline constexpr std::size_t playerGet3DSlot = 0x8C;
		inline constexpr std::uintptr_t playerGet3DRVA_1_11_221 = 0xD5BCC0;

		[[nodiscard]] bool IsReadableMemory(const void* a_address, std::size_t a_size)
		{
			if (!a_address || a_size == 0) {
				return false;
			}

			MEMORY_BASIC_INFORMATION memory{};
			if (VirtualQuery(a_address, &memory, sizeof(memory)) != sizeof(memory) ||
				memory.State != MEM_COMMIT || memory.Protect == PAGE_NOACCESS || (memory.Protect & PAGE_GUARD) != 0) {
				return false;
			}

			const auto begin = reinterpret_cast<std::uintptr_t>(a_address);
			const auto regionEnd = reinterpret_cast<std::uintptr_t>(memory.BaseAddress) + memory.RegionSize;
			return begin <= regionEnd && a_size <= regionEnd - begin;
		}

		[[nodiscard]] bool ValidatePlayerObjectForDiagnostic(
			RE::PlayerCharacter* a_player,
			std::uintptr_t& a_vtable,
			std::uintptr_t& a_get3DTarget)
		{
			a_vtable = 0;
			a_get3DTarget = 0;
			if (!a_player) {
				return false;
			}

			if (!IsReadableMemory(a_player, sizeof(RE::PlayerCharacter))) {
				return false;
			}

			std::memcpy(&a_vtable, a_player, sizeof(a_vtable));
			const auto rdata = REL::Module::get().segment(REL::Segment::rdata);
			if (a_vtable < rdata.address() || a_vtable + (playerGet3DSlot + 1) * sizeof(std::uintptr_t) > rdata.address() + rdata.size()) {
				return false;
			}

			std::memcpy(
				&a_get3DTarget,
				reinterpret_cast<const void*>(a_vtable + playerGet3DSlot * sizeof(std::uintptr_t)),
				sizeof(a_get3DTarget));
			return REL::Module::get().version() == F4SE::RUNTIME_1_11_221 &&
				a_get3DTarget == REL::Module::get().base() + playerGet3DRVA_1_11_221;
		}

		std::mutex alignActorsLock;
		std::vector<ActorAlignInfo> alignActors;

		std::vector<ActorAlignInfo>::iterator GetAlignActorsIter(RE::Actor* a_actor)
		{
			auto iter = alignActors.begin();
			for (;iter != alignActors.end(); iter++)
			{
				if (iter->actor.get() == a_actor) {
					break;
				}
			}
			return iter;
		}

		void ForceAlignActor(RE::Actor* a_actor, const RE::NiPoint3& a_location, const RE::NiPoint3& a_angle)
		{
			if (!a_actor)
				return;

			std::unique_lock l{ alignActorsLock };
			if (auto iter = GetAlignActorsIter(a_actor); iter != alignActors.end()) {
				iter->location = a_location;
				iter->angle = a_angle;
			} else {
				alignActors.emplace_back(RE::NiPointer<RE::Actor>(a_actor), a_location, a_angle);
			}
		}

		bool StopAligningActor(RE::Actor* a_actor)
		{
			std::unique_lock l{ alignActorsLock };
			if (auto iter = GetAlignActorsIter(a_actor); iter != alignActors.end()) {
				alignActors.erase(iter);
				return true;
			}
			return false;
		}

		bool HookedGameLoop(void* qintfc, float unk01, uint32_t unk02) {
			const auto threadID = static_cast<std::uint32_t>(GetCurrentThreadId());
			LARGE_INTEGER entryQPC{};
			QueryPerformanceCounter(&entryQPC);
			const auto previousEntryQPC = diagnosticLastEntryQPC.exchange(entryQPC.QuadPart, std::memory_order_relaxed);
			const auto inFlight = diagnosticInFlight.fetch_add(1, std::memory_order_acq_rel) + 1;
			auto observedMaximum = diagnosticMaxInFlight.load(std::memory_order_relaxed);
			while (observedMaximum < inFlight &&
				!diagnosticMaxInFlight.compare_exchange_weak(observedMaximum, inFlight, std::memory_order_relaxed)) {
			}
			const auto count = diagnosticCallbackCount.fetch_add(1, std::memory_order_relaxed) + 1;
			if (diagnosticOnly && inFlight > 1 && !diagnosticOverlapReported.exchange(true, std::memory_order_relaxed)) {
				logger::warn("Concurrent game-loop diagnostic callbacks detected at #{}: thread={}, queue={}, entryQPC={}, entryInFlight={}",
					count,
					threadID,
					fmt::ptr(qintfc),
					entryQPC.QuadPart,
					inFlight);
			}

			bool res = OriginalProcessQueues(qintfc, unk01, unk02);
			if (diagnosticOnly) {
				const auto* main = RE::Main::GetSingleton();
				if (RE::Main::QGameDataLoaded() && main && main->gameActive &&
					!diagnosticEligibilityAttempted.exchange(true, std::memory_order_relaxed)) {
					REL::Relocation<RE::NiPointer<RE::PlayerCharacter>*> playerSingleton{ REL::RelocationID(303410, 2698073) };
					RE::PlayerCharacter* rawIntrusivePointer = nullptr;
					std::memcpy(&rawIntrusivePointer, reinterpret_cast<const void*>(playerSingleton.address()), sizeof(rawIntrusivePointer));
					auto* playerCharacter = playerSingleton->get();
					std::uintptr_t playerVtable = 0;
					std::uintptr_t playerGet3DTarget = 0;
					logger::info("Test-seven eligibility diagnostic player singleton: thread={}, relocationID={}, global={}, rawNiPointer={}, resolvedPlayer={}",
						threadID,
						REL::Module::get().version() == F4SE::RUNTIME_1_11_221 ? 2698073 : 303410,
						fmt::ptr(reinterpret_cast<void*>(playerSingleton.address())),
						fmt::ptr(rawIntrusivePointer),
						fmt::ptr(playerCharacter));
					if (!ValidatePlayerObjectForDiagnostic(playerCharacter, playerVtable, playerGet3DTarget)) {
						logger::critical("Test-seven eligibility diagnostic rejected PlayerCharacter ABI: player={}, vtable={}, slot8C={}, expected={}; eligibility reads suppressed",
							fmt::ptr(playerCharacter),
							fmt::ptr(reinterpret_cast<void*>(playerVtable)),
							fmt::ptr(reinterpret_cast<void*>(playerGet3DTarget)),
							fmt::ptr(reinterpret_cast<void*>(REL::Module::get().base() + playerGet3DRVA_1_11_221)));
					} else {
						// Reproduce only the read side of the historical scene eligibility path:
						// UpdateScenes' cell-loaded gate, followed by ActorIsEnabled's form flags
						// and its already verified Get3D virtual. No scene object is constructed.
						auto* parentCell = playerCharacter->GetParentCell();
						if (!IsReadableMemory(parentCell, sizeof(RE::TESObjectCELL))) {
							logger::critical("Test-seven eligibility diagnostic rejected parent cell: thread={}, player={}, parentCell={}; remaining reads suppressed",
								threadID,
								fmt::ptr(playerCharacter),
								fmt::ptr(parentCell));
						} else {
							auto* const cellLoadedData = parentCell->loadedData;
							const auto formFlags = playerCharacter->GetFormFlags();
							const bool deleted = playerCharacter->IsDeleted();
							const bool disabled = playerCharacter->IsDisabled();
							const RE::NiAVObject* player3D = nullptr;
							if (!deleted && !disabled) {
								player3D = playerCharacter->Get3D();
							}
							const bool cellReady = cellLoadedData != nullptr;
							const bool actorEnabled = !deleted && !disabled && player3D != nullptr;
							logger::info(
								"Test-seven eligibility diagnostic complete: thread={}, player={}, formFlags=0x{:08X}, deleted={}, disabled={}, parentCell={}, cellLoadedData={}, player3D={}, cellReady={}, actorEnabled={}, syntheticEligible={}, mutationPerformed=false",
								threadID,
								fmt::ptr(playerCharacter),
								formFlags,
								deleted,
								disabled,
								fmt::ptr(parentCell),
								fmt::ptr(cellLoadedData),
								fmt::ptr(player3D),
								cellReady,
								actorEnabled,
								cellReady && actorEnabled);
						}
					}
				}
				const auto bookkeeping = Scene::OrderedActionQueue::UpdateBookkeepingDiagnostic();
				if (bookkeeping.previousDelay != bookkeeping.currentDelay) {
					const auto bookkeepingTick = diagnosticBookkeepingTicks.fetch_add(1, std::memory_order_relaxed) + 1;
					logger::info("Category-A bookkeeping diagnostic #{}: ordered-action delay {} -> {}, queued engine actions={} (not executed)",
						bookkeepingTick,
						bookkeeping.previousDelay,
						bookkeeping.currentDelay,
						bookkeeping.queuedActions);
				}
				LARGE_INTEGER exitQPC{};
				QueryPerformanceCounter(&exitQPC);
				const auto remainingInFlight = diagnosticInFlight.fetch_sub(1, std::memory_order_acq_rel) - 1;
				if (count == 1) {
					diagnosticThreadID.store(threadID, std::memory_order_relaxed);
					LARGE_INTEGER frequency{};
					QueryPerformanceFrequency(&frequency);
					logger::info(
						"Game-loop threading diagnostic #1 after ProcessQueues: thread={}, mainThread={}, gameDataLoaded={}, gameActive={}, inMenuMode={}, queue={}, entryQPC={}, durationQPC={}, frequency={}, entryInFlight={}, remainingInFlight={}, maxInFlight={}",
						threadID,
						main ? main->threadID : 0,
						RE::Main::QGameDataLoaded(),
						main && main->gameActive,
						main && main->inMenuMode,
						fmt::ptr(qintfc),
						entryQPC.QuadPart,
						exitQPC.QuadPart - entryQPC.QuadPart,
						frequency.QuadPart,
						inFlight,
						remainingInFlight,
						diagnosticMaxInFlight.load(std::memory_order_relaxed));
				} else if (count <= 3 || count == 10 || count == 60 || count == 120) {
					logger::info("Game-loop threading diagnostic #{}: thread={}, initialThread={}, queue={}, entryQPC={}, deltaFromPreviousQPC={}, durationQPC={}, entryInFlight={}, remainingInFlight={}, maxInFlight={}",
						count,
						threadID,
						diagnosticThreadID.load(std::memory_order_relaxed),
						fmt::ptr(qintfc),
						entryQPC.QuadPart,
						previousEntryQPC ? entryQPC.QuadPart - previousEntryQPC : 0,
						exitQPC.QuadPart - entryQPC.QuadPart,
						inFlight,
						remainingInFlight,
						diagnosticMaxInFlight.load(std::memory_order_relaxed));
				}
				return res;
			}
			diagnosticInFlight.fetch_sub(1, std::memory_order_acq_rel);

			Scene::SceneManager::UpdateScenes();
			Scene::OrderedActionQueue::Update();

			std::unique_lock l{ alignActorsLock };
			for (const auto& info : alignActors) {
				if (!MathUtil::CoordsWithinError(info.actor->data.angle, info.angle)) {
					info.actor->SetAngleOnReference(info.angle);
				}
				if (!MathUtil::CoordsWithinError(info.actor->data.location, info.location)) {
					info.actor->SetPosition(info.location, true);
					EngineCompat::DisableCollision(info.actor.get());
					EngineCompat::SetNoCollision(info.actor.get(), true);
				}
			}

			return res;
		}

		[[nodiscard]] bool RegisterHook(F4SE::Trampoline& trampoline, bool a_diagnosticOnly = false)
		{
			//Main::UpdateNonRenderSafeAITasks + 0x1E
			//Call to TaskQueueInterface::ProcessQueues
			//1.10.163: 338354 + 0x1E; 1.11.221: 2228937 + 0x1E.
			//The unchanged numeric interior offset is independently verified, not translated.
			REL::Relocation<ProcessQueues> updateCallLoc{ REL::RelocationID(338354, 2228937), 0x1E };
			REL::Relocation<ProcessQueues> processQueues{ REL::RelocationID(882638, 2229181) };
			const auto callSite = updateCallLoc.address();
			std::int32_t displacement = 0;
			std::memcpy(&displacement, reinterpret_cast<const void*>(callSite + 1), sizeof(displacement));
			const auto encodedTarget = callSite + 5 + displacement;
			if (*reinterpret_cast<const std::uint8_t*>(callSite) != 0xE8 || encodedTarget != processQueues.address()) {
				logger::critical("Game-loop diagnostic hook validation failed at Address Library ID {} + 0x1E",
					REL::Module::get().version() == F4SE::RUNTIME_1_11_221 ? 2228937 : 338354);
				return false;
			}

			if (a_diagnosticOnly && !Scene::OrderedActionQueue::InitializeBookkeepingDiagnostic(3)) {
				logger::critical("Category-A bookkeeping diagnostic requires an empty ordered-action queue and zero delay counter");
				return false;
			}

			diagnosticOnly = a_diagnosticOnly;
			OriginalProcessQueues = trampoline.write_call<5>(callSite, &HookedGameLoop);
			logger::info("Installed {}game-loop ProcessQueues hook at Address Library ID {} + 0x1E",
				diagnosticOnly ? "diagnostic " : "",
				REL::Module::get().version() == F4SE::RUNTIME_1_11_221 ? 2228937 : 338354);
			if (diagnosticOnly) {
				logger::info("Category-A bookkeeping diagnostic armed: delayCounter=3; queued engine actions will remain deferred");
			}
			return true;
		}
	}
}
