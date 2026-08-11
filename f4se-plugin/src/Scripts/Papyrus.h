#pragma once
#include "Papyrus/EventRegistrations.h"
#include "Papyrus/NAF.h"
#include "Papyrus/EventProxy.h"

#define PAPYRUS_BIND(funcName) a_VM->BindNativeMethod(NAF::SCRIPT_NAME, #funcName, NAF::funcName, true)
#define PAPYRUS_BIND_LATENT(funcName, retType) a_VM->BindNativeMethod<retType>(NAF::SCRIPT_NAME, #funcName, NAF::funcName, true, true)

namespace Papyrus
{
	bool RegisterFunctions(RE::BSScript::IVirtualMachine* a_VM)
	{
		PAPYRUS_BIND(SetDisableRescaler);
		PAPYRUS_BIND(GetDisableRescaler);
		PAPYRUS_BIND(ToggleMenu);
		PAPYRUS_BIND(SetPackageOverride);
		PAPYRUS_BIND(ClearPackageOverride);
		PAPYRUS_BIND(StartScene);
		PAPYRUS_BIND(WalkToAndStartScene);
		PAPYRUS_BIND(StopScene);
		PAPYRUS_BIND(GetSceneFromActor);
		PAPYRUS_BIND(IsSceneRunning);

		PAPYRUS_BIND(DrawRectangle);
		PAPYRUS_BIND(DrawText);
		PAPYRUS_BIND(DrawLine);
		PAPYRUS_BIND(SetElementPosition);
		PAPYRUS_BIND(TranslateElementTo);
		PAPYRUS_BIND(StopElementTranslation);
		PAPYRUS_BIND(RemoveElement);
		PAPYRUS_BIND(GetElementWidth);
		PAPYRUS_BIND(GetElementHeight);
		PAPYRUS_BIND(MoveElementToFront);
		PAPYRUS_BIND(MoveElementToBack);
		PAPYRUS_BIND(SetElementMask);
		PAPYRUS_BIND(AttachElementTo);

		Papyrus::EventProxy::GetSingleton();
		logger::info("Registered 23 production Papyrus natives: settings (2), menu (1), package/process (2), basic scene lifecycle (5), HUD (13)");
		logger::warn("Scene mutation/query extensions, dynamic-idle, body/IK, managed face, player-input and transform natives remain deliberately unregistered");
		return true;
	}
}
