#include "fluids/NpFluidEmitter.h"
#include "NpSceneGuard.h"
#ifndef NX_FLUID_EMITTER_ABI_TEST
#include "NxApiReentry.h"
#endif
#ifndef NX_FLUID_EMITTER_ABI_TEST
#include "FoundationSDK.h"
#endif
#include "NxMat34.h"

#include <windows.h>
#include <string.h>
#include <vector>

struct NxFluidEmitterNameEntry
	{
	void* internal;
	const char* name;
	};

static std::vector<NxFluidEmitterNameEntry> gFluidEmitterNames;

static void nxSetFluidEmitterName(void* internal, const char* name)
	{
	for(std::vector<NxFluidEmitterNameEntry>::iterator entry = gFluidEmitterNames.begin();
		entry != gFluidEmitterNames.end(); ++entry)
		if(entry->internal == internal)
			{
			if(name)
				entry->name = name;
			else
				gFluidEmitterNames.erase(entry);
			return;
		}
	if(name)
		{
		NxFluidEmitterNameEntry entry = { internal, name };
		gFluidEmitterNames.push_back(entry);
		}
	}

static const char* nxGetFluidEmitterName(void* internal)
	{
	for(std::vector<NxFluidEmitterNameEntry>::const_iterator entry = gFluidEmitterNames.begin();
		entry != gFluidEmitterNames.end(); ++entry)
		if(entry->internal == internal)
			return entry->name;
	return 0;
	}

#define NX_NPFLUIDEMITTER_CPP "\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpFluidEmitter.cpp"

#ifdef NX_FLUID_EMITTER_ABI_TEST
extern "C" void __cdecl nxFluidEmitterAbiUnexpectedWriteLockFailure(unsigned);
extern "C" void __cdecl nxFluidEmitterAbiUnexpectedReentryFailure(unsigned);
#endif

static void nxFluidEmitterReportWriteLocked(unsigned line)
	{
#ifdef NX_FLUID_EMITTER_ABI_TEST
	nxFluidEmitterAbiUnexpectedWriteLockFailure(line);
#else
	NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
		NX_NPFLUIDEMITTER_CPP, line, 0,
		"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
#endif
	}

static void nxFluidEmitterReportReentry(unsigned line)
	{
#ifdef NX_FLUID_EMITTER_ABI_TEST
	nxFluidEmitterAbiUnexpectedReentryFailure(line);
#else
	NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
		NX_NPFLUIDEMITTER_CPP, line, 0,
		"Reentry check: You may not call this API method from a callback!");
#endif
	}

static bool nxFluidEmitterReentryIsActive()
	{
#ifdef NX_FLUID_EMITTER_ABI_TEST
	return false;
#else
	return nxApiReentryIsActive();
#endif
	}

static void nxFluidEmitterSetReentryActive(bool active)
	{
#ifndef NX_FLUID_EMITTER_ABI_TEST
	nxSetApiReentryActive(active);
#else
	(void)active;
#endif
	}

struct NxFluidEmitterBackendArgs
	{
	unsigned context;
	unsigned fluid;
	unsigned emitter;
	};

static NxFluidEmitterBackendArgs nxFluidEmitterBackendArguments(void* internal)
	{
	unsigned char* bytes = static_cast<unsigned char*>(internal);
	unsigned char* fluid = static_cast<unsigned char*>(
		*reinterpret_cast<void**>(bytes + 4));
	unsigned char* backend = static_cast<unsigned char*>(
		*reinterpret_cast<void**>(fluid + 0x7c));
	NxFluidEmitterBackendArgs args;
	args.context = *reinterpret_cast<unsigned*>(backend + 0x30);
	args.fluid = *reinterpret_cast<unsigned*>(fluid + 0x80);
	args.emitter = *reinterpret_cast<unsigned*>(bytes + 8);
	return args;
	}

// These helpers reproduce phys_fn_003569 through phys_fn_003591. They are
// intentionally separate so each callback row can be mutation-tested alone.
extern "C" float __cdecl nxNpFluidEmitterGetDimensionX(void* internal)
	{
	NxFluidEmitterBackendArgs args = nxFluidEmitterBackendArguments(internal);
	typedef float (__cdecl *Callback)(unsigned, unsigned, unsigned);
	HMODULE module = GetModuleHandleA("FluidModel.DLL");
	Callback callback = reinterpret_cast<Callback>(GetProcAddress(module,
		"EmitterGetDimensionX"));
	return callback(args.context, args.fluid, args.emitter);
	}

extern "C" float __cdecl nxNpFluidEmitterGetDimensionY(void* internal)
	{
	NxFluidEmitterBackendArgs args = nxFluidEmitterBackendArguments(internal);
	typedef float (__cdecl *Callback)(unsigned, unsigned, unsigned);
	HMODULE module = GetModuleHandleA("FluidModel.DLL");
	Callback callback = reinterpret_cast<Callback>(GetProcAddress(module,
		"EmitterGetDimensionY"));
	return callback(args.context, args.fluid, args.emitter);
	}

extern "C" int __cdecl nxNpFluidEmitterSetRandomPos(void* internal,
	const NxVec3* value)
	{
	NxFluidEmitterBackendArgs args = nxFluidEmitterBackendArguments(internal);
	typedef int (__cdecl *Callback)(unsigned, unsigned, unsigned, const float*);
	HMODULE module = GetModuleHandleA("FluidModel.DLL");
	Callback callback = reinterpret_cast<Callback>(GetProcAddress(module,
		"EmitterSetRandomPos"));
	return callback(args.context, args.fluid, args.emitter,
		reinterpret_cast<const float*>(value));
	}

extern "C" void* __cdecl nxNpFluidEmitterGetRandomPos(void* internal,
	void* output)
	{
	NxFluidEmitterBackendArgs args = nxFluidEmitterBackendArguments(internal);
	typedef const float* (__cdecl *Callback)(unsigned, unsigned, unsigned);
	HMODULE module = GetModuleHandleA("FluidModel.DLL");
	Callback callback = reinterpret_cast<Callback>(GetProcAddress(module,
		"EmitterGetRandomPos"));
	const float* value = callback(args.context, args.fluid, args.emitter);
	memcpy(output, value, 3 * sizeof(unsigned));
	return output;
	}

extern "C" int __cdecl nxNpFluidEmitterSetRandomAngle(void* internal,
	float value)
	{
	NxFluidEmitterBackendArgs args = nxFluidEmitterBackendArguments(internal);
	typedef int (__cdecl *Callback)(unsigned, unsigned, unsigned, float);
	HMODULE module = GetModuleHandleA("FluidModel.DLL");
	Callback callback = reinterpret_cast<Callback>(GetProcAddress(module,
		"EmitterSetRandomAngle"));
	return callback(args.context, args.fluid, args.emitter, value);
	}

extern "C" float __cdecl nxNpFluidEmitterGetRandomAngle(void* internal)
	{
	NxFluidEmitterBackendArgs args = nxFluidEmitterBackendArguments(internal);
	typedef float (__cdecl *Callback)(unsigned, unsigned, unsigned);
	HMODULE module = GetModuleHandleA("FluidModel.DLL");
	Callback callback = reinterpret_cast<Callback>(GetProcAddress(module,
		"EmitterGetRandomAngle"));
	return callback(args.context, args.fluid, args.emitter);
	}

extern "C" int __cdecl nxNpFluidEmitterSetFluidVelocityMagnitude(
	void* internal, float value)
	{
	NxFluidEmitterBackendArgs args = nxFluidEmitterBackendArguments(internal);
	typedef int (__cdecl *Callback)(unsigned, unsigned, unsigned, float);
	HMODULE module = GetModuleHandleA("FluidModel.DLL");
	Callback callback = reinterpret_cast<Callback>(GetProcAddress(module,
		"EmitterSetFluidVelocityMagnitude"));
	return callback(args.context, args.fluid, args.emitter, value);
	}

extern "C" float __cdecl nxNpFluidEmitterGetFluidVelocityMagnitude(void* internal)
	{
	NxFluidEmitterBackendArgs args = nxFluidEmitterBackendArguments(internal);
	typedef float (__cdecl *Callback)(unsigned, unsigned, unsigned);
	HMODULE module = GetModuleHandleA("FluidModel.DLL");
	Callback callback = reinterpret_cast<Callback>(GetProcAddress(module,
		"EmitterGetFluidVelocityMagnitude"));
	return callback(args.context, args.fluid, args.emitter);
	}

extern "C" int __cdecl nxNpFluidEmitterSetRate(void* internal, float value)
	{
	NxFluidEmitterBackendArgs args = nxFluidEmitterBackendArguments(internal);
	typedef int (__cdecl *Callback)(unsigned, unsigned, unsigned, float);
	HMODULE module = GetModuleHandleA("FluidModel.DLL");
	Callback callback = reinterpret_cast<Callback>(GetProcAddress(module,
		"EmitterSetRate"));
	return callback(args.context, args.fluid, args.emitter, value);
	}

extern "C" float __cdecl nxNpFluidEmitterGetRate(void* internal)
	{
	NxFluidEmitterBackendArgs args = nxFluidEmitterBackendArguments(internal);
	typedef float (__cdecl *Callback)(unsigned, unsigned, unsigned);
	HMODULE module = GetModuleHandleA("FluidModel.DLL");
	Callback callback = reinterpret_cast<Callback>(GetProcAddress(module,
		"EmitterGetRate"));
	return callback(args.context, args.fluid, args.emitter);
	}

extern "C" int __cdecl nxNpFluidEmitterSetParticleLifetime(void* internal,
	float value)
	{
	NxFluidEmitterBackendArgs args = nxFluidEmitterBackendArguments(internal);
	typedef int (__cdecl *Callback)(unsigned, unsigned, unsigned, float);
	HMODULE module = GetModuleHandleA("FluidModel.DLL");
	Callback callback = reinterpret_cast<Callback>(GetProcAddress(module,
		"EmitterSetParticleLifetime"));
	return callback(args.context, args.fluid, args.emitter, value);
	}

extern "C" float __cdecl nxNpFluidEmitterGetParticleLifetime(void* internal)
	{
	NxFluidEmitterBackendArgs args = nxFluidEmitterBackendArguments(internal);
	typedef float (__cdecl *Callback)(unsigned, unsigned, unsigned);
	HMODULE module = GetModuleHandleA("FluidModel.DLL");
	Callback callback = reinterpret_cast<Callback>(GetProcAddress(module,
		"EmitterGetParticleLifetime"));
	return callback(args.context, args.fluid, args.emitter);
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

static void nxFluidEmitterCopyBackendPose(void* output, const float* pose)
	{
	const unsigned* source = reinterpret_cast<const unsigned*>(pose);
	unsigned* destination = reinterpret_cast<unsigned*>(output);
	destination[0] = source[0];
	destination[1] = source[3];
	destination[2] = source[6];
	destination[3] = source[1];
	destination[4] = source[4];
	destination[5] = source[7];
	destination[6] = source[2];
	destination[7] = source[5];
	destination[8] = source[8];
	memcpy(destination + 9, source + 9, 3 * sizeof(unsigned));
	}

// phys_fn_003559 (0x00087ce0) refreshes the emitter's cached world pose from
// FluidModel and returns the matrix at internal+0x48. The backend callback
// receives (context, fluid handle, emitter handle), using the same internal
// Fluid and backend fields as the neighboring emitter operations.
extern "C" void* __cdecl nxNpFluidEmitterStoreGlobalPose(void* internal)
	{
	unsigned char* bytes = static_cast<unsigned char*>(internal);
	void* fluid = *reinterpret_cast<void**>(bytes + 4);
	unsigned char* fluidBytes = static_cast<unsigned char*>(fluid);
	void* backend = *reinterpret_cast<void**>(fluidBytes + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned fluidHandle = *reinterpret_cast<unsigned*>(fluidBytes + 0x80);
	const unsigned emitterHandle = *reinterpret_cast<unsigned*>(bytes + 8);
	typedef const float* (__cdecl *GetGlobalPose)(unsigned, unsigned, unsigned);
	HMODULE fluidModel = GetModuleHandleA("FluidModel.DLL");
	GetGlobalPose getGlobalPose = reinterpret_cast<GetGlobalPose>(
		GetProcAddress(fluidModel, "EmitterGetGlobalPose"));
	if(!getGlobalPose)
		return 0;
	const float* globalPose = getGlobalPose(context, fluidHandle, emitterHandle);
	if(!globalPose)
		return 0;
	nxFluidEmitterCopyBackendPose(bytes + 0x48, globalPose);
	return bytes + 0x48;
	}

// phys_fn_003561 (0x00087d80) is the caller-owned-output form of the same
// FluidModel global-pose read. It writes exactly twelve words and returns the
// supplied NxMat34 buffer.
extern "C" void* __cdecl nxNpFluidEmitterCopyGlobalPose(void* internal,
	void* output)
	{
	unsigned char* bytes = static_cast<unsigned char*>(internal);
	void* fluid = *reinterpret_cast<void**>(bytes + 4);
	unsigned char* fluidBytes = static_cast<unsigned char*>(fluid);
	void* backend = *reinterpret_cast<void**>(fluidBytes + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned fluidHandle = *reinterpret_cast<unsigned*>(fluidBytes + 0x80);
	const unsigned emitterHandle = *reinterpret_cast<unsigned*>(bytes + 8);
	typedef const float* (__cdecl *GetGlobalPose)(unsigned, unsigned, unsigned);
	HMODULE fluidModel = GetModuleHandleA("FluidModel.DLL");
	GetGlobalPose getGlobalPose = reinterpret_cast<GetGlobalPose>(
		GetProcAddress(fluidModel, "EmitterGetGlobalPose"));
	if(!getGlobalPose)
		return 0;
	const float* globalPose = getGlobalPose(context, fluidHandle, emitterHandle);
	if(!globalPose)
		return 0;
	nxFluidEmitterCopyBackendPose(output, globalPose);
	return output;
	}

// Keep the oracle's x87 extended-precision accumulation through the final
// single-precision store.
static float nxFluidEmitterX87MultiplyAdd3(float a, float x, float b, float y,
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

static float nxFluidEmitterX87Translate3(float a, float x, float b, float y,
	float c, float z, float translation)
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
		fld translation
		faddp st(1), st
		fstp result
		}
	return result;
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

// phys_fn_003605 (0x00088310) recomposes an emitter's world pose from its
// local pose and optional frame actor, then publishes that pose to FluidModel.
extern "C" int __cdecl nxNpFluidEmitterUpdateGlobalPose(void* internal)
	{
	unsigned char* bytes = static_cast<unsigned char*>(internal);
	const float* localWords = reinterpret_cast<const float*>(bytes + 0x18);
	float worldPose[12];
	void* actor = *reinterpret_cast<void**>(bytes + 0x14);
	if(actor)
		{
	float actorRotation[9];
	float actorTranslation[3];
		unsigned char* actorBytes = static_cast<unsigned char*>(actor);
		void* body = *reinterpret_cast<void**>(actorBytes + 8);
		if(body)
			{
			const float* state = reinterpret_cast<const float*>(body);
			const float x = state[9], y = state[10], z = state[11], w = state[12];
			// Match the oracle's x87 spill at its first doubled-square temporary.
			volatile float twiceYY = y * y + y * y;
			const double twiceZZ = static_cast<double>(z) * z
				+ static_cast<double>(z) * z;
			const double twiceXX = static_cast<double>(x) * x
				+ static_cast<double>(x) * x;
			float rotation[9];
			rotation[0] = static_cast<float>(1.0 - twiceYY - twiceZZ);
			rotation[1] = (x * y + x * y) - (z * w + z * w);
			rotation[2] = (y * w + y * w) + (z * x + z * x);
			rotation[3] = (z * w + z * w) + (x * y + x * y);
			const double oneMinusTwiceXX = 1.0 - twiceXX;
			rotation[4] = static_cast<float>(oneMinusTwiceXX - twiceZZ);
			rotation[5] = (z * y + z * y) - (w * x + w * x);
			rotation[6] = (z * x + z * x) - (y * w + y * w);
			rotation[7] = (w * x + w * x) + (z * y + z * y);
			rotation[8] = static_cast<float>(static_cast<float>(oneMinusTwiceXX) - twiceYY);
			memcpy(actorRotation, rotation, sizeof(actorRotation));
			memcpy(actorTranslation, state + 6, sizeof(actorTranslation));
			}
		else
			{
			memcpy(actorRotation, actorBytes + 0x20, sizeof(actorRotation));
			memcpy(actorTranslation, actorBytes + 0x20 + 9 * sizeof(float),
				sizeof(actorTranslation));
			}
		// The internal local frame is row-major. The callback ABI stores the
		// composed 3x3 in column-major order followed by its translation.
		worldPose[0] = nxFluidEmitterX87MultiplyAdd3(actorRotation[1], localWords[3],
			actorRotation[2], localWords[6], actorRotation[0], localWords[0]);
		worldPose[1] = nxFluidEmitterX87MultiplyAdd3(actorRotation[3], localWords[0],
			actorRotation[4], localWords[3], actorRotation[5], localWords[6]);
		worldPose[2] = nxFluidEmitterX87MultiplyAdd3(actorRotation[6], localWords[0],
			actorRotation[7], localWords[3], actorRotation[8], localWords[6]);
		worldPose[3] = nxFluidEmitterX87MultiplyAdd3(actorRotation[1], localWords[4],
			actorRotation[2], localWords[7], actorRotation[0], localWords[1]);
		worldPose[4] = nxFluidEmitterX87MultiplyAdd3(actorRotation[3], localWords[1],
			actorRotation[4], localWords[4], actorRotation[5], localWords[7]);
		worldPose[5] = nxFluidEmitterX87MultiplyAdd3(actorRotation[6], localWords[1],
			actorRotation[7], localWords[4], actorRotation[8], localWords[7]);
		worldPose[6] = nxFluidEmitterX87MultiplyAdd3(actorRotation[0], localWords[2],
			actorRotation[1], localWords[5], actorRotation[2], localWords[8]);
		worldPose[7] = nxFluidEmitterX87MultiplyAdd3(actorRotation[3], localWords[2],
			actorRotation[4], localWords[5], actorRotation[5], localWords[8]);
		worldPose[8] = nxFluidEmitterX87MultiplyAdd3(actorRotation[6], localWords[2],
			actorRotation[7], localWords[5], actorRotation[8], localWords[8]);
		worldPose[9] = nxFluidEmitterX87Translate3(actorRotation[0], localWords[9],
			actorRotation[1], localWords[10], actorRotation[2], localWords[11],
			actorTranslation[0]);
		worldPose[10] = actorTranslation[1]
			+ nxFluidEmitterX87MultiplyAdd3(actorRotation[3], localWords[9],
				actorRotation[4], localWords[10], actorRotation[5], localWords[11]);
		worldPose[11] = actorTranslation[2]
			+ nxFluidEmitterX87MultiplyAdd3(actorRotation[6], localWords[9],
				actorRotation[7], localWords[10], actorRotation[8], localWords[11]);
		}
	else
		{
		for(unsigned column = 0; column < 3; ++column)
			for(unsigned row = 0; row < 3; ++row)
				worldPose[column * 3 + row] = localWords[row * 3 + column];
		memcpy(worldPose + 9, localWords + 9, 3 * sizeof(float));
		}
	void* fluid = *reinterpret_cast<void**>(bytes + 4);
	unsigned char* fluidBytes = static_cast<unsigned char*>(fluid);
	void* backend = *reinterpret_cast<void**>(fluidBytes + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned fluidHandle = *reinterpret_cast<unsigned*>(fluidBytes + 0x80);
	const unsigned emitterHandle = *reinterpret_cast<unsigned*>(bytes + 8);
	typedef int (__cdecl *SetGlobalPose)(unsigned, unsigned, unsigned, const float*);
	HMODULE fluidModel = GetModuleHandleA("FluidModel.DLL");
	SetGlobalPose setGlobalPose = reinterpret_cast<SetGlobalPose>(
		GetProcAddress(fluidModel, "EmitterSetGlobalPose"));
	return setGlobalPose(context, fluidHandle, emitterHandle,
		worldPose);
	}

// phys_fn_003624 (0x00089930) refreshes world pose, then mirrors the attached
// body's position and velocities into the fluid emitter backend.
extern "C" int __cdecl nxNpFluidEmitterUpdateBodyState(void* internal)
	{
	nxNpFluidEmitterUpdateGlobalPose(internal);
	unsigned char* bytes = static_cast<unsigned char*>(internal);
	void* actor = *reinterpret_cast<void**>(bytes + 0x14);
	if(!actor)
		return 0;
	void* body = *reinterpret_cast<void**>(static_cast<unsigned char*>(actor) + 8);
	const float* bodyBytes = static_cast<const float*>(body);
	float position[3], linearVelocity[3], angularVelocity[3];
	memcpy(position, bodyBytes + 0x158 / sizeof(float), sizeof(position));
	memcpy(linearVelocity, bodyBytes + 0x34 / sizeof(float), sizeof(linearVelocity));
	memcpy(angularVelocity, bodyBytes + 0x40 / sizeof(float), sizeof(angularVelocity));
	void* fluid = *reinterpret_cast<void**>(bytes + 4);
	unsigned char* fluidBytes = static_cast<unsigned char*>(fluid);
	void* backend = *reinterpret_cast<void**>(fluidBytes + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned fluidHandle = *reinterpret_cast<unsigned*>(fluidBytes + 0x80);
	const unsigned emitterHandle = *reinterpret_cast<unsigned*>(bytes + 8);
	typedef int (__cdecl *SetBodyVector)(unsigned, unsigned, unsigned, const float*);
	HMODULE fluidModel = GetModuleHandleA("FluidModel.DLL");
	SetBodyVector setPosition = reinterpret_cast<SetBodyVector>(
		GetProcAddress(fluidModel, "EmitterSetBodyPosition"));
	SetBodyVector setLinearVelocity = reinterpret_cast<SetBodyVector>(
		GetProcAddress(fluidModel, "EmitterSetBodyLinearVelocity"));
	SetBodyVector setAngularVelocity = reinterpret_cast<SetBodyVector>(
		GetProcAddress(fluidModel, "EmitterSetBodyAngularVelocity"));
	setPosition(context, fluidHandle, emitterHandle, position);
	setLinearVelocity(context, fluidHandle, emitterHandle, linearVelocity);
	return setAngularVelocity(context, fluidHandle, emitterHandle, angularVelocity);
	}

// phys_fn_003601 (0x000881c0) applies the FluidModel body-repulsion result
// to an attached actor after a fluid substep. FluidGetParticleMass supplies
// the scale; the actor update is the already-reconstructed NX_IMPULSE path.
#ifndef NX_FLUID_EMITTER_ABI_TEST
extern void nxNpActorApplyForce(unsigned char*, const NxVec3*, const NxVec3*,
	unsigned, bool);

static int nxNpFluidEmitterApplyBodyRepulsion(void* internal)
	{
	unsigned char* bytes = static_cast<unsigned char*>(internal);
	void* actor = *reinterpret_cast<void**>(bytes + 0x14);
	if(!actor)
		return 0;
	if((*reinterpret_cast<unsigned*>(bytes + 0x10) & 4u) == 0)
		return static_cast<int>(reinterpret_cast<size_t>(actor));
	void* fluid = *reinterpret_cast<void**>(bytes + 4);
	unsigned char* fluidBytes = static_cast<unsigned char*>(fluid);
	void* backend = *reinterpret_cast<void**>(fluidBytes + 0x7c);
	const unsigned system = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned fluidHandle = *reinterpret_cast<unsigned*>(fluidBytes + 0x80);
	const unsigned emitterHandle = *reinterpret_cast<unsigned*>(bytes + 8);
	HMODULE fluidModel = GetModuleHandleA("FluidModel.DLL");
	typedef int (__cdecl *GetEnabled)(unsigned, unsigned, unsigned);
	typedef int (__cdecl *GetBodyRepulsion)(unsigned, unsigned, unsigned,
		float*, float*);
	typedef float (__cdecl *GetParticleMass)(unsigned, unsigned);
	GetEnabled getEnabled = reinterpret_cast<GetEnabled>(
		GetProcAddress(fluidModel, "EmitterGetEnabledFlag"));
	if(getEnabled(system, fluidHandle, emitterHandle) == 0)
		return 0;
	float forceWords[3], torqueWords[3];
	GetBodyRepulsion getBodyRepulsion = reinterpret_cast<GetBodyRepulsion>(
		GetProcAddress(fluidModel, "EmitterGetBodyRepulsion"));
	getBodyRepulsion(system, fluidHandle, emitterHandle, forceWords, torqueWords);
	GetParticleMass getParticleMass = reinterpret_cast<GetParticleMass>(
		GetProcAddress(fluidModel, "FluidGetParticleMass"));
	const float mass = getParticleMass(system, fluidHandle);
	unsigned char* body = static_cast<unsigned char*>(
		*reinterpret_cast<void**>(static_cast<unsigned char*>(actor) + 8));
	if(!body)
		return 0;
	float scaledForce[3], scaledTorque[3];
	for(unsigned axis = 0; axis != 3; ++axis)
		{
		scaledForce[axis] = mass * forceWords[axis];
		scaledTorque[axis] = mass * torqueWords[axis];
		}
	const NxVec3 force(scaledForce[0], scaledForce[1], scaledForce[2]);
	const NxVec3 torque(scaledTorque[0], scaledTorque[1], scaledTorque[2]);
	// NxForceMode::NX_IMPULSE is the pinned mode value 1.
	nxNpActorApplyForce(body, &force, &torque, 1, true);
	return 0;
	}

// phys_fn_003495 (0x00085a40) is the per-fluid substep. The small naked
// entry preserves the original thiscall/ret-4 ABI; the implementation keeps
// the optional extension dispatch and emitter iteration in one place.
static int nxNpFluidStepImpl(void* internal, float timestep)
	{
	unsigned char* bytes = static_cast<unsigned char*>(internal);
	*reinterpret_cast<float*>(bytes + 0x74) = timestep;
	void** emitter = *reinterpret_cast<void***>(bytes + 4);
	void** emittersEnd = *reinterpret_cast<void***>(bytes + 8);
	for(void** current = emitter; current && emittersEnd && current < emittersEnd; ++current)
		nxNpFluidEmitterUpdateBodyState(*current);
	void* backend = *reinterpret_cast<void**>(bytes + 0x7c);
	void* core = *reinterpret_cast<void**>(static_cast<unsigned char*>(backend) + 0x24);
	float systemVector[3];
	memcpy(systemVector, static_cast<unsigned char*>(core) + 0x520,
		sizeof(systemVector));
	typedef float (__cdecl *FluidRun)(unsigned, unsigned, unsigned,
		const float*, float, unsigned);
	HMODULE fluidModel = GetModuleHandleA("FluidModel.DLL");
	FluidRun run = reinterpret_cast<FluidRun>(GetProcAddress(fluidModel, "FluidRun"));
	const float particleMass = run(
		*reinterpret_cast<unsigned*>(static_cast<unsigned char*>(backend) + 0x30),
		*reinterpret_cast<unsigned*>(bytes + 0x80),
		*reinterpret_cast<unsigned*>(bytes + 0x78), systemVector, timestep, 1);
	int result = static_cast<int>((reinterpret_cast<unsigned char*>(emittersEnd)
		- reinterpret_cast<unsigned char*>(emitter)) / sizeof(void*));
	for(void** current = emitter; current && emittersEnd && current < emittersEnd; ++current)
		{
		(void)particleMass;
		result = nxNpFluidEmitterApplyBodyRepulsion(*current);
		}
	void* state = *reinterpret_cast<void**>(bytes + 0x78);
	if(state)
		*reinterpret_cast<unsigned*>(static_cast<unsigned char*>(state) + 4) = 0;
	return result;
	}


extern "C" __declspec(naked) int __fastcall nxNpFluidStep(void*, void*, float)
	{
	__asm
		{
		mov eax, [esp+4]
		push eax
		push ecx
		call nxNpFluidStepImpl
		add esp, 8
		ret 4
		}
	}
#endif

// phys_fn_003419 (0x00084a50) forwards a fluid's implicit mesh to the
// FluidModel extension. The mesh record stores its fluid at +0x04, mesh handle
// at +0x18, and a vertex count at +0x28; the extension consumes triangles.
extern "C" int __cdecl nxNpFluidEmitterGenerateSurfaceMesh(void* internal)
	{
	unsigned char* bytes = static_cast<unsigned char*>(internal);
	unsigned char* mesh = static_cast<unsigned char*>(
		*reinterpret_cast<void**>(bytes + 0x24));
	if(!mesh)
		return 0;
	unsigned char* fluid = static_cast<unsigned char*>(
		*reinterpret_cast<void**>(mesh + 4));
	void* backend = *reinterpret_cast<void**>(fluid + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned fluidHandle = *reinterpret_cast<unsigned*>(fluid + 0x80);
	const unsigned meshHandle = *reinterpret_cast<unsigned*>(mesh + 0x18);
	const unsigned vertexCount = *reinterpret_cast<unsigned*>(mesh + 0x28);
	typedef int (__cdecl *ImplicitFluidMeshRun)(unsigned, unsigned, unsigned, unsigned);
	HMODULE fluidModel = GetModuleHandleA("FluidModel.DLL");
	ImplicitFluidMeshRun run = reinterpret_cast<ImplicitFluidMeshRun>(
		GetProcAddress(fluidModel, "ImplicitFluidMeshRun"));
	return run(context, fluidHandle, meshHandle, vertexCount / 3u);
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

NxReal NpFluidEmitter::getDimensionX() const
	{
	void* link = mReadLockLink;
	nxNpSceneGuardEnter(link);
	const float value = nxNpFluidEmitterGetDimensionX(mInternal);
	nxNpSceneGuardLeave(link);
	return value;
	}

NxReal NpFluidEmitter::getDimensionY() const
	{
	void* link = mReadLockLink;
	nxNpSceneGuardEnter(link);
	const float value = nxNpFluidEmitterGetDimensionY(mInternal);
	nxNpSceneGuardLeave(link);
	return value;
	}

void NpFluidEmitter::setRandomPos(NxVec3 value)
	{
	void* link = mUnknown0c;
	if(!nxNpSceneGuardWriteTry(link))
		{
		nxFluidEmitterReportWriteLocked(149);
		return;
		}
	if(nxFluidEmitterReentryIsActive())
		{
		nxFluidEmitterReportReentry(150);
		nxNpSceneGuardLeave(link);
		return;
		}
	nxFluidEmitterSetReentryActive(true);
	nxNpFluidEmitterSetRandomPos(mInternal, &value);
	nxFluidEmitterSetReentryActive(false);
	nxNpSceneGuardLeave(link);
	}

NxVec3 NpFluidEmitter::getRandomPos() const
	{
	void* link = mReadLockLink;
	nxNpSceneGuardEnter(link);
	NxVec3 value;
	nxNpFluidEmitterGetRandomPos(mInternal, &value);
	nxNpSceneGuardLeave(link);
	return value;
	}

void NpFluidEmitter::setRandomAngle(NxReal value)
	{
	void* link = mUnknown0c;
	if(!nxNpSceneGuardWriteTry(link))
		{
		nxFluidEmitterReportWriteLocked(163);
		return;
		}
	if(nxFluidEmitterReentryIsActive())
		{
		nxFluidEmitterReportReentry(164);
		nxNpSceneGuardLeave(link);
		return;
		}
	nxFluidEmitterSetReentryActive(true);
	nxNpFluidEmitterSetRandomAngle(mInternal, value);
	nxFluidEmitterSetReentryActive(false);
	nxNpSceneGuardLeave(link);
	}

NxReal NpFluidEmitter::getRandomAngle() const
	{
	void* link = mReadLockLink;
	nxNpSceneGuardEnter(link);
	const float value = nxNpFluidEmitterGetRandomAngle(mInternal);
	nxNpSceneGuardLeave(link);
	return value;
	}

void NpFluidEmitter::setFluidVelocityMagnitude(NxReal value)
	{
	void* link = mUnknown0c;
	if(!nxNpSceneGuardWriteTry(link))
		{
		nxFluidEmitterReportWriteLocked(177);
		return;
		}
	if(nxFluidEmitterReentryIsActive())
		{
		nxFluidEmitterReportReentry(178);
		nxNpSceneGuardLeave(link);
		return;
		}
	nxFluidEmitterSetReentryActive(true);
	nxNpFluidEmitterSetFluidVelocityMagnitude(mInternal, value);
	nxFluidEmitterSetReentryActive(false);
	nxNpSceneGuardLeave(link);
	}

NxReal NpFluidEmitter::getFluidVelocityMagnitude() const
	{
	void* link = mReadLockLink;
	nxNpSceneGuardEnter(link);
	const float value = nxNpFluidEmitterGetFluidVelocityMagnitude(mInternal);
	nxNpSceneGuardLeave(link);
	return value;
	}

void NpFluidEmitter::setRate(NxReal value)
	{
	void* link = mUnknown0c;
	if(!nxNpSceneGuardWriteTry(link))
		{
		nxFluidEmitterReportWriteLocked(191);
		return;
		}
	if(nxFluidEmitterReentryIsActive())
		{
		nxFluidEmitterReportReentry(192);
		nxNpSceneGuardLeave(link);
		return;
		}
	nxFluidEmitterSetReentryActive(true);
	nxNpFluidEmitterSetRate(mInternal, value);
	nxFluidEmitterSetReentryActive(false);
	nxNpSceneGuardLeave(link);
	}

NxReal NpFluidEmitter::getRate() const
	{
	void* link = mReadLockLink;
	nxNpSceneGuardEnter(link);
	const float value = nxNpFluidEmitterGetRate(mInternal);
	nxNpSceneGuardLeave(link);
	return value;
	}

void NpFluidEmitter::setParticleLifetime(NxReal value)
	{
	void* link = mUnknown0c;
	if(!nxNpSceneGuardWriteTry(link))
		{
		nxFluidEmitterReportWriteLocked(204);
		return;
		}
	if(nxFluidEmitterReentryIsActive())
		{
		nxFluidEmitterReportReentry(205);
		nxNpSceneGuardLeave(link);
		return;
		}
	nxFluidEmitterSetReentryActive(true);
	nxNpFluidEmitterSetParticleLifetime(mInternal, value);
	nxFluidEmitterSetReentryActive(false);
	nxNpSceneGuardLeave(link);
	}

NxReal NpFluidEmitter::getParticleLifetime() const
	{
	void* link = mReadLockLink;
	nxNpSceneGuardEnter(link);
	const float value = nxNpFluidEmitterGetParticleLifetime(mInternal);
	nxNpSceneGuardLeave(link);
	return value;
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

// phys_fn_003796 (0x0008c350) loads the public fluid pointer from the emitter's
// owning-fluid record at +0x54 while holding the wrapper's read lock.
NxFluid& NpFluidEmitter::getFluid() const
	{
	void* link = mReadLockLink;
	nxNpSceneGuardEnter(link);
	unsigned char* internal = static_cast<unsigned char*>(mInternal);
	void* fluidRecord = *reinterpret_cast<void**>(internal + 4);
	NxFluid* fluid = *reinterpret_cast<NxFluid**>(
		static_cast<unsigned char*>(fluidRecord) + 0x54);
	nxNpSceneGuardLeave(link);
	return *fluid;
	}

// phys_fn_003824 (0x0008c9c0) reads the public NxActor pointer from the body
// stored in the emitter's frame-actor slot under the same read lock.
NxActor* NpFluidEmitter::getFrameActor() const
	{
	void* link = mReadLockLink;
	nxNpSceneGuardEnter(link);
	unsigned char* internal = static_cast<unsigned char*>(mInternal);
	void* body = *reinterpret_cast<void**>(internal + 0x14);
	NxActor* actor = *reinterpret_cast<NxActor**>(body);
	nxNpSceneGuardLeave(link);
	return actor;
	}

// phys_fn_003858 (0x0008cfb0) keeps the supplied pointer in the internal
// emitter-name registry while holding the wrapper's write link.
void NpFluidEmitter::setName(const char* name)
	{
	void* link = mUnknown0c;
	if(!nxNpSceneGuardWriteTry(link))
		{
		nxFluidEmitterReportWriteLocked(255);
		return;
		}
	if(nxFluidEmitterReentryIsActive())
		{
		nxFluidEmitterReportReentry(256);
		nxNpSceneGuardLeave(link);
		return;
		}
	nxFluidEmitterSetReentryActive(true);
	nxSetFluidEmitterName(mInternal, name);
	nxFluidEmitterSetReentryActive(false);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_003860 (0x0008d060) returns the registered pointer under the
// wrapper's read link. The registry does not copy the caller's string.
const char* NpFluidEmitter::getName() const
	{
	void* link = mReadLockLink;
	nxNpSceneGuardEnter(link);
	const char* name = nxGetFluidEmitterName(mInternal);
	nxNpSceneGuardLeave(link);
	return name;
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
