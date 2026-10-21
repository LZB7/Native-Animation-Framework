#include "NAFExtendedAPI.h"
#include <Windows.h>

#include <string>
#include <vector>

const NAFExtFaceAPIv2* AcquireNAFExpressionAPI()
{
	const auto module = GetModuleHandleW(L"NAF.dll");
	const auto getter = module ? reinterpret_cast<NAFExtGetFaceAPIv2Fn>(GetProcAddress(module, "NAFExt_GetFaceAPIv2")) : nullptr;
	const auto* api = getter ? getter(NAFEXT_FACE_API_VERSION_2, sizeof(NAFExtFaceAPIv2)) : nullptr;
	constexpr auto required = NAFEXT_CAP_EXPRESSION_PRESETS | NAFEXT_CAP_EXPRESSION_INTENSITY |
		NAFEXT_CAP_EXPRESSION_RELEASE | NAFEXT_CAP_EXPRESSION_QUERY;
	return api && api->struct_size >= sizeof(NAFExtFaceAPIv2) && api->service_version == NAFEXT_FACE_API_VERSION_2 &&
		(api->capabilities & required) == required && api->request_expression && api->release_expression && api->get_expression_state ? api : nullptr;
}

NAFExtResult RequestSmile(const NAFExtFaceAPIv2* api, std::uint32_t actorFormID, float intensity)
{
	const std::string id{ "example.smile" };
	return api->request_expression(actorFormID, { id.data(), static_cast<std::uint32_t>(id.size()) }, intensity, 0);
}

NAFExtResult QueryExpression(const NAFExtFaceAPIv2* api, std::uint32_t actorFormID,
	NAFExtExpressionState& state, std::string& presetID)
{
	state = { sizeof(state) };
	std::uint32_t required{};
	auto result = api->get_expression_state(actorFormID, &state, nullptr, 0, &required);
	if (result != NAFEXT_E_BUFFER_TOO_SMALL) return result;
	std::vector<char> bytes(required);
	state.struct_size = sizeof(state);
	result = api->get_expression_state(actorFormID, &state, bytes.data(), required, &required);
	if (result == NAFEXT_OK) presetID.assign(bytes.begin(), bytes.end());
	return result;
}
