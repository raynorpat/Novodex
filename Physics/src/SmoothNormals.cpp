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

// Every square root in this file is one the oracle takes with `fsqrt`:
// 0x0005338b in the angle helper and 0x00053560 and 0x000537ad in the two
// normalise loops. MSVC compiles `sqrt()` to `__CIsqrt`, which does not follow
// the x87 control word where `fsqrt` does, so calling the CRT here was a
// transcription defect -- the row was closed against a routine the oracle never
// calls.
//
// IT MOVED NOTHING, AND THAT IS RECORDED RATHER THAN QUIETLY DROPPED. Swapping
// all three sites to `fsqrt` left the candidate digest byte-identical over
// 919,352 checks under both control words, because every argument here is a
// `double` by the time it arrives and the 64-then-53 double rounding of a
// square root agrees with rounding once. The same is true of `fpatan` below.
// The correction stands on the disassembly, not on a digest -- the same footing
// as the `fabs` fix in ContactGeneration.cpp.
//
// The same shape as ContactGeneration.cpp's helper, and the same reason for
// storing the result rather than leaving it in st(0) -- leaving it there makes
// the caller responsible for popping a register the compiler did not put there,
// which moved an *oracle-side* digest once already.
static double nxSqrt(double value)
	{
	double result;
	__asm
		{
		fld value
		fsqrt
		fstp result
		}
	return result;
	}

// phys_fn_002144 (0x000532e0, 217 B)
// The angle helper NxBuildSmoothNormals calls three times per triangle, and
// MeshNormals::Compute (phys_fn_001651, IceMeshTools.cpp) through the same
// register convention: the vertex in eax, the triangle's three indices at edx,
// the vertex array in esi; ebx and edi are preserved, ecx is not; the result
// comes back in st(0). The weight is the triangle's interior angle at the
// vertex, atan2(|A x B|, A.B) with A and B the two edges leaving it (the
// "first" and "second" vertices below), formed with `fpatan`:
//
//   vertex == index[0]: first index[2], second index[1]
//   vertex == index[1]: first index[2], second index[0]
//   vertex == index[2]: first index[0], second index[1]
//   otherwise:          first index[0], second index[0]
//
// The listing keeps all six edge differences in st(1)..st(7) from 0x00053325 to
// 0x000533a5 and stores only B.z (a non-popping `fst` at 0x0005334d, reused
// narrowed for C.x and the dot product), C.y (0x00053365) and C.z (0x00053373,
// squared as the register value times the narrowed one); the cross product's
// length is square-rooted and rounded to a float (0x0005338d) before `fpatan`,
// and the angle is rounded to a float (0x000533a9) and reloaded.
//
// Convex-mesh gap Task 2e writes it as the listing's instructions, naked, so
// the frame, the register convention and every operand (which are loaded and
// which are used from memory, which decides the payload a signalling NaN
// propagates) are the oracle's. The C++ angle helper this file had before
// modelled it with the same groupings in C++ and agreed with the oracle on
// every quiet input (step_smooth_normals); it now calls this row
// (angleAtVertex below).
__declspec(naked) void nxSmoothNormalsAngleAtVertex()
	{
	__asm
		{
		sub	esp, 0x1c		// 0x000532e0
		push	ebx		// 0x000532e3
		mov	ebx, dword ptr [edx]		// 0x000532e4
		push	edi		// 0x000532e6
		xor	ecx, ecx		// 0x000532e7
		xor	edi, edi		// 0x000532e9
		cmp	eax, ebx		// 0x000532eb
		jne	L532f6		// 0x000532ed
		mov	ecx, 2		// 0x000532ef
		jmp	L5330b		// 0x000532f4
L532f6:
		cmp	eax, dword ptr [edx + 4]		// 0x000532f6
		jne	L53304		// 0x000532f9
		mov	ecx, 2		// 0x000532fb
		xor	edi, edi		// 0x00053300
		jmp	L53310		// 0x00053302
L53304:
		cmp	eax, dword ptr [edx + 8]		// 0x00053304
		jne	L53310		// 0x00053307
		xor	ecx, ecx		// 0x00053309
L5330b:
		mov	edi, 1		// 0x0005330b
L53310:
		mov	ecx, dword ptr [edx + ecx*4]		// 0x00053310
		lea	ecx, [ecx + ecx*2]		// 0x00053313
		fld	dword ptr [esi + ecx*4]		// 0x00053316
		lea	ecx, [esi + ecx*4]		// 0x00053319
		lea	eax, [eax + eax*2]		// 0x0005331c
		fsub	dword ptr [esi + eax*4]		// 0x0005331f
		lea	eax, [esi + eax*4]		// 0x00053322
		fld	dword ptr [ecx + 4]		// 0x00053325
		fsub	dword ptr [eax + 4]		// 0x00053328
		fld	dword ptr [ecx + 8]		// 0x0005332b
		mov	ecx, dword ptr [edx + edi*4]		// 0x0005332e
		fsub	dword ptr [eax + 8]		// 0x00053331
		lea	edx, [ecx + ecx*2]		// 0x00053334
		fld	dword ptr [esi + edx*4]		// 0x00053337
		lea	ecx, [esi + edx*4]		// 0x0005333a
		fsub	dword ptr [eax]		// 0x0005333d
		pop	edi		// 0x0005333f
		fld	dword ptr [ecx + 4]		// 0x00053340
		pop	ebx		// 0x00053343
		fsub	dword ptr [eax + 4]		// 0x00053344
		fld	dword ptr [ecx + 8]		// 0x00053347
		fsub	dword ptr [eax + 8]		// 0x0005334a
		fst	dword ptr [esp + 0x18]		// 0x0005334d
		fmul	st, st(4)		// 0x00053351
		fld	st(1)		// 0x00053353
		fmul	st, st(4)		// 0x00053355
		fsubp	st(1), st		// 0x00053357
		fld	st(3)		// 0x00053359
		fmul	st, st(3)		// 0x0005335b
		fld	dword ptr [esp + 0x18]		// 0x0005335d
		fmul	st, st(7)		// 0x00053361
		fsubp	st(1), st		// 0x00053363
		fstp	dword ptr [esp + 8]		// 0x00053365
		fld	st(1)		// 0x00053369
		fmul	st, st(6)		// 0x0005336b
		fld	st(3)		// 0x0005336d
		fmul	st, st(6)		// 0x0005336f
		fsubp	st(1), st		// 0x00053371
		fst	dword ptr [esp + 0xc]		// 0x00053373
		fmul	dword ptr [esp + 0xc]		// 0x00053377
		fld	dword ptr [esp + 8]		// 0x0005337b
		fmul	dword ptr [esp + 8]		// 0x0005337f
		faddp	st(1), st		// 0x00053383
		fld	st(1)		// 0x00053385
		fmul	st, st(2)		// 0x00053387
		faddp	st(1), st		// 0x00053389
		fsqrt		// 0x0005338b
		fstp	dword ptr [esp]		// 0x0005338d
		fstp	st(0)		// 0x00053390
		fld	dword ptr [esp]		// 0x00053392
		fld	dword ptr [esp + 0x18]		// 0x00053395
		fmul	st, st(4)		// 0x00053399
		fxch	st(2)		// 0x0005339b
		fmul	st, st(5)		// 0x0005339d
		faddp	st(2), st		// 0x0005339f
		fxch	st(2)		// 0x000533a1
		fmul	st, st(5)		// 0x000533a3
		faddp	st(1), st		// 0x000533a5
		fpatan		// 0x000533a7
		fstp	dword ptr [esp]		// 0x000533a9
		fstp	st(0)		// 0x000533ac
		fstp	st(0)		// 0x000533ae
		fstp	st(0)		// 0x000533b0
		fld	dword ptr [esp]		// 0x000533b2
		add	esp, 0x1c		// 0x000533b5
		ret		// 0x000533b8
		}
	}

// The C++ entry NxBuildSmoothNormals calls: phys_fn_002144 with the listing's
// registers, its st(0) result stored as the float it already is.
static NxReal angleAtVertex(NxU32 vertex, const NxU32* index, const NxVec3* verts)
	{
	NxReal angle;
	__asm
		{
		push	esi
		mov		eax, vertex
		mov		edx, index
		mov		esi, verts
		call	nxSmoothNormalsAngleAtVertex
		fstp	angle
		pop		esi
		}
	return angle;
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
		const double length = nxSqrt((faceNormals[i].x * (double) faceNormals[i].x
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
		const double length = nxSqrt((target.x * (double) target.x
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
