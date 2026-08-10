#pragma once

// Engine-independent replacements for the NiQuaternion convenience methods
// that existed in Deweh/CommonLibF4.  The maintained CommonLib type is kept as
// the storage/ABI type; all operations below are ordinary quaternion math and
// perform no relocation or engine call.
namespace QuaternionCompat
{
	inline RE::NiQuaternion Add(const RE::NiQuaternion& a_lhs, const RE::NiQuaternion& a_rhs)
	{
		return { a_lhs.w + a_rhs.w, a_lhs.x + a_rhs.x, a_lhs.y + a_rhs.y, a_lhs.z + a_rhs.z };
	}

	inline RE::NiQuaternion Scale(const RE::NiQuaternion& a_value, float a_scale)
	{
		return { a_value.w * a_scale, a_value.x * a_scale, a_value.y * a_scale, a_value.z * a_scale };
	}

	inline RE::NiQuaternion Negate(const RE::NiQuaternion& a_value)
	{
		return { -a_value.w, -a_value.x, -a_value.y, -a_value.z };
	}

	inline RE::NiQuaternion Multiply(const RE::NiQuaternion& a_lhs, const RE::NiQuaternion& a_rhs)
	{
		return {
			a_rhs.w * a_lhs.w - a_rhs.x * a_lhs.x - a_rhs.y * a_lhs.y - a_rhs.z * a_lhs.z,
			a_rhs.w * a_lhs.x + a_rhs.x * a_lhs.w - a_rhs.y * a_lhs.z + a_rhs.z * a_lhs.y,
			a_rhs.w * a_lhs.y + a_rhs.x * a_lhs.z + a_rhs.y * a_lhs.w - a_rhs.z * a_lhs.x,
			a_rhs.w * a_lhs.z - a_rhs.x * a_lhs.y + a_rhs.y * a_lhs.x + a_rhs.z * a_lhs.w
		};
	}

	inline RE::NiQuaternion UnitInverse(const RE::NiQuaternion& a_value)
	{
		return { a_value.w, -a_value.x, -a_value.y, -a_value.z };
	}

	inline void ToRotation(const RE::NiQuaternion& a_value, RE::NiMatrix3& a_out)
	{
		const float x2 = a_value.x + a_value.x;
		const float y2 = a_value.y + a_value.y;
		const float z2 = a_value.z + a_value.z;
		const float xx = a_value.x * x2;
		const float xy = a_value.x * y2;
		const float xz = a_value.x * z2;
		const float yy = a_value.y * y2;
		const float yz = a_value.y * z2;
		const float zz = a_value.z * z2;
		const float wx = a_value.w * x2;
		const float wy = a_value.w * y2;
		const float wz = a_value.w * z2;

		a_out[0][0] = 1.0f - (zz + yy);
		a_out[0][1] = xy + wz;
		a_out[0][2] = xz - wy;
		a_out[0][3] = 0.0f;
		a_out[1][0] = xy - wz;
		a_out[1][1] = 1.0f - (zz + xx);
		a_out[1][2] = yz + wx;
		a_out[1][3] = 0.0f;
		a_out[2][0] = xz + wy;
		a_out[2][1] = yz - wx;
		a_out[2][2] = 1.0f - (yy + xx);
		a_out[2][3] = 0.0f;
	}

	inline void FromRotation(RE::NiQuaternion& a_out, const RE::NiMatrix3& a_value)
	{
		const float trace = a_value[0][0] + a_value[1][1] + a_value[2][2];
		if (trace > 0.0f) {
			float root = std::sqrt(trace + 1.0f);
			a_out.w = root * 0.5f;
			root = 0.5f / root;
			a_out.x = (a_value[1][2] - a_value[2][1]) * root;
			a_out.y = (a_value[2][0] - a_value[0][2]) * root;
			a_out.z = (a_value[0][1] - a_value[1][0]) * root;
			return;
		}

		std::size_t i = a_value[1][1] > a_value[0][0] ? 1 : 0;
		if (a_value[2][2] > a_value[i][i]) {
			i = 2;
		}
		constexpr std::size_t next[3]{ 1, 2, 0 };
		const auto j = next[i];
		const auto k = next[j];
		float root = std::sqrt((a_value[i][i] - a_value[j][j] - a_value[k][k]) + 1.0f);
		float components[4]{};
		components[i + 1] = root * 0.5f;
		root = 0.5f / root;
		components[0] = (a_value[j][k] - a_value[k][j]) * root;
		components[j + 1] = (a_value[j][i] + a_value[i][j]) * root;
		components[k + 1] = (a_value[i][k] + a_value[k][i]) * root;
		a_out = { components[0], components[1], components[2], components[3] };
	}

	inline void FromEulerAnglesXYZ(RE::NiQuaternion& a_out, float a_x, float a_y, float a_z)
	{
		RE::NiMatrix3 rotation;
		rotation.FromEulerAnglesXYZ(a_x, a_y, a_z);
		FromRotation(a_out, rotation);
	}

	inline void FromAngleAxis(RE::NiQuaternion& a_out, float a_angle, const RE::NiPoint3& a_axis)
	{
		const float halfAngle = a_angle * 0.5f;
		const float sine = std::sin(halfAngle);
		a_out = { std::cos(halfAngle), sine * a_axis.x, sine * a_axis.y, sine * a_axis.z };
	}

	inline RE::NiQuaternion Slerp(const RE::NiQuaternion& a_begin, RE::NiQuaternion a_end, float a_time)
	{
		float dot = a_begin.w * a_end.w + a_begin.x * a_end.x + a_begin.y * a_end.y + a_begin.z * a_end.z;
		if (dot < 0.0f) {
			a_end = Negate(a_end);
			dot = -dot;
		}
		dot = std::clamp(dot, -1.0f, 1.0f);
		if (1.0f - dot <= 1.0e-6f) {
			return Add(Scale(a_begin, 1.0f - a_time), Scale(a_end, a_time));
		}
		const float theta = std::acos(dot);
		const float sinTheta = std::sin(theta);
		return Add(
			Scale(a_begin, std::sin((1.0f - a_time) * theta) / sinTheta),
			Scale(a_end, std::sin(a_time * theta) / sinTheta));
	}

	inline RE::NiQuaternion Log(const RE::NiQuaternion& a_value)
	{
		const float clampedW = std::clamp(a_value.w, -1.0f, 1.0f);
		const float angle = std::acos(clampedW);
		const float sine = std::sin(angle);
		const float scale = std::abs(sine) > 1.0e-6f ? angle / sine : 1.0f;
		return { 0.0f, a_value.x * scale, a_value.y * scale, a_value.z * scale };
	}

	inline RE::NiQuaternion Exp(const RE::NiQuaternion& a_value)
	{
		const float angle = std::sqrt(a_value.x * a_value.x + a_value.y * a_value.y + a_value.z * a_value.z);
		const float scale = angle > 1.0e-6f ? std::sin(angle) / angle : 1.0f;
		return { std::cos(angle), a_value.x * scale, a_value.y * scale, a_value.z * scale };
	}

	inline RE::NiQuaternion Intermediate(
		const RE::NiQuaternion& a_before,
		const RE::NiQuaternion& a_current,
		const RE::NiQuaternion& a_after)
	{
		const auto inverse = UnitInverse(a_current);
		const auto sum = Add(Log(Multiply(inverse, a_before)), Log(Multiply(inverse, a_after)));
		return Multiply(a_current, Exp(Scale(sum, -0.25f)));
	}
}
