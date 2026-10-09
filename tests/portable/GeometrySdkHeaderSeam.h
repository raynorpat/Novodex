#ifndef NX_PORTABLE_GEOMETRY_SDK_HEADER_SEAM_H
#define NX_PORTABLE_GEOMETRY_SDK_HEADER_SEAM_H
// Standalone tests use the real SDK types. The unrelated public Foundation
// FPU header still parses Win32 assembly and is owned by the environment task.
// Only declare its unused sin/cos entry; no substitute implementation is linked.
#include "Nx.h"
#define NX_FOUNDATION_NXFPU
#define NX_SIGN_BITMASK 0x80000000
#define NX_IR(x) ((NxU32&)(x))
#define NX_FR(x) ((NxF32&)(x))
#define NX_AIR(x) (NX_IR(x)&0x7fffffff)
void NxSinCos(NxF32&, NxF32&, NxF32);
// Select NxMath's existing standard sqrt branch while parsing the public types.
// None of the geometry kernels depend on the Foundation FPU environment API.
#ifdef _M_IX86
#define NX_GEOMETRY_RESTORE_IX86
#undef _M_IX86
#endif
#include "NxMath.h"
#ifdef NX_GEOMETRY_RESTORE_IX86
#define _M_IX86 600
#undef NX_GEOMETRY_RESTORE_IX86
#endif
#endif
