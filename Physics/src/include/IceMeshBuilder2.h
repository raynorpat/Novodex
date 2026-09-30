#ifndef NX_PHYSICS_ICEMESHBUILDER2_H
#define NX_PHYSICS_ICEMESHBUILDER2_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// Sub-unit C of units/convex-mesh-gap-contract.md: ICE's MeshBuilder2 (Pierre
// Terdiman), 0x0002eb50..0x000313d5, written by convex-mesh gap Task 2d in
// Physics/src/IceMeshBuilder2.cpp. Not vendored (OPCODE 1.3's Ice/ has no
// MeshBuilder2); every layout below is read off the listing sites cited.
//
// Allocation: the rows' own blocks come from the static CRT's `operator new`
// (005701, a jump to 005702, which is __nh_malloc(size, 1)) and are released by
// its `free` (005700, a jump to 005668 _free): one heap, the DLL's CRT. The
// candidate uses its own CRT's pair (nxMb2New / nxMb2Free below). The Containers
// grow through the vendored Container::Resize (the 004803 getter), and the
// vertex reduction the rows call (001647) allocates through the 004803 getter.

#include "IceMeshTools.h"

// The create block 001623 reads (0x00030420, 0x28 bytes): three stream counts
// and pointers and twelve flag bytes, copied to +0x118..+0x123. The flag names
// are the roles the rows give them.
struct MBCREATE
	{
	NxU32					NbVerts;				// +0x00 -> +0xd4
	NxU32					NbFaces;				// +0x04 -> +0xd0
	NxU32					NbTVerts;				// +0x08 -> +0xd8
	NxU32					NbCVerts;				// +0x0c -> +0xdc
	const IceMaths::Point*	Verts;					// +0x10 -> +0xec (copied)
	const IceMaths::Point*	TVerts;					// +0x14 -> +0xf0 (copied)
	const IceMaths::Point*	CVerts;					// +0x18 -> +0xf4 (copied)
	bool					KillZeroAreaFaces;		// +0x1c -> +0x118 (001597)
	bool					UseW;					// +0x1d -> +0x119 (001607, 001623, 001627)
	bool					ComputeVNormals;		// +0x1e -> +0x11a (smoothing groups: 001597, 001599, 001603, 001627)
	bool					ComputeFNormals;		// +0x1f -> +0x11b (001603)
	bool					ComputeNormInfo;		// +0x20 -> +0x11c (001627)
	bool					IndexedGeo;				// +0x21 -> +0x11d (001607, 001627)
	bool					IndexedUVW;				// +0x22 -> +0x11e (001607, 001627)
	bool					IndexedColors;			// +0x23 -> +0x11f (001607, 001627)
	bool					RelativeIndices;		// +0x24 -> +0x120 (001617)
	bool					IsSkin;					// +0x25 -> +0x121 (001599, 001625)
	bool					WeightNormalWithAngles;	// +0x26 -> +0x122 (001627)
	bool					OptimizeVertexList;		// +0x27 -> +0x123 (copied, read by none of these rows)
	};

// One face for 001597 (AddFace; 002087 builds it at 0x000524b4..0x000524e6).
struct MBFACEINFO
	{
	NxU32			Index;			// +0x00, compared with +0xd0, kept in the face at +0x2c
	NxU32			MaterialID;		// +0x04 -> face +0x18
	NxU32			SmoothingGroups;// +0x08 -> face +0x1c (only with +0x11a; else 1)
	const NxU32*	VRefs;			// +0x0c
	const NxU32*	TRefs;			// +0x10
	const NxU32*	CRefs;			// +0x14
	bool			Flip;			// +0x18: corners 1 and 2 swapped
	};

// The Build result 001633 fills (0x00030f50; 002087 zeroes 0x60 bytes).
struct MBRESULT
	{
	NxU32			NbFaces;		// +0x00 total faces over the runs
	NxU32			NbMaxFaces;		// +0x04 = +0xd0
	NxU32			NbSubmeshes;	// +0x08 = +0x10's count (runs)
	const NxU32*	Topology;		// +0x0c = +0x00's entries
	const NxU32*	FacesPerRun;	// +0x10 = +0x10's entries
	const NxU32*	FaceNormals;	// +0x14 = +0x90's entries
	const NxU32*	Runs;			// +0x18 = +0xb0's entries (5 words per run)
	const NxU32*	FaceRemap;		// +0x1c = +0x100, or null when it is the identity
	NxU32			NbVerts;		// +0x20 = +0xd4
	NxU32			NbTVerts;		// +0x24 = +0xd8
	NxU32			NbCVerts;		// +0x28 = +0xdc
	NxU32			NbOutVerts;		// +0x2c total new vertices over the runs
	const NxU32*	VRefs;			// +0x30 = +0x20's entries
	const NxU32*	TRefs;			// +0x34 = +0x30's entries
	const NxU32*	CRefs;			// +0x38 = +0x40's entries
	const NxU32*	Verts;			// +0x3c = +0x50's entries
	const NxU32*	TVerts;			// +0x40 = +0x60's entries
	const NxU32*	CVerts;			// +0x44 = +0x70's entries
	const NxU32*	Normals;		// +0x48 = +0x80's entries
	NxU32			NbNormInfo;		// +0x4c = +0xa0's count
	NxU32*			NormInfo;		// +0x50 = +0xa0's entries (remapped in place)
	bool			UseW;			// +0x54 = +0x119
	NxU32			NbMaterials;	// +0x58 = +0xc0's count / 4
	const NxU32*	Materials;		// +0x5c = +0xc0's entries (4 words per material)
	};

// A face record (0x30 bytes; 001623 allocates them as new[] with a cookie).
struct MBFace
	{
	NxU32		VRef[3];		// +0x00 output vertex per corner (001609)
	NxU32		Ref[3];			// +0x0c reference records (+0xfc) per corner
	NxU32		MaterialID;		// +0x18
	NxU32		SmoothingGroups;// +0x1c (0xffffffff once 001599 has unshared it)
	float		Normal[3];		// +0x20 (001603)
	NxU32		Index;			// +0x2c the AddFace index
	};

// A reference record (12 bytes): vertex, uvw and colour indices of one corner.
struct MBRef
	{
	NxU32		VRef;
	NxU32		TRef;
	NxU32		CRef;
	};

// 001609's context, on 001617's stack (esp+0x14..+0x2b).
struct MBRemapContext
	{
	const MBRef*		Refs;		// +0x00 = +0xfc
	MBFace*				Faces;		// +0x04 = +0xf8
	NxU32*				Remap;		// +0x08 per reference, 0xffffffff when not yet output
	NxU32				SmoothingGroups;	// +0x0c the current face's
	NxU32				Face;		// +0x10 the current face
	IceCore::Container*	Out;		// +0x14 4 words per new vertex
	};

// MeshBuilder2 (0x124 bytes; 001593).
class MeshBuilder2
	{
	public:
				MeshBuilder2();
				~MeshBuilder2();

	bool		Init(const MBCREATE& create);
	bool		AddFace(const MBFACEINFO& face);
	bool		Build(MBRESULT& result);
	MeshBuilder2&	FreeUsedRam();

	bool		ComputeUnsharedVertices();
	bool		ReduceReferences();
	bool		ComputeNormals();
	bool		SaveStreams();
	bool		OptimizeStream(NxU32* nb, IceMaths::Point** stream, NxU32 which);
	bool		OptimizeStreams();
	NxU32		RemapFaces(const NxU32* faces, NxU32 nb_faces, IceCore::Container& out);
	NxU32		OutputRun(const NxU32* faces, NxU32 nb_faces, NxU32 material, NxU32 smoothing);
	bool		SortFaces();

	IceCore::Container	mTopology;		// +0x00
	IceCore::Container	mFacesPerRun;	// +0x10
	IceCore::Container	mVRefs;			// +0x20
	IceCore::Container	mTRefs;			// +0x30
	IceCore::Container	mCRefs;			// +0x40
	IceCore::Container	mVerts;			// +0x50
	IceCore::Container	mTVerts;		// +0x60
	IceCore::Container	mCVerts;		// +0x70
	IceCore::Container	mNormals;		// +0x80
	IceCore::Container	mFaceNormals;	// +0x90
	IceCore::Container	mNormInfo;		// +0xa0
	IceCore::Container	mRuns;			// +0xb0
	IceCore::Container	mMaterials;		// +0xc0
	NxU32				mMaxNbFaces;	// +0xd0
	NxU32				mNbVerts;		// +0xd4
	NxU32				mNbTVerts;		// +0xd8
	NxU32				mNbCVerts;		// +0xdc
	NxU32				mNbFaces;		// +0xe0
	NxU32				mNbRefs;		// +0xe4
	NxU32				mNbOutVerts;	// +0xe8 001617's running vertex counter
	IceMaths::Point*	mVertsCopy;		// +0xec new[] with a cookie
	IceMaths::Point*	mTVertsCopy;	// +0xf0 new[] with a cookie
	IceMaths::Point*	mCVertsCopy;	// +0xf4 new[] with a cookie
	MBFace*				mFaces;			// +0xf8 new[] with a cookie
	MBRef*				mRefs;			// +0xfc plain block
	NxU32*				mFaceRemap;		// +0x100 plain block
	NxU32				mNbFaceRemap;	// +0x104
	NxU32*				mVertFaceCount;	// +0x108 plain block
	NxU32*				mVertFaceOffset;// +0x10c plain block
	NxU32*				mVertFaceList;	// +0x110 plain block
	NxU32				mNbNormInfo;	// +0x114
	bool				mKillZeroAreaFaces;		// +0x118
	bool				mUseW;					// +0x119
	bool				mComputeVNormals;		// +0x11a
	bool				mComputeFNormals;		// +0x11b
	bool				mComputeNormInfo;		// +0x11c
	bool				mIndexedGeo;			// +0x11d
	bool				mIndexedUVW;			// +0x11e
	bool				mIndexedColors;			// +0x11f
	bool				mRelativeIndices;		// +0x120
	bool				mIsSkin;				// +0x121
	bool				mWeightNormalWithAngles;// +0x122
	bool				mOptimizeVertexList;	// +0x123
	};

// 001591: three dwords appended to a Container (ICE's inline Add(const Point&),
// a function of its own in the oracle, thiscall on the Container with `ret 4`;
// here __fastcall with the Container in ecx and an unused edx, which gives the
// same registers and the same callee-popped argument).
IceCore::Container&	__fastcall nxIceContainerAddPoint(IceCore::Container* container, NxU32 edx,
						const NxU32* point);

// 001595: a copy of a 12-byte-element stream (see the row).
bool				nxMb2DuplicateStream(NxU32 nb, const IceMaths::Point* src, IceMaths::Point** dst,
						NxU32* dstNb);

// 001609 (cdecl, 4 arguments; 001617 calls it once per corner).
void				nxMb2RemapCorner(MBRemapContext* context, NxU32 ref, NxU32* counter, NxU32 corner);

#endif
