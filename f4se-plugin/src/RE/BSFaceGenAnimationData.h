#pragma once

namespace RE
{
	struct Expression
	{
		float exp[54];
	};

	namespace FaceEmotionalIdles
	{
		struct InstanceData
		{
			uint32_t pad00;
			uint32_t handle;
			uint32_t pad08;
			float blinkTimer;
			float lidFollowEyes;
			BSSpinLock lock;
			uint64_t pad20;
			uint32_t unk28;
			uint32_t pad2C;
			BSFixedString archeType;
		};
	};
	static_assert(sizeof(FaceEmotionalIdles::InstanceData) == 0x38);

	class BSFaceGenAnimationData : public NiExtraData
	{
	public:
		Expression finalExp;
		Expression modExp;
		Expression baseExp;
		FaceEmotionalIdles::InstanceData instanceData;
		std::byte unk2D8[0x10];
	};
	static_assert(offsetof(BSFaceGenAnimationData, finalExp) == 0x18);
	static_assert(offsetof(BSFaceGenAnimationData, instanceData) == 0x2A0);
	static_assert(sizeof(BSFaceGenAnimationData) == 0x2E8);
}
