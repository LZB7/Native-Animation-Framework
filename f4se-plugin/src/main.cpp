bool g_gameDataReady = false;

#include "Data/Constants.h"
#include "Misc/Utility.h"
#include "Data/Events.h"
#include "RE/ExtraREClasses.h"
#include "Serialization/General.h"
#include "Tasks/TaskFunctor.h"
#include "PackageOverride/EvalHook.h"
#include "LooksMenu/LooksMenu.h"
#include "FaceAnimation/AnimationData.h"
#include "Data/Global.h"
#include "Data/CommandEngine.h"
#include "BodyAnimation/GraphHook.h"
#include "BodyAnimation/SmartIdle.h"
#include "CamHook/CamHook.h"
#include "Misc/GameUtil.h"
#include "Data/Uid.h"
#include "Menu/NAFHUDMenu/SceneHUD.h"
#include "Scene/IScene.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneBase.h"
#include "FaceAnimation/FaceUpdateHook.h"
#include "Tasks/GameLoopHook.h"
#include "Menu/Menu.h"
#include "Scripts/Papyrus.h"
#include "Serialization/Serialization.h"
#include "API/API.h"
namespace
{
	// Restore subsystems coherently. This stage enables the validated HUD,
	// production graph hooks, and production face/lip hooks. Scene playback,
	// package, camera, studio, Papyrus and serialization remain unavailable.
	inline constexpr bool kHUDSubsystemEnabled = true;
	inline constexpr bool kGameplaySubsystemsEnabled = false;

	void MessageHandler(F4SE::MessagingInterface::Message* a_msg)
	{
		if (!a_msg) {
			return;
		}
		switch (a_msg->type) {
			case F4SE::MessagingInterface::kGameDataReady:
			{
				if (static_cast<bool>(a_msg->data)) {
					logger::info("Game data finished loading; initializing the production NAF HUD subsystem");

					auto ui = RE::UI::GetSingleton();
					if (!ui) {
						logger::critical("HUD initialization failed: UI singleton is unavailable");
						break;
					}

					if (!Menu::HUDManager::RegisterHUDModeEvents()) {
						logger::critical("HUD initialization failed: HUD mode event registration was unavailable");
						break;
					}
					ui->RegisterSink(static_cast<RE::BSTEventSink<RE::MenuOpenCloseEvent>*>(Menu::HUDManager::GetSingleton()));
					Menu::RegisterHUD(ui);

					g_gameDataReady = true;
					logger::info("Production NAF HUD registered; all unrelated gameplay subsystems remain gated");
				}

				break;
			}
			case F4SE::MessagingInterface::kPostLoad:
			{
				logger::debug("Post-load received; LooksMenu integration remains gated in the HUD-only stage");
				break;
			}
			default:
				break;
		}
	}

	[[nodiscard]] bool InitializeHooking()
	{
		auto& trampoline = F4SE::GetTrampoline();
		trampoline.create(64);
		if (!Tasks::GameLoopHook::RegisterHook(trampoline)) {
			return false;
		}
		if (!BodyAnimation::GraphHook::RegisterHook()) {
			return false;
		}
		if (!FaceAnimation::FaceUpdateHook::RegisterHook(trampoline)) {
			return false;
		}
		logger::warn("Deferred hooks: package evaluation/check, free camera update");
		logger::info("Legacy HUD interior hook is replaced by the maintained HUDModeEvent source");
		return true;
	}
}

// F4SE 0.7.8 scans this export before loading a plugin.  Legacy Query/Load
// exports alone are rejected as "no version data" on runtime 1.11.221.
F4SE_EXPORT constinit auto F4SEPlugin_Version = []() noexcept {
	F4SE::PluginVersionData v{};
	v.PluginVersion({ Version::MAJOR, Version::MINOR, Version::PATCH, 0 });
	v.PluginName(Version::PROJECT);
	v.AuthorName("Deweh / NAF 1.11.221 port");
	v.UsesAddressLibrary(true);
	v.UsesSigScanning(false);
	v.IsLayoutDependent(true);
	v.HasNoStructUse(false);
	v.CompatibleVersions({ F4SE::RUNTIME_1_11_221 });
	v.MinimumRequiredXSEVersion({ 0, 7, 8, 0 });
	return v;
}();

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::PATCH;

	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= std::format("{}.log", Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(spdlog::level::trace);
	log->flush_on(spdlog::level::trace);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%m/%d/%Y - %T] [%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	if (a_f4se->IsEditor()) {
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver != F4SE::RUNTIME_1_11_221) {
		logger::critical("{} does not support runtime v{}", Version::PROJECT, ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);
	if (!EngineCompat::ExactRuntime::ValidateFreeUpAllMarkers()) {
		logger::critical("Exact-runtime compatibility validation failed; refusing to load NAF");
		return false;
	}
	if (!InitializeHooking()) {
		logger::critical("Hook validation or installation failed; refusing to load NAF");
		return false;
	}

	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageHandler)) {
		logger::critical("Failed to get F4SE messaging interface, marking as incompatible.");
		return false;
	} else {
		logger::info("Registered with F4SE messaging interface.");
	}

	if constexpr (!kGameplaySubsystemsEnabled) {
		logger::info("HUD, animation-graph and face/lip infrastructure initialized; awaiting game data before HUD registration");
		logger::warn("Serialization, Papyrus, scenes, package, camera, studio and LooksMenu gameplay integration remain gated");
		return true;
	}

	const auto serialization = F4SE::GetSerializationInterface();
	if (!serialization) {
		logger::critical("Failed to get F4SE serialization interface, marking as incompatible.");
		return false;
	} else {
		serialization->SetUniqueID('NAF');
		serialization->SetSaveCallback(Serialization::SaveCallback);
		serialization->SetLoadCallback(Serialization::LoadCallback);
		serialization->SetRevertCallback(Serialization::RevertCallback);
		logger::info("Registered with F4SE serialization interface.");
	}

	const auto papyrus = F4SE::GetPapyrusInterface();
	if (!papyrus || !papyrus->Register(Papyrus::RegisterFunctions)) {
		logger::critical("Failed to register Papyrus functions!");
	} else {
		logger::info("Registered Papyrus functions.");
	}

	Data::Global::Init();

	logger::info("{:s} initialization successful, waiting for game data load.", Version::PROJECT);

	return true;
}
