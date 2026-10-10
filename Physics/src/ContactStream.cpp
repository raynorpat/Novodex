#include "ContactGeneration.h"
#include "Containers.h"
#include "NxPhysicsBackend.h"

#if NX_PHYSICS_USE_X87
extern "C" void nxContactCallContainerResize();		// 004840, Container::Resize(udword)
#pragma comment(linker, "/alternatename:_nxContactCallContainerResize=?Resize@Container@IceCore@@AAE_NI@Z")

// phys_fn_002354 (0x0005b620, 86 B)
void __fastcall NxContactSinkResetState(NxU32* state)
	{
	if(state[11] != 0)
		state[11] = 0;
	const NxU32 oldCount = state[11];
	state[0] = 0;
	if(state[11] == state[10])
		{
		NxU32* container = state + 10;
		__asm
			{
			push 1
			mov ecx, container
			call nxContactCallContainerResize
			}
		}
	NxU32* stream = reinterpret_cast<NxU32*>(static_cast<size_t>(state[12]));
	stream[state[11]] = 0;
	++state[11];
	state[1] = oldCount;
	state[2] = 0;
	state[3] = 0;
	state[4] = 0;
	state[5] = 0;
	state[6] = 0;
	state[7] = 0;
	state[8] = 0;
	state[9] = 0;
	}

#else
#include "portable/ContactStreamScalar.inl"
#endif
