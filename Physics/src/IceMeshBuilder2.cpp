/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// IceMeshBuilder2.cpp, sub-unit C of units/convex-mesh-gap-contract.md (the
// file name comes from the ICE correspondence: the rows carry no string and
// have no .rdata block). Written by convex-mesh gap Task 2d from the Capstone
// listing, 0x0002eb50..0x000313d5: ICE's MeshBuilder2 (Pierre Terdiman), which
// is not vendored. Its only caller, 002087 (0x000523c0, the
// EdgeList.cpp..InternalTriangleMesh.cpp gap), is not written, so nothing in
// the candidate calls these rows yet.
//
// What the rows call is: the vendored Container (004836 constructor, 004838
// Empty, 004840 Resize through the inline Add, 004846 destructor), RadixSort
// (005157, 005163 with the unsigned hint, 005159) and the vertex reduction of
// IceMeshTools.cpp (001645, 001647, 001659).
//
// Allocation (the contract's open item on the CRT heap, checked here): the
// rows' own blocks come from 005701 `operator new`, a `jmp` to 005702
// (__nh_malloc(size, 1)), and go back through 005700 `free`, a `jmp` to 005668
// (_free) -- the DLL's static CRT, one heap; 001514's 005702/005668 pair is the
// same heap, reached without the thunks. The candidate's pair is its own CRT's
// nothrow `operator new` and `free` (nxMb2New / nxMb2Free), which pair with
// each other (MSVC's operator new is malloc). `new[]` blocks of the 12-byte
// streams and the 0x30-byte faces carry the count cookie in their first word
// and are released at the pointer minus four, as the listing does.
//
// Every row the oracle has as a function of its own is a function here
// (`noinline`), so that each has an address to trace.
//
// x87: on the /arch:IA32 list. The float sections -- 001597's zero-area test,
// 001603's face normal, 001627's angle-weighted vertex normal, its sum and its
// normalisation, and the `fld`/`fstp` through which 001607 and 001627 pass the
// uvw words -- are assembly blocks transcribed from the listing (operand order
// and memory operands as there, so NaN payloads and signalling NaNs propagate
// as in the oracle). Built with /EHs-c-: the listings are frameless.

#include "IceMeshBuilder2.h"

#include <new>
#include <stdlib.h>
#include <string.h>
#if !NX_PHYSICS_USE_X87
#include <cmath>
#include <cstring>
#endif

#if !defined(NX_PHYSICS_HULL_KERNEL_ONLY)
// .rdata 0x101041f0 (0.0f) and 0x101041ec (1.0f).
static const float gMb2Zero = 0.0f;
static const float gMb2One = 1.0f;

// 005701 / 005700 (see the file comment).
static inline void* nxMb2New(NxU32 size)
	{
	return ::operator new(size, std::nothrow);
	}

static inline void nxMb2Free(void* memory)
	{
	free(memory);
	}

// `new T[count]` for the 12- and 0x30-byte element types: count * size + 4
// bytes (formed in 32 bits, as the listing's `lea`/`shl` form it), the count in
// the first word; the element constructor (0x10027f00, run by the vector
// constructor iterator 000001 where the listing calls it) writes nothing.
static inline void* nxMb2NewArray(NxU32 count, NxU32 size)
	{
	NxU32* block = (NxU32*) nxMb2New(count * size + 4);
	if(!block)
		return 0;
	block[0] = count;
	return block + 1;
	}

static inline void nxMb2DeleteArray(void* array)
	{
	nxMb2Free((NxU32*) array - 1);
	}

#endif

// phys_fn_001591 (0x0002eb50, 141 B)
// The oracle's is thiscall on the Container (`ret 4`): here __fastcall with the
// Container in ecx, an unused edx and the point on the stack, popped by the
// callee -- the same registers and the same `ret 4` -- since the vendored
// Container header is not changed to give it the member.
#if NX_PHYSICS_USE_X87
__declspec(noinline) IceCore::Container& __fastcall nxIceContainerAddPoint(IceCore::Container* container,
	NxU32 /*edx*/, const NxU32* point)
#else
IceCore::Container& nxIceContainerAddPoint(IceCore::Container* container, const NxU32* point)
#endif
	{
	container->Add(point[0]);
	container->Add(point[1]);
	container->Add(point[2]);
	return *container;
	}

#if !defined(NX_PHYSICS_HULL_KERNEL_ONLY)
// phys_fn_001593 (0x0002ebe0, 301 B)
// The thirteen Containers (004836), then the dwords +0xd0..+0xfc, +0x108,
// +0x10c, +0x110, +0x100, +0x114, +0x104 zeroed and the flag bytes cleared,
// except +0x118 and +0x11d, set. +0x123 is not written (it is only ever copied
// from a create block by 001623).
MeshBuilder2::MeshBuilder2()
	{
	mMaxNbFaces = 0;
	mNbVerts = 0;
	mNbTVerts = 0;
	mNbCVerts = 0;
	mNbFaces = 0;
	mNbRefs = 0;
	mNbOutVerts = 0;
	mVertsCopy = 0;
	mTVertsCopy = 0;
	mCVertsCopy = 0;
	mFaces = 0;
	mRefs = 0;
	mVertFaceCount = 0;
	mVertFaceOffset = 0;
	mVertFaceList = 0;
	mFaceRemap = 0;
	mNbNormInfo = 0;
	mNbFaceRemap = 0;
	mUseW = false;
	mComputeVNormals = false;
	mComputeFNormals = false;
	mComputeNormInfo = false;
	mIndexedUVW = false;
	mIndexedColors = false;
	mRelativeIndices = false;
	mIsSkin = false;
	mWeightNormalWithAngles = false;
	mKillZeroAreaFaces = true;
	mIndexedGeo = true;
	}

// phys_fn_001595 (0x0002ed10, 134 B)
// The count is stored first; an empty stream returns true with the pointer
// untouched. The copy is `new Point[nb]` (a count cookie, the trivial element
// constructor 0x10027f00 through 000001), filled from the source or zeroed.
__declspec(noinline) bool nxMb2DuplicateStream(NxU32 nb, const IceMaths::Point* src, IceMaths::Point** dst, NxU32* dstNb)
	{
	*dstNb = nb;
	if(!nb)
		return true;
	IceMaths::Point* Copy = (IceMaths::Point*) nxMb2NewArray(nb, 12);
	*dst = Copy;
	if(!Copy)
		return false;
	if(src)
		memcpy(Copy, src, nb * 12);
	else
		memset(Copy, 0, nb * 12);
	return true;
	}

// phys_fn_001597 (0x0002eda0, 1251 B)
// AddFace. False when the face or reference arrays are missing, when the face
// array is full, or when the face's index exceeds +0xd0 (equal is accepted).
// With +0x118 and vertex references, a face with a repeated reference or a
// zero cross product is dropped -- true, nothing recorded; the test reads the
// copied vertices at the face's references before they are clamped. The
// references of the three streams are stored with corners 1 and 2 swapped when
// Flip is set (0xffffffff for a missing stream) and then, stream by stream,
// references at or past the stream's count are set to 0.
__declspec(noinline) bool MeshBuilder2::AddFace(const MBFACEINFO& face)
	{
	if(!mFaces || !mRefs)
		return false;
	if(mNbFaces == mMaxNbFaces)
		return false;
	if(face.Index > mMaxNbFaces)
		return false;

	if(mKillZeroAreaFaces && face.VRefs)
		{
		const NxU32* VRefs = face.VRefs;
		if(VRefs[0] == VRefs[1] || VRefs[0] == VRefs[2] || VRefs[1] == VRefs[2])
			return true;

		// 0x0002ee29..0x0002eece: n = (p0 - p1) ^ (p0 - p2), x and y spilled to
		// floats, z on the stack; (z*z + y*y) + x*x compared with 0.0f.
		const IceMaths::Point* Verts = mVertsCopy;
		float T10, T14, T1c, T20;
		bool Zero;
#if NX_PHYSICS_USE_X87
		__asm
			{
			mov		esi, VRefs
			mov		edi, Verts
			mov		eax, dword ptr [esi]
			mov		ebx, dword ptr [esi + 4]
			mov		esi, dword ptr [esi + 8]
			lea		eax, [eax + eax*2]
			fld		dword ptr [edi + eax*4]
			lea		eax, [edi + eax*4]
			lea		ebx, [ebx + ebx*2]
			lea		ebx, [edi + ebx*4]
			lea		esi, [esi + esi*2]
			fsub	dword ptr [edi + esi*4]
			lea		edi, [edi + esi*4]
			fld		dword ptr [eax + 4]
			fsub	dword ptr [edi + 4]
			fld		dword ptr [eax + 8]
			fsub	dword ptr [edi + 8]
			fld		dword ptr [eax]
			fsub	dword ptr [ebx]
			fstp	T10
			fld		dword ptr [eax + 4]
			fsub	dword ptr [ebx + 4]
			fstp	T14
			fld		dword ptr [eax + 8]
			fsub	dword ptr [ebx + 8]
			fld		T14
			fmul	st, st(2)
			fld		st(1)
			fmul	st, st(4)
			fsubp	st(1), st
			fstp	T1c
			fmul	st, st(3)
			fld		T10
			fmul	st, st(2)
			fsubp	st(1), st
			fstp	T20
			fstp	st(0)
			fld		T10
			fmul	st, st(1)
			fld		T14
			fmul	st, st(3)
			fsubp	st(1), st
			fstp	st(2)
			fstp	st(0)
			fld		st(0)
			fmul	st, st(1)
			fld		T20
			fmul	T20
			faddp	st(1), st
			fld		T1c
			fmul	T1c
			faddp	st(1), st
			fld		gMb2Zero
			fucompp
			fnstsw	ax
			fstp	st(0)
			test	ah, 0x44
			setnp	Zero
			}
#else
		const IceMaths::Point& p0 = Verts[VRefs[0]];
		            const IceMaths::Point& p1 = Verts[VRefs[1]];
		            const IceMaths::Point& p2 = Verts[VRefs[2]];
		            const double ax = double(p0.x) - p2.x;
		            const double ay = double(p0.y) - p2.y;
		            const double az = double(p0.z) - p2.z;
		            const float bx = float(double(p0.x) - p1.x);
		            const float by = float(double(p0.y) - p1.y);
		            const double bz = double(p0.z) - p1.z;
		            const float x = float(double(by) * az - bz * ay);
		            const float y = float(bz * ax - double(bx) * az);
		            const double z = double(bx) * ay - double(by) * ax;
		            Zero = ((z * z + double(y) * y) + double(x) * x) == 0.0;
#endif
		if(Zero)
			return true;
		}

	MBFace& F = mFaces[mNbFaces];
	F.MaterialID = face.MaterialID;
	F.SmoothingGroups = mComputeVNormals ? face.SmoothingGroups : 1;
	F.Index = face.Index;

	const NxU32 Flip = face.Flip ? 1 : 0;
	MBRef* R = mRefs + mNbRefs;
	R[0].VRef = face.VRefs ? face.VRefs[0] : 0xffffffff;
	R[1].VRef = face.VRefs ? face.VRefs[1 + Flip] : 0xffffffff;
	R[2].VRef = face.VRefs ? face.VRefs[2 - Flip] : 0xffffffff;
	R[0].TRef = face.TRefs ? face.TRefs[0] : 0xffffffff;
	R[1].TRef = face.TRefs ? face.TRefs[1 + Flip] : 0xffffffff;
	R[2].TRef = face.TRefs ? face.TRefs[2 - Flip] : 0xffffffff;
	R[0].CRef = face.CRefs ? face.CRefs[0] : 0xffffffff;
	R[1].CRef = face.CRefs ? face.CRefs[1 + Flip] : 0xffffffff;
	R[2].CRef = face.CRefs ? face.CRefs[2 - Flip] : 0xffffffff;

	if(face.VRefs)
		{
		if(R[0].VRef >= mNbVerts)	R[0].VRef = 0;
		if(R[1].VRef >= mNbVerts)	R[1].VRef = 0;
		if(R[2].VRef >= mNbVerts)	R[2].VRef = 0;
		}
	if(face.TRefs)
		{
		if(R[0].TRef >= mNbTVerts)	R[0].TRef = 0;
		if(R[1].TRef >= mNbTVerts)	R[1].TRef = 0;
		if(R[2].TRef >= mNbTVerts)	R[2].TRef = 0;
		}
	if(face.CRefs)
		{
		if(R[0].CRef >= mNbCVerts)	R[0].CRef = 0;
		if(R[1].CRef >= mNbCVerts)	R[1].CRef = 0;
		if(R[2].CRef >= mNbCVerts)	R[2].CRef = 0;
		}

	F.Ref[0] = mNbRefs++;
	F.Ref[1] = mNbRefs++;
	F.Ref[2] = mNbRefs++;
	mNbFaces++;
	return true;
	}

// phys_fn_001599 (0x0002f290, 61 B)
// phys_fn_001601 (0x0002f2d0, 480 B)
// With +0x11a only. Every face with no smoothing group gets three vertices of
// its own (unless +0x121): copies of its corners' vertices, appended after the
// existing ones, its references pointed at them; either way its smoothing
// groups become 0xffffffff. The new vertex array is `new Point[]` with a
// cookie (no constructor call), old vertices then new; the old one is released
// at the pointer minus four.
__declspec(noinline) bool MeshBuilder2::ComputeUnsharedVertices()
	{
	if(!mComputeVNormals)
		return true;

	NxU32 NewIndex = mNbVerts;
	IceCore::Container Unshared;
	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		MBFace& F = mFaces[i];
		if(F.SmoothingGroups)
			continue;
		if(!mIsSkin)
			{
			#if NX_PHYSICS_USE_X87
			nxIceContainerAddPoint(&Unshared, 0, (const NxU32*) &mVertsCopy[mRefs[F.Ref[0]].VRef]);
			#else
			nxIceContainerAddPoint(&Unshared, (const NxU32*) &mVertsCopy[mRefs[F.Ref[0]].VRef]);
			#endif
			#if NX_PHYSICS_USE_X87
			nxIceContainerAddPoint(&Unshared, 0, (const NxU32*) &mVertsCopy[mRefs[F.Ref[1]].VRef]);
			#else
			nxIceContainerAddPoint(&Unshared, (const NxU32*) &mVertsCopy[mRefs[F.Ref[1]].VRef]);
			#endif
			#if NX_PHYSICS_USE_X87
			nxIceContainerAddPoint(&Unshared, 0, (const NxU32*) &mVertsCopy[mRefs[F.Ref[2]].VRef]);
			#else
			nxIceContainerAddPoint(&Unshared, (const NxU32*) &mVertsCopy[mRefs[F.Ref[2]].VRef]);
			#endif
			mRefs[F.Ref[0]].VRef = NewIndex++;
			mRefs[F.Ref[1]].VRef = NewIndex++;
			mRefs[F.Ref[2]].VRef = NewIndex++;
			}
		mFaces[i].SmoothingGroups = 0xffffffff;
		}

	const NxU32 NbNew = Unshared.GetNbEntries() / 3;
	if(NbNew)
		{
		const NxU32 Total = mNbVerts + NbNew;
		IceMaths::Point* NewVerts = (IceMaths::Point*) nxMb2NewArray(Total, 12);
		if(!NewVerts)
			return false;
		memcpy(NewVerts, mVertsCopy, mNbVerts * 12);
		memcpy(NewVerts + mNbVerts, Unshared.GetEntries(), NbNew * 12);
		if(mVertsCopy)
			{
			nxMb2DeleteArray(mVertsCopy);
			mVertsCopy = 0;
			}
		mVertsCopy = NewVerts;
		mNbVerts += NbNew;
		}
	return true;
	}

// phys_fn_001602 (0x0002f4b0, 272 B)
// The reference records reduced as 12-byte vertices (001645/001647/001659;
// 001647's result is not tested), the faces' references remapped through the
// cross-reference, and the reduced records copied into a fresh plain block.
__declspec(noinline) bool MeshBuilder2::ReduceReferences()
	{
	ReducedVertices Reducer((const IceMaths::Point*) mRefs, mNbRefs);
	REDUCEDCLOUD RC;
	Reducer.Reduce(&RC);
	const IceMaths::Point* Reduced = RC.RVerts;
	const NxU32 NbReduced = RC.NbRVerts;
	const NxU32* XRef = RC.XRef;

	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		mFaces[i].Ref[0] = XRef[mFaces[i].Ref[0]];
		mFaces[i].Ref[1] = XRef[mFaces[i].Ref[1]];
		mFaces[i].Ref[2] = XRef[mFaces[i].Ref[2]];
		}

	if(mRefs)
		{
		nxMb2Free(mRefs);
		mRefs = 0;
		}
	mRefs = (MBRef*) nxMb2New(NbReduced * 12);
	if(!mRefs)
		return false;
	memcpy(mRefs, Reduced, NbReduced * 12);
	mNbRefs = NbReduced;
	return true;
	}

// phys_fn_001603 (0x0002f5c0, 954 B)
// phys_fn_001605 (0x0002f980, 58 B)
// With +0x11a or +0x11b. Each face's normal (p2 - p1) x (p0 - p1), normalised
// in place when its squared length is not zero, and with +0x11b appended to
// the face normals (+0x90). Then per vertex: the number of faces using it
// (+0x108), their offsets (+0x10c) and the face list (+0x110), the offsets
// advanced while filling and formed again afterwards. The two count arrays are
// zeroed; a failed allocation returns false without releasing anything.
__declspec(noinline) bool MeshBuilder2::ComputeNormals()
	{
	if(!mComputeFNormals && !mComputeVNormals)
		return true;
	if(!mNbVerts || !mNbRefs || !mRefs || !mFaces || !mVertsCopy)
		return false;

	const IceMaths::Point* Verts = mVertsCopy;
	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		MBFace* F = &mFaces[i];
		const IceMaths::Point* P0 = &Verts[mRefs[F->Ref[0]].VRef];
		const IceMaths::Point* P1 = &Verts[mRefs[F->Ref[1]].VRef];
		const IceMaths::Point* P2 = &Verts[mRefs[F->Ref[2]].VRef];
		float* N = F->Normal;

		// 0x0002f661..0x0002f734.
		float T14, T18, T20, T24, T28;
#if NX_PHYSICS_USE_X87
		__asm
			{
			mov		eax, P1
			mov		edx, P2
			mov		ecx, P0
			mov		edi, N
			fld		dword ptr [edx]
			fsub	dword ptr [eax]
			fld		dword ptr [edx + 4]
			fsub	dword ptr [eax + 4]
			fld		dword ptr [edx + 8]
			fsub	dword ptr [eax + 8]
			fld		dword ptr [ecx]
			fsub	dword ptr [eax]
			fstp	T14
			fld		dword ptr [ecx + 4]
			fsub	dword ptr [eax + 4]
			fstp	T18
			fld		dword ptr [ecx + 8]
			fsub	dword ptr [eax + 8]
			fld		st(0)
			fmul	st, st(3)
			fld		T18
			fmul	st, st(3)
			fsubp	st(1), st
			fstp	T20
			mov		eax, T20
			fxch	st(1)
			mov		dword ptr [edi], eax
			fmul	T14
			fxch	st(1)
			fmul	st, st(3)
			fsubp	st(1), st
			fstp	T24
			mov		eax, T24
			fld		T18
			mov		dword ptr [edi + 4], eax
			fmul	st, st(2)
			fxch	st(1)
			fmul	T14
			fsubp	st(1), st
			fstp	T28
			mov		eax, T28
			mov		dword ptr [edi + 8], eax
			fstp	st(0)
			fld		dword ptr [edi + 8]
			fld		dword ptr [edi + 4]
			fld		dword ptr [edi]
			fld		st(0)
			fmul	st, st(1)
			fld		st(2)
			fmul	st, st(3)
			faddp	st(1), st
			fld		st(3)
			fmul	st, st(4)
			faddp	st(1), st
			fstp	st(3)
			fstp	st(0)
			fstp	st(0)
			fld		gMb2Zero
			fld		st(1)
			fucompp
			fnstsw	ax
			test	ah, 0x44
			jnp		Skip
			fsqrt
			fdivr	gMb2One
			fld		st(0)
			fmul	dword ptr [edi]
			fstp	dword ptr [edi]
			fld		st(0)
			fmul	dword ptr [edi + 4]
			fstp	dword ptr [edi + 4]
			fmul	dword ptr [edi + 8]
			fstp	dword ptr [edi + 8]
			jmp		Done
		Skip:
			fstp	st(0)
		Done:
			}
#else
		const double ax = double(P2->x) - P1->x;
		            const double ay = double(P2->y) - P1->y;
		            const double az = double(P2->z) - P1->z;
		            const float bx = float(double(P0->x) - P1->x);
		            const float by = float(double(P0->y) - P1->y);
		            const double bz = double(P0->z) - P1->z;
		            N[0] = float(bz * ay - double(by) * az);
		            N[1] = float(az * double(bx) - bz * ax);
		            N[2] = float(double(by) * ax - ay * double(bx));
		            const double square = (double(N[0]) * N[0] + double(N[1]) * N[1])
		                + double(N[2]) * N[2];
		            if(square != 0.0)
		            {
		                const double inverse = 1.0 / std::sqrt(square);
		                for(unsigned component = 0; component < 3; ++component)
		                    N[component] = float(inverse * N[component]);
		            }
#endif

		if(mComputeFNormals)
			#if NX_PHYSICS_USE_X87
			nxIceContainerAddPoint(&mFaceNormals, 0, (const NxU32*) N);
			#else
			nxIceContainerAddPoint(&mFaceNormals, (const NxU32*) N);
			#endif
		}

	mVertFaceCount = (NxU32*) nxMb2New(mNbVerts * 4);
	if(!mVertFaceCount)
		return false;
	mVertFaceOffset = (NxU32*) nxMb2New(mNbVerts * 4);
	if(!mVertFaceOffset)
		return false;
	memset(mVertFaceCount, 0, mNbVerts * 4);
	memset(mVertFaceOffset, 0, mNbVerts * 4);

	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		mVertFaceCount[mRefs[mFaces[i].Ref[0]].VRef]++;
		mVertFaceCount[mRefs[mFaces[i].Ref[1]].VRef]++;
		mVertFaceCount[mRefs[mFaces[i].Ref[2]].VRef]++;
		}
	for(NxU32 i = 1; i < mNbVerts; i++)
		mVertFaceOffset[i] = mVertFaceCount[i - 1] + mVertFaceOffset[i - 1];

	mVertFaceList = (NxU32*) nxMb2New(mNbFaces * 12);
	if(!mVertFaceList)
		return false;
	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		const NxU32 V0 = mRefs[mFaces[i].Ref[0]].VRef;
		const NxU32 V1 = mRefs[mFaces[i].Ref[1]].VRef;
		const NxU32 V2 = mRefs[mFaces[i].Ref[2]].VRef;
		mVertFaceList[mVertFaceOffset[V0]] = i;
		mVertFaceOffset[V0]++;
		mVertFaceList[mVertFaceOffset[V1]] = i;
		mVertFaceOffset[V1]++;
		mVertFaceList[mVertFaceOffset[V2]] = i;
		mVertFaceOffset[V2]++;
		}

	mVertFaceOffset[0] = 0;
	for(NxU32 i = 1; i < mNbVerts; i++)
		mVertFaceOffset[i] = mVertFaceCount[i - 1] + mVertFaceOffset[i - 1];
	return true;
	}

// One word through the FPU, as the inline Container::Add(float) passes it
// (`fld` / `fstp` to a stack slot, then an integer store): a signalling NaN
// comes out quiet.
static NxU32 nxMb2X87Word(const float* p)
	{
	NxU32 Bits;
#if NX_PHYSICS_USE_X87
	__asm
		{
		mov		eax, p
		fld		dword ptr [eax]
		fstp	Bits
		}
#else
	// Finite input domain: ordinary float load/store preserves the word.
	    const float value = *p;
	    std::memcpy(&Bits, &value, sizeof(Bits));
#endif
	return Bits;
	}

// phys_fn_001607 (0x0002f9c0, 380 B)
// The streams that are output as indexed: all vertices (+0x11d) through 001591
// into +0x50; all uvw words (+0x11e) through the FPU into +0x60, z only with
// +0x119; all colours (+0x11f) through 001591 into +0x70. Always true.
__declspec(noinline) bool MeshBuilder2::SaveStreams()
	{
	if(mVertsCopy && mIndexedGeo)
		for(NxU32 i = 0; i < mNbVerts; i++)
			#if NX_PHYSICS_USE_X87
			nxIceContainerAddPoint(&mVerts, 0, (const NxU32*) &mVertsCopy[i]);
			#else
			nxIceContainerAddPoint(&mVerts, (const NxU32*) &mVertsCopy[i]);
			#endif

	if(mTVertsCopy && mIndexedUVW)
		for(NxU32 i = 0; i < mNbTVerts; i++)
			{
			const float* T = (const float*) &mTVertsCopy[i];
			mTVerts.Add(nxMb2X87Word(&T[0]));
			mTVerts.Add(nxMb2X87Word(&T[1]));
			if(mUseW)
				mTVerts.Add(nxMb2X87Word(&T[2]));
			}

	if(mCVertsCopy && mIndexedColors)
		for(NxU32 i = 0; i < mNbCVerts; i++)
			#if NX_PHYSICS_USE_X87
			nxIceContainerAddPoint(&mCVerts, 0, (const NxU32*) &mCVertsCopy[i]);
			#else
			nxIceContainerAddPoint(&mCVerts, (const NxU32*) &mCVertsCopy[i]);
			#endif
	return true;
	}

// phys_fn_001609 (0x0002fb40, 257 B)
// One corner: a reference already output gives its output vertex; a new one
// appends (vertex, uvw, colour, the face's smoothing groups) to the run's
// Container and takes the next vertex number.
__declspec(noinline) void nxMb2RemapCorner(MBRemapContext* context, NxU32 ref, NxU32* counter, NxU32 corner)
	{
	const NxU32 Known = context->Remap[ref];
	if(Known != 0xffffffff)
		{
		context->Faces[context->Face].VRef[corner] = Known;
		return;
		}
	const MBRef& R = context->Refs[ref];
	const NxU32 VRef = R.VRef;
	const NxU32 TRef = R.TRef;
	const NxU32 CRef = R.CRef;
	IceCore::Container* Out = context->Out;
	Out->Add(VRef);
	Out->Add(TRef);
	Out->Add(CRef);
	Out->Add(context->SmoothingGroups);
	context->Faces[context->Face].VRef[corner] = *counter;
	context->Remap[ref] = *counter;
	(*counter)++;
	}

// phys_fn_001611 (0x0002fc50, 938 B)
// phys_fn_001613 (0x00030000, 186 B)
// phys_fn_001615 (0x000300c0, 120 B)
// One stream (`which` 1 vertices, 2 uvw, 4 colours; the reference field it
// selects is +0, +4, +8). The entries some reference uses are marked and packed
// (the references remapped), and the packed stream reduced (001645/001647).
// Without duplicates the packed copy becomes the stream; with them the reduced
// one does, the references go through the cross-reference and, for vertices,
// the faces left with a repeated vertex are dropped (a new face array of the
// old count, with a cookie). Every new stream is `new Point[]` with a cookie;
// the marks and the remap are plain blocks. For any other `which` no
// reference is read: the mark goes to the last selected index, first the
// count pointer's own value, as in the listing (0x0002fcaa).
__declspec(noinline) bool MeshBuilder2::OptimizeStream(NxU32* nb, IceMaths::Point** stream, NxU32 which)
	{
	if(!*nb)
		return true;

	NxU32 NbKept = 0;
	NxU8* Marks = (NxU8*) nxMb2New(*nb);
	if(!Marks)
		return false;
	memset(Marks, 0, *nb);

	NxU32 Index = (NxU32) (size_t) nb;
	for(NxU32 i = 0; i < mNbRefs; i++)
		{
		if(which == 1)			Index = mRefs[i].VRef;
		else if(which == 2)		Index = mRefs[i].TRef;
		else if(which == 4)		Index = mRefs[i].CRef;
		Marks[Index] = 1;
		}

	NxU32* Remap = (NxU32*) nxMb2New(*nb * 4);
	if(!Remap)
		{
		nxMb2Free(Marks);
		return false;
		}

	IceMaths::Point* Packed = (IceMaths::Point*) nxMb2NewArray(*nb, 12);
	if(!Packed)
		{
		nxMb2Free(Remap);
		nxMb2Free(Marks);
		return false;
		}

	for(NxU32 i = 0; i < *nb; i++)
		{
		if(Marks[i])
			{
			Remap[i] = NbKept;
			Packed[NbKept] = (*stream)[i];
			NbKept++;
			}
		}
	nxMb2Free(Marks);

	for(NxU32 i = 0; i < mNbRefs; i++)
		{
		if(which == 1)			mRefs[i].VRef = Remap[mRefs[i].VRef];
		else if(which == 2)		mRefs[i].TRef = Remap[mRefs[i].TRef];
		else if(which == 4)		mRefs[i].CRef = Remap[mRefs[i].CRef];
		}
	nxMb2Free(Remap);

	ReducedVertices Reducer(Packed, NbKept);
	REDUCEDCLOUD RC;
	Reducer.Reduce(&RC);

	if(RC.NbRVerts == NbKept)
		{
		IceMaths::Point* Copy = (IceMaths::Point*) nxMb2NewArray(NbKept, 12);
		if(!Copy)
			return false;
		memcpy(Copy, Packed, NbKept * 12);
		*nb = NbKept;
		if(*stream)
			{
			nxMb2DeleteArray(*stream);
			*stream = 0;
			}
		*stream = Copy;
		nxMb2DeleteArray(Packed);
		return true;
		}

	nxMb2DeleteArray(Packed);
	for(NxU32 i = 0; i < mNbRefs; i++)
		{
		if(which == 1)			mRefs[i].VRef = RC.XRef[mRefs[i].VRef];
		else if(which == 2)		mRefs[i].TRef = RC.XRef[mRefs[i].TRef];
		else if(which == 4)		mRefs[i].CRef = RC.XRef[mRefs[i].CRef];
		}

	IceMaths::Point* Copy = (IceMaths::Point*) nxMb2NewArray(RC.NbRVerts, 12);
	if(!Copy)
		return false;
	memcpy(Copy, RC.RVerts, RC.NbRVerts * 12);
	*nb = RC.NbRVerts;
	if(*stream)
		{
		nxMb2DeleteArray(*stream);
		*stream = 0;
		}
	*stream = Copy;

	if(which != 1)
		return true;

	// 0x0002ffdd..0x00030123: the faces whose three vertices still differ.
	IceCore::Container Kept;
	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		const NxU32 V0 = mRefs[mFaces[i].Ref[0]].VRef;
		const NxU32 V1 = mRefs[mFaces[i].Ref[1]].VRef;
		const NxU32 V2 = mRefs[mFaces[i].Ref[2]].VRef;
		if(V0 != V1 && V0 != V2 && V1 != V2)
			Kept.Add(i);
		}
	MBFace* NewFaces = (MBFace*) nxMb2NewArray(mNbFaces, sizeof(MBFace));
	if(!NewFaces)
		return false;
	for(NxU32 i = 0; i < Kept.GetNbEntries(); i++)
		NewFaces[i] = mFaces[Kept.GetEntry(i)];
	if(mFaces)
		{
		nxMb2DeleteArray(mFaces);
		mFaces = 0;
		}
	mFaces = NewFaces;
	mNbFaces = Kept.GetNbEntries();
	return true;
	}

// phys_fn_001617 (0x00030140, 141 B)
// phys_fn_001619 (0x000301d0, 222 B)
// The faces of one run through 001609, corner by corner, with a remap per
// reference (a plain block, not tested, filled with 0xff). With +0x120 the
// vertex numbering starts again at 0 for the run. The run's face count and
// new-vertex count are appended to +0xb0; the new-vertex count is returned.
__declspec(noinline) NxU32 MeshBuilder2::RemapFaces(const NxU32* faces, NxU32 nb_faces, IceCore::Container& out)
	{
	NxU32* Remap = (NxU32*) nxMb2New(mNbRefs * 4);
	memset(Remap, 0xff, mNbRefs * 4);
	if(mRelativeIndices)
		mNbOutVerts = 0;

	MBRemapContext Context;
	Context.Refs = mRefs;
	Context.Faces = mFaces;
	Context.Remap = Remap;
	Context.Out = &out;
	const NxU32 First = mNbOutVerts;
	for(NxU32 i = 0; i < nb_faces; i++)
		{
		const NxU32 Index = faces[i];
		Context.Face = Index;
		const MBFace& F = mFaces[Index];
		Context.SmoothingGroups = F.SmoothingGroups;
		const NxU32 Ref0 = F.Ref[0];
		const NxU32 Ref1 = F.Ref[1];
		const NxU32 Ref2 = F.Ref[2];
		nxMb2RemapCorner(&Context, Ref0, &mNbOutVerts, 0);
		nxMb2RemapCorner(&Context, Ref1, &mNbOutVerts, 1);
		nxMb2RemapCorner(&Context, Ref2, &mNbOutVerts, 2);
		}
	if(Remap)
		nxMb2Free(Remap);

	const NxU32 NbNew = mNbOutVerts - First;
	mRuns.Add(nb_faces);
	mRuns.Add(NbNew);
	return NbNew;
	}

// phys_fn_001621 (0x000302b0, 364 B)
// FreeUsedRam: the thirteen Containers emptied (004838) in order; the three
// stream copies and the faces released at the pointer minus four; the
// references, the three per-vertex arrays and the face remap as plain blocks.
__declspec(noinline) MeshBuilder2& MeshBuilder2::FreeUsedRam()
	{
	mTopology.Empty();
	mFacesPerRun.Empty();
	mVRefs.Empty();
	mTRefs.Empty();
	mCRefs.Empty();
	mVerts.Empty();
	mTVerts.Empty();
	mCVerts.Empty();
	mNormals.Empty();
	mFaceNormals.Empty();
	mNormInfo.Empty();
	mRuns.Empty();
	mMaterials.Empty();
	if(mVertsCopy)		{ nxMb2DeleteArray(mVertsCopy);		mVertsCopy = 0;		}
	if(mTVertsCopy)		{ nxMb2DeleteArray(mTVertsCopy);	mTVertsCopy = 0;	}
	if(mCVertsCopy)		{ nxMb2DeleteArray(mCVertsCopy);	mCVertsCopy = 0;	}
	if(mFaces)			{ nxMb2DeleteArray(mFaces);			mFaces = 0;			}
	if(mRefs)			{ nxMb2Free(mRefs);					mRefs = 0;			}
	if(mVertFaceCount)	{ nxMb2Free(mVertFaceCount);		mVertFaceCount = 0;	}
	if(mVertFaceOffset)	{ nxMb2Free(mVertFaceOffset);		mVertFaceOffset = 0;}
	if(mVertFaceList)	{ nxMb2Free(mVertFaceList);			mVertFaceList = 0;	}
	if(mFaceRemap)		{ nxMb2Free(mFaceRemap);			mFaceRemap = 0;		}
	return *this;
	}

// phys_fn_001623 (0x00030420, 398 B)
// Init: FreeUsedRam, the twelve flag bytes, the three streams copied (001595;
// a failure returns false at once), the w of every uvw zeroed without +0x119,
// then the face array (`new[]` of 0x30-byte records with a cookie, through the
// vector constructor iterator) and the reference records (a plain block of 36
// bytes per face). False when the create block has no faces.
__declspec(noinline) bool MeshBuilder2::Init(const MBCREATE& create)
	{
	FreeUsedRam();
	mKillZeroAreaFaces = create.KillZeroAreaFaces;
	mUseW = create.UseW;
	mComputeVNormals = create.ComputeVNormals;
	mComputeFNormals = create.ComputeFNormals;
	mComputeNormInfo = create.ComputeNormInfo;
	mIndexedGeo = create.IndexedGeo;
	mIndexedUVW = create.IndexedUVW;
	mIndexedColors = create.IndexedColors;
	mRelativeIndices = create.RelativeIndices;
	mIsSkin = create.IsSkin;
	mWeightNormalWithAngles = create.WeightNormalWithAngles;
	mOptimizeVertexList = create.OptimizeVertexList;

	if(!nxMb2DuplicateStream(create.NbVerts, create.Verts, &mVertsCopy, &mNbVerts))
		return false;
	if(!nxMb2DuplicateStream(create.NbTVerts, create.TVerts, &mTVertsCopy, &mNbTVerts))
		return false;
	if(!nxMb2DuplicateStream(create.NbCVerts, create.CVerts, &mCVertsCopy, &mNbCVerts))
		return false;

	if(!mUseW && mTVertsCopy)
		for(NxU32 i = 0; i < mNbTVerts; i++)
			((NxU32*) mTVertsCopy)[i * 3 + 2] = 0;

	mMaxNbFaces = create.NbFaces;
	if(!mMaxNbFaces)
		return false;
	mFaces = (MBFace*) nxMb2NewArray(mMaxNbFaces, sizeof(MBFace));
	if(!mFaces)
		return false;
	mRefs = (MBRef*) nxMb2New(mMaxNbFaces * 36);
	return mRefs != 0;
	}

// phys_fn_001625 (0x000305b0, 101 B)
// The three streams through 001611: vertices (1) unless +0x121, uvw (2),
// colours (4); the first failure returns false.
__declspec(noinline) bool MeshBuilder2::OptimizeStreams()
	{
	if(!mIsSkin && !OptimizeStream(&mNbVerts, &mVertsCopy, 1))
		return false;
	if(!OptimizeStream(&mNbTVerts, &mTVertsCopy, 2))
		return false;
	return OptimizeStream(&mNbCVerts, &mCVertsCopy, 4);
	}

// phys_fn_001627 (0x00030620, 1764 B)
// One run of faces of equal material and smoothing groups (001631): the run
// header (material, smoothing groups; 001617 appends the face and vertex
// counts; a closing 0), then per new vertex its uvw and colour (as indices or
// as values), its normal with +0x11a, and its position (as an index or a
// value); then the face count, the faces' output corners and the face remap.
// The vertex normal sums the normals of the faces around the vertex that share
// a smoothing group with it -- each weighted by the angle of that face at the
// vertex with +0x122 -- and is normalised unless its squared length is 0;
// with +0x11c the faces are listed in +0xa0 after their count.
__declspec(noinline) NxU32 MeshBuilder2::OutputRun(const NxU32* faces, NxU32 nb_faces, NxU32 material, NxU32 smoothing)
	{
	if(!mFaces)
		return 0;

	mRuns.Add(material);
	mRuns.Add(smoothing);

	IceCore::Container Local;
	const NxU32 NbNew = RemapFaces(faces, nb_faces, Local);
	const NxU32* Entries = Local.GetEntries();
	for(NxU32 NbToGo = NbNew; NbToGo; NbToGo--)
		{
		const NxU32 VRef = Entries[0];
		const NxU32 TRef = Entries[1];
		const NxU32 CRef = Entries[2];
		const NxU32 SmoothingGroups = Entries[3];
		Entries += 4;

		if(mTVertsCopy)
			{
			if(mIndexedUVW)
				mTRefs.Add(TRef);
			else
				{
				const float* T = (const float*) &mTVertsCopy[TRef];
				mTVerts.Add(nxMb2X87Word(&T[0]));
				mTVerts.Add(nxMb2X87Word(&T[1]));
				if(mUseW)
					mTVerts.Add(nxMb2X87Word(&T[2]));
				}
			}

		if(mCVertsCopy)
			{
			if(mIndexedColors)
				mCRefs.Add(CRef);
			else
				#if NX_PHYSICS_USE_X87
				nxIceContainerAddPoint(&mCVerts, 0, (const NxU32*) &mCVertsCopy[CRef]);
				#else
				nxIceContainerAddPoint(&mCVerts, (const NxU32*) &mCVertsCopy[CRef]);
				#endif
			}

		if(mComputeVNormals)
			{
			NxU32 Count = 0;
			NxU32 Slot = 0;
			// The sum starts as three zero words (integer stores, 0x0003085a..
			// 0x00030876).
			NxU32 SumWords[3];
			SumWords[0] = 0;
			SumWords[1] = 0;
			SumWords[2] = 0;
			float* Sum = (float*) SumWords;
			if(mComputeNormInfo)
				{
				Slot = mNormInfo.GetNbEntries();
				mNormInfo.Add(NxU32(0));
				}

			for(NxU32 j = 0; j < mVertFaceCount[VRef]; j++)
				{
				const NxU32 FaceIndex = mVertFaceList[mVertFaceOffset[VRef] + j];
				const MBFace* F = &mFaces[FaceIndex];
				if(!(F->SmoothingGroups & SmoothingGroups))
					continue;

				float* S = Sum;
				const bool Weighted = mWeightNormalWithAngles;
				const IceMaths::Point* A = 0;
				const IceMaths::Point* B = 0;
				const IceMaths::Point* C = 0;
				if(Weighted)
					{
					// 0x00030911..0x0003096b: the two other corners, as the
					// listing picks them (the first and second of the
					// stack triple at 0x38 indexed by edi and ebx).
					NxU32 Corners[3];
					Corners[0] = mRefs[F->Ref[0]].VRef;
					Corners[1] = mRefs[F->Ref[1]].VRef;
					Corners[2] = mRefs[F->Ref[2]].VRef;
					NxU32 E0 = 0, E1 = 0;
					if(VRef == Corners[0])			{ E0 = 2; E1 = 1; }
					else if(VRef == Corners[1])		{ E0 = 2; E1 = 0; }
					else if(VRef == Corners[2])		{ E0 = 0; E1 = 1; }
					A = &mVertsCopy[Corners[E0]];
					B = &mVertsCopy[Corners[E1]];
					C = &mVertsCopy[VRef];
					}

				// Weighted, 0x0003097c..0x00030a57; plain, 0x00030a59..0x00030a73;
				// the z store they share at 0x00030a7d.
				float T20, T48, T4c, T58, T60, T64;
#if NX_PHYSICS_USE_X87
				__asm
					{
					mov		eax, F
					mov		esi, S
					cmp		Weighted, 0
					je		SumPlain
					mov		edi, A
					mov		ecx, C
					mov		edx, B
					fld		dword ptr [edi]
					fsub	dword ptr [ecx]
					fld		dword ptr [edi + 4]
					fsub	dword ptr [ecx + 4]
					fld		dword ptr [edi + 8]
					fsub	dword ptr [ecx + 8]
					fld		dword ptr [edx]
					fsub	dword ptr [ecx]
					fld		dword ptr [edx + 4]
					fsub	dword ptr [ecx + 4]
					fld		dword ptr [edx + 8]
					fsub	dword ptr [ecx + 8]
					fst		T58
					fmul	st, st(4)
					fld		st(1)
					fmul	st, st(4)
					fsubp	st(1), st
					fld		st(3)
					fmul	st, st(3)
					fld		T58
					fmul	st, st(7)
					fsubp	st(1), st
					fstp	T48
					fld		st(1)
					fmul	st, st(6)
					fld		st(5)
					fmul	st, st(4)
					fsubp	st(1), st
					fstp	T4c
					fld		st(0)
					fmul	st, st(1)
					fld		T4c
					fmul	T4c
					faddp	st(1), st
					fld		T48
					fmul	T48
					faddp	st(1), st
					fsqrt
					fstp	T20
					fstp	st(0)
					fld		T20
					fxch	st(1)
					fmul	st, st(4)
					fxch	st(2)
					fmul	st, st(5)
					faddp	st(2), st
					fld		T58
					fmul	st, st(3)
					faddp	st(2), st
					fxch	st(1)
					fpatan
					fstp	T20
					fstp	st(0)
					fstp	st(0)
					fstp	st(0)
					fld		T20
					fld		st(0)
					fmul	dword ptr [eax + 0x20]
					fld		st(1)
					fmul	dword ptr [eax + 0x24]
					fstp	T60
					fxch	st(1)
					fmul	dword ptr [eax + 0x28]
					fstp	T64
					fadd	dword ptr [esi]
					fstp	dword ptr [esi]
					fld		T60
					fadd	dword ptr [esi + 4]
					fstp	dword ptr [esi + 4]
					fld		T64
					fadd	dword ptr [esi + 8]
					jmp		SumTail
				SumPlain:
					fld		dword ptr [esi]
					fadd	dword ptr [eax + 0x20]
					fstp	dword ptr [esi]
					fld		dword ptr [esi + 4]
					fadd	dword ptr [eax + 0x24]
					fstp	dword ptr [esi + 4]
					fld		dword ptr [esi + 8]
					fadd	dword ptr [eax + 0x28]
				SumTail:
					fstp	dword ptr [esi + 8]
					}
#else
				if(Weighted)
				                    {
				                        const NxU32 corners[3] = {mRefs[F->Ref[0]].VRef,
				                            mRefs[F->Ref[1]].VRef, mRefs[F->Ref[2]].VRef};
				                        const float angle = nxSmoothNormalsAngleAtVertex(VRef,
				                            corners, reinterpret_cast<const NxVec3*>(mVertsCopy));
				                        const double x = double(angle) * F->Normal[0];
				                        const float y = float(double(angle) * F->Normal[1]);
				                        const float z = float(double(angle) * F->Normal[2]);
				                        S[0] = float(x + S[0]);
				                        S[1] = float(double(y) + S[1]);
				                        S[2] = float(double(z) + S[2]);
				                    }
				                    else
				                    {
				                        for(unsigned component = 0; component < 3; ++component)
				                            S[component] = float(double(S[component]) + F->Normal[component]);
				                    }
#endif

				Count++;
				if(mComputeNormInfo)
					mNormInfo.Add(FaceIndex);
				}

			if(mComputeNormInfo)
				{
				mNormInfo.GetEntries()[Slot] = Count;
				mNbNormInfo++;
				}

			// 0x00030b06..0x00030b5d: (z*z + y*y) + x*x; unless 0, each
			// component times 1.0f / sqrt, the component loaded first.
			float* S = Sum;
#if NX_PHYSICS_USE_X87
			__asm
				{
				mov		esi, S
				fld		dword ptr [esi + 8]
				fmul	dword ptr [esi + 8]
				fld		dword ptr [esi + 4]
				fmul	dword ptr [esi + 4]
				faddp	st(1), st
				fld		dword ptr [esi]
				fmul	dword ptr [esi]
				faddp	st(1), st
				fld		gMb2Zero
				fld		st(1)
				fucompp
				fnstsw	ax
				test	ah, 0x44
				jnp		SkipNormalize
				fsqrt
				fdivr	gMb2One
				fld		dword ptr [esi]
				fmul	st, st(1)
				fstp	dword ptr [esi]
				fld		dword ptr [esi + 4]
				fmul	st, st(1)
				fstp	dword ptr [esi + 4]
				fld		dword ptr [esi + 8]
				fmul	st, st(1)
				fstp	dword ptr [esi + 8]
			SkipNormalize:
				fstp	st(0)
				}
#else
			const double square = (double(S[2]) * S[2] + double(S[1]) * S[1])
			                + double(S[0]) * S[0];
			            if(square != 0.0)
			            {
			                const double inverse = 1.0 / std::sqrt(square);
			                for(unsigned component = 0; component < 3; ++component)
			                    S[component] = float(double(S[component]) * inverse);
			            }
#endif
			#if NX_PHYSICS_USE_X87
			nxIceContainerAddPoint(&mNormals, 0, (const NxU32*) Sum);
			#else
			nxIceContainerAddPoint(&mNormals, (const NxU32*) Sum);
			#endif
			}

		if(mVertsCopy)
			{
			if(mIndexedGeo)
				mVRefs.Add(VRef);
			else
				#if NX_PHYSICS_USE_X87
				nxIceContainerAddPoint(&mVerts, 0, (const NxU32*) &mVertsCopy[VRef]);
				#else
				nxIceContainerAddPoint(&mVerts, (const NxU32*) &mVertsCopy[VRef]);
				#endif
			}
		}

	mFacesPerRun.Add(nb_faces);
	for(NxU32 i = 0; i < nb_faces; i++)
		{
		const NxU32 Index = faces[i];
		mTopology.Add(mFaces[Index].VRef[0]);
		mTopology.Add(mFaces[Index].VRef[1]);
		mTopology.Add(mFaces[Index].VRef[2]);
		if(mFaceRemap)
			mFaceRemap[mNbFaceRemap++] = Index;
		}
	mRuns.Add(NxU32(0));
	return nb_faces;
	}

// phys_fn_001629 (0x00030d10, 127 B)
// FreeUsedRam, then the thirteen Containers destroyed (004846) from +0xc0 down
// to +0x00 -- the members' own destruction order.
MeshBuilder2::~MeshBuilder2()
	{
	FreeUsedRam();
	}

// phys_fn_001631 (0x00030d90, 441 B)
// The faces radix-sorted on the smoothing groups, then on the material (both
// unsigned), and cut into runs of equal (material, smoothing groups), each
// handed to 001627 as it closes. The three key and list blocks are plain; a
// failure releases whichever exist (smoothing, material, list) and returns
// false.
__declspec(noinline) bool MeshBuilder2::SortFaces()
	{
	NxU32* FaceList = (NxU32*) nxMb2New(mNbFaces * 4);
	NxU32* Materials = (NxU32*) nxMb2New(mNbFaces * 4);
	NxU32* Smoothing = (NxU32*) nxMb2New(mNbFaces * 4);
	if(!FaceList || !Materials || !Smoothing)
		{
		if(Smoothing)	nxMb2Free(Smoothing);
		if(Materials)	nxMb2Free(Materials);
		if(FaceList)	nxMb2Free(FaceList);
		return false;
		}

	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		Materials[i] = mFaces[i].MaterialID;
		Smoothing[i] = mFaces[i].SmoothingGroups;
		}

	IceCore::RadixSort Radix;
	const NxU32* Sorted = Radix.Sort(Smoothing, mNbFaces, IceCore::RADIX_UNSIGNED)
		.Sort(Materials, mNbFaces, IceCore::RADIX_UNSIGNED).GetRanks();

	NxU32 CurrentMaterial = Materials[Sorted[0]];
	NxU32 CurrentSmoothing = Smoothing[Sorted[0]];
	NxU32 NbInRun = 0;
	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		const NxU32 Index = Sorted[i];
		if(Materials[Index] == CurrentMaterial && Smoothing[Index] == CurrentSmoothing)
			FaceList[NbInRun++] = Index;
		else
			{
			OutputRun(FaceList, NbInRun, CurrentMaterial, CurrentSmoothing);
			CurrentSmoothing = Smoothing[Index];
			CurrentMaterial = Materials[Index];
			FaceList[0] = Index;
			NbInRun = 1;
			}
		}
	OutputRun(FaceList, NbInRun, CurrentMaterial, CurrentSmoothing);

	nxMb2Free(Smoothing);
	nxMb2Free(Materials);
	nxMb2Free(FaceList);
	return true;
	}

// phys_fn_001633 (0x00030f50, 346 B)
// phys_fn_001635 (0x000310b0, 711 B)
// phys_fn_001637 (0x00031380, 88 B)
// Build: false with no faces. The face remap (+0x100, a plain block filled
// with 0xff), then the passes 001625, 001599, 001602, 001603, 001607, 001631,
// the first failure returning false. The result points into the Containers;
// the runs are gathered per material into +0xc0 (material, faces, vertices,
// runs), a material of 0xffffffff counting as "none yet" (0x000310bf) so its
// runs are not flushed; the normal-face lists are remapped from sorted to
// output face order; the face remap is turned into the faces' AddFace indices
// and handed out only when it is not the identity.
__declspec(noinline) bool MeshBuilder2::Build(MBRESULT& result)
	{
	if(!mNbFaces)
		return false;
	mFaceRemap = (NxU32*) nxMb2New(mNbFaces * 4);
	if(!mFaceRemap)
		return false;
	memset(mFaceRemap, 0xff, mNbFaces * 4);
	mNbFaceRemap = 0;

	if(!OptimizeStreams())
		return false;
	if(!ComputeUnsharedVertices())
		return false;
	if(!ReduceReferences())
		return false;
	if(!ComputeNormals())
		return false;
	if(!SaveStreams())
		return false;
	if(!SortFaces())
		return false;

	const NxU32* FacesPerRun = mFacesPerRun.GetEntries();
	const NxU32 NbRunsCounted = mFacesPerRun.GetNbEntries();
	const NxU32 NbNormInfo = mNormInfo.GetNbEntries();
	result.Topology = mTopology.GetEntries();
	result.FacesPerRun = FacesPerRun;
	result.FaceNormals = mFaceNormals.GetEntries();
	result.Runs = mRuns.GetEntries();
	result.VRefs = mVRefs.GetEntries();
	result.Verts = mVerts.GetEntries();
	result.TRefs = mTRefs.GetEntries();
	result.TVerts = mTVerts.GetEntries();
	result.UseW = mUseW;
	result.CRefs = mCRefs.GetEntries();
	result.CVerts = mCVerts.GetEntries();
	result.Normals = mNormals.GetEntries();
	result.NormInfo = mNormInfo.GetEntries();

	const NxU32 NbRuns = mRuns.GetNbEntries() / 5;
	NxU32 CurrentMaterial = 0xffffffff;
	NxU32 MaterialFaces = 0;
	NxU32 MaterialVerts = 0;
	NxU32 MaterialRuns = 0;
	NxU32 TotalFaces = 0;
	NxU32 TotalVerts = 0;
	const NxU32* Run = mRuns.GetEntries();
	for(NxU32 k = 0; k < NbRuns; k++)
		{
		const NxU32 Material = Run[k * 5 + 0];
		if(Material != CurrentMaterial)
			{
			if(CurrentMaterial != 0xffffffff)
				{
				mMaterials.Add(CurrentMaterial);
				mMaterials.Add(MaterialFaces);
				mMaterials.Add(MaterialVerts);
				mMaterials.Add(MaterialRuns);
				MaterialRuns = 0;
				MaterialFaces = 0;
				MaterialVerts = 0;
				}
			CurrentMaterial = Material;
			}
		MaterialRuns++;
		const NxU32 Faces = FacesPerRun[k];
		const NxU32 Verts = Run[k * 5 + 3];
		MaterialFaces += Faces;
		MaterialVerts += Verts;
		TotalFaces += Faces;
		TotalVerts += Verts;
		}
	mMaterials.Add(CurrentMaterial);
	mMaterials.Add(MaterialFaces);
	mMaterials.Add(MaterialVerts);
	mMaterials.Add(MaterialRuns);

	result.NbMaterials = mMaterials.GetNbEntries() >> 2;
	result.Materials = mMaterials.GetEntries();
	result.NbFaces = TotalFaces;
	result.NbMaxFaces = mMaxNbFaces;
	result.NbSubmeshes = NbRunsCounted;
	result.NbOutVerts = TotalVerts;
	result.NbVerts = mNbVerts;
	result.NbTVerts = mNbTVerts;
	result.NbCVerts = mNbCVerts;
	result.NbNormInfo = NbNormInfo;

	NxU32* NormInfo = result.NormInfo;
	if(NormInfo)
		{
		NxU32* Inverse = (NxU32*) nxMb2New(mNbFaces * 4);
		if(!Inverse)
			return false;
		for(NxU32 i = 0; i < mNbFaceRemap; i++)
			Inverse[mFaceRemap[i]] = i;
		for(NxU32 k = 0; k < mNbNormInfo; k++)
			{
			NxU32 Nb = *NormInfo++;
			while(Nb--)
				{
				*NormInfo = Inverse[*NormInfo];
				NormInfo++;
				}
			}
		nxMb2Free(Inverse);
		}

	bool Identity = true;
	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		mFaceRemap[i] = mFaces[mFaceRemap[i]].Index;
		if(mFaceRemap[i] != i)
			Identity = false;
		}
	result.FaceRemap = Identity ? 0 : mFaceRemap;
	return true;
	}

#endif
