/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// NxBuildSmoothNormals, 0x000533c0.
//
// The odd one out among the Phase 3 geometry exports. Every other one is a pure
// arithmetic leaf; this one takes arrays, allocates, and calls a helper. It is
// its own translation unit because the oracle puts it a long way from the
// ray/segment kernels, at 0x000533c0 against their 0x00036xxx.
//
// The allocation does NOT reach the SDK allocator. The call at 0x000533fd goes
// through an incremental-link thunk to the CRT's own `operator new`, which
// reaches `_nh_malloc` and then `HeapAlloc` on the CRT heap -- there is no
// NxUserAllocator function pointer and no SDK singleton anywhere on the path.
// The standing Phase 3 rule that no geometry kernel may reach the SDK allocator
// therefore holds, and this export is the only one that had to be checked
// rather than argued from an empty call list.
//
// The same file conventions apply as in Geometry.cpp: a value the oracle keeps
// in an x87 register is `double` and a value it stores to a 32-bit slot is
// `NxReal`, and the file is built /arch:IA32.

#include "Nxp.h"
#include "NxVec3.h"
#include "NxSmoothNormals.h"

#include <math.h>
#include <string.h>

// 0x000532e0. The weight the accumulation uses is the triangle's interior angle
// at the vertex, formed as atan2(|A x B|, A.B) -- the header's claim that it
// "takes angles into account" is literally true. The oracle computes it with
// the x87 `fpatan` instruction.
//
// Three values appear at two precisions inside one expression here, which is
// what makes this helper worth its own function rather than being folded in:
// B.z is stored with a non-popping `fst` at 0x0005334d and then used as the
// register copy for C.x and as the narrowed copy for C.y and for the dot
// product; C.z is stored the same way at 0x00053373 and then squared as the
// register value times the narrowed one.
static NxReal angleAtVertex(NxU32 vertex, const NxU32* index, const NxVec3* verts)
	{
	NxU32 first, second;
	if(vertex == index[0])      { first = index[2]; second = index[1]; }
	else if(vertex == index[1]) { first = index[2]; second = index[0]; }
	else if(vertex == index[2]) { first = index[0]; second = index[1]; }
	else                        { first = index[0]; second = index[0]; }

	const double ax = verts[first].x - (double) verts[vertex].x;
	const double ay = verts[first].y - (double) verts[vertex].y;
	const double az = verts[first].z - (double) verts[vertex].z;

	const double bx = verts[second].x - (double) verts[vertex].x;
	const double by = verts[second].y - (double) verts[vertex].y;
	const double bzRegister = verts[second].z - (double) verts[vertex].z;
	const NxReal bz = (NxReal) bzRegister;

	const double cx = bzRegister * ay - by * az;
	// The narrowing of C.y here, and the narrowed B.z in the dot product below,
	// are both transcribed from the disassembly and neither is observable:
	// mutations that keep either at register precision move 0 matrix cases and
	// 0 digests. atan2 is flat enough near its argument that a one-ulp change to
	// a cross-product component does not survive into the weight.
	const NxReal cy = (NxReal) (az * bx - bz * ax);
	const double czRegister = by * ax - bx * ay;
	const NxReal cz = (NxReal) czRegister;

	// z, then y, then x -- and the z term is the register value times the
	// narrowed one.
	const NxReal length = (NxReal) sqrt((czRegister * cz + cy * (double) cy) + cx * cx);

	// Same z, y, x order, and the z term uses the narrowed B.z.
	const double dot = ((bz * az) + by * ay) + bx * ax;

	return (NxReal) atan2((double) length, dot);
	}

// 0x000533c0. Three passes: unit face normals into a scratch array, an
// angle-weighted accumulation into the caller's array, then a per-vertex
// normalize in place.
//
// Things here that a correct implementation would not do, all reproduced:
//
//   * no index is ever bounds-checked against nbVerts. An out-of-range face
//     index reads outside `verts` and, in the second pass, read-modify-writes
//     outside `normals`. That is a shipped memory-safety defect and it is
//     recorded rather than fixed.
//   * when both index arrays are null the code does not fall back to sequential
//     triangles: it uses the constants 0, 1, 2 for *every* triangle, ignores
//     `flip`, and still returns true.
//   * `dFaces` silently shadows `wFaces` when both are supplied.
//   * `flip` is applied in the first pass only. The angle weight is symmetric
//     in its two edges so this is numerically harmless, but the asymmetry is
//     transcribed rather than unified.
//   * both size computations are unchecked, so nbTris >= 0x15555556 wraps and
//     under-allocates.
//   * the zero guard on both normalizes is an exact equality, so a denormal
//     length survives it and 1/denormal overflows to infinity.
//
// A zero-length accumulated normal is left as (0,0,0) rather than becoming a
// NaN: the guard skips the divide and leaves the components alone.
bool NX_CALL_CONV NxBuildSmoothNormals(NxU32 nbTris, NxU32 nbVerts, const NxVec3* verts,
	const NxU32* dFaces, const NxU16* wFaces, NxVec3* normals, bool flip)
	{
	if(!verts)
		return false;
	if(!normals)
		return false;
	if(!nbTris)
		return false;
	if(!nbVerts)
		return false;

	// The oracle's element constructor is empty, so the raw allocation is the
	// whole of what it does; the buffer is fully written by the first pass
	// before the second reads it.
	NxVec3* faceNormals = (NxVec3*) ::operator new(nbTris * sizeof(NxVec3));
	if(!faceNormals)
		return false;

	// 0x00053425 is `setne dl` on the raw argument byte (0x0005341d is the
	// `mov cl, byte ptr [esp+0x74]` that loads it), so *any* nonzero byte
	// normalises to exactly 1. That matters: a caller can pass a `bool` holding
	// a value other than 0 or 1, and the case matrix does exactly that in
	// NxBuildSmoothNormals.12. Written as `flip ? 1 : 0` the compiler is
	// entitled to assume the byte is already 0 or 1 and forward it unchanged,
	// which makes the index offsets below run off the end of the triangle.
	// The read is volatile because nothing weaker survives the optimiser: given
	// a `bool` the compiler is free to assume the byte is 0 or 1 and fold the
	// test away, and it does so through a memcpy as readily as through a
	// ternary. With flip = 2 that leaves f = 2, the two index offsets below
	// become +3 and +0, and the face collapses to a zero normal.
	const volatile unsigned char* flipByte = (const volatile unsigned char*) &flip;
	const NxU32 f = *flipByte != 0 ? 1 : 0;

	for(NxU32 i = 0; i < nbTris; ++i)
		{
		NxU32 i0, i1, i2;
		if(dFaces)
			{
			i0 = dFaces[i * 3 + 0];
			i1 = dFaces[i * 3 + 1 + f];
			i2 = dFaces[i * 3 + 2 - f];
			}
		else if(wFaces)
			{
			i0 = wFaces[i * 3 + 0];
			i1 = wFaces[i * 3 + 1 + f];
			i2 = wFaces[i * 3 + 2 - f];
			}
		else
			{
			i0 = 0;
			i1 = 1;
			i2 = 2;
			}

		// The x component of the first edge stays in a register and the other
		// five are narrowed, so the cross product below is not symmetric in its
		// three components.
		const double px = verts[i1].x - (double) verts[i0].x;
		const NxReal py = (NxReal) (verts[i1].y - (double) verts[i0].y);
		const NxReal pz = (NxReal) (verts[i1].z - (double) verts[i0].z);
		const NxReal qx = (NxReal) (verts[i2].x - (double) verts[i0].x);
		const NxReal qy = (NxReal) (verts[i2].y - (double) verts[i0].y);
		const NxReal qz = (NxReal) (verts[i2].z - (double) verts[i0].z);

		// The baseline winding is the reverse of the textbook one: with flip
		// clear this is (v2 - v0) x (v1 - v0).
		const double nx = (qy * (double) pz) - (qz * (double) py);
		const double ny = (qz * px) - (pz * (double) qx);
		const double nz = (py * (double) qx) - (qy * px);

		// Stored z first, then x, then y.
		faceNormals[i].z = (NxReal) nz;
		faceNormals[i].x = (NxReal) nx;
		faceNormals[i].y = (NxReal) ny;

		// The three products reload the narrowed components, summed (x + y) + z.
		const double length = sqrt((faceNormals[i].x * (double) faceNormals[i].x
			+ faceNormals[i].y * (double) faceNormals[i].y)
			+ faceNormals[i].z * (double) faceNormals[i].z);
		if(length != 0.0f)
			{
			// A reciprocal reused three times, not three divisions.
			const double inverse = 1.0f / length;
			faceNormals[i].x = (NxReal) (inverse * faceNormals[i].x);
			faceNormals[i].y = (NxReal) (inverse * faceNormals[i].y);
			faceNormals[i].z = (NxReal) (inverse * faceNormals[i].z);
			}
		}

	memset(normals, 0, nbVerts * sizeof(NxVec3));

	for(NxU32 i = 0; i < nbTris; ++i)
		{
		NxU32 index[3];
		if(dFaces)
			{
			index[0] = dFaces[i * 3 + 0];
			index[1] = dFaces[i * 3 + 1];
			index[2] = dFaces[i * 3 + 2];
			}
		else if(wFaces)
			{
			index[0] = wFaces[i * 3 + 0];
			index[1] = wFaces[i * 3 + 1];
			index[2] = wFaces[i * 3 + 2];
			}
		else
			{
			index[0] = 0;
			index[1] = 1;
			index[2] = 2;
			}

		for(NxU32 k = 0; k < 3; ++k)
			{
			const NxReal weight = angleAtVertex(index[k], index, verts);
			// x keeps register precision where y and z are narrowed, in all
			// three unrolled copies of this block.
			const double weightedX = weight * (double) faceNormals[i].x;
			const NxReal weightedY = (NxReal) (weight * (double) faceNormals[i].y);
			const NxReal weightedZ = (NxReal) (weight * (double) faceNormals[i].z);
			NxVec3& target = normals[index[k]];
			target.x = (NxReal) (weightedX + target.x);
			target.y = (NxReal) (weightedY + (double) target.y);
			target.z = (NxReal) (weightedZ + (double) target.z);
			}
		}

	for(NxU32 v = 0; v < nbVerts; ++v)
		{
		NxVec3& target = normals[v];
		const double length = sqrt((target.x * (double) target.x
			+ target.y * (double) target.y) + target.z * (double) target.z);
		if(length != 0.0f)
			{
			const double inverse = 1.0f / length;
			target.x = (NxReal) (inverse * target.x);
			target.y = (NxReal) (inverse * target.y);
			target.z = (NxReal) (inverse * target.z);
			}
		}

	::operator delete(faceNormals);
	return true;
	}
