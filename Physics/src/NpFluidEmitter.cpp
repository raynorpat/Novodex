#include "fluids/NpFluidEmitter.h"
#include "NpSceneGuard.h"
#include "FoundationSDK.h"
#include "NxMat34.h"

#include <windows.h>
#include <string.h>

#define NX_NPFLUIDEMITTER_CPP "\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpFluidEmitter.cpp"

static void nxFluidEmitterReportWriteLocked(unsigned line)
	{
	NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
		NX_NPFLUIDEMITTER_CPP, line, 0,
		"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
	}

// phys_fn_003593 (0x00088090) owns the internal bitfield transition. The
// extension callbacks are resolved from FluidModel.DLL when a live emitter is
// possible; the FluidManager has already loaded that module and populated the
// corresponding exports before it can construct an emitter.
static void nxFluidEmitterSetInternalFlag(void* internal, NxFluidEmitterFlag flag,
	bool enabled)
	{
	unsigned char* bytes = static_cast<unsigned char*>(internal);
	const unsigned mask = static_cast<unsigned>(flag);
	unsigned* flags = reinterpret_cast<unsigned*>(bytes + 0x10);
	if(enabled)
		*flags |= mask;
	else
		*flags &= ~mask;

	const char* exportName = 0;
	switch(mask)
		{
		case 4: exportName = "EmitterSetBodyRepulsionFlag"; break;
		case 8: exportName = "EmitterSetAddBodyVelocityFlag"; break;
		case 16: exportName = "EmitterSetEnabledFlag"; break;
		default: return;
		}

	HMODULE fluidModel = GetModuleHandleA("FluidModel.DLL");
	typedef int (__cdecl *FluidEmitterFlagCallback)(unsigned, unsigned, unsigned, unsigned char);
	FluidEmitterFlagCallback callback = reinterpret_cast<FluidEmitterFlagCallback>(
		GetProcAddress(fluidModel, exportName));
	void* fluid = *reinterpret_cast<void**>(bytes + 4);
	void* backend = *reinterpret_cast<void**>(static_cast<unsigned char*>(fluid) + 0x7c);
	const unsigned argument0 = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned argument1 = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(fluid) + 0x80);
	const unsigned argument2 = *reinterpret_cast<unsigned*>(bytes + 8);
	callback(argument0, argument1, argument2, enabled ? 1 : 0);
	}

// phys_fn_003485 (0x000857b0): a fluid owns an array of internal emitter
// records. On actor release, only records whose frame-actor pointer at +0x14
// matches the body are marked broken and detached. The zero-actor detach path
// preserves the backend's global pose in the emitter's local pose and sends
// the corresponding FluidModel callbacks.
extern "C" void __cdecl nxNpFluidNotifyActorReleased(void* fluid, void* body)
	{
	unsigned char* fluidBytes = static_cast<unsigned char*>(fluid);
	void** emitter = *reinterpret_cast<void***>(fluidBytes + 4);
	void** end = *reinterpret_cast<void***>(fluidBytes + 8);
	while(emitter < end)
		{
		unsigned char* internal = static_cast<unsigned char*>(*emitter++);
		if(*reinterpret_cast<void**>(internal + 0x14) != body)
			continue;
		nxFluidEmitterSetInternalFlag(internal, NX_FEF_BROKEN_ACTOR_REF, true);
		float pose[12];
		void* emitterFluid = *reinterpret_cast<void**>(internal + 4);
		unsigned char* emitterFluidBytes = static_cast<unsigned char*>(emitterFluid);
		void* backend = *reinterpret_cast<void**>(emitterFluidBytes + 0x7c);
		unsigned char* backendBytes = static_cast<unsigned char*>(backend);
		const unsigned context = *reinterpret_cast<unsigned*>(backendBytes + 0x30);
		const unsigned fluidHandle = *reinterpret_cast<unsigned*>(emitterFluidBytes + 0x80);
		const unsigned emitterHandle = *reinterpret_cast<unsigned*>(internal + 8);
		typedef const float* (__cdecl *GetGlobalPose)(unsigned, unsigned, unsigned);
		typedef int (__cdecl *SetGlobalPose)(unsigned, unsigned, unsigned, const float*);
		typedef int (__cdecl *SetAttachedFlag)(unsigned, unsigned, unsigned, unsigned char);
		HMODULE fluidModel = GetModuleHandleA("FluidModel.DLL");
		GetGlobalPose getGlobalPose = reinterpret_cast<GetGlobalPose>(
			GetProcAddress(fluidModel, "EmitterGetGlobalPose"));
		SetGlobalPose setGlobalPose = reinterpret_cast<SetGlobalPose>(
			GetProcAddress(fluidModel, "EmitterSetGlobalPose"));
		SetAttachedFlag setAttachedFlag = reinterpret_cast<SetAttachedFlag>(
			GetProcAddress(fluidModel, "EmitterSetAttachedFlag"));
		if(!getGlobalPose || !setGlobalPose || !setAttachedFlag)
			continue;
		const float* globalPose = getGlobalPose(context, fluidHandle, emitterHandle);
		if(!globalPose)
			continue;
		memcpy(pose, globalPose, sizeof(pose));
		*reinterpret_cast<void**>(internal + 0x14) = 0;
		memcpy(internal + 0x18, pose, sizeof(pose));
		setGlobalPose(context, fluidHandle, emitterHandle,
			reinterpret_cast<const float*>(internal + 0x18));
		setAttachedFlag(context, fluidHandle, emitterHandle, 0);
		}
	}

// phys_fn_003622 (0x000898d0) binds an emitter to an actor while preserving its
// world-space pose. The stored local pose is derived through the actor's rigid
// transform, then recomposed for the backend before the attached bit changes.
extern "C" int __cdecl nxNpFluidEmitterSetActor(
	void* internal, void* actor)
	{
	unsigned char* bytes = static_cast<unsigned char*>(internal);
	void* fluid = *reinterpret_cast<void**>(bytes + 4);
	unsigned char* fluidBytes = static_cast<unsigned char*>(fluid);
	void* backend = *reinterpret_cast<void**>(fluidBytes + 0x7c);
	unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	unsigned fluidHandle = *reinterpret_cast<unsigned*>(fluidBytes + 0x80);
	unsigned emitterHandle = *reinterpret_cast<unsigned*>(bytes + 8);
	typedef const float* (__cdecl *GetGlobalPose)(unsigned, unsigned, unsigned);
	typedef int (__cdecl *SetGlobalPose)(unsigned, unsigned, unsigned, const float*);
	typedef int (__cdecl *SetAttachedFlag)(unsigned, unsigned, unsigned, unsigned char);
	HMODULE fluidModel = GetModuleHandleA("FluidModel.DLL");
	GetGlobalPose getGlobalPose = reinterpret_cast<GetGlobalPose>(
		GetProcAddress(fluidModel, "EmitterGetGlobalPose"));
	SetGlobalPose setGlobalPose = reinterpret_cast<SetGlobalPose>(
		GetProcAddress(fluidModel, "EmitterSetGlobalPose"));
	SetAttachedFlag setAttachedFlag = reinterpret_cast<SetAttachedFlag>(
		GetProcAddress(fluidModel, "EmitterSetAttachedFlag"));
	const float* globalWords = getGlobalPose(context, fluidHandle, emitterHandle);
	NxMat34 globalPose;
	memcpy(&globalPose, globalWords, sizeof(globalPose));
	*reinterpret_cast<void**>(bytes + 0x14) = actor;
	NxMat34 localPose = globalPose;
	NxMat34 recomposedPose = globalPose;
	if(actor)
		{
		NxMat34 actorPose;
		unsigned char* actorBytes = static_cast<unsigned char*>(actor);
		void* body = *reinterpret_cast<void**>(actorBytes + 8);
		if(body)
			{
			const float* state = reinterpret_cast<const float*>(body);
			const float x = state[9], y = state[10], z = state[11], w = state[12];
			// The oracle builds this matrix with x87 products and explicitly
			// rounds the doubled square terms before the diagonal subtraction.
			volatile float twiceYY = y * y + y * y;
			const double twiceZZ = static_cast<double>(z) * z
				+ static_cast<double>(z) * z;
			const double twiceXX = static_cast<double>(x) * x
				+ static_cast<double>(x) * x;
			float rotation[9];
			rotation[0] = static_cast<float>(1.0 - twiceYY - twiceZZ);
			rotation[1] = (x * y + x * y) - (z * w + z * w);
			rotation[2] = (x * w + x * w) + (y * z + y * z);
			rotation[3] = (z * w + z * w) + (x * y + x * y);
			const double oneMinusTwiceXX = 1.0 - twiceXX;
			rotation[4] = static_cast<float>(oneMinusTwiceXX - twiceZZ);
			rotation[5] = (z * y + z * y) - (w * x + w * x);
			rotation[6] = (z * x + z * x) - (y * w + y * w);
			rotation[7] = (w * x + w * x) + (z * y + z * y);
			rotation[8] = static_cast<float>(static_cast<float>(oneMinusTwiceXX) - twiceYY);
			NxMat33 matrix;
			matrix.setRowMajor(rotation);
			NxVec3 translation(state[6], state[7], state[8]);
			actorPose = NxMat34(matrix, translation);
			}
		else
			memcpy(&actorPose, actorBytes + 0x20, sizeof(actorPose));
		localPose.multiplyInverseRTLeft(actorPose, globalPose);
		recomposedPose.multiply(actorPose, localPose);
		}
	memcpy(bytes + 0x18, &localPose, sizeof(localPose));
	setGlobalPose(context, fluidHandle, emitterHandle,
		reinterpret_cast<const float*>(&recomposedPose));
	return setAttachedFlag(context, fluidHandle, emitterHandle, actor ? 1 : 0);
	}

void NpFluidEmitter::setFrameActor(NxActor* actor)
	{
	void* link = mUnknown0c;
	if(!nxNpSceneGuardWriteTry(link))
		{
		nxFluidEmitterReportWriteLocked(0xdd);
		return;
		}
	nxNpFluidEmitterSetActor(mInternal, actor);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_003792 (0x0008c2d0). The oracle first installs the interface and
// read-lock-base construction vptrs, clears NxFluidEmitter::userData and the
// two read-lock-base words, then stores the internal emitter at +0x14 and
// installs the final interface and secondary-base tables. C++ performs the
// same base construction and final vptr installation for this measured layout.
NpFluidEmitter::NpFluidEmitter(void* internal)
	: mInternal(internal)
	{
	}

// The aggregate rows lock [this+0x10], copy through the caller-provided hidden
// result pointer, unlock, and return that pointer in eax. Keeping the copying
// in this shared wrapper makes the x86 MSVC aggregate-return ABI come from the
// public NxFluidEmitter virtual declarations themselves.
void NpFluidEmitter::copyInternal(void* output, unsigned offset, unsigned bytes) const
	{
	void* link = mReadLockLink;
	nxNpSceneGuardEnter(link);
	memcpy(output, static_cast<const unsigned char*>(mInternal) + offset, bytes);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_003804 (0x0008c590), 12 dwords from internal+0x48.
NxMat34 NpFluidEmitter::getGlobalPoseVal() const
	{
	NxMat34 value;
	copyInternal(&value, 0x48, 0x30);
	return value;
	}

// phys_fn_003806 (0x0008c5e0), three dwords from internal+0x6c.
NxVec3 NpFluidEmitter::getGlobalPositionVal() const
	{
	NxVec3 value;
	copyInternal(&value, 0x6c, 0x0c);
	return value;
	}

// phys_fn_003808 (0x0008c620), nine dwords from internal+0x48.
NxMat33 NpFluidEmitter::getGlobalOrientationVal() const
	{
	NxMat33 value;
	copyInternal(&value, 0x48, 0x24);
	return value;
	}

// phys_fn_003816 (0x0008c870), helper 003563 copies 12 dwords from +0x18.
NxMat34 NpFluidEmitter::getLocalPoseVal() const
	{
	NxMat34 value;
	copyInternal(&value, 0x18, 0x30);
	return value;
	}

// phys_fn_003818 (0x0008c8a0), helper 003565 copies three dwords from +0x3c.
NxVec3 NpFluidEmitter::getLocalPositionVal() const
	{
	NxVec3 value;
	copyInternal(&value, 0x3c, 0x0c);
	return value;
	}

// phys_fn_003820 (0x0008c8d0), helper 003567 copies nine dwords from +0x18.
NxMat33 NpFluidEmitter::getLocalOrientationVal() const
	{
	NxMat33 value;
	copyInternal(&value, 0x18, 0x24);
	return value;
	}

// phys_fn_003850 (0x0008cec0). The wrapper uses the emitter's write link at
// +0x0c, delegates the mask transition to phys_fn_003593, and reports a failed
// try-lock at source line 0xdd.
void NpFluidEmitter::setFlag(NxFluidEmitterFlag flag, bool enabled)
	{
	void* link = mUnknown0c;
	if(!nxNpSceneGuardWriteTry(link))
		{
		nxFluidEmitterReportWriteLocked(0xdd);
		return;
		}
	nxFluidEmitterSetInternalFlag(mInternal, flag, enabled);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_003852 (0x0008cf20), returns flag & (internal+0x10) under the read
// link at wrapper+0x10.
NX_BOOL NpFluidEmitter::getFlag(NxFluidEmitterFlag flag) const
	{
	void* link = mReadLockLink;
	nxNpSceneGuardEnter(link);
	const unsigned flags = *reinterpret_cast<const unsigned*>(
		static_cast<const unsigned char*>(mInternal) + 0x10);
	nxNpSceneGuardLeave(link);
	return static_cast<NX_BOOL>(flags & static_cast<unsigned>(flag));
	}
