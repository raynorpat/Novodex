/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The nine NxFluid* exports of the shipped NxPhysics.dll: an allocator pair, an
// assertion hook and six wireframe draw helpers. They are a C shim for a fluid
// solver compiled without the C++ headers -- every one is __cdecl (stack purge
// 0), every one takes its vectors as a raw float array, and not one of them
// validates an input. They contain no simulation, and nothing else in the image
// calls them.
//
// Rows, all Phase 2, all in inventory.json:
//   phys_fn_001583 0x0002ea70 NxFluidAssert
//   phys_fn_003907 0x0008e840 NxFluidDebugTriangle
//   phys_fn_003909 0x0008e8f0 NxFluidDebugLine
//   phys_fn_003911 0x0008e960 NxFluidDebugSphere
//   phys_fn_003913 0x0008eb00 NxFluidDebugPoint
//   phys_fn_003915 0x0008ec00 NxFluidDebugArrow
//   phys_fn_003917 0x0008ec80 NxFluidPAlloc
//   phys_fn_003919 0x0008eca0 NxFluidFree
//   phys_fn_003921 0x0008ecc0 NxFluidDebugAABB
// The only row outside this file they reach is phys_fn_000443,
// PhysicsSDK::getDebugRenderable.
#include "PhysicsSDK.h"

#include "NxDebugRenderable.h"

// Every draw export loads the singleton from .data 0x00123c04 into ecx and calls
// phys_fn_000443 with it, even though that row never reads it. Neither the
// singleton nor the Foundation behind it is checked, so a draw made before
// NxCreatePhysicsSDK faults inside phys_fn_000443 -- reproduced here, not
// guarded.
static NxDebugRenderable* getDebugRenderable()
	{
	return PhysicsSDK::instance->getDebugRenderable();
	}

// 0x0008ec90 calls slot +8 of nxFoundationSDKAllocator with a memory type of 0,
// which is NX_MEMORY_PERSISTENT, and returns what it returns. The import is
// dereferenced twice with no null check.
extern "C" NXP_DLL_EXPORT void* NX_CALL_CONV NxFluidPAlloc(size_t size)
	{
	return nxFoundationSDKAllocator->malloc(size, NX_MEMORY_PERSISTENT);
	}

// 0x0008ecae calls slot +0x14, free. NX_FREE is not used: it null-checks and
// clears the pointer, and 0x0008eca0 does neither.
extern "C" NXP_DLL_EXPORT void NX_CALL_CONV NxFluidFree(void* memory)
	{
	nxFoundationSDKAllocator->free(memory);
	}

// One byte, 0xc3. The parameter list is not recoverable from a bare ret: what
// the encoding does fix is that the caller cleans the stack and that there is no
// return value.
extern "C" NXP_DLL_EXPORT void NX_CALL_CONV NxFluidAssert()
	{
	}

// 0x0008e94d calls slot +0x20 of the renderable vtable, addLine, with the two
// points copied into locals first. The copies are float by float through the x87
// stack, which is why the shim takes six floats and not two NxVec3s.
extern "C" NXP_DLL_EXPORT void NX_CALL_CONV NxFluidDebugLine(const NxF32* points, NxU32 color)
	{
	NxVec3 p0(points[0], points[1], points[2]);
	NxVec3 p1(points[3], points[4], points[5]);
	getDebugRenderable()->addLine(p0, p1, color);
	}

// Three addLine calls, not addTriangle: 0x0008e8bf, 0x0008e8d1 and 0x0008e8e3 all
// call slot +0x20, and the argument pairs are (p0,p1), (p1,p2), (p2,p0). Slot
// +0x24 is addTriangle and the oracle never reaches it from here.
extern "C" NXP_DLL_EXPORT void NX_CALL_CONV NxFluidDebugTriangle(const NxF32* points, NxU32 color)
	{
	NxVec3 p0(points[0], points[1], points[2]);
	NxVec3 p1(points[3], points[4], points[5]);
	NxVec3 p2(points[6], points[7], points[8]);
	NxDebugRenderable* renderable = getDebugRenderable();
	renderable->addLine(p0, p1, color);
	renderable->addLine(p1, p2, color);
	renderable->addLine(p2, p0, color);
	}

// A three axis cross of half extent `extent` about `point`, drawn as three
// addLine calls. In each pair the + end is built first: 0x0008eb49 adds before
// 0x0008eb5f subtracts, and the same order holds for y at 0x0008eb70/0x0008eb8e
// and for z at 0x0008ebb6/0x0008ebd2.
extern "C" NXP_DLL_EXPORT void NX_CALL_CONV NxFluidDebugPoint(const NxF32* point, NxF32 extent, NxU32 color)
	{
	NxDebugRenderable* renderable = getDebugRenderable();
	NxVec3 p(point[0], point[1], point[2]);

	renderable->addLine(NxVec3(p.x + extent, p.y, p.z), NxVec3(p.x - extent, p.y, p.z), color);
	renderable->addLine(NxVec3(p.x, p.y + extent, p.z), NxVec3(p.x, p.y - extent, p.z), color);
	renderable->addLine(NxVec3(p.x, p.y, p.z + extent), NxVec3(p.x, p.y, p.z - extent), color);
	}

// Three 40 segment circles through slot +0x38, addCircle, one per coordinate
// plane. The frame holds three separate 48 byte NxMat34s at esp+0, esp+0x30 and
// esp+0x60, all filled before the first call; their rotations are the three
// row permutations below and all three translations are the centre. The
// semicircle argument is a literal 0.
extern "C" NXP_DLL_EXPORT void NX_CALL_CONV NxFluidDebugSphere(const NxF32* center, NxF32 radius, NxU32 color)
	{
	NxDebugRenderable* renderable = getDebugRenderable();
	NxVec3 c(center[0], center[1], center[2]);

	NxMat34 xy;
	xy.M.setRow(0, NxVec3(1.0f, 0.0f, 0.0f));
	xy.M.setRow(1, NxVec3(0.0f, 1.0f, 0.0f));
	xy.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
	xy.t = c;

	NxMat34 zy;
	zy.M.setRow(0, NxVec3(0.0f, 0.0f, 1.0f));
	zy.M.setRow(1, NxVec3(0.0f, 1.0f, 0.0f));
	zy.M.setRow(2, NxVec3(1.0f, 0.0f, 0.0f));
	zy.t = c;

	NxMat34 xz;
	xz.M.setRow(0, NxVec3(1.0f, 0.0f, 0.0f));
	xz.M.setRow(1, NxVec3(0.0f, 0.0f, 1.0f));
	xz.M.setRow(2, NxVec3(0.0f, 1.0f, 0.0f));
	xz.t = c;

	renderable->addCircle(40, xy, color, radius, false);
	renderable->addCircle(40, zy, color, radius, false);
	renderable->addCircle(40, xz, color, radius, false);
	}

// 0x0008ec2b calls slot +0x30, addArrow, with the position copied from the first
// argument and the direction from the second, and the remaining three arguments
// forwarded untouched.
extern "C" NXP_DLL_EXPORT void NX_CALL_CONV NxFluidDebugArrow(const NxF32* position, const NxF32* direction,
	NxF32 length, NxF32 scale, NxU32 color)
	{
	NxDebugRenderable* renderable = getDebugRenderable();
	NxVec3 p(position[0], position[1], position[2]);
	NxVec3 d(direction[0], direction[1], direction[2]);
	renderable->addArrow(p, d, length, scale, color);
	}

// 0x0008ed19 calls slot +0x2c, addAABB, with the six floats copied into one
// NxBounds3 and a literal 0 for renderFrame.
extern "C" NXP_DLL_EXPORT void NX_CALL_CONV NxFluidDebugAABB(const NxF32* bounds, NxU32 color)
	{
	NxBounds3 box;
	box.set(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
	getDebugRenderable()->addAABB(box, color, false);
	}
