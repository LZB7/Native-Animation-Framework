#pragma once
#include "Misc/Easing.h"
#include "Misc/MathUtil.h"

namespace CamHook
{
	struct AtomicPoint3
	{
		std::atomic<float> x = 0.0f;
		std::atomic<float> y = 0.0f;
		std::atomic<float> z = 0.0f;

		RE::NiPoint3 get() {
			return { x, y, z };
		}

		void operator=(const RE::NiPoint3& rhs) {
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
		}

		bool IsIdentity() {
			return x == 0.0f && y == 0.0f && z == 0.0f;
		}

		void MakeIdentity() {
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
		}
	};

	inline static std::atomic<bool> useExternalInput = false;
	inline static std::atomic<bool> useExternalOrbit = false;
	inline static std::atomic<float> leftRightInput = 0.0f;
	inline static std::atomic<float> forwardBackInput = 0.0f;
	inline static std::atomic<float> upDownInput = 0.0f;
	inline static std::atomic<float> turnRightLeftInput = 0.0f;
	inline static std::atomic<float> turnUpDownInput = 0.0f;
	inline static AtomicPoint3 orbitTarget;

	void SetUseExternalInput(bool use)
	{
		useExternalInput = use;
		leftRightInput = 0.0f;
		forwardBackInput = 0.0f;
		upDownInput = 0.0f;
		turnRightLeftInput = 0.0f;
		turnUpDownInput = 0.0f;
	}

	namespace detail
	{
		using UpdateSig = void(RE::FreeCameraState*, RE::BSTSmartPointer<RE::TESCameraState>&);

		REL::Relocation<UpdateSig> OriginalUpdate;
		DetourXS updateHook;
		std::atomic<uint16_t> pendingInput = 0;
		std::atomic_bool loggedProductionUpdate = false;
		std::atomic_uint32_t lookAtDiagnosticCount = 0;

		bool active = false;
		RE::NiPointer<RE::NiAVObject> lookAtBase;
		RE::NiPointer<RE::NiAVObject> lookAtNode;
		safe_mutex lock;

		bool UpdateLookAtNode() {
			if (lookAtBase != nullptr) {
				auto n = lookAtBase->GetObjectByName(Data::Settings::Values.sLookAtCamTarget.get());
				if (n != nullptr) {
					lookAtNode = RE::NiPointer<RE::NiAVObject>(n);
					logger::info("LookAt Cam resolved target node '{}': base={}, node={}, baseWorld=({:.2f}, {:.2f}, {:.2f}), targetWorld=({:.2f}, {:.2f}, {:.2f})",
						Data::Settings::Values.sLookAtCamTarget.get(), fmt::ptr(lookAtBase.get()), fmt::ptr(lookAtNode.get()),
						lookAtBase->world.translate.x, lookAtBase->world.translate.y, lookAtBase->world.translate.z,
						lookAtNode->world.translate.x, lookAtNode->world.translate.y, lookAtNode->world.translate.z);
					return true;
				}
			}
			logger::warn("LookAt Cam could not resolve target node '{}' from base={}",
				Data::Settings::Values.sLookAtCamTarget.get(), fmt::ptr(lookAtBase.get()));
			return false;
		}

		class InputListener : public Data::EventListener<InputListener>
		{
		public:
			InputListener()
			{
				RegisterListener(Data::Events::HUD_E_KEY_DOWN, &InputListener::OnHudKey);
				RegisterListener(Data::Events::HUD_Q_KEY_DOWN, &InputListener::OnHudKey);
				RegisterListener(Data::Events::HUD_E_KEY_UP, &InputListener::OnHudKey);
				RegisterListener(Data::Events::HUD_Q_KEY_UP, &InputListener::OnHudKey);
			}

			void OnHudKey(Data::Events::event_type a_key, Data::Events::EventData&)
			{
				switch (a_key) {
				case Data::Events::HUD_E_KEY_DOWN:
					pendingInput = 2;
					break;
				case Data::Events::HUD_Q_KEY_DOWN:
					pendingInput = 1;
					break;
				case Data::Events::HUD_Q_KEY_UP:
					if (pendingInput == 1)
						pendingInput = 0;
					break;
				case Data::Events::HUD_E_KEY_UP:
					if (pendingInput == 2)
						pendingInput = 0;
					break;
				}
			}

			~InputListener() {
				pendingInput = 0;
			}
		};

		std::unique_ptr<InputListener> inputReceiver = nullptr;

		//RE::NiPoint3 location;
		//RE::NiMatrix3 rotation;

		void HookedUpdate(RE::FreeCameraState* s, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState)
		{
			OriginalUpdate(s, a_nextState);
			if (!loggedProductionUpdate.exchange(true, std::memory_order_relaxed)) {
				logger::info("Production FreeCameraState::Update interception observed: state={}, camera={}, nextState={}",
					fmt::ptr(s), fmt::ptr(s ? s->camera : nullptr), fmt::ptr(a_nextState.get()));
			}
			bool updateCameraRoot = false;

			std::unique_lock l{ lock };
			if (s->camera && s->camera->cameraRoot) {
				auto camNode = s->camera->cameraRoot;

				if (useExternalInput) {
					if (leftRightInput != 0.0f || forwardBackInput != 0.0f || upDownInput != 0.0f) {
						updateCameraRoot = true;
						auto& mat = camNode->local.rotate;

						RE::NiPoint3 up = { mat.entry[2][0], mat.entry[2][1], mat.entry[2][2] };
						RE::NiPoint3 forward = { mat.entry[1][0], mat.entry[1][1], mat.entry[1][2] };
						RE::NiPoint3 left = { mat.entry[0][0], mat.entry[0][1], mat.entry[0][2] };

						camNode->local.translate += (up * upDownInput);
						camNode->local.translate += (forward * forwardBackInput);
						camNode->local.translate += (left * leftRightInput);

						s->translation = camNode->local.translate;
						
						if (useExternalOrbit.load() == true && !orbitTarget.IsIdentity()) {
							auto r = MathUtil::GetLookAtRotation(camNode->local.translate, orbitTarget.get());
							s->rotation.y = r.x;
							camNode->local.rotate.FromEulerAnglesZXY(s->rotation.y, s->rotation.x, 0.0f);
						}
						
					}

					if (turnRightLeftInput != 0.0f || turnUpDownInput != 0.0f) {
						updateCameraRoot = true;
						s->rotation.x += MathUtil::DegreeToRadian(turnUpDownInput * 0.5f);
						s->rotation.y += MathUtil::DegreeToRadian(turnRightLeftInput * 0.5f);
						camNode->local.rotate.FromEulerAnglesZXY(s->rotation.y, s->rotation.x, 0.0f);
					}
				}

				if (active && Data::Settings::Values.bUseLookAtCam && lookAtNode) {
					// AE FreeCameraState::Update consumes the authoritative state fields and applies
					// them to a separately resolved render-camera object.  The historical cameraRoot
					// write targeted the render node on 1.10.163, but corrupts the wrong hierarchy
					// layer on 1.11.221.  Preserve normal free-camera translation and update only
					// the state consumed by the next vanilla update.
					switch (pendingInput) {
					case 1:
						s->translation.z += 1.0f;
						break;
					case 2:
						s->translation.z -= 1.0f;
						break;
					}

					auto r = MathUtil::GetLookAtRotation(s->translation, lookAtNode->world.translate);
					s->rotation.x = r.y;
					s->rotation.y = r.x;

					const auto diagnosticCount = ++lookAtDiagnosticCount;
					if (diagnosticCount <= 3 || diagnosticCount == 10) {
						const auto direction = lookAtNode->world.translate - s->translation;
						const auto distance = std::sqrt(
							(direction.x * direction.x) + (direction.y * direction.y) + (direction.z * direction.z));
						auto desiredDirection = direction;
						if (distance > 0.0f) {
							desiredDirection /= distance;
						}
						RE::NiMatrix3 stateRotation;
						stateRotation.FromEulerAnglesZXY(s->rotation.y, s->rotation.x, 0.0f);
						const RE::NiPoint3 stateForward{
							stateRotation.entry[1][0], stateRotation.entry[1][1], stateRotation.entry[1][2]
						};
						const auto forwardDot = stateForward.Dot(desiredDirection);
						logger::info(
							"LookAt Cam update #{}: stateCamera=({:.2f}, {:.2f}, {:.2f}), cameraRootLocal=({:.2f}, {:.2f}, {:.2f}), cameraRootWorld=({:.2f}, {:.2f}, {:.2f}), actorRoot=({:.2f}, {:.2f}, {:.2f}), target=({:.2f}, {:.2f}, {:.2f}), desired=({:.4f}, {:.4f}, {:.4f}), stateForward=({:.4f}, {:.4f}, {:.4f}), dot={:.5f}, distance={:.2f}, yaw={:.4f}, pitch={:.4f}",
							diagnosticCount,
							s->translation.x, s->translation.y, s->translation.z,
							camNode->local.translate.x, camNode->local.translate.y, camNode->local.translate.z,
							camNode->world.translate.x, camNode->world.translate.y, camNode->world.translate.z,
							lookAtBase->world.translate.x, lookAtBase->world.translate.y, lookAtBase->world.translate.z,
							lookAtNode->world.translate.x, lookAtNode->world.translate.y, lookAtNode->world.translate.z,
							desiredDirection.x, desiredDirection.y, desiredDirection.z,
							stateForward.x, stateForward.y, stateForward.z,
							forwardDot, distance, r.x, r.y);
					}
				}

				if (updateCameraRoot) {
					RE::NiUpdateData d;
					camNode->Update(d);
				}
			}
		}

		[[nodiscard]] bool RegisterHook()
		{
			if (REL::Module::get().version() != F4SE::RUNTIME_1_11_221) {
				logger::critical("Free-camera hook requires Fallout 4 runtime 1.11.221.0");
				return false;
			}

			REL::Relocation<UpdateSig> UpdateLoc{ REL::RelocationID(541110, 2248280) };
			constexpr std::array<std::uint8_t, 18> expectedPrologue{
				0x48, 0x89, 0x7C, 0x24, 0x20, 0x55, 0x48, 0x8D, 0x6C,
				0x24, 0xA9, 0x48, 0x81, 0xEC, 0x90, 0x00, 0x00, 0x00
			};
			std::array<std::uint8_t, expectedPrologue.size()> actual{};
			MEMORY_BASIC_INFORMATION memoryInfo{};
			const auto address = UpdateLoc.address();
			const auto queried = ::VirtualQuery(reinterpret_cast<const void*>(address), &memoryInfo, sizeof(memoryInfo));
			const auto protection = queried != 0 ? memoryInfo.Protect : 0;
			const bool executable = queried != 0 &&
				(protection & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0 &&
				(protection & (PAGE_GUARD | PAGE_NOACCESS)) == 0;
			if (executable) {
				std::memcpy(actual.data(), reinterpret_cast<const void*>(address), actual.size());
			}
			if (!executable || actual != expectedPrologue) {
				logger::critical(
					"FreeCameraState::Update validation failed: ID 2248280, address=0x{:X}, executable={}, protection=0x{:X}",
					address, executable, protection);
				return false;
			}

			if (!updateHook.Create(reinterpret_cast<LPVOID>(address), &HookedUpdate)) {
				logger::critical("Failed to create FreeCameraState::Update hook");
				return false;
			} else {
				OriginalUpdate = reinterpret_cast<uintptr_t>(updateHook.GetTrampoline());
			}
			logger::info("Installed production FreeCameraState::Update hook at ID 2248280");
			return true;
		}
	}

	void SetActive(bool a_active)
	{
		std::unique_lock l{ detail::lock };
		detail::active = a_active;

		if (detail::active) {
			detail::inputReceiver = std::make_unique<detail::InputListener>();
		} else {
			detail::lookAtNode = nullptr;
			detail::lookAtBase = nullptr;
			detail::inputReceiver.reset();
		}
	}

	bool LookAt(RE::NiAVObject* a_lookAtBase)
	{
		if (!a_lookAtBase) {
			return false;
		}

		std::unique_lock l{ detail::lock };
		detail::lookAtDiagnosticCount = 0;
		detail::lookAtBase = RE::NiPointer<RE::NiAVObject>(a_lookAtBase);
		if (detail::UpdateLookAtNode()) {
			SetActive(true);
			return true;
		} else {
			SetActive(false);
			return false;
		}
	}

	void OnSettingsChanged(Data::Events::event_type, Data::Events::EventData&)
	{
		std::unique_lock l{ detail::lock };
		if (detail::active && !detail::UpdateLookAtNode()) {
			SetActive(false);
		}
	}

	inline static bool settingsListenerRegistered = ([]() {
		Data::Events::Subscribe(Data::Events::SETTINGS_CHANGED, &OnSettingsChanged);
		return true;
	})();

	[[nodiscard]] bool RegisterHook() {
		return detail::RegisterHook();
	}

	/*
	void SetPosition(const RE::NiPoint3& a_pos) {
		std::unique_lock l{ detail::lock };
		detail::location = a_pos;
	}

	
	void SetRotation(const RE::NiPoint3& a_rot)
	{
		std::unique_lock l{ detail::lock };
		detail::rotation.MakeIdentity();
		detail::rotation.FromEulerAnglesZXY(a_rot.z, a_rot.x, a_rot.y);
	}
	
	void SetLookAt(const RE::NiPoint3& a_target, float a_distance, float a_direction, float a_height) {
		auto p = MathUtil::MovePointAlongXY(a_target, a_direction, a_distance);
		p.z += a_height;
		SetPosition(p);
		auto r = MathUtil::GetLookAtRotation(p, a_target);
		SetRotation(r);
	}
	*/
}
