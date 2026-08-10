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
			bool res = OriginalProcessQueues(qintfc, unk01, unk02);
			if (diagnosticOnly) {
				const auto count = diagnosticCallbackCount.fetch_add(1, std::memory_order_relaxed) + 1;
				const auto threadID = static_cast<std::uint32_t>(GetCurrentThreadId());
				if (count == 1) {
					diagnosticThreadID.store(threadID, std::memory_order_relaxed);
					const auto* main = RE::Main::GetSingleton();
					logger::info(
						"Game-loop diagnostic callback #1 after ProcessQueues: thread={}, mainThread={}, gameDataLoaded={}, gameActive={}, inMenuMode={}",
						threadID,
						main ? main->threadID : 0,
						RE::Main::QGameDataLoaded(),
						main && main->gameActive,
						main && main->inMenuMode);
				} else if (count <= 3 || count == 10 || count == 60) {
					logger::info("Game-loop diagnostic callback #{} on thread {} (initial thread {})",
						count,
						threadID,
						diagnosticThreadID.load(std::memory_order_relaxed));
				}
				return res;
			}

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

			diagnosticOnly = a_diagnosticOnly;
			OriginalProcessQueues = trampoline.write_call<5>(callSite, &HookedGameLoop);
			logger::info("Installed {}game-loop ProcessQueues hook at Address Library ID {} + 0x1E",
				diagnosticOnly ? "diagnostic " : "",
				REL::Module::get().version() == F4SE::RUNTIME_1_11_221 ? 2228937 : 338354);
			return true;
		}
	}
}
