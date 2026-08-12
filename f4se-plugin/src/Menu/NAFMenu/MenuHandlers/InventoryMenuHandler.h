#pragma once
#include "Menu/BindableMenu.h"
#include "Misc/MathUtil.h"
#include "Scene/SceneBase.h"
#include "Scene/SceneManager.h"

namespace Menu::NAF
{
	class InventoryMenuHandler : public BindableMenu<InventoryMenuHandler, SUB_MENU_TYPE::kInventories>
	{
	public:
		class MenuOpenCloseEventHandler :
			public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
			public Singleton<MenuOpenCloseEventHandler>,
			public Data::EventListener<MenuOpenCloseEventHandler>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
			{
				if (event.menuName != "ContainerMenu")
					return RE::BSEventNotifyControl::kContinue;

				auto state = Menu::PersistentMenuState::GetSingleton();
				if (state->restoreSubmenu != Menu::SUB_MENU_TYPE::kInventories)
					return RE::BSEventNotifyControl::kContinue;

				if (event.opening) {
					logger::info("Active Scenes inventory: ContainerMenu opened; inventory enumeration, metadata, selection, and actions are engine-owned");
					EngineCompat::UpdateMenuMode(RE::UI::GetSingleton(), "ContainerMenu", false);
				} else {
					logger::info("Active Scenes inventory: ContainerMenu closed; restoring NAF menu state");
					if (auto& a = state->sceneData->pendingActor; a.has_value()) {
						auto targetActor = a.value().get().get();
						if (targetActor) {
							logger::debug("Active Scenes inventory: removing temporary ShowWornItems keyword from actor {:p}", static_cast<void*>(targetActor));
							EngineCompat::ModifyKeyword(targetActor, Data::Forms::ShowWornItemsKW, false);
							a = std::nullopt;
						}
					}

					F4SE::GetTaskInterface()->AddTask([]() {
						RE::UIMessageQueue::GetSingleton()->AddMessage("NAFMenu", RE::UI_MESSAGE_TYPE::kShow);
					});
				}

				return RE::BSEventNotifyControl::kContinue;
			}

			void OnGameDataReady(Data::Events::event_type, Data::Events::EventData&)
			{
				RE::UI::GetSingleton()->RegisterSink(this);
			}

		protected:
			friend class Singleton;
			MenuOpenCloseEventHandler()
			{
				RegisterListener(Data::Events::GAME_DATA_READY, &MenuOpenCloseEventHandler::OnGameDataReady);
			}
		};

		using BindableMenu::BindableMenu;
		virtual ~InventoryMenuHandler() {}

		uint64_t curSceneId = 0;

		virtual void InitSubmenu() override
		{
			BindableMenu::InitSubmenu();

			auto sceneData = PersistentMenuState::SceneData::GetSingleton();
			curSceneId = sceneData->pendingSceneId;
			sceneData->pendingSceneId = 0;
		}

		virtual BindingsVector GetBindings() override
		{
			manager->SetMenuTitle("Actor Inventories");

			BindingsVector result;
			Scene::SceneManager::VisitScene(curSceneId, [&](Scene::IScene* scn) {
				scn->ForEachActor([&](RE::Actor* currentActor, Scene::ActorPropertyMap&) {
					result.push_back({ GameUtil::GetActorName(currentActor),
						Bind(&InventoryMenuHandler::ShowActorInventory, currentActor->GetActorHandle()) });
				});
			});
			logger::info("Active Scenes inventory: prepared {} actor selection entries for scene {}", result.size(), curSceneId);

			return result;
		}

		void ShowActorInventory(RE::ActorHandle h, int)
		{
			RE::Actor* a = h.get().get();
			if (a)
			{
				logger::info("Active Scenes inventory: resolved actor {:p} for scene {}; opening production ContainerMenu", static_cast<void*>(a), curSceneId);
				bool hadSWIKeyword = a->HasKeyword(Data::Forms::ShowWornItemsKW);
				if (!hadSWIKeyword)
				{
					EngineCompat::ModifyKeyword(a, Data::Forms::ShowWornItemsKW, true);
				}

				auto state = PersistentMenuState::GetSingleton();
				state->sceneData->pendingSceneId = curSceneId;
				state->restoreSubmenu = kInventories;							
				if (!hadSWIKeyword) {
					state->sceneData->pendingActor = h;
				} else {
					state->sceneData->pendingActor = std::nullopt;
				}

				manager->CloseMenu();
				logger::debug("Active Scenes inventory: dispatching OpenContainerMenu(mode=3, ownedNoCrime=false)");
				RE::OpenContainerMenu(a, 3, false);
				logger::debug("Active Scenes inventory: OpenContainerMenu event dispatch returned");
			} else {
				logger::warn("Active Scenes inventory: actor handle no longer resolves; suppressing menu open");
			}
		}

		virtual void BackImpl() override
		{
			manager->GotoMenu(kManageScenes, true);
		}
	};
}
