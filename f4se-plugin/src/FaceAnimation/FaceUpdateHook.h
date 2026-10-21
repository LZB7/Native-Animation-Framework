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

		typedef bool(Update)(RE::BSFaceGenAnimationData*, float, bool);
		typedef bool(UpdateLip)(RE::BSFaceGenAnimationData*, float);

		static REL::Relocation<Update> OriginalUpdate;
		static REL::Relocation<UpdateLip> OriginalUpdateLip;
		inline static std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();
		static std::shared_mutex stateLock;
		inline static std::unordered_set<SerializableActorHandle> pendingResets;
		static std::mutex pendingResetsLock;
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

		enum class RequestStatus : std::uint32_t
		{
			kIdle,
			kPlayPending,
			kActive,
			kResetPending,
			kStopped,
			kCompleted,
			kCanceled,
			kFailed
		};

		struct RequestState
		{
			RequestStatus status{ RequestStatus::kIdle };
			std::uint64_t generation{};
			std::string animationId;
			bool managed{};
			bool faceDataAvailable{};
			bool loop{};
			bool havokSync{};
		};

		static std::unordered_map<SerializableActorHandle, PendingRequest> loadingAnims;
		inline static std::atomic<std::uint64_t> nextRequestGeneration{ 1 };
		static std::mutex loadingAnimsLock;
		inline static std::unordered_map<std::uint32_t, RequestState> requestStates;
		static std::shared_mutex requestStatesLock;
		static std::unordered_map<SerializableActorHandle, RE::NiPointer<RE::BSGeometry>> eyeGeoCache;
		static std::shared_mutex geoCacheLock;

		void SetRequestState(std::uint32_t actorFormID, RequestState value)
		{
			if (!actorFormID) return;
			std::unique_lock lock{ requestStatesLock };
			requestStates[actorFormID] = std::move(value);
		}

		void SetRequestStateIfCurrent(std::uint32_t actorFormID, std::uint64_t generation, RequestState value)
		{
			if (!actorFormID) return;
			std::unique_lock lock{ requestStatesLock };
			if (const auto found = requestStates.find(actorFormID);
				found != requestStates.end() && found->second.generation == generation) {
				found->second = std::move(value);
			}
		}

		std::optional<RequestState> GetRequestState(std::uint32_t actorFormID)
		{
			std::shared_lock lock{ requestStatesLock };
			if (const auto found = requestStates.find(actorFormID); found != requestStates.end()) return found->second;
			return std::nullopt;
		}

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
					auto data = GameUtil::GetFaceAnimData(actor.get());
					if (data) {
						state->managedDatas[data] = pair.first;
					}
					const auto hasAnimation = pair.second.anim != nullptr;
					SetRequestState(actor->formID, {
						hasAnimation ? RequestStatus::kActive : RequestStatus::kIdle,
						nextRequestGeneration.fetch_add(1, std::memory_order_relaxed), pair.second.animationId,
						true, data != nullptr, hasAnimation && pair.second.anim->loop,
						hasAnimation && pair.second.anim->havokSync });
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
					const auto actor = h.get();
					const auto formID = actor ? actor->formID : 0;
					const auto request = GetRequestState(formID);
					const auto generation = request ? request->generation : nextRequestGeneration.fetch_add(1, std::memory_order_relaxed);
					a->second.anim = nullptr;
					EraseIfEmpty_NonThreadSafe(a->first);
					ul.unlock();
					SetRequestState(formID, { RequestStatus::kCompleted, generation, {}, false, data != nullptr, false, false });
				}
			}
			return result;
		}

		//Once an animation is passed to this function, it is considered "managed" by FaceUpdateHook.
		//If the animation needs to be accessed and/or modified after this point, use VisitAnimation().
		void StartAnimation(RE::ActorHandle targetActor, std::unique_ptr<FaceAnimation> anim, std::string id = "", bool animOverride = false, std::uint64_t requestGeneration = 0)
		{
			auto a = targetActor.get();
			if (a == nullptr) {
				return;
			}
			{
				std::scoped_lock resetLock{ pendingResetsLock };
				pendingResets.erase(targetActor);
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
			const auto loop = anim->loop;
			const auto havokSync = anim->havokSync;
			managedActor.anim = std::move(anim);
			managedActor.animationId = id;
			auto faceData = GameUtil::GetFaceAnimData(a.get());
			BindManagedData_NonThreadSafe(targetActor, faceData);
			l.unlock();
			SetRequestState(a->formID, { RequestStatus::kActive,
				requestGeneration ? requestGeneration : nextRequestGeneration.fetch_add(1, std::memory_order_relaxed),
				std::move(id), true, faceData != nullptr, loop, havokSync });
		}

		bool LoadAndPlayAnimation(RE::ActorHandle targetActor, std::string id, bool loop = false, bool havokSync = false)
		{
			auto actor = targetActor.get();
			if (!actor) {
				return false;
			}
			if (auto targetAnim = Data::GetFaceAnim(id); targetAnim == nullptr) {
				SetRequestState(actor->formID, { RequestStatus::kFailed,
					nextRequestGeneration.fetch_add(1, std::memory_order_relaxed), id, false,
					GameUtil::GetFaceAnimData(actor.get()) != nullptr, loop, havokSync });
				return false;
			}
			{
				std::scoped_lock resetLock{ pendingResetsLock };
				pendingResets.erase(targetActor);
			}

			auto newAnim = std::make_unique<FaceAnimation>();
			newAnim->loop = loop;
			newAnim->havokSync = havokSync;

			const auto generation = nextRequestGeneration.fetch_add(1, std::memory_order_relaxed);
			const auto actorFormID = actor->formID;
			SetRequestState(actorFormID, { RequestStatus::kPlayPending, generation, id, false,
				GameUtil::GetFaceAnimData(actor.get()) != nullptr, loop, havokSync });
			std::unique_lock l{ loadingAnimsLock };
			loadingAnims[targetActor] = PendingRequest{ PendingRequestKind::kLoad, id, generation };
			l.unlock();

			std::thread([targetActor = std::move(targetActor), inst = std::move(newAnim), id = std::move(id), generation, actorFormID]() mutable {
				bool successful = inst->LoadData(id);
				std::unique_lock l{ loadingAnimsLock };
				auto pending = loadingAnims.find(targetActor);
				if (pending != loadingAnims.end() &&
					pending->second.kind == PendingRequestKind::kLoad &&
					pending->second.generation == generation) {
					loadingAnims.erase(targetActor);
					l.unlock();
					if (successful) {
						StartAnimation(targetActor, std::move(inst), id, false, generation);
					} else {
						SetRequestState(actorFormID, { RequestStatus::kFailed, generation, std::move(id), false, false, false, false });
					}
				} else {
					l.unlock();
					SetRequestStateIfCurrent(actorFormID, generation,
						{ RequestStatus::kCanceled, generation, std::move(id), false, false, false, false });
				}
			}).detach();

			return true;
		}

		void StopAnimation(RE::ActorHandle targetActor, bool animOverride = false) {
			auto actor = targetActor.get();
			const auto actorFormID = actor ? actor->formID : 0;
			const auto generation = nextRequestGeneration.fetch_add(1, std::memory_order_relaxed);
			std::unique_lock l1{ loadingAnimsLock };
			if (loadingAnims.contains(targetActor)) {
				loadingAnims.erase(targetActor);
			}
			std::unique_lock l2{ stateLock };
			auto managedActor = state->managedAnims.find(targetActor);
			std::optional<FaceData::AnimInfo> restore;

			if (managedActor != state->managedAnims.end()) {
				if (!managedActor->second.animBackup.has_value()) {
					managedActor->second.anim = nullptr;
					state->managedAnims[targetActor].animationId = "";
					EraseIfEmpty_NonThreadSafe(targetActor);
				} else {
					if (animOverride) {
						restore = managedActor->second.animBackup.value();
						managedActor->second.animBackup = std::nullopt;
					} else {
						managedActor->second.animBackup = std::nullopt;
					}
				}
			}
			if (l2.owns_lock()) l2.unlock();
			if (l1.owns_lock()) l1.unlock();
			if (restore) {
				LoadAndPlayAnimation(targetActor, restore->animationId, restore->loop, restore->havokSync);
				return;
			}
			SetRequestState(actorFormID, { RequestStatus::kStopped, generation, {}, false,
				actor && GameUtil::GetFaceAnimData(actor.get()) != nullptr, false, false });
		}

		// Reset is intentionally stronger than StopAnimation. It removes all NAF-owned
		// managed-face state for the actor, invalidates pending asynchronous loads, and
		// clears only the composed expression output. baseExp/modExp remain engine-owned
		// and are left untouched so the next vanilla update can resume normal composition.
		bool ResetFaceExpression(RE::ActorHandle targetActor)
		{
			if (!targetActor.get()) {
				return false;
			}

			const auto generation = nextRequestGeneration.fetch_add(1, std::memory_order_relaxed);
			const auto actorFormID = targetActor.get()->formID;
			SetRequestState(actorFormID, { RequestStatus::kResetPending, generation, {}, false,
				GameUtil::GetFaceAnimData(targetActor.get().get()) != nullptr, false, false });
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
			{
				std::scoped_lock l{ pendingResetsLock };
				pendingResets.insert(targetActor);
			}
			F4SE::GetTaskInterface()->AddTask([targetActor, actorFormID, generation, resetEyes]() {
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
					SetRequestState(actorFormID, { RequestStatus::kFailed, generation, {}, false, false, false, false });
					return;
				}
				auto data = GameUtil::GetFaceAnimData(actor.get());
				if (!data) {
					SetRequestState(actorFormID, { RequestStatus::kResetPending, generation, {}, false, false, false, false });
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

				{
					std::scoped_lock l{ pendingResetsLock };
					pendingResets.erase(targetActor);
				}
				SetRequestState(actorFormID, { RequestStatus::kCompleted, generation, {}, false, true, false, false });
			});

			return true;
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
				std::scoped_lock resetLock{ pendingResetsLock };
				pendingResets.erase(targetActor);
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
			std::scoped_lock l{ loadingAnimsLock, stateLock, geoCacheLock, pendingResetsLock, requestStatesLock };
			state = std::make_unique<PersistentState>();
			pendingResets.clear();
			loadingAnims.clear();
			eyeGeoCache.clear();
			requestStates.clear();
		}

		void OnSet3D(RE::TESObjectREFR* a_ref)
		{
			auto actor = a_ref ? a_ref->As<RE::Actor>() : nullptr;
			if (!actor) {
				return;
			}
			const auto handle = actor->GetActorHandle();
			const auto faceData = GameUtil::GetFaceAnimData(actor);
			if (auto request = GetRequestState(actor->formID); request) {
				request->faceDataAvailable = faceData != nullptr;
				SetRequestState(actor->formID, std::move(*request));
			}
			{
				std::scoped_lock resetLock{ pendingResetsLock };
				if (pendingResets.contains(handle)) {
					F4SE::GetTaskInterface()->AddTask([handle]() { ResetFaceExpression(handle); });
				}
			}
			std::scoped_lock l{ stateLock, geoCacheLock };
			if (!state->managedAnims.contains(handle)) {
				return;
			}
			eyeGeoCache.erase(handle);
			BindManagedData_NonThreadSafe(handle, faceData);
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
