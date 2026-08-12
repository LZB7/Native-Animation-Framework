#pragma once
#include "Menu/BindableMenu.h"
#include "Misc/MathUtil.h"
#include "Scene/SceneBase.h"

namespace Menu::NAF
{
	class ItemExplorerHandler : public BindableMenu<ItemExplorerHandler, SUB_MENU_TYPE::kItemExplorer>
	{
	public:
		enum Stage
		{
			kPlugins,
			kForms
		};

		Stage currentStage = kPlugins;

		std::map<std::string, std::vector<RE::TESObjectARMO*>> formMap;
		std::unordered_set<RE::TESObjectARMO*> selectedItems;
		std::string selectedPlugin = "";

		ItemExplorerHandler(IStateManager* _stateInstance) :
			BindableMenu(_stateInstance)
		{
			logger::info("Item Explorer opened; starting playable armor enumeration");
			auto playerRace = player->race;
			auto& list = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESObjectARMO>();
			std::size_t acceptedForms = 0;
			for (auto& armo : list) {
				if (armo->GetPlayable(nullptr) && armo->GetFormRace() == playerRace) {
					const auto file = armo->GetFile();
					if (file) {
						const auto filename = file->GetFilename();
						if (!filename.empty()) {
							formMap[std::string{ filename }].push_back(armo);
							++acceptedForms;
						}
					}
				}
			}

			auto& equipped = Scene::OrderedActionQueue::state->equipment[player->GetActorHandle()].addedEquipment;
			for (auto& armo : equipped) {
				RE::TESForm* base = armo.get();
				selectedItems.insert(base->As<RE::TESObjectARMO>());
			}

			SetSearchWidgetEnabled();
			logger::info("Item Explorer source enumeration complete: source forms={}, accepted forms={}, plugins={}",
				list.size(), acceptedForms, formMap.size());
		}

		virtual ~ItemExplorerHandler()
		{
			logger::info("Item Explorer closed");
		}

		virtual BindingsVector GetBindings() override
		{
			manager->SetMenuTitle("Item Explorer");
			ConfigurePanel();
			BindingsVector result;

			switch (currentStage) {
				case kPlugins:
				{
					for (auto& iter : formMap) {
						result.push_back({ iter.first, Bind(&ItemExplorerHandler::SelectPlugin, iter.first), false, 0, 0, 0, std::nullopt, ContainsAnySelections(iter.second) });
					}
					break;
				}
				case kForms:
				{
					manager->SetMenuTitle(selectedPlugin);
					auto iter = formMap.find(selectedPlugin);
					if (iter == formMap.end()) {
						break;
					}
					for (auto& item : iter->second) {
						const auto name = item->GetFullName();
						result.push_back({ name && *name ? name : "[Unnamed Armor]", Bind(&ItemExplorerHandler::SelectForm, item), false, 0, 0, 0, std::nullopt, selectedItems.contains(item) });
					}
					break;
				}
			}

			std::sort(result.begin(), result.end(), [](const BindingInfo& b1, const BindingInfo& b2) { return b1.text < b2.text; });
			logger::info("Item Explorer list populated: stage={}, entries={}",
				currentStage == kPlugins ? "plugins" : "forms", result.size());

			return result;
		}

		bool ContainsAnySelections(const std::vector<RE::TESObjectARMO*>& list)
		{
			for (auto& item : list) {
				if (selectedItems.contains(item)) {
					return true;
				}
			}
			return false;
		}

		void SelectPlugin(const std::string& name, int)
		{
			logger::info("Item Explorer plugin selected: {}", name);
			selectedPlugin = name;
			currentStage = kForms;
			manager->RefreshList(true);
		}

		void SelectForm(RE::TESObjectARMO* frm, int)
		{
			logger::info("Item Explorer form selected: form=0x{:08X}, name='{}', action={}",
				frm->GetFormID(), frm->GetFullName(), selectedItems.contains(frm) ? "remove" : "equip");
			if (selectedItems.contains(frm)) {
				DoUnSelect(frm);
			} else {
				auto slots = frm->As<RE::TESForm>()->GetFilledSlots();
				for (auto& i : selectedItems) {
					if (i->As<RE::TESForm>()->GetFilledSlots() & slots) {
						DoUnSelect(i);
					}
				}
				DoSelect(frm);
			}
			manager->RefreshList(false);
		}

		void DoSelect(RE::TESObjectARMO* frm) {
			logger::info("Item Explorer invoking ordered equip action: form=0x{:08X}", frm->GetFormID());
			Scene::OrderedActionQueue::AddEquipment(player, frm);
			selectedItems.insert(frm);
		}

		void DoUnSelect(RE::TESObjectARMO* frm) {
			logger::info("Item Explorer invoking ordered remove action: form=0x{:08X}", frm->GetFormID());
			Scene::OrderedActionQueue::RemoveEquipment(player, frm);
			selectedItems.erase(frm);
		}

		virtual void BackImpl() override
		{
			if (currentStage == kForms) {
				currentStage = kPlugins;
				manager->RefreshList(true);
			} else {
				manager->GotoMenu(SUB_MENU_TYPE::kMain, true);
			}
		}
	};
}
