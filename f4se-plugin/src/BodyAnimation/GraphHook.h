#pragma once
#include "NodeAnimation.h"
#include "NodeAnimationGraph.h"

namespace BodyAnimation
{
	class GraphHook
	{
	public:
		typedef bool(GraphUpdate)(RE::IAnimationGraphManagerHolder* a_graphHolder, RE::BSAnimationUpdateData& a_updateData);
		typedef void(Set3d)(RE::TESObjectREFR* a_ref, RE::NiAVObject* a_object, bool a_queue);
		inline static DetourXS UpdateDetour;
		inline static DetourXS Set3dDetour;
		inline static REL::Relocation<GraphUpdate> OriginalUpdate;
		inline static REL::Relocation<Set3d> OriginalSet3d;
		inline static void (*Set3DObserver)(RE::TESObjectREFR*) = nullptr;

		struct AnimationInfo
		{
			std::string filePath = "";
			std::string id = "";

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(filePath, id);
			}
		};

		struct GraphStateInfo
		{
			Serialization::General::SerializableRefHandle ref;
			AnimationInfo animInfo;
			float animTime = 0.0f;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(ref, animInfo, animTime);
			}
		};

		struct PersistentState
		{
			std::unordered_map<RE::IAnimationGraphManagerHolder*, NodeAnimationGraph> graphs;
			// BODY v5 serializes stable actor/resource/time descriptors. Engine graph,
			// 3D and node pointers are rebuilt only after the world is active.
			std::vector<GraphStateInfo> pendingRestores;

			template <class Archive>
			void save(Archive& ar, const uint32_t) const
			{
				std::vector<GraphStateInfo> infos;
				for (const auto& pair : graphs) {
					const auto& g = pair.second;
					if (g.IsAnimating() && g.targetHandle.get() != nullptr) {
						infos.push_back({ g.targetHandle, { g.animationFilePath, g.animationId }, g.generator.localTime });
					}
				}
				for (const auto& pending : pendingRestores) {
					const auto duplicate = std::ranges::any_of(infos, [&](const GraphStateInfo& info) {
						return info.ref.hash() == pending.ref.hash();
					});
					if (!duplicate) {
						infos.push_back(pending);
					}
				}
				ar(infos);
			}

			template <class Archive>
			void load(Archive& ar, const uint32_t)
			{
				std::vector<GraphStateInfo> infos;
				ar(infos);
				pendingRestores = std::move(infos);
			}
		};

		inline static std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();
		inline static std::shared_mutex stateLock;
		inline static std::unordered_map<SerializableRefHandle, AnimationInfo> loadingAnims;
		inline static std::mutex loadingAnimsLock;
		inline static std::unordered_map<SerializableRefHandle, std::set<RE::IAnimationGraphManagerHolder*>> regsFor3d;
		inline static std::shared_mutex regsFor3dLock;
		inline static std::unordered_map<SerializableRefHandle, AnimationInfo> restoredAnimations;

		static std::unique_ptr<NodeAnimation> LoadAnimation(RE::TESObjectREFR* ref, const std::string& filePath, const std::string& id)
		{
			if (!ref)
				return nullptr;

			return LoadAnimation(GetGraphInfo(ref), filePath, id);
		}

		static std::unique_ptr<NodeAnimation> LoadAnimation(std::shared_ptr<const Data::GraphInfo> info, const std::string& filePath, const std::string& id) {
			if (!info)
				return nullptr;

			NANIM file;
			if (!file.LoadFromFile(filePath))
				return nullptr;

			std::unique_ptr<NodeAnimation> animData;
			if (!file.GetAnimation(id, info->nodeList, animData))
				return nullptr;

			return animData;
		}

		static bool IsAnimationLoading(RE::TESObjectREFR* ref, const std::string& filePath, const std::string& animId) {
			if (!ref)
				return false;

			std::unique_lock l{ loadingAnimsLock };
			if (auto iter = loadingAnims.find(ref->GetHandle());
				iter != loadingAnims.end() &&
				Utility::StringToLower(iter->second.filePath) == Utility::StringToLower(filePath) &&
				iter->second.id == animId)
			{
				return true;
			} else {
				return false;
			}
		}

		static bool LoadAndPlayAnimation(RE::TESObjectREFR* ref, const std::string& filePath, float transitionDur = 1.3f, const std::string& animName = "default")
		{
			if (!ref)
				return false;

			// Scene persistence re-requests its serialized SmartIdle assignment. If
			// BODY already reconstructed this exact NANIM, consume the one-shot
			// marker instead of racing a second asynchronous load against it.
			{
				std::unique_lock l{ stateLock };
				if (auto iter = restoredAnimations.find(ref->GetHandle()); iter != restoredAnimations.end() &&
					Utility::StringToLower(iter->second.filePath) == Utility::StringToLower(filePath) &&
					iter->second.id == animName) {
					restoredAnimations.erase(iter);
					return true;
				}
			}

			auto info = GetGraphInfo(ref);
			if (!info)
				return false;

			Serialization::General::SerializableRefHandle hndl = ref->GetHandle();

			std::unique_lock l{ loadingAnimsLock };
			loadingAnims[hndl] = { filePath, animName };
			logger::info("NANIM load queued: reference={}, file='{}', animation='{}', transition={}",
				fmt::ptr(ref), filePath, animName, transitionDur);

			std::thread([filePath = filePath, animName = animName, info = info, hndl = hndl, transitionDur = transitionDur]() {
				auto animData = LoadAnimation(info, filePath, animName);
				auto ref = hndl.get();

				std::unique_lock l{ loadingAnimsLock };
				if (auto iter = loadingAnims.find(hndl); iter != loadingAnims.end() && iter->second.filePath == filePath && iter->second.id == animName) {
					if (animData != nullptr && ref != nullptr) {
						if (!StartAnimation(ref.get(), std::move(animData), transitionDur, filePath, animName)) {
							logger::warn("NANIM load completed but graph installation failed: file='{}', animation='{}'", filePath, animName);
						}
					} else {
						logger::warn("NANIM load failed or target became unavailable: file='{}', animation='{}'", filePath, animName);
					}
					loadingAnims.erase(hndl);
				}

			}).detach();

			return true;
		}

		static bool StartAnimation(RE::TESObjectREFR* ref, std::unique_ptr<NodeAnimation> anim, float transitionDur = 1.3f, const std::string_view& filePath = "", const std::string_view& id = "")
		{
			if (!ref || !anim)
				return false;

			std::unique_lock l{ stateLock };
			auto g = GetOrCreateGraph(ref);
			g->TransitionToAnimation(std::move(anim), transitionDur);
			g->animationFilePath = filePath;
			g->animationId = id;
			logger::info("NANIM graph installed: reference={}, file='{}', animation='{}', transition={}",
				fmt::ptr(ref), filePath, id, transitionDur);

			return true;
		}

		static bool StopAnimation(RE::TESObjectREFR* ref, float transitionDur = 1.3f) {
			if (!ref)
				return false;

			std::unique_lock l1{ loadingAnimsLock };
			loadingAnims.erase(ref->GetHandle());

			std::unique_lock l2{ stateLock };
			auto g = GetGraph(ref);
			if (!g)
				return false;

			g->TransitionToAnimation(nullptr, transitionDur);
			logger::info("NANIM stop requested: reference={}, transition={}", fmt::ptr(ref), transitionDur);
			return true;
		}

		static bool VisitGraph(RE::TESObjectREFR* ref, std::function<void(NodeAnimationGraph*)> visitFunc, bool createIfNeeded = true, bool onlyVisitIfAnimating = false) {
			if (!ref)
				return false;

			std::unique_lock le{ stateLock, std::defer_lock };
			std::shared_lock ls{ stateLock };
			NodeAnimationGraph* g = nullptr;
			g = GetGraph(ref);
			if (g == nullptr && createIfNeeded) {
				//Only acquire exclusive access to the graphs map if a graph needs to be created.
				ls.unlock();
				le.lock();
				g = CreateGraph(ref);
			}

			if (g != nullptr) {
				std::unique_lock l{ g->updateLock };
				if (!onlyVisitIfAnimating || g->IsAnimating()) {
					if (visitFunc != nullptr)
						visitFunc(g);
					return true;
				}
			}

			return false;
		}

		static bool DeleteGraph(RE::TESObjectREFR* ref) {
			if (!ref)
				return false;

			std::unique_lock l{ stateLock };
			state->graphs.erase(ref);
			return true;
		}

		static void RegisterGraphFor3DChange(RE::IAnimationGraphManagerHolder* a_graphHolder, SerializableRefHandle a_ref) {
			std::unique_lock l{ regsFor3dLock };
			regsFor3d[a_ref].insert(a_graphHolder);
		}

		static void UnregisterGraphFor3DChange(RE::IAnimationGraphManagerHolder* a_graphHolder, SerializableRefHandle a_ref) {
			std::unique_lock l{ regsFor3dLock };
			auto& target = regsFor3d[a_ref];
			target.erase(a_graphHolder);
			if (target.empty()) {
				regsFor3d.erase(a_ref);
			}
		}

		static void UnregisterGraphForAll3DChanges(RE::IAnimationGraphManagerHolder* a_graphHolder) {
			std::unique_lock l{ regsFor3dLock };
			for (auto iter = regsFor3d.begin(); iter != regsFor3d.end();) {
				iter->second.erase(a_graphHolder);
				if (iter->second.empty()) {
					iter = regsFor3d.erase(iter);
				} else {
					iter++;
				}
			}
		}

		static bool HookedGraphUpdate(RE::IAnimationGraphManagerHolder* a_graphHolder, RE::BSAnimationUpdateData& a_updateData)
		{
			const bool result = OriginalUpdate(a_graphHolder, a_updateData);
			float deltaTime = 0.0F;
			std::memcpy(&deltaTime, std::addressof(a_updateData), sizeof(deltaTime));
			std::shared_lock l1{ stateLock };

			auto iter = state->graphs.find(a_graphHolder);
			if (iter == state->graphs.end() || !a_graphHolder->ShouldUpdateAnimation())
				return result;

			auto& g = iter->second;
			std::unique_lock l2{ g.updateLock };

			g.Update(deltaTime);

			if (g.flags.all(NodeAnimationGraph::kTemporary, NodeAnimationGraph::kNoActiveIKChains) && g.state == NodeAnimationGraph::kIdle) {
				l1.unlock();
				l2.unlock();
				DeleteGraphInternal(a_graphHolder);
			}
			return result;
		}

		static void HookedSet3D(RE::TESObjectREFR* a_ref, RE::NiAVObject* a_object, bool a_queue) {
			OriginalSet3d(a_ref, a_object, a_queue);
			if (Set3DObserver) {
				Set3DObserver(a_ref);
			}
			{
				std::shared_lock l1{ stateLock };
				std::shared_lock l2{ regsFor3dLock };

				if (auto iter = regsFor3d.find(a_ref->GetHandle()); iter != regsFor3d.end()) {
					for (auto& g : iter->second) {
						if (auto iter2 = state->graphs.find(g); iter2 != state->graphs.end()) {
							std::unique_lock l3{ iter2->second.updateLock };
							iter2->second.UpdateNodes(a_ref);
						}
					}
				}
			}
			TryRestorePendingForReference(a_ref);
		}

		static void RegisterSet3DObserver(void (*a_observer)(RE::TESObjectREFR*))
		{
			Set3DObserver = a_observer;
		}

		static void Reset() {
			std::scoped_lock l{ loadingAnimsLock, stateLock, regsFor3dLock };
			state->graphs.clear();
			state->pendingRestores.clear();
			loadingAnims.clear();
			regsFor3d.clear();
			restoredAnimations.clear();
		}

		static void FixupAfterSerialization()
		{
			std::vector<GraphStateInfo> pending;
			{
				std::unique_lock l{ stateLock };
				pending.swap(state->pendingRestores);
				restoredAnimations.clear();
			}

			std::vector<GraphStateInfo> unresolved;
			std::size_t restored = 0;
			for (const auto& info : pending) {
				auto ref = info.ref.get();
				if (!ref || !ref->Get3D()) {
					unresolved.push_back(info);
					continue;
				}

				auto animation = LoadAnimation(ref.get(), info.animInfo.filePath, info.animInfo.id);
				if (!animation || !StartAnimation(ref.get(), std::move(animation), 0.0f,
						info.animInfo.filePath, info.animInfo.id)) {
					unresolved.push_back(info);
					continue;
				}

				{
					std::unique_lock l{ stateLock };
					if (auto graph = GetGraph(ref.get()); graph) {
						graph->generator.localTime = info.animTime;
						restoredAnimations[ref->GetHandle()] = info.animInfo;
						++restored;
					} else {
						unresolved.push_back(info);
					}
				}
			}

			{
				std::unique_lock l{ stateLock };
				state->pendingRestores.insert(state->pendingRestores.end(), unresolved.begin(), unresolved.end());
			}
			logger::info("BODY persistence fixup: {} NANIM graph(s) restored, {} descriptor(s) retained unresolved",
				restored, unresolved.size());
		}

		static void FinishSerializationFixup()
		{
			std::unique_lock l{ stateLock };
			restoredAnimations.clear();
		}

		static bool RegisterHook()
		{
			if (REL::Module::get().version() != F4SE::RUNTIME_1_11_240) {
				logger::critical("Animation graph hooks require Fallout 4 runtime 1.11.240.0");
				return false;
			}

			//IAnimationGraphManagerHolder::UpdateAnimationGraphManager(BSAnimationUpdateData)
			REL::Relocation<GraphUpdate> hookLoc1{ REL::RelocationID(1492656, 2214536) };
			constexpr std::array<std::uint8_t, 16> updatePrologue{
				0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74,
				0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x40
			};

			//TESObjectREFR::Set3D
			REL::Relocation<Set3d> hookLoc2{ REL::RelocationID(1216262, 2201055) };
			constexpr std::array<std::uint8_t, 16> set3DPrologue{
				0x44, 0x88, 0x44, 0x24, 0x18, 0x48, 0x89, 0x54,
				0x24, 0x10, 0x48, 0x89, 0x4C, 0x24, 0x08, 0x55
			};

			if (std::memcmp(reinterpret_cast<const void*>(hookLoc1.address()), updatePrologue.data(), updatePrologue.size()) != 0 ||
				std::memcmp(reinterpret_cast<const void*>(hookLoc2.address()), set3DPrologue.data(), set3DPrologue.size()) != 0) {
				logger::critical("Animation graph hook validation failed; refusing to patch the runtime");
				return false;
			}

			if (!UpdateDetour.Create(reinterpret_cast<LPVOID>(hookLoc1.address()), &HookedGraphUpdate)) {
				logger::critical("Failed to create IAnimationGraphManagerHolder::UpdateAnimationGraphManager hook");
				return false;
			}
			OriginalUpdate = reinterpret_cast<uintptr_t>(UpdateDetour.GetTrampoline());

			if (!Set3dDetour.Create(reinterpret_cast<LPVOID>(hookLoc2.address()), &HookedSet3D)) {
				logger::critical("Failed to create TESObjectREFR::Set3D hook");
				return false;
			}
			OriginalSet3d = reinterpret_cast<uintptr_t>(Set3dDetour.GetTrampoline());
			logger::info("Installed production animation graph hooks at IDs 2214536 and 2201055");
			return true;
		}

	private:
		static bool TryRestorePendingForReference(RE::TESObjectREFR* ref)
		{
			if (!ref || !ref->Get3D()) {
				return false;
			}
			std::optional<GraphStateInfo> pending;
			{
				std::unique_lock l{ stateLock };
				auto handle = ref->GetHandle();
				if (auto iter = std::ranges::find_if(state->pendingRestores, [&](const GraphStateInfo& info) {
						return info.ref.hash() == handle.native_handle();
					}); iter != state->pendingRestores.end()) {
					pending = *iter;
					state->pendingRestores.erase(iter);
				}
			}
			if (!pending) {
				return false;
			}
			auto animation = LoadAnimation(ref, pending->animInfo.filePath, pending->animInfo.id);
			if (!animation || !StartAnimation(ref, std::move(animation), 0.0f,
					pending->animInfo.filePath, pending->animInfo.id)) {
				std::unique_lock l{ stateLock };
				state->pendingRestores.push_back(*pending);
				return false;
			}
			std::unique_lock l{ stateLock };
			if (auto graph = GetGraph(ref); graph) {
				graph->generator.localTime = pending->animTime;
				logger::info("BODY persistence restored NANIM after Set3D for reference {}", fmt::ptr(ref));
				return true;
			}
			state->pendingRestores.push_back(*pending);
			return false;
		}

		static NodeAnimationGraph* GetOrCreateGraph(RE::TESObjectREFR* ref) {
			if (auto g = GetGraph(ref); g != nullptr) {
				return g;
			}

			return CreateGraph(ref);
		}

		static NodeAnimationGraph* GetGraph(RE::TESObjectREFR* ref) {
			if (auto iter = state->graphs.find(ref); iter != state->graphs.end()) {
				return &iter->second;
			}

			return nullptr;
		}

		static NodeAnimationGraph* CreateGraph(RE::TESObjectREFR* ref)
		{
			NodeAnimationGraph* result = &state->graphs[ref];
			result->targetHandle = ref->GetHandle();
			result->reg3dCallback = std::bind(RegisterGraphFor3DChange, static_cast<RE::IAnimationGraphManagerHolder*>(ref), std::placeholders::_1);
			result->ikManager.reg3dCallback = &result->reg3dCallback;
			result->unreg3dCallback = std::bind(UnregisterGraphForAll3DChanges, static_cast<RE::IAnimationGraphManagerHolder*>(ref));
			RegisterGraphFor3DChange(ref, ref->GetHandle());
			if (auto info = GetGraphInfo(ref); info != nullptr) {
				result->SetGraphData(*info);
			}
			result->UpdateNodes(ref);
			return result;
		}

		static void DeleteGraphInternal(RE::IAnimationGraphManagerHolder* a_graphHolder) {
			std::unique_lock l{ stateLock };
			state->graphs.erase(a_graphHolder);
		}

		static std::shared_ptr<const Data::GraphInfo> GetGraphInfo(RE::TESObjectREFR* ref) {
			RE::BSFixedString graphName;
			EngineCompat::GetAnimationGraphProjectName(ref, graphName);
			if (auto info = Data::GetGraphInfo(graphName.c_str(), ref->As<RE::Actor>()); info != nullptr) {
				return info;
			}
			return nullptr;
		}
	};
}
