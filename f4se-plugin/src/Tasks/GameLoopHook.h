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
			Scene::SceneManager::UpdateScenes();
			Scene::OrderedActionQueue::Update();
			NAFExtended::NativeAPI::ProcessPending();

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

		[[nodiscard]] bool RegisterHook(F4SE::Trampoline& trampoline)
		{
			// Resolve the unique direct call from Main::UpdateNonRenderSafeAITasks
			// to TaskQueueInterface::ProcessQueues. Known tested offsets are used
			// only after the resolver validates their decoded target.
			constexpr REL::ID owner{ 338354, 2228937 };
			constexpr REL::ID target{ 882638, 2229181 };
			REL::Relocation<ProcessQueues> updateCallLoc{
				owner,
				REL::VariantOffset{ REL::AUTO_CALLSITE(target)
					.or_offset(0x1E, REL::Version{ 1, 11, 221, 0 }, REL::Version{ 1, 11, 240, 0 }) }
			};
			REL::Relocation<ProcessQueues> processQueues{ target };
			const auto callSite = updateCallLoc.address();
			std::int32_t displacement = 0;
			std::memcpy(&displacement, reinterpret_cast<const void*>(callSite + 1), sizeof(displacement));
			const auto encodedTarget = callSite + 5 + displacement;
			if (*reinterpret_cast<const std::uint8_t*>(callSite) != 0xE8 || encodedTarget != processQueues.address()) {
				logger::critical("Game-loop hook validation failed for Runtime Database owner ID {}",
					2228937);
				return false;
			}

			OriginalProcessQueues = trampoline.write_call<5>(callSite, &HookedGameLoop);
			logger::info("Installed game-loop ProcessQueues hook resolved from Runtime Database owner ID {}; basic scene and ordered work enabled",
				2228937);
			return true;
		}
	}
}
