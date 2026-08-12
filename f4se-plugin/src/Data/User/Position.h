#pragma once
#include "Data/ControlSystemTypes.h"
#include "Data/XMLUtil.h"

namespace Data
{
	class Position : public IdentifiableObject, public TagHolder
	{
	public:
		enum Type : uint8_t
		{
			kAnimation = 0,
			kAnimationGroup = 1,
			kPositionTree = 2,
		};

		struct ControlSystemInfo
		{
			ControlSystemType type = kAnimationSystem;
			std::string id;
			std::shared_ptr<const Animation> anim;
			std::string startMorphSet;
			std::string stopMorphSet;
		};

		struct AnimationGroupInfo : public ControlSystemInfo
		{
			AnimationGroupInfo(std::shared_ptr<const AnimationGroup> g)
			{
				type = kAnimationGroupSystem;
				group = g;
			}

			std::shared_ptr<const AnimationGroup> group;
		};

		struct PositionTreeInfo : public ControlSystemInfo
		{
			PositionTreeInfo(std::shared_ptr<const PositionTree> t)
			{
				type = kPositionTreeSystem;
				tree = t;
			}

			std::shared_ptr<const PositionTree> tree;
		};

		std::unique_ptr<ControlSystemInfo> GetControlSystemInfo() const {
			std::unique_ptr<ControlSystemInfo> info;
			switch (posType) {
			case kAnimation:
				info = std::make_unique<ControlSystemInfo>();
				break;
			case kAnimationGroup:
				info = std::make_unique<AnimationGroupInfo>(GetAnimationGroup(idForType));
				break;
			case kPositionTree:
				info = std::make_unique<PositionTreeInfo>(GetPositionTree(idForType));
				break;
			}
			info->id = id;
			info->anim = GetBaseAnimation();
			info->startMorphSet = startMorphSet;
			info->stopMorphSet = stopMorphSet;
			return info;
		}

		std::shared_ptr<const Animation> GetBaseAnimation() const {
			std::unordered_set<std::string> visitedTrees;
			return GetBaseAnimation(visitedTrees);
		}

		std::shared_ptr<const Animation> GetBaseAnimation(std::unordered_set<std::string>& visitedTrees) const {
			switch (posType) {
				case kAnimation:
				{
					return GetAnimation(idForType);
				}
				case kAnimationGroup:
				{
					if (auto group = GetAnimationGroup(idForType); group != nullptr) {
						return group->GetBaseAnimation();
					} else {
						return nullptr;
					}
				}
				case kPositionTree:
				{
					if (!visitedTrees.insert(idForType).second) {
						logger::error("Position tree cycle encountered while resolving base animation at '{}'", idForType);
						return nullptr;
					}
					if (auto tree = GetPositionTree(idForType); tree != nullptr) {
						if (auto rootPos = GetPosition(tree->tree->position); rootPos != nullptr) {
							auto result = rootPos->GetBaseAnimation(visitedTrees);
							visitedTrees.erase(idForType);
							return result;
						}
					}
					visitedTrees.erase(idForType);
				}
				default:
					return nullptr;
			}
			
		}

		[[nodiscard]] bool IsBasicSceneCompatible() const
		{
			std::unordered_set<std::string> activeTrees;
			std::function<bool(const Position&)> validate = [&](const Position& position) -> bool {
				switch (position.posType) {
				case kAnimation:
					if (const auto animation = GetAnimation(position.idForType); animation) {
						return animation->IsBasicSceneCompatible();
					}
					return false;
				case kAnimationGroup:
					if (const auto group = GetAnimationGroup(position.idForType); group && !group->stages.empty()) {
						return std::ranges::all_of(group->stages, [](const AnimationGroup::Stage& stage) {
							const auto animation = GetAnimation(stage.animation);
							return animation && animation->IsBasicSceneCompatible();
						});
					}
					return false;
				case kPositionTree:
					{
						if (!activeTrees.insert(position.idForType).second) {
							logger::error("Rejected cyclic position-tree reference at '{}'", position.idForType);
							return false;
						}
						const auto tree = GetPositionTree(position.idForType);
						bool valid = tree && tree->tree;
						std::stack<std::shared_ptr<const PositionTree::Node>> pending;
						if (valid) {
							pending.push(tree->tree);
						}
						while (valid && !pending.empty()) {
							const auto node = pending.top();
							pending.pop();
							const auto childPosition = node ? GetPosition(node->position) : nullptr;
							valid = childPosition && validate(*childPosition);
							if (node) {
								for (const auto& child : node->children) {
									pending.push(child);
								}
							}
						}
						activeTrees.erase(position.idForType);
						return valid;
					}
				default:
					return false;
				}
			};
			return validate(*this);
		}

		static bool Parse(XMLUtil::Mapper& m, Position& out)
		{
			out.ParseID(m);
			out.ParseTags(m);
			m(&out.hidden, false, true, false, "", "isHidden");
			m(&out.startMorphSet, XMLUtil::Mapper::emptyStr, true, false, "", "startMorphSet");
			m(&out.stopMorphSet, XMLUtil::Mapper::emptyStr, true, false, "", "stopMorphSet");
			m(&out.startEquipSet, XMLUtil::Mapper::emptyStr, true, false, "", "startEquipmentSet");
			m(&out.stopEquipSet, XMLUtil::Mapper::emptyStr, true, false, "", "stopEquipmentSet");
			
			std::string locs;
			m(&locs, XMLUtil::Mapper::emptyStr, true, false, "", "location");
			if (!locs.empty()) {
				Utility::ForEachSubstring(locs, ",", [&](const std::string_view& s) {
					out.locations.emplace_back(s);
				});
			}

			out.posType = kAnimation;
			std::string foundId = "";
			if (m(&foundId, XMLUtil::Mapper::emptyStr, true, false, "", "animation"); foundId.size() > 0) {
				out.idForType = foundId;
			} else if (m(&foundId, XMLUtil::Mapper::emptyStr, true, false, "", "animationGroup"); foundId.size() > 0) {
				out.posType = kAnimationGroup;
				out.idForType = foundId;
			} else if (m(&foundId, XMLUtil::Mapper::emptyStr, true, false, "", "positionTree"); foundId.size() > 0) {
				out.posType = kPositionTree;
				out.idForType = foundId;
			} else {
				out.idForType = out.id;
			}

			return m;
		}

		bool hidden = false;
		Type posType;
		std::string idForType;
		std::string startEquipSet;
		std::string stopEquipSet;
		std::string startMorphSet;
		std::string stopMorphSet;
		std::vector<std::string> locations;

		template <class Archive>
		void save(Archive& ar, const uint32_t) const
		{
			ar(id, hidden, posType, idForType, startEquipSet, stopEquipSet, startMorphSet, stopMorphSet, locations);
		}

		template <class Archive>
		void load(Archive& ar, const uint32_t ver)
		{
			ar(id, hidden, posType, idForType, startEquipSet, stopEquipSet, startMorphSet, stopMorphSet);
			if (ver < 2) {
				std::optional<std::string> loc;
				ar(loc);
				if (loc.has_value()) {
					locations.push_back(loc.value());
				}
			} else {
				ar(locations);
			}
			
		}
	};
}

CEREAL_CLASS_VERSION(Data::Position, 2);
