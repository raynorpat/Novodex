#include "fluids/NpFluidEmitter.h"
#include "NpSceneGuard.h"
#include "FoundationSDK.h"

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
