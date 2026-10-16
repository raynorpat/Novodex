#ifndef NX_PHYSICS_EMBEDDEDHOOKBASE
#define NX_PHYSICS_EMBEDDEDHOOKBASE

#include "Nxp.h"

// Shared 12-byte polymorphic subobject used by NpScene, actor wrappers,
// joints, and shape-owned collision objects. The pinned constructor installs
// its base vptr and clears the two words before each complete type installs its
// own secondary table.
struct EmbeddedHookBase
	{
	EmbeddedHookBase() : mWord04(0), mWord08(0) {}
	virtual ~EmbeddedHookBase() {}
	union { NxU32 mWord04; void* mPointer04; };
	union { NxU32 mWord08; void* mPointer08; };
	};

static_assert(sizeof(EmbeddedHookBase) == 0x0c,
	"EmbeddedHookBase is a vptr and two 32-bit words");

#endif
