#pragma once

namespace EngineCompat
{
	inline RE::ShaderReferenceEffect* PlayEffectShader(RE::TESEffectShader* a_shader, RE::TESObjectREFR* a_refr)
	{
		if (a_shader != nullptr && a_refr != nullptr && a_refr->parentCell != nullptr &&
			a_refr->parentCell->loadedData != nullptr && a_refr->GetFullyLoaded3D() != nullptr) {
			return a_refr->ApplyEffectShader(a_shader);
		}

		return nullptr;
	}

	inline void StopEffectShader(RE::TESEffectShader* a_shader, RE::TESObjectREFR* a_refr)
	{
		if (a_shader == nullptr || a_refr == nullptr) {
			return;
		}

		const auto target = a_refr->GetHandle();
		RE::ProcessLists::GetSingleton()->ForEachShaderEffect([&](RE::ShaderReferenceEffect* a_effect) {
			if (a_effect->target == target && a_effect->effectData == a_shader) {
				a_effect->finished = true;
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});
	}
}
