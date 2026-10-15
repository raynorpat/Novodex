#include <windows.h>
#include <string.h>

extern "C" int __cdecl nxNpFluidEmitterUpdateGlobalPose(void* internal);

// Match the oracle's x87 extended-precision accumulation through the final
// single-precision store. This helper is local to the row's translation unit.
static float nxFluidEmitterPoseX87MultiplyAdd3(float a, float x, float b, float y,
	float c, float z)
	{
	float result;
	__asm
		{
		fld a
		fmul x
		fld b
		fmul y
		faddp st(1), st
		fld c
		fmul z
		faddp st(1), st
		fstp result
		}
	return result;
	}

// phys_fn_003611 (0x00089410) stores a requested world pose in the emitter's
// local frame, then refreshes the cached world pose and FluidModel callback.
extern "C" int __cdecl nxNpFluidEmitterSetGlobalPoseInternal(void* internal,
	const float* globalPose)
	{
	unsigned char* bytes = static_cast<unsigned char*>(internal);
	void* actor = *reinterpret_cast<void**>(bytes + 0x14);
	if(!actor)
		memcpy(bytes + 0x18, globalPose, 0x30);
	else
		{
		float rotation[9];
		float translation[3];
		unsigned char* actorBytes = static_cast<unsigned char*>(actor);
		void* body = *reinterpret_cast<void**>(actorBytes + 8);
		if(body)
			{
			const float* state = reinterpret_cast<const float*>(body);
			const float x = state[9], y = state[10], z = state[11], w = state[12];
			volatile float twiceYY = y * y + y * y;
			const double twiceZZ = static_cast<double>(z) * z
				+ static_cast<double>(z) * z;
			const double twiceXX = static_cast<double>(x) * x
				+ static_cast<double>(x) * x;
			const double twiceXY = static_cast<double>(y) * x
				+ static_cast<double>(y) * x;
			const double twiceZW = static_cast<double>(z) * w
				+ static_cast<double>(z) * w;
			volatile float twiceZX = z * x + z * x;
			const double twiceYW = static_cast<double>(y) * w
				+ static_cast<double>(y) * w;
			volatile float twiceZY = z * y + z * y;
			const double twiceWX = static_cast<double>(w) * x
				+ static_cast<double>(w) * x;
			rotation[0] = static_cast<float>(1.0 - twiceYY - twiceZZ);
			rotation[1] = static_cast<float>(twiceXY - twiceZW);
			rotation[2] = static_cast<float>(twiceYW + twiceZX);
			rotation[3] = static_cast<float>(twiceZW + twiceXY);
			rotation[4] = static_cast<float>(1.0 - twiceXX - twiceZZ);
			rotation[5] = static_cast<float>(twiceZY - twiceWX);
			rotation[6] = static_cast<float>(twiceZX - twiceYW);
			rotation[7] = static_cast<float>(twiceWX + twiceZY);
			rotation[8] = static_cast<float>(static_cast<float>(1.0 - twiceXX) - twiceYY);
			memcpy(translation, state + 6, sizeof(translation));
			}
		else
			{
			memcpy(rotation, actorBytes + 0x20, sizeof(rotation));
			memcpy(translation, actorBytes + 0x20 + sizeof(rotation),
				sizeof(translation));
			}
		const float dx = globalPose[9] - translation[0];
		const float dy = globalPose[10] - translation[1];
		const float dz = globalPose[11] - translation[2];
		float* local = reinterpret_cast<float*>(bytes + 0x18);
		local[0] = nxFluidEmitterPoseX87MultiplyAdd3(rotation[6], globalPose[6],
			rotation[3], globalPose[3], rotation[0], globalPose[0]);
		local[1] = nxFluidEmitterPoseX87MultiplyAdd3(rotation[6], globalPose[7],
			rotation[3], globalPose[4], rotation[0], globalPose[1]);
		local[2] = nxFluidEmitterPoseX87MultiplyAdd3(rotation[6], globalPose[8],
			rotation[3], globalPose[5], rotation[0], globalPose[2]);
		local[3] = nxFluidEmitterPoseX87MultiplyAdd3(rotation[7], globalPose[6],
			rotation[4], globalPose[3], rotation[1], globalPose[0]);
		local[4] = nxFluidEmitterPoseX87MultiplyAdd3(rotation[7], globalPose[7],
			rotation[4], globalPose[4], rotation[1], globalPose[1]);
		local[5] = nxFluidEmitterPoseX87MultiplyAdd3(rotation[7], globalPose[8],
			rotation[4], globalPose[5], rotation[1], globalPose[2]);
		local[6] = nxFluidEmitterPoseX87MultiplyAdd3(rotation[8], globalPose[6],
			rotation[5], globalPose[3], rotation[2], globalPose[0]);
		local[7] = nxFluidEmitterPoseX87MultiplyAdd3(rotation[8], globalPose[7],
			rotation[5], globalPose[4], rotation[2], globalPose[1]);
		local[8] = nxFluidEmitterPoseX87MultiplyAdd3(rotation[8], globalPose[8],
			rotation[5], globalPose[5], rotation[2], globalPose[2]);
		local[9] = nxFluidEmitterPoseX87MultiplyAdd3(rotation[6], dz,
			rotation[3], dy, rotation[0], dx);
		local[10] = nxFluidEmitterPoseX87MultiplyAdd3(rotation[7], dz,
			rotation[4], dy, rotation[1], dx);
		local[11] = nxFluidEmitterPoseX87MultiplyAdd3(rotation[8], dz,
			rotation[5], dy, rotation[2], dx);
		}
	return nxNpFluidEmitterUpdateGlobalPose(internal);
	}
