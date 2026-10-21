#include "NAFExtendedAPI.h"

#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

const NAFExtFaceAPIv1* AcquireNAFFaceAPI()
{
	auto module = GetModuleHandleW(L"NAF.dll");
	auto getter = module ? reinterpret_cast<NAFExtGetFaceAPIFn>(GetProcAddress(module, "NAFExt_GetFaceAPI")) : nullptr;
	auto api = getter ? getter(NAFEXT_FACE_API_VERSION_1, sizeof(NAFExtFaceAPIv1)) : nullptr;
	constexpr auto required = NAFEXT_CAP_FACE_PLAYBACK | NAFEXT_CAP_FACE_STOP |
		NAFEXT_CAP_FACE_RESET | NAFEXT_CAP_FACE_QUERY;
	if (!api || api->struct_size < sizeof(NAFExtFaceAPIv1) || api->service_version != NAFEXT_FACE_API_VERSION_1 ||
		(api->capabilities & required) != required || !api->play_face_animation || !api->stop_face_animation ||
		!api->reset_face_expression || !api->get_face_animation_state) return nullptr;
	return api;
}

bool PlayAuthoredFace(const NAFExtFaceAPIv1* api, std::uint32_t actorFormID, const std::string& animationID)
{
	const NAFExtStringView id{ animationID.data(), static_cast<std::uint32_t>(animationID.size()) };
	return api && api->play_face_animation(actorFormID, id, 0, 0) == NAFEXT_OK;
}

NAFExtResult QueryAuthoredFace(const NAFExtFaceAPIv1* api, std::uint32_t actorFormID,
	NAFExtFaceState& state, std::string& animationID)
{
	if (!api) return NAFEXT_E_PROVIDER_UNAVAILABLE;
	state = { sizeof(state) };
	std::uint32_t required{};
	auto result = api->get_face_animation_state(actorFormID, &state, nullptr, 0, &required);
	if (result != NAFEXT_E_BUFFER_TOO_SMALL && result != NAFEXT_OK) return result;
	std::vector<char> bytes(required);
	state.struct_size = sizeof(state);
	result = api->get_face_animation_state(actorFormID, &state, bytes.data(), required, &required);
	if (result == NAFEXT_OK) animationID.assign(bytes.begin(), bytes.end());
	return result;
}

