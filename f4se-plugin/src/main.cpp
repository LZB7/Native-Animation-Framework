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
	void MessageHandler(F4SE::MessagingInterface::Message* a_msg)
	{
		if (!a_msg) {
			return;
		}
		switch (a_msg->type) {
			case F4SE::MessagingInterface::kGameDataReady:
			{
				if (static_cast<bool>(a_msg->data)) {
					logger::info("Game data finished loading; initializing NAF data, HUD and production scene lifecycle");

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
					ui->RegisterSink(Tasks::TimerThread::GetSingleton());
					Menu::RegisterHUD(ui);
					Menu::RegisterProductionSceneMenu(ui);
					PackageOverride::PkgEventListener::GetSingleton()->RegisterWithEngine();
					Scene::EventProxy::GetSingleton();

					Utility::StartPerformanceCounter();
					Data::Global::InitGameData();
					logger::info("Linked NAF game-data references in {:.3f}s", Utility::GetPerformanceCounter());

					g_gameDataReady = true;
					Data::Events::Send(Data::Events::GAME_DATA_READY);
					logger::info("Production NAF HUD, package events and full runtime scene-control lifecycle are ready");
				}

				break;
			}
			case F4SE::MessagingInterface::kPostLoad:
			{
				LooksMenu::Init();
				logger::info("Post-load LooksMenu/body-morph integration initialized (when supported f4ee.dll is present)");
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
		if (!PackageOverride::EvalHook::RegisterHook()) {
			return false;
		}
		if (!CamHook::RegisterHook()) {
			return false;
		}
		logger::info("Legacy HUD interior hook is replaced by the maintained HUDModeEvent source");
		return true;
	}
}

namespace
{
	[[nodiscard]] constexpr F4SE::PluginVersionData MakePluginVersionData() noexcept
	{
		F4SE::PluginVersionData data{};
		data.PluginVersion({ Version::MAJOR, Version::MINOR, Version::PATCH });
		data.PluginName(Version::PROJECT);
		data.AuthorName("Deweh / NAF Extended");
		data.UsesSigScanning(true);
		data.UsesAddressLibrary(false);
		data.IsLayoutDependent(true);
		data.MinimumRequiredXSEVersion({ 0, 7, 9 });
		return data;
	}
}

// Runtime patch selection is delegated to CommonLibF4RD. Required symbols,
// hook callsites and ABI guards still fail closed during plugin initialization.
F4SE_EXPORT constinit F4SE::PluginVersionData F4SEPlugin_Version = MakePluginVersionData();

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
	// Release logs retain actionable initialization/lifecycle information while
	// suppressing per-update trace/debug diagnostics from hot engine paths.
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%m/%d/%Y - %T] [%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	if (a_f4se->IsEditor()) {
		return false;
	}

	logger::info("Runtime {} will be validated through CommonLibF4RD", a_f4se->RuntimeVersion().string());

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);
	if (!EngineCompat::RuntimeValidated::ValidateFreeUpAllMarkers()) {
		logger::critical("Runtime Database compatibility validation failed; refusing to load NAF");
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
		return false;
	} else {
		logger::info("Registered the production Papyrus callback with F4SE 0.7.8.");
	}

	Data::Global::Init();

	logger::info("{:s} initialization successful, waiting for game data load.", Version::PROJECT);

	return true;
}
