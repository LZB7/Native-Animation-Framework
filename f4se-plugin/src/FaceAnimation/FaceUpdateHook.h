#pragma once
#include <shared_mutex>
#include "Serialization/General.h"
#include "FaceAnimation/Animation.h"

namespace FaceAnimation
{
	namespace FaceUpdateHook
	{
		using namespace Serialization::General;

		struct FaceData
		{
			struct AnimInfo
			{
				std::string animationId = "";
				bool loop = false;
				bool havokSync = false;
			};

			std::unique_ptr<FaceAnimation> anim = nullptr;
			std::string animationId = "";
			std::optional<EyeVector> eyeOverride = std::nullopt;
			std::optional<AnimInfo> animBackup = std::nullopt;
			GameUtil::GraphTime syncInfoCache;

			template <class Archive>
			void save(Archive& ar, const uint32_t) const
			{
				if (anim == nullptr) {
					ar(animationId, eyeOverride, false);
				} else {
					ar(animationId, eyeOverride, true, anim->timeElapsed, anim->loop, anim->havokSync);
				}
			}

			template <class Archive>
			void load(Archive& ar, const uint32_t)
			{
				bool hasAnim;
				ar(animationId, eyeOverride, hasAnim);
				if (hasAnim) {
					auto loadedAnim = std::make_unique<FaceAnimation>();
					ar(loadedAnim->timeElapsed, loadedAnim->loop, loadedAnim->havokSync);

					if (loadedAnim->LoadData(animationId)) {
						anim = std::move(loadedAnim);
					}
				}
			}
		};

		struct PersistentState
		{
			std::unordered_map<RE::BSFaceGenAnimationData*, SerializableActorHandle> managedDatas;
			std::unordered_map<SerializableActorHandle, FaceData> managedAnims;

			template <class Archive>
			void save(Archive& ar, const uint32_t) const
			{
				ar(managedAnims);
			}

			template <class Archive>
			void load(Archive& ar, const uint32_t)
			{
				ar(managedAnims);
			}
		};

		struct ResetPersistentState
		{
			std::unordered_set<SerializableActorHandle> pendingResets;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(pendingResets);
			}
		};

		typedef bool(Update)(RE::BSFaceGenAnimationData*, float, bool);
		typedef bool(UpdateLip)(RE::BSFaceGenAnimationData*, float);

		static REL::Relocation<Update> OriginalUpdate;
		static REL::Relocation<UpdateLip> OriginalUpdateLip;
		inline static std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();
		inline static std::unique_ptr<ResetPersistentState> resetState = std::make_unique<ResetPersistentState>();
		static std::shared_mutex stateLock;
		static std::mutex resetStateLock;
		enum class PendingRequestKind : std::uint8_t
		{
			kLoad,
			kReset
		};

		struct PendingRequest
		{
			PendingRequestKind kind{ PendingRequestKind::kLoad };
			std::string animationId;
			std::uint64_t generation{ 0 };
		};

		static std::unordered_map<SerializableActorHandle, PendingRequest> loadingAnims;
		inline static std::atomic<std::uint64_t> nextRequestGeneration{ 1 };
		static std::mutex loadingAnimsLock;
		static std::unordered_map<SerializableActorHandle, RE::NiPointer<RE::BSGeometry>> eyeGeoCache;
		static std::shared_mutex geoCacheLock;

		void BindManagedData_NonThreadSafe(SerializableActorHandle a_handle, RE::BSFaceGenAnimationData* a_data)
		{
			for (auto iter = state->managedDatas.begin(); iter != state->managedDatas.end();) {
				if (iter->second.hash() == a_handle.hash()) {
					iter = state->managedDatas.erase(iter);
				} else {
					++iter;
				}
			}
			if (a_data) {
				state->managedDatas[a_data] = a_handle;
			}
		}

		void FixupAfterSerialization()
		{
			std::scoped_lock l{ stateLock, geoCacheLock };
			state->managedDatas.clear();
			eyeGeoCache.clear();
			for (auto& pair : state->managedAnims) {
				auto actor = pair.first.get();
				if (actor) {
					if (auto data = GameUtil::GetFaceAnimData(actor.get()); data) {
						state->managedDatas[data] = pair.first;
					}
				}
			}
		}

		RE::BSGeometry* GetCachedEyeGeometry(SerializableActorHandle hndl)
		{
			RE::BSGeometry* result = nullptr;
			std::shared_lock ls{ geoCacheLock };
			auto cacheIter = eyeGeoCache.find(hndl);
			if (cacheIter != eyeGeoCache.end()) {
				result = cacheIter->second.get();
			}
			if (!result) {
				result = GameUtil::GetEyeGeometry(hndl.get().get());
				ls.unlock();
				std::unique_lock le{ geoCacheLock };
				eyeGeoCache[hndl] = RE::NiPointer<RE::BSGeometry>(result);
			}
			return result;
		}

		SerializableActorHandle IsDataManaged(RE::BSFaceGenAnimationData* data) {
			SerializableActorHandle result;
			auto iter = state->managedDatas.find(data);
			if (iter != state->managedDatas.end()) {
				result = iter->second;
			}
			return result;
		}

		void EraseIfEmpty_NonThreadSafe(RE::ActorHandle targetActor)
		{
			if (state->managedAnims[targetActor].anim == nullptr && !state->managedAnims[targetActor].eyeOverride.has_value()) {
				state->managedAnims.erase(targetActor);
				for (auto iter = state->managedDatas.begin(); iter != state->managedDatas.end();) {
					if (iter->second.hash() == targetActor.native_handle()) {
						iter = state->managedDatas.erase(iter);
						break;
					} else {
						iter++;
					}
				}
				std::unique_lock l{ geoCacheLock };
				eyeGeoCache.erase(targetActor);
			}
		}

		void EraseManagedActor_NonThreadSafe(RE::ActorHandle targetActor)
		{
			state->managedAnims.erase(targetActor);
			for (auto iter = state->managedDatas.begin(); iter != state->managedDatas.end();) {
				if (iter->second.hash() == targetActor.native_handle()) {
					iter = state->managedDatas.erase(iter);
				} else {
					++iter;
				}
			}
		}

		bool HookedUpdateLip(RE::BSFaceGenAnimationData* data, float ptimeDelta) {
			std::shared_lock l{ stateLock };

			if (auto h = IsDataManaged(data); h) {
				l.unlock();
				return true;
			}

			l.unlock();
			return OriginalUpdateLip(data, ptimeDelta);
		}

		bool HookedUpdate(RE::BSFaceGenAnimationData* data, float timeDelta, bool force)
		{
			bool result = OriginalUpdate(data, timeDelta, force);
			std::shared_lock l{ stateLock };

			if (auto h = IsDataManaged(data); h) {
				auto eyeGeo = GetCachedEyeGeometry(h);
				auto a = state->managedAnims.find(h);
				if (a == state->managedAnims.end()) {
					return result;
				}
				bool pendingDelete = false;
				RE::BSAutoLock bl{ data->instanceData.lock };

				if (a->second.anim != nullptr) {
					if (!a->second.anim->havokSync) {
						if (!a->second.anim->Update(data, eyeGeo, timeDelta)) {
							pendingDelete = true;
						}
					} else {
						const auto actor = h.get().get();
						auto& syncInfo = a->second.syncInfoCache;
						std::scoped_lock al{ a->second.anim->lock };
						if (!a->second.anim->paused && BodyAnimation::SmartIdle::GetGraphTime(actor, syncInfo) && syncInfo.current > 0.0f) {
							a->second.anim->timeElapsed = a->second.anim->loop ? std::fmod(syncInfo.current, a->second.anim->data.duration) : syncInfo.current;
						}
						a->second.anim->UpdateNoDelta(data, eyeGeo);
					}
				}

				if (a->second.eyeOverride.has_value()) {
					auto& eyes = a->second.eyeOverride.value();
					GameUtil::SetEyeCoords(eyeGeo, static_cast<float>(eyes.u), static_cast<float>(eyes.v));
				}

				l.unlock();

				if (pendingDelete) {
					std::unique_lock ul{ stateLock };
					a->second.anim = nullptr;
					EraseIfEmpty_NonThreadSafe(a->first);
				}
			}
			return result;
		}

		//Once an animation is passed to this function, it is considered "managed" by FaceUpdateHook.
		//If the animation needs to be accessed and/or modified after this point, use VisitAnimation().
		void StartAnimation(RE::ActorHandle targetActor, std::unique_ptr<FaceAnimation> anim, std::string id = "", bool animOverride = false)
		{
			auto a = targetActor.get();
			if (a == nullptr) {
				return;
			}
			{
				std::scoped_lock resetLock{ resetStateLock };
				resetState->pendingResets.erase(targetActor);
			}
			std::unique_lock l{ stateLock };
			auto& managedActor = state->managedAnims[targetActor];
			if (managedActor.animBackup.has_value() && !animOverride) {
				managedActor.animBackup = { id, anim->loop, anim->havokSync };
				return;
			}
			if (animOverride && managedActor.anim != nullptr) {
				managedActor.animBackup = { managedActor.animationId, managedActor.anim->loop, managedActor.anim->havokSync };
			}
			anim->SetStartNow();
			managedActor.anim = std::move(anim);
			managedActor.animationId = id;
			BindManagedData_NonThreadSafe(targetActor, GameUtil::GetFaceAnimData(a.get()));
		}

		bool LoadAndPlayAnimation(RE::ActorHandle targetActor, std::string id, bool loop = false, bool havokSync = false)
		{
			if (auto targetAnim = Data::GetFaceAnim(id); targetAnim == nullptr) {
				return false;
			}

			auto newAnim = std::make_unique<FaceAnimation>();
			newAnim->loop = loop;
			newAnim->havokSync = havokSync;

			const auto generation = nextRequestGeneration.fetch_add(1, std::memory_order_relaxed);
			std::unique_lock l{ loadingAnimsLock };
			loadingAnims[targetActor] = PendingRequest{ PendingRequestKind::kLoad, id, generation };
			l.unlock();

			std::thread([targetActor = std::move(targetActor), inst = std::move(newAnim), id = std::move(id), generation]() mutable {
				bool successful = inst->LoadData(id);
				std::unique_lock l{ loadingAnimsLock };
				auto pending = loadingAnims.find(targetActor);
				if (pending != loadingAnims.end() &&
					pending->second.kind == PendingRequestKind::kLoad &&
					pending->second.generation == generation) {
					loadingAnims.erase(targetActor);
					l.unlock();
					if (successful) {
						StartAnimation(targetActor, std::move(inst), id);
					}
				}
			}).detach();

			return true;
		}

		void StopAnimation(RE::ActorHandle targetActor, bool animOverride = false) {
			std::unique_lock l1{ loadingAnimsLock };
			if (loadingAnims.contains(targetActor)) {
				loadingAnims.erase(targetActor);
			}
			std::unique_lock l2{ stateLock };
			auto managedActor = state->managedAnims.find(targetActor);

			if (managedActor != state->managedAnims.end()) {
				if (!managedActor->second.animBackup.has_value()) {
					managedActor->second.anim = nullptr;
					state->managedAnims[targetActor].animationId = "";
					EraseIfEmpty_NonThreadSafe(targetActor);
				} else {
					if (animOverride) {
						FaceData::AnimInfo animInfo = managedActor->second.animBackup.value();
						managedActor->second.animBackup = std::nullopt;
						l1.unlock();
						l2.unlock();
						LoadAndPlayAnimation(targetActor, animInfo.animationId, animInfo.loop, animInfo.havokSync);
					} else {
						managedActor->second.animBackup = std::nullopt;
					}
				}
			}
		}

		// Reset is intentionally stronger than StopAnimation. It removes all NAF-owned
		// managed-face state for the actor, invalidates pending asynchronous loads, and
		// clears only the composed expression output. baseExp/modExp remain engine-owned
		// and are left untouched so the next vanilla update can resume normal composition.
		bool ResetFaceExpression(RE::ActorHandle targetActor, bool persistAcrossLoad = true)
		{
			if (!targetActor.get()) {
				return false;
			}

			const auto generation = nextRequestGeneration.fetch_add(1, std::memory_order_relaxed);
			bool resetEyes = false;
			{
				std::scoped_lock l{ loadingAnimsLock, stateLock, geoCacheLock };
				loadingAnims[targetActor] = PendingRequest{ PendingRequestKind::kReset, {}, generation };

				auto managed = state->managedAnims.find(targetActor);
				if (managed != state->managedAnims.end()) {
					resetEyes = managed->second.eyeOverride.has_value();
					if (managed->second.anim) {
						for (const auto& timeline : managed->second.anim->data.timelines) {
							resetEyes = resetEyes || timeline.isEyes;
						}
					}
				}

				EraseManagedActor_NonThreadSafe(targetActor);
				eyeGeoCache.erase(targetActor);
			}
			if (persistAcrossLoad) {
				std::scoped_lock l{ resetStateLock };
				resetState->pendingResets.insert(targetActor);
			}

			F4SE::GetTaskInterface()->AddTask([targetActor, generation, resetEyes, consumePersistence = !persistAcrossLoad]() {
				{
					std::unique_lock l{ loadingAnimsLock };
					auto pending = loadingAnims.find(targetActor);
					if (pending == loadingAnims.end() ||
						pending->second.kind != PendingRequestKind::kReset ||
						pending->second.generation != generation) {
						return;
					}
					loadingAnims.erase(pending);
				}

				auto actor = targetActor.get();
				if (!actor) {
					return;
				}
				auto data = GameUtil::GetFaceAnimData(actor.get());
				if (!data) {
					return;
				}

				{
					RE::BSAutoLock l{ data->instanceData.lock };
					std::fill(std::begin(data->finalExp.exp), std::end(data->finalExp.exp), 0.0f);
				}

				if (resetEyes) {
					if (auto eyeGeo = GameUtil::GetEyeGeometry(actor.get()); eyeGeo) {
						GameUtil::SetEyeCoords(eyeGeo, 0.0f, 0.0f);
					}
				}

				if (consumePersistence) {
					std::scoped_lock l{ resetStateLock };
					resetState->pendingResets.erase(targetActor);
				}
			});

			return true;
		}

		void ApplyPendingResetsAfterSerialization()
		{
			std::vector<SerializableActorHandle> pending;
			{
				std::scoped_lock l{ resetStateLock };
				pending.reserve(resetState->pendingResets.size());
				pending.insert(pending.end(), resetState->pendingResets.begin(), resetState->pendingResets.end());
			}
			for (const auto& handle : pending) {
				ResetFaceExpression(handle, false);
			}
		}

		//Thread-safe method for modifying an animation object already playing on an actor.
		bool VisitAnimation(RE::ActorHandle targetActor, std::function<void(FaceAnimation*)> visitFunc) {
			std::shared_lock l1{ stateLock };
			if (state->managedAnims.contains(targetActor) && state->managedAnims[targetActor].anim != nullptr) {
				auto& anim = state->managedAnims[targetActor].anim;
				std::scoped_lock l2{ anim->lock };
				visitFunc(anim.get());
				return true;
			} else {
				return false;
			}
		}

		void SetEyeOverride(RE::ActorHandle targetActor, double u, double v)
		{
			auto actor = targetActor.get();
			if (!actor) {
				return;
			}
			{
				std::scoped_lock resetLock{ resetStateLock };
				resetState->pendingResets.erase(targetActor);
			}
			std::unique_lock l{ stateLock };
			state->managedAnims[targetActor].eyeOverride = EyeVector{ u * -0.25, v * 0.2 };
			BindManagedData_NonThreadSafe(targetActor, GameUtil::GetFaceAnimData(actor.get()));
		}

		void ClearEyeOverride(RE::ActorHandle targetActor)
		{
			std::unique_lock l{ stateLock };
			if (state->managedAnims.contains(targetActor)) {
				state->managedAnims[targetActor].eyeOverride = std::nullopt;
				EraseIfEmpty_NonThreadSafe(targetActor);
			}
		}

		void Reset() {
			std::scoped_lock l{ loadingAnimsLock, stateLock, geoCacheLock, resetStateLock };
			state = std::make_unique<PersistentState>();
			resetState = std::make_unique<ResetPersistentState>();
			loadingAnims.clear();
			eyeGeoCache.clear();
		}

		void OnSet3D(RE::TESObjectREFR* a_ref)
		{
			auto actor = a_ref ? a_ref->As<RE::Actor>() : nullptr;
			if (!actor) {
				return;
			}
			const auto handle = actor->GetActorHandle();
			{
				std::scoped_lock resetLock{ resetStateLock };
				if (resetState->pendingResets.contains(handle)) {
					F4SE::GetTaskInterface()->AddTask([handle]() { ResetFaceExpression(handle, false); });
				}
			}
			std::scoped_lock l{ stateLock, geoCacheLock };
			if (!state->managedAnims.contains(handle)) {
				return;
			}
			eyeGeoCache.erase(handle);
			BindManagedData_NonThreadSafe(handle, GameUtil::GetFaceAnimData(actor));
			logger::debug("Managed face state rebound after Set3D for actor {:p}", static_cast<void*>(actor));
		}

		bool RegisterHook(F4SE::Trampoline& trampoline)
		{
			constexpr REL::ID faceOwner{ 317245, 2209487 };
			constexpr REL::ID faceTarget{ 3139, 2209139 };
			REL::Relocation<Update> faceGenUpdateCallLoc{
				faceOwner,
				REL::VariantOffset{ REL::AUTO_CALLSITE(faceTarget)
					.or_offset(0xBD, REL::Version{ 1, 11, 221, 0 }, REL::Version{ 1, 11, 240, 0 }) }
			};
			REL::Relocation<Update> faceGenUpdateTarget{ faceTarget };

			// BSFaceGenAnimationData::Update + 0x14B: direct call to
			// BSFaceGenAnimationData::UpdateMorphsFromLip(float).
			constexpr REL::ID lipTarget{ 1550953, 2209124 };
			REL::Relocation<UpdateLip> updateLipMorphsCallLoc{
				faceTarget,
				REL::VariantOffset{ REL::AUTO_CALLSITE(lipTarget)
					.or_offset(0x14B, REL::Version{ 1, 11, 221, 0 }, REL::Version{ 1, 11, 240, 0 }) }
			};
			REL::Relocation<UpdateLip> updateLipMorphsTarget{ lipTarget };

			auto validatesDirectCall = [](std::uintptr_t a_site, std::uintptr_t a_target) {
				if (*reinterpret_cast<const std::uint8_t*>(a_site) != 0xE8) {
					return false;
				}
				std::int32_t displacement = 0;
				std::memcpy(&displacement, reinterpret_cast<const void*>(a_site + 1), sizeof(displacement));
				return a_site + 5 + displacement == a_target;
			};

			if (!validatesDirectCall(faceGenUpdateCallLoc.address(), faceGenUpdateTarget.address()) ||
				!validatesDirectCall(updateLipMorphsCallLoc.address(), updateLipMorphsTarget.address())) {
				logger::critical("Face/lip hook validation failed; refusing to patch the runtime");
				return false;
			}

			OriginalUpdate = trampoline.write_call<5>(faceGenUpdateCallLoc.address(), &HookedUpdate);
			OriginalUpdateLip = trampoline.write_call<5>(updateLipMorphsCallLoc.address(), &HookedUpdateLip);
			BodyAnimation::GraphHook::RegisterSet3DObserver(&OnSet3D);
			logger::info("Installed production face/lip hooks through Runtime Database callsite resolution");
			return true;
		}
	}
}
