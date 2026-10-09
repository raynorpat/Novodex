#ifndef NX_PHYSICS_FLUIDS_NPFLUIDEMITTER
#define NX_PHYSICS_FLUIDS_NPFLUIDEMITTER

#include "fluids/NxFluidEmitter.h"

// Secondary +0x08 state owned by NpFluidEmitter. phys_fn_003792 constructs
// this 0x0c-byte base at object+0x08: its vptr is replaced with the emitter
// table, its +0x04 word is cleared, and its +0x08 read-lock link is cleared.
class NpFluidEmitterReadLock
	{
	public:
	NpFluidEmitterReadLock() : mUnknown0c(0), mReadLockLink(0) {}
	virtual ~NpFluidEmitterReadLock() {}
	protected:
	void* mUnknown0c;
	void* mReadLockLink;
	};

// The fluid backend is disabled in the pinned build, so NpScene cannot create
// one through its public factory. Keep the known public aggregate getters on
// the actual wrapper type; unimplemented API operations remain abstract until
// their own rows are reconstructed.
class NpFluidEmitter : public NxFluidEmitter, public NpFluidEmitterReadLock
	{
	public:
	//! phys_fn_003792 (0x0008c2d0), 0x18-byte wrapper.
	explicit NpFluidEmitter(void* internal);

	//! phys_fn_003804 (0x0008c590), struct-return ABI: hidden output pointer,
	//! this in ecx, ret 4. Copies the global pose from internal+0x48.
	virtual NxMat34 getGlobalPoseVal() const;
	//! phys_fn_003806 (0x0008c5e0), copies internal+0x6c.
	virtual NxVec3 getGlobalPositionVal() const;
	//! phys_fn_003808 (0x0008c620), copies internal+0x48.
	virtual NxMat33 getGlobalOrientationVal() const;
	//! phys_fn_003816 (0x0008c870), helper 003563 copies internal+0x18.
	virtual NxMat34 getLocalPoseVal() const;
	//! phys_fn_003818 (0x0008c8a0), helper 003565 copies internal+0x3c.
	virtual NxVec3 getLocalPositionVal() const;
	//! phys_fn_003820 (0x0008c8d0), helper 003567 copies internal+0x18.
	virtual NxMat33 getLocalOrientationVal() const;

	protected:
	void copyInternal(void* output, unsigned offset, unsigned bytes) const;
	void* mInternal;
	};

static_assert(sizeof(NpFluidEmitterReadLock) == 0x0c,
	"NpFluidEmitter's secondary read-lock base occupies +0x08..+0x13");
static_assert(sizeof(NpFluidEmitter) == 0x18,
	"phys_fn_003792 constructs an 0x18-byte NxFluidEmitter wrapper");

#endif
