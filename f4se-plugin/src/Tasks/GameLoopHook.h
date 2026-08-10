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
		inline bool diagnosticOnly = false;

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
					const auto* main = RE::Main::GetSingleton();
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
