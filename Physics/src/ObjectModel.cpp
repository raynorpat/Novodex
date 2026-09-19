/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ObjectModel.h"
#include "Containers.h"
#include "NxIntersectionSegmentBox.h"

#include <math.h>
#include <string.h>

// The scene guard pair runs real Foundation-side primitives -- the image
// reaches them through kernel32 import slots (0x10104010/14/2c/44).
#define NOMINMAX
#include <windows.h>

// phys_fn_002404 (0x0005ba70) is the shared member constructor; the oracle's
// collision-object ctor calls it at 0x000247d7 and then overwrites the vptr
// with the container's final table. The transcription constructs the member
// directly and lets C++ install this container's vptr for +0x00, which lands
// in the same post-construction state without replaying the intermediate
// base-vtable stores.
CollisionObject::CollisionObject(void* argument)
	{
	mWord04 = 0;							// 0x000247cb
	mArgument08 = argument;					// 0x000247e9
	mArgument18 = argument;					// 0x000247e6
	}

// phys_fn_001281 (0x000257a0): mov eax,[ecx+4]; ret. The whole row -- note
// the offset is FOUR bytes past the shape's vptr.
const void* nxShapeOwner(const void* shape)
	{
	return *reinterpret_cast<const void* const*>(
		static_cast<const unsigned char*>(const_cast<void*>(shape)) + 4);
	}

// ---------------------------------------------------------------------------
// The box hull facade. The static tables' CONTENTS are transcribed from the
// pinned image (.rdata 0x10122180/0x101221e0/0x10122240); this binary's
// copies live at different addresses, which is fine -- consumers get them
// through these getters, and the layout gate compares content, not pointers.

static const NxU32 gEdgeTable[12] =
	{ 0, 1, 1, 2, 2, 3, 3, 0, 7, 6, 6, 5 };			// 0x10122180..
static const NxU32 gFaceCornerTable[12] =
	{ 131073, 0, 131073, 2, 131073, 4, 131073, 6, 131073, 8, 131073, 10 };	// 0x101221e0..
static const NxU32 gAdjacencyTable[12] =
	{ 0, 5, 0, 1, 0, 4, 0, 3, 2, 4, 1, 2 };			// 0x10122240..

const NxU32* BoxHullFacade::vertices() const
	{
	return mVertices;							// lea eax,[ecx+0x10]
	}

const BoxFaceRecord* BoxHullFacade::face(unsigned index) const
	{
	// lea eax,[eax+eax*8]; lea eax,[ecx+eax*4+0x70] -- index*36 + (this+0x70).
	return &mFaces[index];
	}

const NxU32* BoxHullFacade::edgeTable()
	{
	return gEdgeTable;
	}

const NxU32* BoxHullFacade::faceCornerTable()
	{
	return gFaceCornerTable;
	}

const NxU32* BoxHullFacade::adjacencyTable()
	{
	return gAdjacencyTable;
	}

// ---------------------------------------------------------------------------
// The error stream. Evidence section 3t: the image reports through an
// indirect cdecl five-argument call guarded by a non-zero flag word; this
// reconstruction mirrors the shape with its own slot so differentials can
// capture reports on both sides.

const char* const	nxSourceFileSphereShapeCpp =
	"\\Epic\\Novodex\\SDKs\\Physics\\src\\SphereShape.cpp";
const char* const	nxMsgSetRadiusPositive =
	"SphereShape::setRadius: radius should be positive!";
const char* const	nxSourceFileShapeCpp =
	"\\Epic\\Novodex\\SDKs\\Physics\\src\\Shape.cpp";
const char* const	nxMsgGroupBelow32 =
	"group ID must be < 32!";
const char* const	nxSourceFileSphereLoadCpp =
	"\\Epic\\Novodex\\SDKs\\Physics\\src\\SphereShape.cpp";
const char* const	nxMsgSphereLoadRadius =
	"SphereShape::loadFromDesc: radius should be positive!";
const char* const	nxSourceFileCapsuleShapeCpp =
	"\\Epic\\Novodex\\SDKs\\Physics\\src\\CapsuleShape.cpp";
const char* const	nxMsgCapsuleLoadRadius =
	"CapsuleShape::loadFromDesc: radius should be positive!";

// ---------------------------------------------------------------------------
// NxMaterialRecord. Evidence: phase2-sdk.md section 4.6 and section 3v here.
// setToDefault matches the pinned header inline exactly; the internal flag
// bit is the store at 0x0000e9ee that runs on the TEMPLATE after the copy
// into the SDK's materials array.
NxMaterialRecord::NxMaterialRecord(void)
	{
	setToDefault();
	}

void NxMaterialRecord::setToDefault(void)
	{
	dynamicFriction = 0.0f;
	staticFriction = 0.0f;
	spinFriction = 0.0f;
	rollFriction = 0.0f;
	restitution = 0.0f;
	dynamicFrictionV = 0.0f;
	staticFrictionV = 0.0f;
	dirOfAnisotropy.set(1.0f, 0.0f, 0.0f);
	dirOfMotion.set(1.0f, 0.0f, 0.0f);
	speedOfMotion = 0.0f;
	flags = 0;
	frictionCombineMode = 0;			// NX_CM_AVERAGE
	restitutionCombineMode = 0;			// NX_CM_AVERAGE
	programData = nullptr;
	}

void NxMaterialRecord::setInternalFlagBit31(void)
	{
	flags |= 0x80000000u;				// or edi,0x80000000 at 0x0000e9ee
	}

static NxReportFn	gReportSink = nullptr;

void nxInstallReportSink(NxReportFn sink)
	{
	gReportSink = sink;
	}

void nxReport(int kind, const char* file, int line, int code,
	const char* message)
	{
	if(gReportSink != nullptr)
		gReportSink(kind, file, line, code, message);
	}


// phys_fn_000975 (0x000217c0). The three per-corner values are column dots
// plus translation: A = col0.v + tx, B = col1.v + ty, C = col2.v + tz; the
// projection combines them as A*dx + (C*dz + B*dy) -- that exact association,
// because the x87 stack built C first and folded B before A. Bounds updates
// reproduce fcom/fnstsw exactly: the minimum replaces on strictly less, the
// maximum on strictly greater, and a NaN projection replaces neither (the
// unordered case sets C0, which both masks include). Intermediates are kept
// in double so a spilled value cannot truncate what the x87 stack held at
// 64-bit. The row reads neither of its two unread stack arguments.
void BoxHullFacade::supportBounds(const float* direction, float* outMin,
	float* outMax, const float* pose) const
	{
	NxU32 minBits = 0x7f7fffffu;				// +FLT_MAX: the minimum starts high (store 0x000217d7 -> a2)
	NxU32 maxBits = 0xff7fffffu;				// -FLT_MAX: the maximum starts low (store 0x000217dd -> a3)
	float minValue;
	float maxValue;
	memcpy(&minValue, &minBits, sizeof(minValue));
	memcpy(&maxValue, &maxBits, sizeof(maxValue));
	for(unsigned i = 0; i < 8; ++i)				// mov ebp,8 at 0x000217e3
		{
		const float* v = reinterpret_cast<const float*>(&mVertices[i * 3]);
		double a = (double)v[0] * pose[0]
			+ (double)v[1] * pose[4]
			+ (double)v[2] * pose[8]
			+ pose[12];
		double b = (double)v[1] * pose[5]
			+ (double)v[0] * pose[1]
			+ (double)v[2] * pose[9]
			+ pose[13];
		double c = pose[10] * v[2]
			+ pose[2] * v[0]
			+ pose[6] * v[1]
			+ pose[14];
		double projection = a * direction[0]
			+ (c * direction[2] + b * direction[1]);
		float asFloat = (float) projection;
		if(asFloat < minValue)
			minValue = asFloat;
		if(asFloat > maxValue)
			maxValue = asFloat;
		}
	memcpy(outMin, &minValue, sizeof(minValue));
	memcpy(outMax, &maxValue, sizeof(maxValue));
	}

// phys_fn_000985 (0x00021a10). The image's initializer array at .rdata
// 0x10103010 is twelve `ret` stubs, so the CRT helper runs nothing and the
// twelve-byte global stays zeroed. Same observable state here: a static
// all-zero object behind a once-flag.
const void* BoxHullFacade::sharedHook()
	{
	static NxU32 shared[3] = { 0, 0, 0 };	// .data 0x10123c64..0x10123c70
	static bool initialised = false;		// the guard byte at .data 0x10123c70
	if(!initialised)
		initialised = true;
	return shared;
	}

// ---------------------------------------------------------------------------
// The Prunable owner adapters. The base-shape constructor installs their
// addresses into .data 0x10128470/74/78 (stores 0x0002562b..3f); Phase 4 had
// to leave the globals null because their only writer sat outside its
// population. All three are cdecl, owner first, and ignore nothing they are
// handed -- the shipped rows carry no null tests.

// phys_fn_000965 (0x000213e0): `xor eax,eax; ret` -- the same folded stub the
// facade exposes as kZero, installed as .data 0x10128470.
static udword shapeOwnerQuery(void* /*owner*/)
	{
	return 0;
	}

// phys_fn_001271 (0x00025520, 15 bytes): reads [owner], pushes the box, calls
// vtable slot 10 (+0x28). Installed at .data 0x10128474.
static void shapeOwnerNotify(void* owner, AABB* box)
	{
	void** vtable = *reinterpret_cast<void***>(owner);
	typedef void (__thiscall* NotifyFn)(void*, AABB*);
	reinterpret_cast<NotifyFn>(vtable[10])(owner, box);
	}

// phys_fn_001269 (0x00025510, 15 bytes): the same frame through slot 9
// (+0x24) -- the owner recomputing the box. Installed at .data 0x10128478.
static void shapeOwnerWorldAABB(void* owner, AABB* box)
	{
	void** vtable = *reinterpret_cast<void***>(owner);
	typedef void (__thiscall* WorldAABBFn)(void*, AABB*);
	reinterpret_cast<WorldAABBFn>(vtable[9])(owner, box);
	}

// ---------------------------------------------------------------------------
// ShapeBase. See ObjectModel.h for the row map; every store below carries the
// instruction address that fixes it.

ShapeBase::ShapeBase(void* owner, unsigned argument)
	{
	mOwner04 = owner;						// 0x00025543
	mWord08 = 0;							// 0x00025549

	const NxU32 one = 0x3f800000u;			// mov ebx,0x3f800000 at 0x00025535
	ShapePose identity;
	memset(&identity, 0, sizeof(identity));
	identity.mRotation[0] = one;			// stores like 0x00025555
	identity.mRotation[4] = one;
	identity.mRotation[8] = one;
	mPose0C = identity;						// 0x00025555..0x000255cd
	mPose3C = identity;						// second instruction run, +0x3c
	mPose6C = identity;						// third instruction run, +0x6c
	// The image runs the identical pose stores a SECOND time (0x0002564f..
	// 0x000256cd) after the hook stores; both passes write the same bytes, so
	// the transcription writes them once.

	mWord9C = 0;							// 0x000255d3
	mWordA0 = 0;							// 0x000255d9

	// mPrunable is constructed here by member semantics; the image reaches
	// Prunable::Prunable (phys_fn_004874) at 0x000255df.

	gPrunableOwnerQuery = shapeOwnerQuery;		// .data 0x10128470, store 0x0002563f
	gPrunableOwnerNotify = shapeOwnerNotify;	// .data 0x10128474, store 0x00025635
	gPrunableOwnerWorldAABB = shapeOwnerWorldAABB;	// 0x10128478, store 0x0002562b

	// Then the constructor registers THIS SHAPE as the prunable's owner.
	mPrunable.mOwner = this;				// mov [esi+0xa8],esi at 0x00025649

	mSentinelD0 = 0x7fffffffu;				// 0x000255ed
	mArgumentD4 = argument;					// 0x000255f7 ([esp+0x14]: entry [esp+8])
	mHalfwordD8 = 0;						// 0x000255fd
	mHalfwordDA = 0;						// 0x00025604
	mHalfwordDC = 6;						// 0x0002560b
	mHalfwordDE = 8;						// 0x00025614

	// The owner-registration arm (0x0002561f..26): when the owner is non-null,
	// [owner+4]'s word at +0x48 becomes the `this` for phys_fn_002423 with the
	// shape pushed. Transcribed through nxSceneInsertShape, which reproduces
	// the registrar's three observable writes (evidence 3y/3z).
	if(mOwner04 != nullptr)
		{
		NxU32 scene = *reinterpret_cast<const NxU32*>(
			reinterpret_cast<const unsigned char*>(mOwner04) + 4);
		void* container = *reinterpret_cast<void* const*>(
			reinterpret_cast<const unsigned char*>(scene) + 0x48);
		nxSceneInsertShape(container, this, mArgumentD4);
		}
	}

// phys_fn_001287 (0x000257d0, 14 bytes): the +0xde halfword under a mask.
unsigned ShapeBase::nxFlagBitsDE(unsigned mask) const
	{
	return static_cast<unsigned>(mHalfwordDE) & mask;
	}

// phys_fn_000931 (0x00020490, 70 bytes): translation, forward rotation
// copy, THEN dimensions. Word-wise volatile accesses retain the image's
// load/store order when out aliases this (including forward rep movsd).
// A bulk memcpy/memmove or dimensions-first copy is not equivalent.
void BoxShape::nxFillShapeDescriptor(unsigned* out) const
	{
	const volatile unsigned* self =
		reinterpret_cast<const volatile unsigned*>(this);
	volatile unsigned* dest = out;
	for(unsigned i = 0; i < 3; ++i)
		dest[i] = self[0x30 / 4 + i];		// 0x20496..a4
	for(unsigned i = 0; i < 9; ++i)
		dest[6 + i] = self[0x0c / 4 + i];	// 0x204a9..b4
	for(unsigned i = 0; i < 3; ++i)
		dest[3 + i] = self[0xe4 / 4 + i];	// 0x204b6..cf
	}

// Provisional phys_fn_000949 (BOX slot 5): the local-space raycast wrapper.
// Store order per the listing: point z/x/y (0x20a10/17/1a) and t (0x20a1d)
// BEFORE the distance gate (0x20a2f); acceptance then writes colobj, the
// three zero words and tag 0x13, and the flags&4 arm overwrites tag 0x17
// and the normal z/x/y (0x20b05..0x20b0b). Driven coverage must precede
// census closure.
void* BoxShape::nxBoxRaycast(const float* ray, float maxDistance,
	unsigned reserved, unsigned flags, void* hit) const
	{
	(void) reserved;
	const float* rot = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
	const float* trn = reinterpret_cast<const float*>(&mBase.mPose0C.mTranslation);
	const float dx = ray[0] - trn[0];			// 0x00020887..8c
	const float dy = ray[1] - trn[1];			// 0x0002088f..92
	const float dz = ray[2] - trn[2];			// 0x00020899..9c
	// R^T per column with the image association (dz*R2c + dy*R1c) + dx*R0c.
	const float lo[3] =
		{
		dz * rot[6] + dy * rot[3] + dx * rot[0],
		dz * rot[7] + dy * rot[4] + dx * rot[1],
		dz * rot[8] + dy * rot[5] + dx * rot[2]
		};
	const float ld[3] =
		{
		ray[5] * rot[6] + ray[4] * rot[3] + ray[3] * rot[0],
		ray[5] * rot[7] + ray[4] * rot[4] + ray[3] * rot[1],
		ray[5] * rot[8] + ray[4] * rot[5] + ray[3] * rot[2]
		};
	const NxVec3 negDims(-mHull.mDims04[0], -mHull.mDims04[1], -mHull.mDims04[2]);
	const NxVec3 posDims(mHull.mDims04[0], mHull.mDims04[1], mHull.mDims04[2]);
	NxVec3 coord(0.0f, 0.0f, 0.0f);
	NxReal t = 0.0f;
	// 0x0002098c: (negDims, posDims, localOrigin, localDirection, coord, t).
	const NxU32 plane = NxRayAABBIntersect2(negDims, posDims,
		*reinterpret_cast<const NxVec3*>(lo),
		*reinterpret_cast<const NxVec3*>(ld), coord, t);
	if(plane == 0)
		return nullptr;							// 0x00020998

	unsigned char* rec = static_cast<unsigned char*>(hit);
	// Point z/x/y then t, all BEFORE the gate: 0x00020a10/17/1a, 0x20a1d.
	// World transform = R·coord + t, rows of the row-major R (listing
	// 0x2099e..0x20a03: worldX=row0, worldY=row1, worldZ=row2), not the
	// transposed form. The transposed indices passed every axis-aligned
	// permutation and were caught by the quarter turn.
	const float wz = coord.x * rot[6] + coord.y * rot[7] + coord.z * rot[8]
		+ trn[2];								// row2 dot, 0x209d8..0x20a03
	const float wx = coord.x * rot[0] + coord.y * rot[1] + coord.z * rot[2]
		+ trn[0];
	const float wy = coord.x * rot[3] + coord.y * rot[4] + coord.z * rot[5]
		+ trn[1];
	memcpy(rec + 0x0c, &wz, 4);
	memcpy(rec + 0x04, &wx, 4);
	memcpy(rec + 0x08, &wy, 4);
	memcpy(rec + 0x20, &t, 4);

	if(t > maxDistance) // 0x20a20..2d: reject ordered greater; accept unordered
		return nullptr;							// 0x00020a2f

	memcpy(rec + 0x00, &mBase.mWord9C, 4);		// 0x00020a38..3e
	const unsigned zero = 0;
	memcpy(rec + 0x1c, &zero, 4);				// 0x00020a45
	memcpy(rec + 0x24, &zero, 4);				// 0x00020a4c
	memcpy(rec + 0x28, &zero, 4);				// 0x00020a53
	unsigned tag = 0x13;						// 0x00020a5a
	memcpy(rec + 0x2c, &tag, 4);
	if(flags & 4)								// 0x00020a40, mask 0x04
		{
		tag = 0x17;								// 0x00020a72
		memcpy(rec + 0x2c, &tag, 4);
		float n[3] = { 0.0f, 0.0f, 0.0f };
		n[plane - 1] = coord[plane - 1] < 0.0f ? -1.0f : 1.0f;
		const float nz = n[0] * rot[6] + n[1] * rot[7] + n[2] * rot[8];
		const float nx = n[0] * rot[0] + n[1] * rot[1] + n[2] * rot[2];
		const float ny = n[0] * rot[3] + n[1] * rot[4] + n[2] * rot[5];
		memcpy(rec + 0x18, &nz, 4);				// 0x00020b05
		memcpy(rec + 0x10, &nx, 4);				// 0x00020b08
		memcpy(rec + 0x14, &ny, 4);				// 0x00020b0b
		}
	return const_cast<BoxShape*>(this);		// 0x00020b0e: eax = this
	}

// phys_fn_000951 (BOX slot 7) -- swept-AABB entry parameter. PROVISIONAL:
// transcribed per the 3z36 verified model (out0 = min over valid axes of
// |col_k . H| / swept[k]), NOT yet differentially closed. phys_fn_000951
// stays `discovered`.
bool BoxShape::nxBoxSweep(void* out, const float* swept) const
	{
	const NxF32* R = reinterpret_cast<const NxF32*>(mBase.mPose0C.mRotation);
	const NxF32* H = mHull.mDims04;
	float tmin = 3.4028235e38f;
	bool any = false;
	for(unsigned k = 0; k < 3; ++k)
		{
		// x87 stores the face projection to m32 (fstp), so the arithmetic
		// is single-precision: (Rk0*H0 + Rk1*H1 + Rk2*H2) then /swept[k].
		const float colDotH = R[3*k+0]*H[0] + R[3*k+1]*H[1] + R[3*k+2]*H[2];
		const float s = swept[k];
		// Valid swept: non-zero normal finite value (3z36). Degenerate
		// (0/subnormal/FLT_MAX here) -> axis rejected.
		unsigned sw; memcpy(&sw, &s, 4);
		if(sw == 0 || sw == 0x7f7fffffu || sw == 0xff7fffffu)
			continue;
		// Entry parameter uses the swept MAGNITUDE (negative swept still
		// gives a positive entry) with the reciprocal-forced rounding:
		// |col_k.H| * |1.0/swept[k]|. Pin: sh0/p0/sw{-3,-2,-1} -> 0.5.
		volatile float recip = 1.0f / (s < 0.0f ? -s : s);
		const float t = (colDotH < 0.0f ? -colDotH : colDotH) * recip;
		any = true;
		if(t < tmin)
			tmin = t;
		}
	if(!any)
		return false;
	*static_cast<float*>(out) = tmin;
	return true;
	}

// Candidate guard bindings: the candidate reads the SAME live storage the
// drive mutates (oracle .data via the rendercap driver), not its own copy.
static float* g_nxGuardA = nullptr;		// 0x10123bc8
static float* g_nxGuardB = nullptr;		// 0x10123bd8
static float* g_nxRenderScale = nullptr;	// 0x10123b4c
static float* g_nxGuardRef = nullptr;	// 0x101041f0 (SDK-side reference)

// Provisional phys_fn_001305 (0x25960): conditional debug render, `ret 4`.
// Both blocks SKIP when their guard equals the reference word (fucompp
// equality -> jnp taken at 0x2597f/0x25aa6); they execute when the guard
// differs or is unordered. Block 1 (guard A) draws three axis lines from
// the pose-one translation to K*scaled-column endpoints, colors
// 0xCF0000/0xCF00/0xCF, K = scale * guard A (0x25985..0x2598e). Block 2
// (guard B) draws three circles: slot-10 center+diagonal, the 9 rotation
// words, then per call a column-cyclically rotated 3x3 in a 48-byte pose
// buffer, args (0x14, pose, 0xFFFF00FF, diagBits, 0) at vtable +0x38.
// Driven by the 5-mask rendercap differential; census closure awaits a
// family registration.
void BoxShape::nxDebugRender(const void* renderer) const
	{
	if(!g_nxGuardRef || !renderer)
		return;
	const float ref = *g_nxGuardRef;
	void** table = *reinterpret_cast<void** const*>(renderer);
	typedef void (__fastcall* NxDrawLineFn)(void*, void*, const float*,
		const float*, unsigned);
	typedef void (__fastcall* NxDrawPoseFn)(void*, void*, unsigned count,
		const void* pose, unsigned color, unsigned radiusBits, unsigned reserved);
	typedef NxDrawLineFn NxDrawLine;
	typedef NxDrawPoseFn NxDrawPose;
	const NxDrawLine drawLine = reinterpret_cast<NxDrawLine>(table[8]);
	const NxDrawPose drawPose = reinterpret_cast<NxDrawPose>(table[14]);
	void* rendererArg = const_cast<void*>(renderer);

	// Block 1 (0x25985..0x25a90): scaled axis lines, guard A.
	const float guardA = *g_nxGuardA;
	if(guardA != ref || guardA != guardA)
		{
		const float k = *g_nxRenderScale * guardA;	// 0x2598e
		const float* rot = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
		const float* trn = reinterpret_cast<const float*>(&mBase.mPose0C.mTranslation);
		static const unsigned axisColors[3] = { 0xcf0000u, 0xcf00u, 0xcfu };
		for(unsigned axis = 0; axis < 3; ++axis)
			{
			const float start[3] = { trn[0], trn[1], trn[2] };
			// 0x25a0e..0x25a28: endpoint = t + K * column(axis), scaled ONCE
			// (each element fmul'd by st(2)=K exactly one time in the chain).
			const float end[3] =
				{
				k * rot[axis] + trn[0],
				k * rot[3 + axis] + trn[1],
				k * rot[6 + axis] + trn[2]
				};
			drawLine(rendererArg, nullptr, start, end, axisColors[axis]);
			}
		}

	// Block 2 (0x25aac..0x25c01): slot-10 center+diagonal, 9 rotation
	// words, three column-cyclic pose draws.
	const float guardB = *g_nxGuardB;
	if(guardB != ref || guardB != guardB)
		{
		float center[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		nxBoxCenterAndDiagonal(center);
		const float* rot = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
		float pose[12];
		for(unsigned i = 0; i < 9; ++i)
			pose[i] = rot[i];
		pose[9] = center[0];
		pose[10] = center[1];
		pose[11] = center[2];
		for(unsigned round = 0; round < 3; ++round)
			{
			drawPose(rendererArg, nullptr, 0x14, pose, 0xffff00ffu,
				reinterpret_cast<const unsigned&>(center[3]), 0);
			if(round < 2)
				{
				// Column-cyclic rotation of the 9 matrix words (0x25b40..0x25c01):
				// each row rotates left by one, which cycles the COLUMNS of
				// the frame between calls.
				float next[9];
				for(unsigned row = 0; row < 3; ++row)
					for(unsigned col = 0; col < 3; ++col)
						next[row * 3 + col] = pose[row * 3 + (col + 1) % 3];
				memcpy(pose, next, 9 * sizeof(float));
				}
			}
		}
	}

void nxBindDebugRenderGuards(float* guardA, float* guardB,
	float* renderScale, float* guardRef)
	{
	g_nxGuardA = guardA;
	g_nxGuardB = guardB;
	g_nxRenderScale = renderScale;
	g_nxGuardRef = guardRef;
	}

static float* g_nxGuardC = nullptr;		// 0x10123bc4 (slot-3 guard)

void nxBindDebugRenderGuardC(float* guardC)
	{
	g_nxGuardC = guardC;
	}

// Provisional phys_fn_000945 (0x207e0, 104 B): debug-render dispatcher,
// `ret 4`. Stage order and gates per build/slot3-full.txt:
// 0x207e4  gate: word[+0xde] & 8 (call 0x257d0 with mask 8), zero exits;
// 0x207f9  call 001305 (nxDebugRender) unconditionally after the gate;
// 0x207fe  guard C (0x123bc4) vs ref (0x1041f0): equality skips, unordered
//          executes (test ah,0x44; jnp skip -- same pattern as 001305);
// 0x2081a  nxFillShapeDescriptor fills a 60-byte local;
// 0x2081f..39  color = ((+0xde & 7) == 0 ? 0xffffffff : 0xffff00ff) --
//          `and al,7; neg al; sbb eax,eax; and eax,0xffff0100; dec eax`;
// 0x2083d  renderer vtable slot +0x28 with (descriptor, color, 0).
// Driven by the 64-case slot3cap differential (3z28); census closure
// awaits a family registration.
void BoxShape::nxDebugRenderDispatch(const void* renderer) const
	{
	if(!mBase.nxFlagBitsDE(8))
		return;
	nxDebugRender(renderer);
	if(!g_nxGuardC || !g_nxGuardRef || !renderer)
		return;
	const float ref = *g_nxGuardRef;
	const float guardC = *g_nxGuardC;
	if(guardC == ref && guardC == guardC)
		return;
	unsigned descriptor[15];
	nxFillShapeDescriptor(descriptor);
	void** table = *reinterpret_cast<void** const*>(renderer);
	typedef void (__fastcall* NxDrawShapeFn)(void*, void*, const unsigned*,
		unsigned, unsigned);
	const NxDrawShapeFn drawShape = reinterpret_cast<NxDrawShapeFn>(table[10]);
	// mov al,[esi+0xde] addresses the flag halfword directly (the +0xde
	// byte offset, not an aligned dword index); nxFlagBitsDE reads it.
	const unsigned lowBits = mBase.nxFlagBitsDE(7);
	const unsigned color = lowBits == 0 ? 0xffffffffu : 0xffff00ffu;
	drawShape(const_cast<void*>(renderer), nullptr, descriptor, color, 0);
	}

// Task 4 scaffolding: the scene shape-array insert. Write order follows the
// image -- registrar's shape store first (0x5c5a4), then the notify helper's
// free-list sentinel (0x5c093) and count mirror (0x5c0a8). Slot must be
// within every pre-sized vector's count: growth is not modelled.
void nxSceneInsertShape(void* container, void* shape, NxU32 slot)
	{
	unsigned self = reinterpret_cast<unsigned>(shape);
	unsigned c = reinterpret_cast<unsigned>(container);

	unsigned* shapesBegin = *reinterpret_cast<unsigned**>(c + 0x90);
	shapesBegin[slot] = self;					// [begin+slot*4] = shape

	unsigned* sentBegin = *reinterpret_cast<unsigned**>(c + 0x00);
	sentBegin[slot] = 0xFFFFFFFFu;				// free-list sentinel

	unsigned* bBegin = *reinterpret_cast<unsigned**>(c + 0x10);
	unsigned* bEnd = *reinterpret_cast<unsigned**>(c + 0x14);
	unsigned countB = static_cast<NxU32>(
		(reinterpret_cast<unsigned>(bEnd) - reinterpret_cast<unsigned>(bBegin)) >> 2);
	unsigned* cBegin = *reinterpret_cast<unsigned**>(c + 0x20);
	cBegin[slot] = countB;						// count mirror
	}

// The base dtor's owner arms, image order (0x00026be1..0x00026c35).
void ShapeBase::nxBaseDtorOwnerArms(void)
	{
	if(mOwner04 == nullptr)
		return;
	unsigned scene = *reinterpret_cast<const NxU32*>(
		reinterpret_cast<const unsigned char*>(mOwner04) + 4);
	unsigned char* sc = reinterpret_cast<unsigned char*>(scene);
	*reinterpret_cast<NxU32*>(sc + 0x70c) |= 2;

	void* c1 = *reinterpret_cast<void* const*>(sc + 0x48);
	nxSceneRemoveShape(c1, this);

	// 0x00026c0f..19: ecx = scene + 0x5d4 -- the ADDRESS OF THE FIELD, so
	// remover #2's [this] dereference lands on the field's {begin,end}
	// header. Not the field's value.
	nxSceneRemovePairs(reinterpret_cast<unsigned char*>(scene) + 0x5d4, this);

	void* c3 = *reinterpret_cast<void* const*>(sc + 0x6e4);
	nxSceneSlotFree(c3, mArgumentD4);
	}

// Task 4 scaffolding: remover chain, decoded in full this round.
//
// phys_fn_002410 (0x5bac0) is the release arm, and both guards are
// narrower than they look: sentinel != -1 gates ONLY the free-vector push,
// and sentinel == 0 gates only the unlink -- so a virgin index (-1) skips
// the push but still runs the unlink against whatever the mirror word
// names, and an already-released index pushes a duplicate while skipping
// the unlink. The push itself is the same growth arm phys_fn_000028
// carries, inlined (malloc/copy/free through adapter slots +8/+0x14 when
// the cursor sits at capacity). The unlink swaps the count vector's last
// value into the released slot's mirror position across the +0x10 counts /
// +0x14 end cursor / +0x20 mirrors arrays, pops the cursor, zeroes the
// sentinel and poisons the mirror word with 0xD00BEED0. The shapes-array
// clear its callers perform lives in 0x5bbe0 and stays in
// nxSceneRemoveShape below.
void nxSceneReleaseIndex(void* hdr, NxU32 idx)
	{
	unsigned h = reinterpret_cast<unsigned>(hdr);
	unsigned* sent = *reinterpret_cast<unsigned**>(h);
	if(sent[idx] != 0xFFFFFFFFu)
		nxU32VectorPushBack(reinterpret_cast<unsigned*>(h + 0x2c), idx);
	if(sent[idx] == 0)
		return;
	unsigned* cntA = *reinterpret_cast<unsigned**>(h + 0x10);
	unsigned* cntBEnd = *reinterpret_cast<unsigned**>(h + 0x14);
	unsigned* mir = *reinterpret_cast<unsigned**>(h + 0x20);
	unsigned lastVal = *(cntBEnd - 1);
	NxU32 u = mir[idx];
	cntA[u] = lastVal;
	mir[lastVal] = u;
	*reinterpret_cast<unsigned**>(h + 0x14) = cntBEnd - 1;
	sent[idx] = 0;
	mir[idx] = 0xD00BEED0u;
	}

// Remover #1: the deregistration chain's shape-side wrapper -- the slot
// read off Shape+0xd4, released through phys_fn_002410, then the
// shapes-array clear that lives one call over at 0x5bbe0.
void nxSceneRemoveShape(void* container, void* shape)
	{
	unsigned c = reinterpret_cast<unsigned>(container);
	const unsigned char* sh = static_cast<const unsigned char*>(shape);
	NxU32 slot = *reinterpret_cast<const NxU32*>(sh + 0xd4);

	nxSceneReleaseIndex(container, slot);

	unsigned* shapes = *reinterpret_cast<unsigned**>(c + 0x90);
	shapes[slot] = 0;							// 0x5bbe0's clear
	}

// Remover #2 (phys_fn_002344, 0x5aae0): pair-list swap-remove. The caller
// hands the ADDRESS OF THE SCENE'S +0x5d4 FIELD (0x00026c13 add ecx,0x5d4),
// so [this] names a second header whose two words are {begin,end} of the
// stride-8 pair array. The image re-reads that header each match, shrinks
// its cursor by 8 per removal, and leaves a matched FINAL element in place
// rather than copied over itself. A match is either half equal to the value.
void nxSceneRemovePairs(void* container, void* shape)
	{
	unsigned c = reinterpret_cast<unsigned>(container);
	unsigned self = reinterpret_cast<unsigned>(shape);
	unsigned* hdr = *reinterpret_cast<unsigned**>(c);
	unsigned* begin = reinterpret_cast<unsigned*>(hdr[0]);
	unsigned count = static_cast<NxU32>(
		(hdr[1] - reinterpret_cast<unsigned>(begin)) >> 3);
	unsigned i = 0;
	while(i < count)
		{
		if(begin[i * 2] == self || begin[i * 2 + 1] == self)
			{
			if(i != count - 1)					// the image skips a self-copy
				{
				begin[i * 2] = begin[(count - 1) * 2];
				begin[i * 2 + 1] = begin[(count - 1) * 2 + 1];
				}
			count -= 1;
			}
		else
			i += 1;
		}
	hdr[1] = reinterpret_cast<unsigned>(begin) + count * 8;
	}

// phys_fn_000028 (0x1b90): dword-vector push_back over the VC9 layout
// {_Myproxy@+0x00 untouched by this row, _Myfirst@+0x04, _Mylast@+0x08,
// _Myend@+0x0c}. The growth arm allocates 2*size + 2 dwords through the SDK
// allocator (adapter vtable slot +8 with flag word 0), copies the live
// elements dword-wise, releases the old block (slot +0x14) and repoints all
// three cursors; the compiler's own escape (`jae` over the arm when the old
// capacity already reads >= the new one) is arithmetically unreachable
// while the vector is full but is transcribed for faithfulness.
void nxU32VectorPushBack(void* vecHeader, NxU32 value)
	{
	unsigned f = reinterpret_cast<unsigned>(vecHeader);
	unsigned* begin = *reinterpret_cast<unsigned**>(f + 0x04);
	unsigned* end = *reinterpret_cast<unsigned**>(f + 0x08);
	unsigned* capEnd = *reinterpret_cast<unsigned**>(f + 0x0c);
	if(capEnd > end)
		{
		*end = value;						// ja: room at the cursor
		*reinterpret_cast<unsigned**>(f + 0x08) = end + 1;
		return;
		}
	unsigned size = static_cast<unsigned>(
		(reinterpret_cast<unsigned>(end) - reinterpret_cast<unsigned>(begin)) >> 2);
	unsigned newCapDwords = size + size + 2;	// lea eax,[eax+eax+2]
	unsigned oldCapDwords = begin == 0 ? 0u : static_cast<unsigned>(
		(reinterpret_cast<unsigned>(capEnd) - reinterpret_cast<unsigned>(begin)) >> 2);
	if(oldCapDwords < newCapDwords)
		{
		void* fresh = nxGetSdkAllocator()->malloc(
			newCapDwords * sizeof(unsigned), NX_MEMORY_PERSISTENT);
		unsigned* run = static_cast<unsigned*>(fresh);
		for(unsigned i = 0; i < size; ++i)	// copy before release
			run[i] = begin[i];
		if(begin != 0)
			nxGetSdkAllocator()->free(begin);
		*reinterpret_cast<unsigned**>(f + 0x04) = run;
		*reinterpret_cast<unsigned**>(f + 0x0c) = run + newCapDwords;
		end = run + size;
		}
	*end = value;
	*reinterpret_cast<unsigned**>(f + 0x08) = end + 1;
	}

// Remover #3: the deregistration chain's name for phys_fn_000028 applied to
// the slot-free vector header.
void nxSceneSlotFree(void* container, NxU32 slot)
	{
	nxU32VectorPushBack(container, slot);
	}

// ---------------------------------------------------------------------------
// NxActor scaffolding. The actor body reads decode as a uniform shape:
// edi = [this+0x10] (the scene lock context), enter guard, read through
// [this+0x14] (the body pointer), leave guard, return. The guard pair
// (0x5b700 / 0x5b790) runs Foundation primitives over the critical section
// POINTER at *[scene]: Enter/LeaveCriticalSection, an InterlockedCompare-
// Exchange writer flag at block+0x18, and the owning thread id at +0x1c.

void nxSceneGuardEnter(void* scene)
	{
	CRITICAL_SECTION* cs =
		*reinterpret_cast<CRITICAL_SECTION**>(scene);
	unsigned* flag = reinterpret_cast<unsigned*>(cs) + 6;
	::EnterCriticalSection(cs);
	::InterlockedCompareExchange(reinterpret_cast<volatile long*>(flag),
		1, 0);
	flag[1] = ::GetCurrentThreadId();
	}

void nxSceneGuardLeave(void* scene)
	{
	CRITICAL_SECTION* cs =
		*reinterpret_cast<CRITICAL_SECTION**>(scene);
	unsigned* flag = reinterpret_cast<unsigned*>(cs) + 6;
	::InterlockedCompareExchange(reinterpret_cast<volatile long*>(flag),
		0, 1);
	::LeaveCriticalSection(cs);
	}

// phys_fn_000110 (slot 19, 0x3580): guard; bool of [body+8]; unguard.
bool nxActorBodyPresent(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned r = *reinterpret_cast<unsigned*>(body + 8);
	nxSceneGuardLeave(scene);
	return r != 0;
	}

// phys_fn_000114 (slot 86, 0x3610): guard; the WORD at [body+0x1c]; unguard.
NxU16 nxActorGetGroupWord(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	NxU16 r = *reinterpret_cast<NxU16*>(body + 0x1c);
	nxSceneGuardLeave(scene);
	return r;
	}

// phys_fn_000078 (slot 77, 0x2c60): guard; ([body+0x14] & mask) != 0 --
// the actor flag word tested against the caller's mask; unguard.
bool nxActorFlagsMasked(void* self, unsigned mask)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned flags = *reinterpret_cast<unsigned*>(body + 0x14);
	nxSceneGuardLeave(scene);
	return (flags & mask) != 0;
	}

// phys_fn_000066 (slot 69, 0x29e0): guard; [body+8] names a nested record;
// fsqrt of its float at +0xd0, or 0.0f when that record is null (the only
// guard the image carries -- a null body would fault it); unguard. x87
// fsqrt computes at full precision and the fstp m32 narrows -- typed as
// double here for the same rounding shape.
float nxActorSqrtFieldD0(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = body != 0
		? *reinterpret_cast<unsigned*>(body + 8) : 0u;
	unsigned bits = 0;
	if(rec != 0)
		bits = *reinterpret_cast<unsigned*>(rec + 0xd0);
	nxSceneGuardLeave(scene);
	float v;
	memcpy(&v, &bits, 4);
	double d = v;
	float r = static_cast<float>(::sqrt(d));
	return rec != 0 ? r : 0.0f;
	}

// phys_fn_000068 (slot 71, 0x2a30): same shape over [record+0xd4].
float nxActorSqrtFieldD4(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = body != 0
		? *reinterpret_cast<unsigned*>(body + 8) : 0u;
	unsigned bits = 0;
	if(rec != 0)
		bits = *reinterpret_cast<unsigned*>(rec + 0xd4);
	nxSceneGuardLeave(scene);
	float v;
	memcpy(&v, &bits, 4);
	double d = v;
	float r = static_cast<float>(::sqrt(d));
	return rec != 0 ? r : 0.0f;
	}

// phys_fn_000015 (0x14f0): body helper. [body+0x10] names the shape list
// head; null yields 0, a non-mesh shape (type word at +0xd0 != 5) yields 1,
// and a mesh yields its triangle-array span ([+0xe4]-[+0xe0])>>2.
unsigned nxBodyShapeRecordCount(void* body)
	{
	unsigned sh = *reinterpret_cast<unsigned*>(
		reinterpret_cast<unsigned>(body) + 0x10);
	if(sh == 0)
		return 0;
	if(*reinterpret_cast<unsigned*>(sh + 0xd0) != 5)
		return 1;
	return (*reinterpret_cast<unsigned*>(sh + 0xe4)
		- *reinterpret_cast<unsigned*>(sh + 0xe0)) >> 2;
	}

// phys_fn_000019 (0x1540): body helper. Null shape list yields null; a mesh
// yields [+0xf0]; anything else yields shape+0x9c -- the collision object
// the Shape layout names.
void* nxBodyCollisionObject(void* body)
	{
	unsigned sh = *reinterpret_cast<unsigned*>(
		reinterpret_cast<unsigned>(body) + 0x10);
	if(sh == 0)
		return nullptr;
	if(*reinterpret_cast<unsigned*>(sh + 0xd0) == 5)
		return reinterpret_cast<void*>(*reinterpret_cast<unsigned*>(sh + 0xf0));
	return reinterpret_cast<void*>(sh + 0x9c);
	}

// phys_fn_000082 (slot 15, 0x2d00): guard; nxBodyShapeRecordCount(body);
// unguard.
unsigned nxActorShapeRecordCount(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned r = nxBodyShapeRecordCount(
		reinterpret_cast<void*>(body));
	nxSceneGuardLeave(scene);
	return r;
	}

// phys_fn_000084 (slot 16, 0x2d30): guard; nxBodyCollisionObject(body);
// unguard.
void* nxActorCollisionObject(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	void* r = nxBodyCollisionObject(reinterpret_cast<void*>(body));
	nxSceneGuardLeave(scene);
	return r;
	}

// phys_fn_000086 (slot 84, 0x2d60): guard; the SDK pointer binding keyed on
// the body pointer (phys_fn_000454, closed in Phase 2); unguard.
void* nxActorBoundTarget(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	void* r = nxGetSdkPointerBinding(reinterpret_cast<void*>(body));
	nxSceneGuardLeave(scene);
	return r;
	}

// The +0x08 member subobject. Its pre-member state lives under a third
// table (0x101088b8) that both the constructor (phys_fn_002404) and the
// destructor (phys_fn_002406) install -- the dtor RESTORES it rather than
// leaving the one-slot member table behind.
void nxActorMemberInit(void* memberAtPlus8)
	{
	unsigned m = reinterpret_cast<unsigned>(memberAtPlus8);
	*reinterpret_cast<unsigned**>(m) =
		reinterpret_cast<unsigned*>(0x101088b8u);
	*reinterpret_cast<unsigned*>(m + 4) = 0;
	*reinterpret_cast<unsigned*>(m + 8) = 0;
	}

void nxActorMemberReset(void* memberAtPlus8)
	{
	*reinterpret_cast<unsigned**>(memberAtPlus8) =
		reinterpret_cast<unsigned*>(0x101088b8u);
	}

// phys_fn_000044 (0x2480): the actor constructor chain tail. Chained-
// construction intermediates are observable here only as order: wall vptr,
// owner zero, member init over +0x08..+0x13, the one-slot member table over
// +0x08, the body pointer at +0x14, then the dynamic final over +0x00.
void nxActorConstruct(void* self, void* body)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	*reinterpret_cast<unsigned**>(a) =
		reinterpret_cast<unsigned*>(0x101043d0u);
	*reinterpret_cast<unsigned*>(a + 4) = 0;
	nxActorMemberInit(reinterpret_cast<unsigned char*>(self) + 8);
	*reinterpret_cast<unsigned**>(a + 8) =
		reinterpret_cast<unsigned*>(0x1010468cu);
	*reinterpret_cast<unsigned*>(a + 0x14) =
		reinterpret_cast<unsigned>(body);
	*reinterpret_cast<unsigned**>(a) =
		reinterpret_cast<unsigned*>(0x10104530u);
	}

// phys_fn_000042 (0x2460): the interface-wall scalar-deleting destructor:
// install the wall table, then release through the linked CRT -- 0x0002471
// calls 0x100f41f0 directly, NOT the SDK allocator adapter its sibling at
// slot 0 uses.
void nxActorInterfaceDtor(void* self, unsigned flags)
	{
	*reinterpret_cast<unsigned**>(self) =
		reinterpret_cast<unsigned*>(0x101043d0u);
	if(flags & 1)
		::free(self);
	}

// phys_fn_000118 (slot 0, 0x3650): the actor scalar-deleting destructor:
// final tables again, member reset to the third table, allocator release
// through slot +0x14 when flagged.
void nxActorDeletingDtor(void* self, unsigned flags)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	*reinterpret_cast<unsigned**>(a) =
		reinterpret_cast<unsigned*>(0x10104530u);
	*reinterpret_cast<unsigned**>(a + 8) =
		reinterpret_cast<unsigned*>(0x1010468cu);
	nxActorMemberReset(reinterpret_cast<unsigned char*>(self) + 8);
	*reinterpret_cast<unsigned**>(a) =
		reinterpret_cast<unsigned*>(0x101043d0u);
	if(flags & 1)
		nxGetSdkAllocator()->free(self);
	}

// phys_fn_000116 (slot 87, 0x3640): the member table's this-adjustor
// thunk -- `sub ecx,8` onto the actor base, then the deleting dtor. Eight
// bytes because that is where the member subobject sits.
void nxActorDeletingDtorThunk(void* memberThis, unsigned flags)
	{
	nxActorDeletingDtor(
		reinterpret_cast<unsigned char*>(memberThis) - 8, flags);
	}

// ---------------------------------------------------------------------------
// Actor slate 2: the write side and two record readers.
//
// phys_fn_000742 (0x16dd0): record helper -- a pure x87 chain whose cross
// terms carry their scale factors SQUARED (the fxch/faddp ladder multiplies
// each product by its second word a second time): 0.5 * (([+0x74]^2 +
// [+0x70]^2 + [+0x6c]^2)*[+0x188] + [+0x194]*[+0x80]^2 + [+0x190]*[+0x7c]^2
// + [+0x18c]*[+0x78]^2). Every intermediate stays at full precision and
// there is ONE rounding -- the helpers return in st(0) and their callers'
// fstp m32 does that rounding, so the C++ computes in double and casts once.
float nxBodyRecordEnergyWord(void* rec)
	{
	unsigned r = reinterpret_cast<unsigned>(rec);
	double v6c = *reinterpret_cast<const float*>(r + 0x6c);
	double v70 = *reinterpret_cast<const float*>(r + 0x70);
	double v74 = *reinterpret_cast<const float*>(r + 0x74);
	double m78 = *reinterpret_cast<const float*>(r + 0x78);
	double m7c = *reinterpret_cast<const float*>(r + 0x7c);
	double m80 = *reinterpret_cast<const float*>(r + 0x80);
	double m188 = *reinterpret_cast<const float*>(r + 0x188);
	double v18c = *reinterpret_cast<const float*>(r + 0x18c);
	double v190 = *reinterpret_cast<const float*>(r + 0x190);
	double v194 = *reinterpret_cast<const float*>(r + 0x194);
	return static_cast<float>(
		(((v74 * v74 + v70 * v70) + v6c * v6c) * m188
		+ v194 * (m80 * m80) + v190 * (m7c * m7c)
		+ v18c * (m78 * m78)) * 0.5);
	}

// phys_fn_000730-equivalent guard upgrade (0x5b730): try to take the writer
// flag at [cs]+0x18; if it is already held by ANOTHER thread, fail without
// entering -- the caller reports and skips to avoid a deadlock. Held by this
// thread or free: enter, re-take, record the tid, succeed.
bool nxSceneGuardWriteTry(void* ctx)
	{
	CRITICAL_SECTION* cs =
		*reinterpret_cast<CRITICAL_SECTION**>(ctx);
	unsigned* flag = reinterpret_cast<unsigned*>(cs) + 6;
	long held = ::InterlockedCompareExchange(
		reinterpret_cast<volatile long*>(flag), 1, 0);
	if(held != 0
		&& flag[1] != static_cast<unsigned>(::GetCurrentThreadId()))
		return false;
	::EnterCriticalSection(cs);
	::InterlockedCompareExchange(reinterpret_cast<volatile long*>(flag),
		1, 0);
	flag[1] = ::GetCurrentThreadId();
	return true;
	}

// The NpActor.cpp write-lock literals, exposed for the transcript pin.
const char* const	nxSourceFileNpActorCpp =
	"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpActor.cpp";
const char* const	nxMsgWriteLockStillAcquired =
	"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to "
	"avoid a deadlock!";

// Shared body of both flag writers: guard-upgrade on the member field at
// +0xc, report kind 2 and skip on failure, else mutate body+0x14. The line
// differs between the two rows (0x1bb raise, 0x1c1 clear).
static bool nxActorWriteFlagsGuarded(void* self, unsigned mask,
	bool raise, int line)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* ctx = *reinterpret_cast<void**>(a + 0xc);
	if(!nxSceneGuardWriteTry(ctx))
		{
		nxReport(2, nxSourceFileNpActorCpp, line, 0,
			nxMsgWriteLockStillAcquired);
		return false;
		}
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned flags = *reinterpret_cast<unsigned*>(body + 0x14);
	flags = raise ? (flags | mask) : (flags & ~mask);
	*reinterpret_cast<unsigned*>(body + 0x14) = flags;
	nxSceneGuardLeave(ctx);
	return true;
	}

// phys_fn_000074 (slot 75, 0x2ba0): flags |= mask under the write guard.
void nxActorRaiseFlags(void* self, unsigned mask)
	{
	nxActorWriteFlagsGuarded(self, mask, true, 0x1bb);
	}

// phys_fn_000076 (slot 76, 0x2c00): flags &= ~mask under the write guard.
void nxActorClearFlags(void* self, unsigned mask)
	{
	nxActorWriteFlagsGuarded(self, mask, false, 0x1c1);
	}

// phys_fn_000060 (slot 62, 0x2900): guarded; the nested record's energy
// word from phys_fn_000742, exact float zero when the record is null.
float nxActorRecordEnergyWord(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = body != 0
		? *reinterpret_cast<unsigned*>(body + 8) : 0u;
	float out = 0.0f;
	if(rec != 0)
		out = nxBodyRecordEnergyWord(reinterpret_cast<void*>(rec));
	nxSceneGuardLeave(scene);
	return out;
	}

// phys_fn_000064 (slot 68, 0x2990): guarded; true when the nested record is
// null OR its word at +0x84 reads zero.
bool nxActorRecordWord84Zero(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = body != 0
		? *reinterpret_cast<unsigned*>(body + 8) : 0u;
	unsigned w84 = rec != 0
		? *reinterpret_cast<unsigned*>(rec + 0x84) : 0u;
	nxSceneGuardLeave(scene);
	return w84 == 0;
	}

// ---------------------------------------------------------------------------
// Actor slate 3: the damping getters. Both read one float off the nested
// record and REPORT kind 1 when the record is null -- the actor must be
// dynamic -- returning the zero constant at 0x101041f0 afterwards.
const char* const	nxMsgLinearDampingDynamic =
	"Actor::setLinearDamping: Actor must be dynamic!";
const char* const	nxMsgAngularDampingDynamic =
	"Actor::getAngularDamping: Actor must be dynamic!";

// phys_fn_000050 (slot 42, 0x2610): getLinearDamping -- [+0xb8].
float nxActorGetLinearDamping(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, 0xd9, 0,
			nxMsgLinearDampingDynamic);
		nxSceneGuardLeave(scene);
		return 0.0f;
		}
	float v = *reinterpret_cast<const float*>(rec + 0xb8);
	nxSceneGuardLeave(scene);
	return v;
	}

// phys_fn_000052 (slot 44, 0x2680): getAngularDamping -- [+0xbc], line 0xe8.
float nxActorGetAngularDamping(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, 0xe8, 0,
			nxMsgAngularDampingDynamic);
		nxSceneGuardLeave(scene);
		return 0.0f;
		}
	float v = *reinterpret_cast<const float*>(rec + 0xbc);
	nxSceneGuardLeave(scene);
	return v;
	}

// phys_fn_000048 (slot 36, 0x25c0): guarded float at [record+0x188]; exact
// float zero when the record is null -- no report on this one.
float nxActorRecordField188(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = body != 0
		? *reinterpret_cast<unsigned*>(body + 8) : 0u;
	float out = 0.0f;
	if(rec != 0)
		out = *reinterpret_cast<const float*>(rec + 0x188);
	nxSceneGuardLeave(scene);
	return out;
	}

// ---------------------------------------------------------------------------
// Actor slate 4.

// phys_fn_000088 (slot 83, 0x2d90): guarded SDK pointer-binding WRITE --
// phys_fn_000480 keyed on the body pointer, under the write guard; report
// line 0x1ff on a failed upgrade, skip without binding.
void nxActorSetBoundTarget(void* self, void* value)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* ctx = *reinterpret_cast<void**>(a + 0xc);
	if(!nxSceneGuardWriteTry(ctx))
		{
		nxReport(2, nxSourceFileNpActorCpp, 0x1ff, 0,
			nxMsgWriteLockStillAcquired);
		return;
		}
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	nxSetSdkPointerBinding(reinterpret_cast<void*>(body),
		reinterpret_cast<void*>(value));
	nxSceneGuardLeave(ctx);
	}

// phys_fn_000092 (slot 6, 0x2ed0): guarded three-word read into `out`.
// With a nested record the words come from record+0x50/54/58; without one,
// from the BODY defaults at +0x44/48/4c. Returns the out pointer.
void* nxActorGetPoseWords(void* self, void* out)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	unsigned* o = reinterpret_cast<unsigned*>(out);
	if(rec != 0)
		{
		o[0] = *reinterpret_cast<const unsigned*>(rec + 0x50);
		o[1] = *reinterpret_cast<const unsigned*>(rec + 0x54);
		o[2] = *reinterpret_cast<const unsigned*>(rec + 0x58);
		}
	else
		{
		o[0] = *reinterpret_cast<const unsigned*>(body + 0x44);
		o[1] = *reinterpret_cast<const unsigned*>(body + 0x48);
		o[2] = *reinterpret_cast<const unsigned*>(body + 0x4c);
		}
	nxSceneGuardLeave(scene);
	return out;
	}

// phys_fn_000038 (0x2400, ret 8) and phys_fn_000040 (0x2430, ret 8): the
// actor vtable thunks. Dispatch through the object's own vtable slot +0x104
// / +0x108 as (self, &local, arg1), then copy the first three words of the
// returned record into out.
static void* nxActorVtThunk(void* self, void* arg1, unsigned* out, unsigned slot)
	{
	void** vt = *reinterpret_cast<void***>(self);
	typedef void* (__thiscall* SlotFn)(void*, void*, void*);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[slot / 4]);
	unsigned local[3] = { 0, 0, 0 };
	void* r = fn(self, local, arg1);
	const unsigned* src = reinterpret_cast<const unsigned*>(r);
	out[0] = src[0]; out[1] = src[1]; out[2] = src[2];
	return out;
	}

void* nxActorVtThunk104(void* self, void* arg1, unsigned* out)
	{ return nxActorVtThunk(self, arg1, out, 0x104u); }
void* nxActorVtThunk108(void* self, void* arg1, unsigned* out)
	{ return nxActorVtThunk(self, arg1, out, 0x108u); }

// phys_fn_003268 (0x7e560, ret 8): batch index/vertex append. Bails when
// [self+0x18] >= [self+0x1c]; accumulates (count-2) into [self+0x20]; records
// the count in the [self+0x403c]/[self+0x4044] list when it has room; then for
// each index below [self+0x10] copies the 3-dword vertex record through the
// [self+8] map into the [self+0x4034]/[self+0x4038] output array (assigning a
// fresh id when the map slot is zero) and appends id-1 to the aux list.
void nxBatchAppend3268(void* self, unsigned count, const unsigned* indices)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	if(*reinterpret_cast<unsigned*>(p + 0x18) >=
		*reinterpret_cast<unsigned*>(p + 0x1c))
		return;
	unsigned auxCount = *reinterpret_cast<unsigned*>(p + 0x403c);
	*reinterpret_cast<unsigned*>(p + 0x20) += (count - 2);
	if(auxCount < *reinterpret_cast<unsigned*>(p + 0x4040))
		{
		reinterpret_cast<unsigned*>(
			*reinterpret_cast<void**>(p + 0x4044))[auxCount] = count;
		++*reinterpret_cast<unsigned*>(p + 0x403c);
		}
	for(unsigned i = 0; i < count; ++i)
		{
		const unsigned idx = indices[i];
		if(idx >= *reinterpret_cast<unsigned*>(p + 0x10))
			continue;
		unsigned* map = *reinterpret_cast<unsigned**>(p + 8);
		unsigned slot = map[idx];
		if(slot == 0)
			{
			const unsigned* verts = *reinterpret_cast<const unsigned**>(p + 0xc);
			unsigned* outArr = *reinterpret_cast<unsigned**>(p + 0x4038);
			unsigned outCount = *reinterpret_cast<unsigned*>(p + 0x4034);
			const unsigned* src = verts + idx * 3u;
			unsigned* dst = outArr + outCount * 3u;
			dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
			++outCount;
			*reinterpret_cast<unsigned*>(p + 0x4034) = outCount;
			map[idx] = outCount;
			slot = outCount;
			}
		unsigned c2 = *reinterpret_cast<unsigned*>(p + 0x403c);
		if(c2 < *reinterpret_cast<unsigned*>(p + 0x4040))
			{
			reinterpret_cast<unsigned*>(
				*reinterpret_cast<void**>(p + 0x4044))[c2] = slot - 1u;
			++*reinterpret_cast<unsigned*>(p + 0x403c);
			}
		}
	++*reinterpret_cast<unsigned*>(p + 0x18);
	}

namespace
	{
	// The shared quaternion -> 3x3 rotation expansion used by 000132 and
	// 000130. The x87 body accumulates in extended precision and stores to
	// float, so a double intermediate reproduces the staged rounding.
	inline void nxQuatToMatrix9(const float* q, float* m)
		{
		const double x = q[0], y = q[1], z = q[2], w = q[3];
		m[0] = static_cast<float>(1.0 - 2.0 * (y*y + z*z));
		m[1] = static_cast<float>(2.0 * (x*y - z*w));
		m[2] = static_cast<float>(2.0 * (x*z + y*w));
		m[3] = static_cast<float>(2.0 * (x*y + z*w));
		m[4] = static_cast<float>(1.0 - 2.0 * (x*x + z*z));
		m[5] = static_cast<float>(2.0 * (y*z - x*w));
		m[6] = static_cast<float>(2.0 * (x*z - y*w));
		m[7] = static_cast<float>(2.0 * (y*z + x*w));
		m[8] = static_cast<float>(1.0 - 2.0 * (x*x + y*y));
		}
	}

// phys_fn_000130 (0x4580, ret 4): writes the full 0x30-byte pose to out -- the
// quaternion matrix from record+0x5c, then the translation at
// record+0x50/0x54/0x58 -- or copies the cached pose at [self+0x14]+0x20 when
// the record pointer is null. Returns out.
float* nxPoseFromQuat0130(void* self, float* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* body = *reinterpret_cast<const unsigned char* const*>(p + 0x14);
	const unsigned char* rec = *reinterpret_cast<const unsigned char* const*>(body + 8);
	if(rec == nullptr)
		{
		memcpy(out, body + 0x20, 0x30);
		return out;
		}
	unsigned char pose[0x30];
	float q[4];
	memcpy(&q[0], rec + 0x5c, 4);
	memcpy(&q[1], rec + 0x60, 4);
	memcpy(&q[2], rec + 0x64, 4);
	memcpy(&q[3], rec + 0x68, 4);
	nxQuatToMatrix9(q, reinterpret_cast<float*>(pose));
	memcpy(pose + 0x24, rec + 0x50, 12);
	memcpy(out, pose, 0x30);
	return out;
	}

// The locked accessor family (see 3z138/3z139): lock [self+0x10], call a
// plain field getter on [self+0x24], unlock, return the result. Since the
// getter is a bare field read, the whole row reduces to this.
unsigned nxLockedFieldRead(void* self, unsigned offset)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + 0x24);
	unsigned v;
	memcpy(&v, field + offset, 4);
	return v;
	}

// The pointer-returning member of the family: the wrapped getter is
// `lea eax,[field+offset]`, so the row returns that address.
void* nxLockedPointerRead(void* self, unsigned offset)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + 0x24);
	return const_cast<unsigned char*>(field + offset);
	}

// Second variant of the family: lock at [self+lockOff], field at
// [self+fieldOff], wrapped getter a plain dword read of [field+dataOff].
unsigned nxLockedFieldReadEx(void* self, unsigned lockOff, unsigned fieldOff,
	unsigned dataOff)
	{
	(void) lockOff;
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned v;
	memcpy(&v, field + dataOff, 4);
	return v;
	}

// phys_fn_003824 (0x8c9c0): the getter is `lea eax,[field+0x14]` on
// [self+0x14] -- wait, the getter reads [ecx+0x14] and the row then
// dereferences the result, so the row returns *(*(field+0x14)).
unsigned nxLockedDeref3824(void* self)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + 0x14);
	unsigned ptr;
	memcpy(&ptr, field + 0x14, 4);
	unsigned v;
	memcpy(&v, reinterpret_cast<const unsigned char*>(static_cast<size_t>(ptr)), 4);
	return v;
	}

// phys_fn_003872 (0x8d1f0, ret 4): the getter is `lea eax,[field+8]`; the row
// copies twelve dwords from it into out and returns out.
void* nxLockedCopy12_3872(void* self, unsigned* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + 0x14);
	memcpy(out, field + 8, 48);
	return out;
	}

// phys_fn_004479 (0xb0d20, ret 4): compares arg with [field+0x168] (field from
// [self+0x18]) and returns SELF when they are equal, else zero -- the mask is
// `and esi, ecx` with esi still holding this.
unsigned nxLockedMatch4479(void* self, unsigned arg)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + 0x18);
	unsigned v;
	memcpy(&v, field + 0x168, 4);
	return (arg == v)
		? static_cast<unsigned>(reinterpret_cast<size_t>(self))
		: 0u;
	}

// The copy members of the family: the wrapped helper copies `count` dwords
// from [field+dataOff] into out.
void nxLockedCopyOut(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned count, unsigned* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	memcpy(out, field + dataOff, count * 4u);
	}

// The pose-copy member: nine dwords from [field+dataOff] followed by the three
// at [field+dataOff+0x24].
void nxLockedCopyPose(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	memcpy(out, field + dataOff, 36);
	memcpy(reinterpret_cast<unsigned char*>(out) + 0x24, field + dataOff + 0x24, 12);
	}

// The mask member: the wrapped helper is `mov eax,[ecx+dataOff]; and eax,arg`.
unsigned nxLockedAndRead(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned arg)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned v;
	memcpy(&v, field + dataOff, 4);
	return v & arg;
	}

// The bit-extract member: `mov eax,[ecx+dataOff]; shr eax,3; and eax,3`.
unsigned nxLockedBitExtract(void* self, unsigned fieldOff, unsigned dataOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned v;
	memcpy(&v, field + dataOff, 4);
	return (v >> 3) & 3u;
	}

// The element-count member: `mov eax,[ecx+8]; sub eax,[ecx+4]; sar eax,2`.
int nxLockedElementCount(void* self, unsigned fieldOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned hi, lo;
	memcpy(&hi, field + 8, 4);
	memcpy(&lo, field + 4, 4);
	return static_cast<int>(static_cast<signed>(hi - lo) >> 2);
	}

// The same count over caller-chosen offsets (rows that compute it inline).
int nxLockedElementCountAt(void* self, unsigned fieldOff, unsigned hiOff,
	unsigned loOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned hi, lo;
	memcpy(&hi, field + hiOff, 4);
	memcpy(&lo, field + loOff, 4);
	return static_cast<int>(static_cast<signed>(hi - lo) >> 2);
	}

// The interval-flag member: returns [field+0x14] when the span from it to
// [field+0x18] carries any bit above the low two, else zero.
unsigned nxLockedIntervalFlag(void* self, unsigned fieldOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned lo, hi;
	memcpy(&lo, field + 0x14, 4);
	memcpy(&hi, field + 0x18, 4);
	return ((hi - lo) & 0xfffffffcu) ? lo : 0u;
	}

// The address-returning member.
void* nxLockedFieldAddress(void* self, unsigned fieldOff, unsigned dataOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	return const_cast<unsigned char*>(field + dataOff);
	}

// The word-mask member.
unsigned nxLockedWordAndRead(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned arg)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned short w;
	memcpy(&w, field + dataOff, 2);
	return static_cast<unsigned>(w) & arg;
	}

// The two-pointer copy member.
void nxLockedCopyTwoPointers(void* self, unsigned fieldOff, unsigned* out1,
	unsigned* out2)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	memcpy(out1, field + 0x3c, 4);
	memcpy(out2, field + 0x40, 4);
	}

// The float member: `fld [field+dataOff]; fadd st(0),st(0)`, stored to float
// by the caller's fstp, so a double intermediate matches.
float nxLockedDoubleField(void* self, unsigned fieldOff, unsigned dataOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	float v;
	memcpy(&v, field + dataOff, 4);
	return static_cast<float>(2.0 * static_cast<double>(v));
	}

// The copy-and-flag member.
unsigned char nxLockedCopyAndFlag(void* self, unsigned fieldOff,
	unsigned dataOff, unsigned count, unsigned shift, unsigned* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	memcpy(out, field + dataOff, count * 4u);
	unsigned f;
	memcpy(&f, field + 0x1a8, 4);
	return static_cast<unsigned char>((f >> shift) & 1u);
	}

// The three-pointer copy member.
void nxLockedCopyThreePointers(void* self, unsigned fieldOff, unsigned* out1,
	unsigned* out2, unsigned* out3)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	memcpy(out1, field + 0x52c, 4);
	memcpy(out2, field + 0x530, 4);
	memcpy(out3, field + 0x534, 4);
	}

// The conditional-deref member: read the pointer at [field+dataOff]; when it
// is non-null return the word at [that+derefOff], else zero.
unsigned nxLockedDerefField(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned derefOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned ptr;
	memcpy(&ptr, field + dataOff, 4);
	if(ptr == 0u)
		return 0u;
	unsigned v;
	memcpy(&v, reinterpret_cast<const unsigned char*>(static_cast<size_t>(ptr)) + derefOff, 4);
	return v;
	}

// The N-pointer copy member.
void nxLockedCopyNPointers(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned count, unsigned** outs)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	for(unsigned k = 0; k < count; ++k)
		memcpy(outs[k], field + dataOff + 4u * k, 4);
	}

// The conditional-count member.
int nxLockedConditionalCount(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned hiOff, unsigned loOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned ptr;
	memcpy(&ptr, field + dataOff, 4);
	if(ptr == 0u)
		return 0;
	const unsigned char* q = reinterpret_cast<const unsigned char*>(static_cast<size_t>(ptr));
	unsigned hi, lo;
	memcpy(&hi, q + hiOff, 4);
	memcpy(&lo, q + loOff, 4);
	return static_cast<int>(static_cast<signed>(hi - lo) >> 2);
	}

// The nested-deref member.
void nxLockedTwoNestedDerefs(void* self, unsigned fieldOff, unsigned off1,
	unsigned off2, unsigned nested, unsigned* out1, unsigned* out2)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	const unsigned offs[2] = { off1, off2 };
	unsigned* outs[2] = { out1, out2 };
	for(unsigned k = 0; k < 2; ++k)
		{
		unsigned ptr;
		memcpy(&ptr, field + offs[k], 4);
		unsigned v = 0;
		if(ptr != 0u)
			{
			// [ptr+nested] is itself a pointer, and the word at it is the value.
			unsigned inner = 0;
			memcpy(&inner, reinterpret_cast<const unsigned char*>(static_cast<size_t>(ptr)) + nested, 4);
			if(inner != 0u)
				memcpy(&v, reinterpret_cast<const unsigned char*>(static_cast<size_t>(inner)), 4);
			}
		memcpy(outs[k], &v, 4);
		}
	}

// The match-or-self shape over caller-chosen offsets.
unsigned nxLockedMatchEx(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned arg)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned v;
	memcpy(&v, field + dataOff, 4);
	return (arg == v)
		? static_cast<unsigned>(reinterpret_cast<size_t>(self))
		: 0u;
	}

// The list-advance reader: pop the head at [field+linkOff], relink to
// [head+nextOff] and return the word at [head+readOff].
unsigned nxLockedAdvanceRead(void* self, unsigned fieldOff, unsigned linkOff,
	unsigned nextOff, unsigned readOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	unsigned char* field = const_cast<unsigned char*>(
		*reinterpret_cast<const unsigned char* const*>(p + fieldOff));
	unsigned head;
	memcpy(&head, field + linkOff, 4);
	if(head == 0u)
		return 0u;
	unsigned next;
	memcpy(&next, reinterpret_cast<const unsigned char*>(static_cast<size_t>(head)) + nextOff, 4);
	memcpy(field + linkOff, &next, 4);
	unsigned v;
	memcpy(&v, reinterpret_cast<const unsigned char*>(static_cast<size_t>(head)) + readOff, 4);
	return v;
	}

// phys_fn_005450 (0xef690, ret 4): the 004886-calling wrapper. 004886 runs the
// registered callback at [0x10128478] when set and then sets [node+8] bit 2;
// with that slot bound to a no-op the whole call reduces to the bit set.
unsigned char nxWrap5450(void* self, void* node)
	{
	unsigned char* n = reinterpret_cast<unsigned char*>(node);
	unsigned short w;
	memcpy(&w, n + 0x28, 2);
	if(w != 0xffffu && !(n[8] & 2))
		*(unsigned*)(n + 8) |= 2u;
	++*reinterpret_cast<unsigned*>(reinterpret_cast<unsigned char*>(self) + 0x38);
	return 1;
	}

// phys_fn_001787 (0x3f570): the sibling of 005450, reading its object from the
// second stack argument (bare `ret`), calling 004886 on object+0xa4.
unsigned char nxWrap1787(void* unused, void* node)
	{
	(void) unused;
	unsigned char* n = reinterpret_cast<unsigned char*>(node);
	unsigned short w;
	memcpy(&w, n + 0xcc, 2);
	unsigned char* sub = n + 0xa4;
	if(w != 0xffffu && !(sub[8] & 2))
		*(unsigned*)(sub + 8) |= 2u;
	return 0;
	}

// The assertion-report hook and the six pure report rows. Each row's whole
// body is `report(0xce, file, line, 0, expression)` behind the guarded int3,
// and the guard dereferences the non-null [0x101041b0] word, so the hook is
// the only observable.
static NxAssertReportFn gNxAssertReport = nullptr;

void nxSetAssertReport(NxAssertReportFn fn)
	{
	gNxAssertReport = fn;
	}

static void nxAssertReport(unsigned file, unsigned line, unsigned expression)
	{
	if(gNxAssertReport)
		gNxAssertReport(0xceu, file, line, 0u, expression);
	}

void nxAssertReport0364() { nxAssertReport(0x10105ba8u, 0x17fu, 0x10105d4cu); }
void nxAssertReport0408() { nxAssertReport(0x10105ba8u, 0x28du, 0x10106064u); }
void nxAssertReport0410() { nxAssertReport(0x10105ba8u, 0x293u, 0x1010609cu); }
void nxAssertReport3750() { nxAssertReport(0x101160ccu, 0x0f6u, 0x101163acu); }
void nxAssertReport3754() { nxAssertReport(0x101160ccu, 0x106u, 0x10116418u); }
void nxAssertReport3782() { nxAssertReport(0x101160ccu, 0x176u, 0x101165dcu); }
void nxAssertReport000362() { nxAssertReport(0x10105ba8u, 0x179u, 0x10105d0cu); }
void nxAssertReport000404() { nxAssertReport(0x10105ba8u, 0x281u, 0x10105ff4u); }
void nxAssertReport000406() { nxAssertReport(0x10105ba8u, 0x287u, 0x1010602cu); }
void nxAssertReport003736() { nxAssertReport(0x101160ccu, 0x0bcu, 0x10116268u); }
void nxAssertReport003740() { nxAssertReport(0x101160ccu, 0x0cbu, 0x101162f4u); }
void nxAssertReport003748() { nxAssertReport(0x101160ccu, 0x0edu, 0x10116374u); }
void nxAssertReport003752() { nxAssertReport(0x101160ccu, 0x0fdu, 0x101163e4u); }
void nxAssertReport003756() { nxAssertReport(0x101160ccu, 0x111u, 0x1011644cu); }
void nxAssertReport003758() { nxAssertReport(0x101160ccu, 0x11bu, 0x10116488u); }
void nxAssertReport003760() { nxAssertReport(0x101160ccu, 0x123u, 0x101164c4u); }
void nxAssertReport003762() { nxAssertReport(0x101160ccu, 0x12du, 0x101164f8u); }
void nxAssertReport003764() { nxAssertReport(0x101160ccu, 0x134u, 0x1011652cu); }
void nxAssertReport003766() { nxAssertReport(0x101160ccu, 0x13eu, 0x10116564u); }
void nxAssertReport003884() { nxAssertReport(0x1011681cu, 0x03au, 0x10116858u); }

// The word member: the row's return is the zero-extended word because the
// unlock helper leaves eax at 1 before the `mov ax,si` mask.
unsigned nxLockedWordRead(void* self, unsigned lockOff, unsigned fieldOff,
	unsigned dataOff)
	{
	(void) lockOff;
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned short w;
	memcpy(&w, field + dataOff, 2);
	return static_cast<unsigned>(w);
	}

// phys_fn_003708 (0x8b420): the gated locked reader. The image's gate byte is
// non-zero, so the shipped path is the work arm; the mirror lets the harness
// drive the report arm too.
static unsigned char gNxGate3708 = 1u;

void nxSetGate3708(unsigned char on)
	{
	gNxGate3708 = on;
	}

unsigned nxGuardedField3708(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	if(!gNxGate3708)
		{
		if(gNxAssertReport)
			gNxAssertReport(0xceu, 0x101160ccu, 0x4cu, 0u, 0x10116170u);
		return 0u;
		}
	unsigned char* field = *reinterpret_cast<unsigned char**>(p + 0x14);
	unsigned r;
	memcpy(&r, field + 0x24, 4);
	unsigned v;
	memcpy(&v, reinterpret_cast<const unsigned char*>(static_cast<size_t>(r)) + 0x38, 4);
	return v;
	}

// phys_fn_000392 (0xd660): the lock probe. The recursive acquire behind
// 002364 succeeds when the lock's owner word already holds the id the query
// stub reports; the acquire then re-stores the same id, and the release is a
// no-op under the bound stubs.
static unsigned gNxLockOwner = 0x2222u;

void nxSetLockOwner(unsigned id)
	{
	gNxLockOwner = id;
	}

unsigned char nxLockProbe0392(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* lock = *reinterpret_cast<unsigned char**>(p + 0xc);
	// the acquire reads the owner through the lock's first word
	unsigned char* inner = *reinterpret_cast<unsigned char**>(lock);
	unsigned owner;
	memcpy(&owner, inner + 0x1c, 4);
	if(owner != gNxLockOwner)
		return 0;
	return 1;
	}

// The acquire test shared by the mutex family: the owner word is reached
// through the lock's first word.
static bool nxTryAcquireLock(unsigned char* lock)
	{
	unsigned char* inner = *reinterpret_cast<unsigned char**>(lock);
	unsigned owner;
	memcpy(&owner, inner + 0x1c, 4);
	return owner == gNxLockOwner;
	}

// The mutex-guarded virtual dispatch family. The slot call site pushes one
// argument and does not clean it, so the slot is a __stdcall one-argument
// function with `this` in ecx.
void nxMutexVirtualEx(void* self, void* arg, unsigned slot, unsigned code,
	unsigned file, unsigned line, unsigned expression)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* lock = *reinterpret_cast<unsigned char**>(p + 0x10);
	if(!nxTryAcquireLock(lock))
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		return;
		}
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	void** vt = *reinterpret_cast<void***>(obj);
	typedef void (__stdcall* SlotFn)(void*);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[slot / 4]);
	fn(arg);
	// the release is a no-op under the bound stubs
	}

// The constant-argument virtual thunk family: the thunk pushes its constant
// and does not clean it, so the slot is a __stdcall one-argument function.
void nxVtConstEx(void* self, unsigned slot, unsigned arg)
	{
	void** vt = *reinterpret_cast<void***>(self);
	typedef void (__stdcall* SlotFn)(unsigned);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[slot / 4]);
	fn(arg);
	}

// phys_fn_002390 (0x5b910): the __cdecl slot takes one argument.
void nxVtCall2390(void* self)
	{
	unsigned char* obj = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(self) + 4);
	// the target is a plain callback field at +0xc, not a vtable entry
	void* target;
	memcpy(&target, obj + 0xc, 4);
	unsigned arg;
	memcpy(&arg, obj + 0x10, 4);
	typedef void (__cdecl* Fn)(unsigned);
	reinterpret_cast<Fn>(target)(arg);
	}

// phys_fn_003924 (0x8ed50): the __stdcall slot takes two arguments.
void nxVtCall3924(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	void** vt = *reinterpret_cast<void***>(p);
	unsigned a, b;
	memcpy(&a, p + 0x24, 4);
	memcpy(&b, p + 0x28, 4);
	typedef void (__stdcall* Fn)(unsigned, unsigned);
	reinterpret_cast<Fn>(vt[0xc / 4])(a, b);
	}

// phys_fn_001965 (0x4c000): three arguments, callee-cleaned.
void nxVtCall1965(void* unused, void* obj)
	{
	(void) unused;
	void** vt = *reinterpret_cast<void***>(obj);
	typedef void (__stdcall* Fn)(unsigned, unsigned, unsigned);
	reinterpret_cast<Fn>(vt[0x2c / 4])(static_cast<unsigned>(
		reinterpret_cast<size_t>(unused)), 0xff00ffffu, 0u);
	}

// phys_fn_001237 (0x24db0): the slot takes `this` in ecx and no stack
// arguments, which is the __fastcall layout for a one-argument function.
unsigned nxLockedVtCall1237(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	void** vt = *reinterpret_cast<void***>(obj);
	typedef unsigned (__fastcall* SlotFn)(void*);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[0x44 / 4]);
	return fn(obj);
	}

// phys_fn_001119 (0x23c50): the same dispatch through slot +0x3c, float result.
float nxLockedVtCall1119(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	void** vt = *reinterpret_cast<void***>(obj);
	typedef float (__fastcall* SlotFn)(void*);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[0x3c / 4]);
	return fn(obj);
	}

// phys_fn_001022 (0x22970, ret 4): the per-element virtual dispatch loop.
void nxArrayVtCall1022(void* self, void* arg)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned* begin = *reinterpret_cast<unsigned**>(p + 0xe0);
	unsigned* end = *reinterpret_cast<unsigned**>(p + 0xe4);
	const unsigned n = static_cast<unsigned>(
		reinterpret_cast<unsigned char*>(end) - reinterpret_cast<unsigned char*>(begin)) >> 2;
	for(unsigned i = 0; i < n; ++i)
		{
		unsigned char* elem = reinterpret_cast<unsigned char*>(begin[i]);
		void** vt = *reinterpret_cast<void***>(elem);
		typedef void (__stdcall* Fn)(void*);
		reinterpret_cast<Fn>(vt[0xc / 4])(arg);
		}
	}

// phys_fn_002142 (0x532b0, ret 4): the scalar deleting destructor. The free
// arm goes through the allocator singleton's slot +0x14.
static NxAllocFreeFn gNxAllocFree = nullptr;

void nxSetAllocFree(NxAllocFreeFn fn)
	{
	gNxAllocFree = fn;
	}

void* nxScalarDeletingDtor2142(void* self, unsigned flags)
	{
	unsigned vtable = 0x1010829cu;
	memcpy(self, &vtable, 4);
	if(flags & 1u)
		{
		if(gNxAllocFree)
			gNxAllocFree(self);
		}
	return self;
	}

void nxMutexApplyGroupEx(void* self, unsigned arg, unsigned code, unsigned file,
	unsigned line, unsigned expression)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* lock = *reinterpret_cast<unsigned char**>(p + 0x10);
	if(!nxTryAcquireLock(lock))
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		return;
		}
	ShapeBase* obj = reinterpret_cast<ShapeBase*>(*reinterpret_cast<void**>(p + 0x18));
	obj->nxApplyGroup(static_cast<unsigned short>(arg));
	}

void nxMutexNoopEx(void* self, unsigned code, unsigned file, unsigned line,
	unsigned expression)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* lock = *reinterpret_cast<unsigned char**>(p + 0x10);
	if(!nxTryAcquireLock(lock))
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		return;
		}
	// 004248 is a bare `ret 4`, so the work arm has no observable beyond the
	// release.
	}

// The lock-bracketed vtable call with a caller-chosen slot and no arguments.
unsigned nxLockedVtCallNoArg(void* self, unsigned slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	void** vt = *reinterpret_cast<void***>(obj);
	typedef unsigned (__fastcall* SlotFn)(void*);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[slot / 4]);
	return fn(obj);
	}

// The same bracket where the slot takes one argument as well.
void nxLockedVtCallArg(void* self, unsigned slot, void* arg)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	void** vt = *reinterpret_cast<void***>(obj);
	typedef void (__stdcall* SlotFn)(void*);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[slot / 4]);
	fn(arg);
	}

// The report-once dispatch rows.
void nxOnceReportVtEx(void* self, unsigned char* gate, unsigned slot,
	unsigned nargs, unsigned a1, unsigned a2, unsigned code, unsigned file,
	unsigned line, unsigned expression)
	{
	if(*gate == 0)
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		*gate = 1;
		}
	void** vt = *reinterpret_cast<void***>(self);
	if(nargs <= 1)
		{
		typedef void (__stdcall* Fn1)(unsigned);
		reinterpret_cast<Fn1>(vt[slot / 4])(a1);
		}
	else
		{
		typedef void (__stdcall* Fn2)(unsigned, unsigned);
		reinterpret_cast<Fn2>(vt[slot / 4])(a1, a2);
		}
	}

// phys_fn_003938 (0x8eec0, ret 4): the deleting destructor that also calls a
// process-wide global slot with no arguments.
static void (*gNxGlobalHook3938)(void) = nullptr;

void nxSetGlobalHook3938(void (*fn)(void))
	{
	gNxGlobalHook3938 = fn;
	}

void* nxDtorWithGlobal3938(void* self, unsigned flags)
	{
	unsigned vtable = 0x10117920u;
	memcpy(self, &vtable, 4);
	if(gNxGlobalHook3938)
		gNxGlobalHook3938();
	if(flags & 1u)
		{
		if(gNxAllocFree)
			gNxAllocFree(self);
		}
	return self;
	}

// The guarded store family: the 004334 guard without the clamp tail.
void nxGuardedStoreEx(void* self, unsigned arg, unsigned fieldOff, unsigned code,
	unsigned file, unsigned line, unsigned expression)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	if((p[0x2c] & 0x18) == 0x10)
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		return;
		}
	memcpy(p + fieldOff, &arg, 4);
	}

// The lock-first direct-call family.
void nxMutexWorkEx(void* self, unsigned lockOff, unsigned objOff,
	const unsigned* args, unsigned nargs, int workKind, unsigned baseOff,
	unsigned nwords, unsigned code, unsigned file, unsigned line,
	unsigned expression)
	{
	(void) nargs;
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* lock = *reinterpret_cast<unsigned char**>(p + lockOff);
	if(!nxTryAcquireLock(lock))
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		return;
		}
	if(workKind == 0)
		return;
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + objOff);
	if(workKind == 1)
		{
		memcpy(obj + baseOff, &args[0], 4);
		}
	else if(workKind == 2)
		{
		for(unsigned k = 0; k < nwords; ++k)
			memcpy(obj + baseOff + 4u * k, &args[k], 4);
		}
	else if(workKind == 3)
		{
		const unsigned char* src = reinterpret_cast<const unsigned char*>(
			static_cast<size_t>(args[0]));
		for(unsigned k = 0; k < nwords; ++k)
			memcpy(obj + baseOff + 4u * k, src + 4u * k, 4);
		}
	else if(workKind == 4)
		{
		unsigned v;
		memcpy(&v, obj + baseOff, 4);
		v = args[1] ? (v | args[0]) : (v & ~args[0]);
		memcpy(obj + baseOff, &v, 4);
		}
	}

// phys_fn_001024 (0x229b0, ret 0xc): the guarded three-argument dispatch loop.
unsigned char nxArrayVtCall3Args1024(void* self, unsigned a1, unsigned a2,
	unsigned a3)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned* begin = *reinterpret_cast<unsigned**>(p + 0xe0);
	unsigned* end = *reinterpret_cast<unsigned**>(p + 0xe4);
	const unsigned n = static_cast<unsigned>(
		reinterpret_cast<unsigned char*>(end) - reinterpret_cast<unsigned char*>(begin)) >> 2;
	for(unsigned i = 0; i < n; ++i)
		{
		unsigned char* elem = reinterpret_cast<unsigned char*>(begin[i]);
		if(elem[0xde] & 7)
			continue;
		void** vt = *reinterpret_cast<void***>(elem);
		typedef unsigned char (__stdcall* Fn)(unsigned, unsigned, unsigned);
		if(reinterpret_cast<Fn>(vt[0x10 / 4])(a1, a2, a3) == 0)
			return 0;
		}
	return 1;
	}

// phys_fn_004491 (0xb0f10): the lock-bracketed global-flag read. The helper
// 004083 tests the data word at [0x10127180] and returns 0 or 1.
static unsigned gNxGlobalFlag4491 = 0;

void nxSetGlobalFlag4491(unsigned value)
	{
	gNxGlobalFlag4491 = value;
	}

unsigned char nxLockedGlobalFlag4491(void* self)
	{
	(void) self;
	return gNxGlobalFlag4491 != 0u ? 1u : 0u;
	}

// phys_fn_003950 (0x8f0d0): lock [self+0x10], unlock, return self. The lock
// pair brackets the whole body and has no other observable.
void* nxLockedSelf3950(void* self)
	{
	return self;
	}

// phys_fn_000418 (0xda50): under the lock at [self+0x10], read the word at
// [[self+0x24]+0x55c] and return it.
unsigned nxFieldRead0418(void* self)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* owner = *reinterpret_cast<const unsigned char* const*>(p + 0x24);
	unsigned v;
	memcpy(&v, owner + 0x55c, 4);
	return v;
	}

// phys_fn_000094 (0x2f30, ret 4): the orientation quaternion. With a record it
// copies the stored (x, y, z, w) at record+0x5c. Without one it derives the
// quaternion from the cached matrix at [self+0x14]+0x20 using the
// largest-diagonal method: the trace arm when the trace is non-negative, else
// the case selected by the largest of m00/m11/m22 (index stride 4 floats).
float* nxOrientation0094(void* self, float* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* body = *reinterpret_cast<const unsigned char* const*>(p + 0x14);
	const unsigned char* rec = *reinterpret_cast<const unsigned char* const*>(body + 8);
	if(rec != nullptr)
		{
		memcpy(out, rec + 0x5c, 16);
		return out;
		}
	const float* m = reinterpret_cast<const float*>(body + 0x20);
	const double m00 = m[0], m01 = m[1], m02 = m[2];
	const double m10 = m[3], m11 = m[4], m12 = m[5];
	const double m20 = m[6], m21 = m[7], m22 = m[8];
	double x, y, z, w;
	const double trace = m00 + m11 + m22;
	if(!(trace < 0.0))
		{
		const double s = sqrt(trace + 1.0);
		const double inv = 0.5 / s;
		x = (m21 - m12) * inv;
		y = (m02 - m20) * inv;
		z = (m10 - m01) * inv;
		w = 0.5 * s;
		}
	else
		{
		unsigned edx = (m11 > m00) ? 1u : 0u;
		if(m22 > m[edx * 4u]) edx = 2u;
		if(edx == 0u)
			{
			const double s = sqrt(m00 - m11 - m22 + 1.0);
			const double inv = 0.5 / s;
			x = 0.5 * s;
			y = (m10 + m01) * inv;
			z = (m20 + m02) * inv;
			w = (m21 - m12) * inv;
			}
		else if(edx == 1u)
			{
			const double s = sqrt(m11 - m00 - m22 + 1.0);
			const double inv = 0.5 / s;
			x = (m10 + m01) * inv;
			y = 0.5 * s;
			z = (m21 + m12) * inv;
			w = (m02 - m20) * inv;
			}
		else if(edx == 2u)
			{
			const double s = sqrt(m22 - m00 - m11 + 1.0);
			const double inv = 0.5 / s;
			x = (m20 + m02) * inv;
			y = (m21 + m12) * inv;
			z = 0.5 * s;
			w = (m10 - m01) * inv;
			}
		else
			{
			x = y = z = w = 0.0;
			}
		}
	out[0] = static_cast<float>(x);
	out[1] = static_cast<float>(y);
	out[2] = static_cast<float>(z);
	out[3] = static_cast<float>(w);
	return out;
	}

// phys_fn_000132 (0x46c0, ret 4): quaternion at record+0x5c (x, y, z, w) to a
// 3x3 rotation matrix in out[0..8]; when the record pointer is null it copies
// the cached 36 bytes at [self+0x14]+0x20 instead. Returns out.
float* nxQuatToMatrix0132(void* self, float* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* body = *reinterpret_cast<const unsigned char* const*>(p + 0x14);
	const unsigned char* rec = *reinterpret_cast<const unsigned char* const*>(body + 8);
	if(rec == nullptr)
		{
		memcpy(out, body + 0x20, 36);
		return out;
		}
	float q[4];
	memcpy(&q[0], rec + 0x5c, 4);
	memcpy(&q[1], rec + 0x60, 4);
	memcpy(&q[2], rec + 0x64, 4);
	memcpy(&q[3], rec + 0x68, 4);
	nxQuatToMatrix9(q, out);
	return out;
	}

// phys_fn_000046 (0x24c0, ret 4): gather the descriptor record at
// [[self+0x14]+8] into out. Nine dwords come from record+0xdc, three more
// from record+0x100, then the tail fields; out[25], out[27] and out[28] are
// the square roots of the floats at record+0xd8, +0xd0 and +0xd4. The row
// returns false (and writes nothing) when the record pointer is null. The
// lock pair it calls around the body is a no-op for this output.
bool nxGatherDescriptor0046(void* self, unsigned* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* body = *reinterpret_cast<const unsigned char* const*>(p + 0x14);
	const unsigned char* rec = *reinterpret_cast<const unsigned char* const*>(body + 8);
	if(rec == nullptr)
		return false;
	unsigned char* o = reinterpret_cast<unsigned char*>(out);
	memcpy(o + 0x00, rec + 0xdc, 36);		// out[0..8]
	memcpy(o + 0x24, rec + 0x100, 12);		// out[9..11]
	static const unsigned kFields[13] = {
		0x18c, 0x190, 0x194, 0x188, 0x6c, 0x70, 0x74,
		0x78, 0x7c, 0x80, 0x84, 0xb8, 0xbc };
	for(unsigned i = 0; i < 13; ++i)
		{
		unsigned v; memcpy(&v, rec + kFields[i], 4);
		memcpy(o + (12u + i) * 4u, &v, 4);		// out[12..24]
		}
	float d8; memcpy(&d8, rec + 0xd8, 4);
	const float r8 = static_cast<float>(sqrt(static_cast<double>(d8)));
	memcpy(o + 25u * 4u, &r8, 4);				// out[25]
	unsigned f26; memcpy(&f26, rec + 0x10c, 4);
	memcpy(o + 26u * 4u, &f26, 4);				// out[26]
	float d0; memcpy(&d0, rec + 0xd0, 4);
	const float r0 = static_cast<float>(sqrt(static_cast<double>(d0)));
	memcpy(o + 27u * 4u, &r0, 4);				// out[27]
	float d4; memcpy(&d4, rec + 0xd4, 4);
	const float r4 = static_cast<float>(sqrt(static_cast<double>(d4)));
	memcpy(o + 28u * 4u, &r4, 4);				// out[28]
	unsigned f29; memcpy(&f29, rec + 0x110, 4);
	memcpy(o + 29u * 4u, &f29, 4);				// out[29]
	return true;
	}

// phys_fn_002156 (0x538e0, ret 4): nine doubles at [self+0x18] (8-byte stride)
// to nine floats at out. The row's interleaved fld/fstp schedule reads
// d0,d3,d6 / d1,d4,d7 / d2,d5,d8 and writes +0,+0xc,+0x18 / +4,+0x10,+0x1c /
// +8,+0x14,+0x20, which is exactly dst[k] = (float)src[k].
void nxDoubleToFloat9_2156(void* self, float* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	for(unsigned k = 0; k < 9; ++k)
		{
		double d;
		memcpy(&d, p + 0x18 + k * 8u, sizeof(d));
		const float f = static_cast<float>(d);
		memcpy(reinterpret_cast<unsigned char*>(out) + k * 4u, &f, sizeof(f));
		}
	}

// phys_fn_001030 (0x22bf0, ret 4): aggregate the local AABBs of the shape list
// at [self+0xe0]..[self+0xe4] into out. Seeded FLT_MAX / -FLT_MAX; each shape
// contributes the 6-dword record selected by the PLANE slot-8 row, merged
// with min on the low triple and max on the high triple.
void nxAggregateAABB1030(void* self, float* out)
	{
	const unsigned kMax = 0x7f7fffffu;		//  1.7014117e38
	const unsigned kMin = 0xff7fffffu;		// -1.7014117e38
	unsigned seed[6] = { kMax, kMax, kMax, kMin, kMin, kMin };
	memcpy(out, seed, sizeof(seed));
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned* it = *reinterpret_cast<unsigned**>(p + 0xe0);
	unsigned* end = *reinterpret_cast<unsigned**>(p + 0xe4);
	const unsigned n = static_cast<unsigned>(
		(reinterpret_cast<unsigned char*>(end) -
		 reinterpret_cast<unsigned char*>(it)) >> 2);
	for(unsigned i = 0; i < n; ++i)
		{
		unsigned char* shape = reinterpret_cast<unsigned char*>(it[i]);
		unsigned rec[6];
		memcpy(rec, seed, sizeof(rec));
		reinterpret_cast<PlaneShape*>(shape)->nxPlaneIndexed6_1267(rec);
		float f[6];
		memcpy(f, rec, sizeof(f));
		if(!(out[0] <= f[0])) out[0] = f[0];
		if(!(out[1] <= f[1])) out[1] = f[1];
		if(!(out[2] <= f[2])) out[2] = f[2];
		if(out[3] <= f[3]) out[3] = f[3];
		if(out[4] <= f[4]) out[4] = f[4];
		if(out[5] <= f[5]) out[5] = f[5];
		}
	}

// ---------------------------------------------------------------------------
// Actor slate 5: the sleep-chain readers.

// phys_fn_000713 (0x15d50): recursive path compression over the record
// chain -- each record caches its group root at +0x1e8, a self-pointing
// cache naming the root. Recursion terminates on the self-parented node.
unsigned nxBodyRecordFixRoot(void* rec)
	{
	unsigned r = reinterpret_cast<unsigned>(rec);
	unsigned c = *reinterpret_cast<unsigned*>(r + 0x1e8);
	if(r != c)
		{
		unsigned root = nxBodyRecordFixRoot(reinterpret_cast<void*>(c));
		*reinterpret_cast<unsigned*>(r + 0x1e8) = root;
		}
	return *reinterpret_cast<unsigned*>(r + 0x1e8);
	}

// phys_fn_000744 (0x16e30): compress the record's chain to its root, then
// walk the +0x1fc list answering whether EVERY node's word at +0x84 reads
// zero -- the group-wide sleep test. A null root also answers true.
bool nxBodyRecordChainSettled(void* rec)
	{
	unsigned d = reinterpret_cast<unsigned>(rec);
	unsigned cache = *reinterpret_cast<unsigned*>(d + 0x1e8);
	if(d != cache)
		{
		unsigned root = nxBodyRecordFixRoot(reinterpret_cast<void*>(cache));
		*reinterpret_cast<unsigned*>(d + 0x1e8) = root;
		}
	unsigned node = *reinterpret_cast<unsigned*>(d + 0x1e8);
	while(node != 0)
		{
		float v = *reinterpret_cast<const float*>(node + 0x84);
		double dv = v;
		if(dv > 0.0)
			return false;
		node = *reinterpret_cast<unsigned*>(node + 0x1fc);
		}
	return true;
	}

// phys_fn_000062 (slot 67, 0x2950): guarded; the group sleep test over the
// body's nested record; TRUE when the record itself is null.
bool nxActorChainSettled(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	bool r = rec == 0
		? true
		: nxBodyRecordChainSettled(reinterpret_cast<void*>(rec));
	nxSceneGuardLeave(scene);
	return r;
	}

// ---------------------------------------------------------------------------
// Actor slate 6: the center-of-mass local frame getters. Both answer from
// record+0xdc (a nine-word rotation followed by a three-word translation at
// +0x100); a STATIC actor (null record) reports kind 1 and hands back an
// identity-plus-zero pose instead.
const char* const	nxMsgCMassLocalPoseStatic =
	"Actor::getCMassLocalPose: Cannot be called on a static actor!";
const char* const	nxMsgCMassLocalOrientationStatic =
	"Actor::getCMassLocalOrientation: Cannot be called on a static actor!";

// phys_fn_000096 (slot 29, 0x3140): twelve words into `out` -- rotation
// then translation -- identity and zero for a static actor.
void* nxActorGetCMassLocalPose(void* self, void* out)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, 0x2f2, 0,
			nxMsgCMassLocalPoseStatic);
		unsigned* o = reinterpret_cast<unsigned*>(out);
		o[0] = 0x3f800000u;
		o[1] = 0; o[2] = 0; o[3] = 0;
		o[4] = 0x3f800000u;
		o[5] = 0; o[6] = 0; o[7] = 0;
		o[8] = 0x3f800000u;
		o[9] = 0; o[10] = 0; o[11] = 0;
		nxSceneGuardLeave(scene);
		return out;
		}
	memcpy(out, reinterpret_cast<const void*>(rec + 0xdc), 48);
	nxSceneGuardLeave(scene);
	return out;
	}

// phys_fn_000100 (slot 31, 0x32a0): nine-word orientation into `out`;
// static actors get the shipped identity table (.rdata 0x10122078) after
// the line-0x301 warning.
void* nxActorGetCMassLocalOrientation(void* self, void* out)
	{
	static const unsigned kIdentity3x3[9] =
		{ 0x3f800000u, 0, 0, 0, 0x3f800000u, 0, 0, 0, 0x3f800000u };
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, 0x301, 0,
			nxMsgCMassLocalOrientationStatic);
		memcpy(out, kIdentity3x3, sizeof(kIdentity3x3));
		nxSceneGuardLeave(scene);
		return out;
		}
	memcpy(out, reinterpret_cast<const void*>(rec + 0xdc), 36);
	nxSceneGuardLeave(scene);
	return out;
	}

// ---------------------------------------------------------------------------
// Actor slate 7: five guarded three-word readers sharing one shape -- read
// guard, record+offset into `out` when a record backs the actor, otherwise
// report kind 1 and fill from defaults. The .data-backed defaults copy the
// triple at 0x10123c1c, zero-initialized in a fresh image; the C++
// transcribes those zeros directly rather than dereferencing an image
// absolute that only means something inside the oracle's own mapping.
const char* const	nxMsgCMassLocalPositionStatic =
	"Actor::getCMassLocalPosition: Cannot be called on a static actor!";
const char* const	nxMsgMassSpaceInertiaStatic =
	"Actor::getMassSpaceInertiaTensorVal: Cannot be called on a static actor!";
const char* const	nxMsgLinearVelocityDynamic =
	"Actor::getLinearVelocity: Actor must be dynamic!";
const char* const	nxMsgAngularVelocityDynamic =
	"Actor::getAngularVelocity: Actor must be dynamic!";
const char* const	nxMsgLinearMomentumStatic =
	"Actor::getLinearMomentumVal: Cannot be called on a static actor!";

static void nxActorFillThreeWords(void* self, unsigned base, int line,
	const char* msg, bool defaultsFromData, void* out)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	unsigned* o = reinterpret_cast<unsigned*>(out);
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, line, 0, msg);
		if(defaultsFromData)
			{
			o[0] = 0;						// [.data 0x10123c1c] triple:
			o[1] = 0;						// zero-initialized in any
			o[2] = 0;						// fresh image
			}
		else
			{
			o[0] = 0;
			o[1] = 0;
			o[2] = 0;
			}
		nxSceneGuardLeave(scene);
		return;
		}
	o[0] = *reinterpret_cast<const unsigned*>(rec + base);
	o[1] = *reinterpret_cast<const unsigned*>(rec + base + 4);
	o[2] = *reinterpret_cast<const unsigned*>(rec + base + 8);
	nxSceneGuardLeave(scene);
	}

// phys_fn_000098 (slot 30, 0x3200): getCMassLocalPosition -- [+0x100].
void nxActorGetCMassLocalPosition(void* self, void* out)
	{
	nxActorFillThreeWords(self, 0x100, 0x2fa,
		nxMsgCMassLocalPositionStatic, true, out);
	}

// phys_fn_000102 (slot 44, 0x3310): getMassSpaceInertiaTensorVal --
// the inertia diagonal at [+0x18c], line 0x328.
void nxActorGetMassSpaceInertia(void* self, void* out)
	{
	nxActorFillThreeWords(self, 0x18c, 0x328,
		nxMsgMassSpaceInertiaStatic, true, out);
	}

// phys_fn_000104 (slot 47, 0x33b0): getLinearVelocity -- [+0x6c], inline
// zeros on the static arm, line 0x343.
void nxActorGetLinearVelocity(void* self, void* out)
	{
	nxActorFillThreeWords(self, 0x6c, 0x343,
		nxMsgLinearVelocityDynamic, false, out);
	}

// phys_fn_000106 (slot 48, 0x3440): getAngularVelocity -- [+0x78], inline
// zeros, line 0x34a.
void nxActorGetAngularVelocity(void* self, void* out)
	{
	nxActorFillThreeWords(self, 0x78, 0x34a,
		nxMsgAngularVelocityDynamic, false, out);
	}

// phys_fn_000108 (slot 52, 0x34d0): getLinearMomentumVal -- mass at
// [+0x188] times the velocity words [+0x6c/70/74] in natural order; zeros
// from .data on the static arm, line 0x353.
void nxActorGetLinearMomentum(void* self, void* out)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	unsigned* o = reinterpret_cast<unsigned*>(out);
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, 0x353, 0,
			nxMsgLinearMomentumStatic);
		o[0] = 0;
		o[1] = 0;
		o[2] = 0;
		nxSceneGuardLeave(scene);
		return;
		}
	float m = *reinterpret_cast<const float*>(rec + 0x188);
	float v6c = *reinterpret_cast<const float*>(rec + 0x6c);
	float v70 = *reinterpret_cast<const float*>(rec + 0x70);
	float v74 = *reinterpret_cast<const float*>(rec + 0x74);
	double dm = m;
	float r0 = static_cast<float>(dm * v6c);
	float r1 = static_cast<float>(dm * v70);
	float r2 = static_cast<float>(dm * v74);
	memcpy(o, &r0, 4);
	memcpy(o + 1, &r1, 4);
	memcpy(o + 2, &r2, 4);
	nxSceneGuardLeave(scene);
	}

// ---------------------------------------------------------------------------
// Actor slate 8: the group WRITER and two primitives.

// phys_fn_000112 (slot 85, 0x35b0): setGroup -- the word argument into
// body+0x1c under the write guard; line 0x3cd on a failed upgrade. The
// write counterpart to slate 1's group reader.
void nxActorSetGroupWord(void* self, unsigned group)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* ctx = *reinterpret_cast<void**>(a + 0xc);
	if(!nxSceneGuardWriteTry(ctx))
		{
		nxReport(2, nxSourceFileNpActorCpp, 0x3cd, 0,
			nxMsgWriteLockStillAcquired);
		return;
		}
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	*reinterpret_cast<NxU16*>(body + 0x1c) = static_cast<NxU16>(group);
	nxSceneGuardLeave(ctx);
	}

// phys_fn_000004 (0x1070): sub-object virtual forwarder -- [this+0x10]
// names a sub-object whose vtable slot +0x18 receives the call with the
// original stack argument; a null sub-object pointer returns without
// touching anything observable.
unsigned nxForwardSubobjectCall(void* self, void* arg)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	unsigned sub = *reinterpret_cast<unsigned*>(a + 0x10);
	if(sub == 0)
		return 0;
	unsigned vtable = *reinterpret_cast<unsigned*>(sub);
	typedef unsigned (__thiscall* NxSlot18Fn)(void* sub, void* arg);
	NxSlot18Fn fn = reinterpret_cast<NxSlot18Fn>(
		*reinterpret_cast<unsigned*>(vtable + 0x18));
	return fn(reinterpret_cast<void*>(sub), arg);
	}

// phys_fn_000012 (0x1430): id-allocation primitive over {counter@+0x00,
// freelist begin@+0x04, cursor@+0x08}: a non-empty list pops its LAST
// dword and shrinks the cursor; an exhausted list returns the counter and
// increments it.
unsigned nxIdAllocNext(void* container)
	{
	unsigned c = reinterpret_cast<unsigned>(container);
	unsigned count = (*reinterpret_cast<const unsigned*>(c + 8)
		- *reinterpret_cast<const unsigned*>(c + 4)) >> 2;
	if(count != 0)
		{
		unsigned v = *reinterpret_cast<const unsigned*>(
			*reinterpret_cast<const unsigned*>(c + 4) + count * 4 - 4);
		*reinterpret_cast<unsigned*>(c + 8) += -4u;
		return v;
		}
	unsigned old = *reinterpret_cast<const unsigned*>(c);
	*reinterpret_cast<unsigned*>(c) = old + 1;
	return old;
	}

// ---------------------------------------------------------------------------
// Actor slate 10: sibling pool-class lifecycle rows.

// phys_fn_002328 (0x5a470): chained deleting destructor -- member vptr at
// +8 restored through two third tables (0x101087a8 then 0x10108798),
// primary vptr 0x1010878c installed, then ADAPTER release when flagged.
void nxChainedDeletingDtor(void* self, unsigned flags)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	*reinterpret_cast<unsigned**>(a + 8) =
		reinterpret_cast<unsigned*>(0x101087a8u);
	*reinterpret_cast<unsigned**>(a + 8) =
		reinterpret_cast<unsigned*>(0x10108798u);
	*reinterpret_cast<unsigned**>(a) =
		reinterpret_cast<unsigned*>(0x1010878cu);
	if(flags & 1)
		nxGetSdkAllocator()->free(self);
	}

// phys_fn_002322 (0x5a230): member-table adjustor thunk -- `sub ecx,8` onto
// the pool base, tail-call to the chained deleting destructor (002328).
void nxChainedDeletingDtorThunk(void* memberThis, unsigned flags)
	{
	nxChainedDeletingDtor(
		reinterpret_cast<unsigned char*>(memberThis) - 8, flags);
	}

// phys_fn_002312 (0x5a080): third pool class deleting destructor -- vptr
// 0x1010878c installed, then LINKED-CRT release when flagged.
void nxPoolDeletingDtor78c(void* self, unsigned flags)
	{
	*reinterpret_cast<unsigned**>(self) =
		reinterpret_cast<unsigned*>(0x1010878cu);
	if(flags & 1)
		::free(self);
	}

// phys_fn_002320 (0x5a1e0): destroys the node LIST cached at this+0x5a8.
// Per iteration: cache's +0x30 link is read and zeroed, the cache node is
// deleted through its OWN virtual slot 0 with argument 1, the cache field
// is nulled, then refilled with the saved link -- looping until it reads
// null.
void nxDestroyCachedList(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	unsigned cache = *reinterpret_cast<unsigned*>(a + 0x5a8);
	while(cache != 0)
		{
		unsigned next = *reinterpret_cast<unsigned*>(cache + 0x30);
		*reinterpret_cast<unsigned*>(cache + 0x30) = 0;
		unsigned vtable = *reinterpret_cast<unsigned*>(cache);
		unsigned slot0 = *reinterpret_cast<const unsigned*>(vtable);
		typedef void (__thiscall* NxNodeKillFn)(void* node, unsigned arg);
		NxNodeKillFn kill = reinterpret_cast<NxNodeKillFn>(slot0);
		kill(reinterpret_cast<void*>(cache), 1);
		*reinterpret_cast<unsigned*>(a + 0x5a8) = 0;
		cache = next;
		*reinterpret_cast<unsigned*>(a + 0x5a8) = next;
		}
	}

// phys_fn_002352 (0x5b610): adjustor thunk -- `add ecx,0x28`, tail-call to
// phys_fn_004846 at 0xb4f50 (SdkContainer::empty). The thunk's return is
// whatever empty() leaves in eax -- unpinned, like the null path of
// phys_fn_000004. The first spelling hard-coded the oracle's absolute
// 0x100b4f50: in the candidate process that is oracle code reached with
// ecx invalid -- the differential caught it as c0000005 at 0xb4f53.
void nxContainerAddThunk(void* innerThis)
	{
	reinterpret_cast<SdkContainer*>(
		reinterpret_cast<unsigned>(innerThis) + 0x28)->empty();
	}

// phys_fn_002379 (0x5b860): virtual slot-1 wrapper -- invokes arg's own
// virtual slot 1 and always returns false.
bool nxVirtualSlot1Wrapper(void* arg)
	{
	unsigned a = reinterpret_cast<unsigned>(arg);
	unsigned vtable = *reinterpret_cast<unsigned*>(a);
	typedef void (__thiscall* NxSlot1Fn)(void* self);
	NxSlot1Fn fn = reinterpret_cast<NxSlot1Fn>(
		*reinterpret_cast<unsigned*>(vtable + 4));
	fn(arg);
	return false;
	}

// ---------------------------------------------------------------------------
// Actor slate 9: five small rows.

// phys_fn_000080 (0x2c90): readBodyFlag -- ([record+0x10c] byte AND mask)
// under the READ guard; kind-1 warning ("readBodyFlag: Actor must be
// dynamic!") and false on a static actor.
const char* const	nxMsgReadBodyFlagDynamic =
	"Actor::readBodyFlag: Actor must be dynamic!";
bool nxActorReadBodyFlag(void* self, unsigned mask)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = body != 0
		? *reinterpret_cast<unsigned*>(body + 8) : 0u;
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, 0x1e3, 0,
			nxMsgReadBodyFlagDynamic);
		nxSceneGuardLeave(scene);
		return false;
		}
	unsigned flagByte = *reinterpret_cast<const unsigned char*>(rec + 0x10c);
	nxSceneGuardLeave(scene);
	return (flagByte & mask) != 0;
	}

// phys_fn_002408 (0x5baa0): the member subobject's scalar-deleting
// destructor -- third table again, then release through the LINKED CRT
// (0x100f41f0) when flagged, exactly like phys_fn_000042.
void nxMemberDeletingDtor(void* self, unsigned flags)
	{
	*reinterpret_cast<unsigned**>(self) =
		reinterpret_cast<unsigned*>(0x101088b8u);
	if(flags & 1)
		::free(self);
	}

// phys_fn_002411 (0x5bbb0): the shapes-clear entry its deregistration
// callers use instead of a raw store: release slot=[arg+0x104] through the
// pool header at this+0x40 (so the release arm's own +0x30-family offsets
// land at this+0x70 family), then zero shapes[slot] in the array named by
// [this+0x80].
void nxSceneClearShapeSlot(void* scenePool, void* shapeArg)
	{
	unsigned s = reinterpret_cast<unsigned>(scenePool);
	unsigned slot = *reinterpret_cast<const unsigned*>(
		reinterpret_cast<unsigned>(shapeArg) + 0x104);
	nxSceneReleaseIndex(reinterpret_cast<unsigned char*>(scenePool) + 0x40,
		slot);
	unsigned shapes = *reinterpret_cast<unsigned*>(s + 0x80);
	*reinterpret_cast<unsigned*>(shapes + slot * 4) = 0;
	}

// phys_fn_002326 (0x5a440): deleting destructor installing vptr 0x10108798,
// adapter release through slot +0x14 when flagged.
void nxBoundDeletingDtor798(void* self, unsigned flags)
	{
	*reinterpret_cast<unsigned**>(self) =
		reinterpret_cast<unsigned*>(0x10108798u);
	if(flags & 1)
		nxGetSdkAllocator()->free(self);
	}

// phys_fn_002340 (0x5aa60): same shape for the second pool class, vptr
// 0x1010884c.
void nxBoundDeletingDtor84c(void* self, unsigned flags)
	{
	*reinterpret_cast<unsigned**>(self) =
		reinterpret_cast<unsigned*>(0x1010884cu);
	if(flags & 1)
		nxGetSdkAllocator()->free(self);
	}

// ---------------------------------------------------------------------------
// BoxShape. See ObjectModel.h for the row map.

BoxShape::BoxShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x0002187c..80
	{
	// vptr stores. The image stores the abstract wall at +0xe0 first
	// (0x00021885), the BOX primary table next (0x0002188f), then replaces
	// the wall with the facade's final twelve-slot table (0x00021895). The
	// wall store is a chained-construction intermediate; C++ installs both
	// final tables and it is never observable after the constructor returns.
	// The transcription writes the face-record pointer words once; the image
	// reaches record 5 through a walking pointer with identical effect.
	for(unsigned r = 0; r < 6; ++r)			// 0x000218a1..0x000218ee
		{
		mHull.mFaces[r].mCorners = 0;
		mHull.mFaces[r].mIndexListA = 0;
		mHull.mFaces[r].mIndexListB = 0;
		}

	// The embedded collision object: a fresh 0x1c-byte block through the SDK
	// allocator (0x000218f1..fe -- malloc slot, size 0x1c, flag 0), built by
	// phys_fn_001075 (0x00023580), whose body is phys_fn_001193 with the
	// box-family tables and which stores the box at BOTH +0x08 and +0x18.
	void* memory = nxGetSdkAllocator()->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObject(this)
		: 0;								// null arm: 0x0002190f
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00021911

	const float one = 1.0f;					// mov eax,0x3f800000 at 0x00021917
	mBase.mSentinelD0 = 2;					// OVERWRITES the base's sentinel: 0x0002191c
	mHull.mDims04[0] = one;					// 0x00021926
	mHull.mDims04[1] = one;					// 0x0002192c
	mHull.mDims04[2] = one;					// 0x00021932

	// The vertices and every face-record float word are written by nobody
	// here; a fresh box carries poison there until the face builder runs.
	}

// ---------------------------------------------------------------------------
// SphereShape. See ObjectModel.h for the row map.

SphereShape::SphereShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x000277bb..cf
	{
	mRadiusE0 = 0.0f;						// mov [esi+0xe0],0 at 0x000277da

	// The embedded collision object: a fresh 0x1c-byte block through the SDK
	// allocator (0x00027de4..f2), built by phys_fn_001193 itself -- the
	// GENERIC collision-object constructor, not a per-type variant -- with
	// the sphere stored at BOTH +0x08 and +0x18.
	void* memory = nxGetSdkAllocator()->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObject(this)
		: 0;								// null arm: 0x00027803
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00027805

	mBase.mSentinelD0 = 1;					// OVERWRITES the base's sentinel: 0x0002780b
	}

// ---------------------------------------------------------------------------
// SPHERE-table rows. See ObjectModel.h.

// phys_fn_001355 (0x000278a0), SPHERE-table slot 13.
bool SphereShape::nxSphereSaveState(void* record)
	{
	memcpy(reinterpret_cast<unsigned char*>(record) + 0x4c,
		&mRadiusE0, sizeof(mRadiusE0));		// mov [+eax+0x4c] at 0x000278aa
	return mBase.nxBaseSaveState(record);	// jmp 0x000256f0 at 0x000278b1
	}

// phys_fn_001365 (0x000279b0), SPHERE-table slot 11.
void SphereShape::nxSphereZeroCenterRadius(float* out) const
	{
	out[0] = 0.0f;							// xor edx,edx; three stores
	out[1] = 0.0f;
	out[2] = 0.0f;
	out[3] = mRadiusE0;						// mov ecx,[ecx+0xe0]
	}

// phys_fn_001363 (0x00027980), SPHERE-table slot 10.
void SphereShape::nxSphereCenterRadius(float* out) const
	{
	out[0] = mBase.mPose0C.mTranslation[0];	// +0x30
	out[1] = mBase.mPose0C.mTranslation[1];	// +0x34
	out[2] = mBase.mPose0C.mTranslation[2];	// +0x38
	out[3] = mRadiusE0;
	}

// phys_fn_001361 (0x00027930), SPHERE-table slot 9.
void SphereShape::nxSphereWorldAABB(float* out) const
	{
	const float* t = mBase.mPose0C.mTranslation;
	out[0] = t[0] - mRadiusE0;				// fsub chain, 0x00027961..6a
	out[1] = t[1] - mRadiusE0;
	out[2] = t[2] - mRadiusE0;
	out[3] = t[0] + mRadiusE0;				// fadd chain, 0x00027937..49
	out[4] = t[1] + mRadiusE0;
	out[5] = t[2] + mRadiusE0;
	}

// phys_fn_001367 (0x000279d0), SPHERE-table slot 8. The x87 dance (fchs,
// dup, fxch) stores the negated radius three times then the raw radius
// three times -- an AABB around the sphere's own center, like BOX slot 8.
void SphereShape::nxSphereLocalAABB(float* out) const
	{
	out[0] = -mRadiusE0;					// fchs; fst [esp]; ... fstp [eax]
	out[1] = -mRadiusE0;
	out[2] = -mRadiusE0;
	out[3] = mRadiusE0;						// mov [eax+0x14],edx
	out[4] = mRadiusE0;						// fstp [eax+0xc]
	out[5] = mRadiusE0;						// fstp [eax+0x10]
	}

// phys_fn_001357 (0x000278c0), SPHERE-table slot 14.
void SphereShape::nxSphereSetRadius(float radius)
	{
	mRadiusE0 = radius;						// mov [esi+0xe0],eax at 0x000278d1
	// The image then fcomp's the stored value against the zero global
	// (0x101041f0): a report fires unless radius > 0 (unordered and less
	// both fall through the jp). Report shape at 0x000278de..0x27900:
	// assert-guarded indirect cdecl call with (1, SphereShape.cpp,
	// 0x4a, 0, "radius should be positive!"). With a valid radius both
	// remaining arms are no-ops on a detached shape: BASE slot 6
	// owner-notify and dirty-flag 0x20 via phys_fn_001315-adjacent helper
	// 0x26c90 (see evidence 3k/3t).
	if(!(radius > 0.0f))
		nxReport(1, nxSourceFileSphereShapeCpp, 0x4a, 0,
			nxMsgSetRadiusPositive);
	}

// phys_fn_001329 (0x00026d90): apply a GROUP. See ObjectModel.h.
void ShapeBase::nxApplyGroup(unsigned short group)
	{
	if(group >= 0x20)
		{
		// The image reports (1, Shape.cpp, 0xe0, 0, "group ID must be
		// < 32!") through the guarded slot -- int3 when the flag word is
		// zero -- and then JOINS the valid path at the dirty-flag call,
		// skipping only the store. Evidence section 3u.
		nxReport(1, nxSourceFileShapeCpp, 0xe0, 0, nxMsgGroupBelow32);
		}
	else
		{
		mHalfwordD8 = group;			// mov [esi+0xd8],ax at 0x00026dc7
		}
	// Both paths rejoin at 0x00026dce: dirty-flag 0x04 via 0x26c90 -- a
	// null-owner no-op on a detached shape -- then the prunable's
	// unidentified dword at abs +0xc8 (mPrunable24) becomes the group mask
	// 1 << low-byte(group). The x86 shift masks its count to five bits;
	// reproduced.
	mPrunable.mPrunable24 =
		1u << ((mHalfwordD8 & 0xff) & 0x1f);
	}

// phys_fn_001315 (0x000266a0), BASE-table slot 6. Detached-shape path:
// owner == null takes the early exit at 0x00026abb which is a pure no-op
// (pop ebp / add esp,0x84 / ret 4). The owned path needs scene
// infrastructure from Task 4 and is not reachable for detached shapes.
// phys_fn_000981 (0x00021990), BOX-table slot 12.
void BoxShape::nxBoxLoadFromDesc(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	memcpy(&mHull.mDims04[0], rec + 0x4c, sizeof(mHull.mDims04[0]));
	memcpy(&mHull.mDims04[1], rec + 0x50, sizeof(mHull.mDims04[1]));
	memcpy(&mHull.mDims04[2], rec + 0x54, sizeof(mHull.mDims04[2]));
	// helper 0x21420 recomputes hull data -- Task 3 deep chain
	mBase.nxApplyDescriptor(rec);
	}

// phys_fn_001383 (0x00027e30), MESH-table slot 12.
bool MeshShape::nxMeshLoadFromDesc(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	void* wrapper = nullptr;
	memcpy(&wrapper, rec + 0x4c, sizeof(wrapper));
	if(wrapper == nullptr)
		return false;
	void* inner = nullptr;
	memcpy(&inner, static_cast<unsigned char*>(wrapper) + 4, sizeof(inner));
	mWordE0 = reinterpret_cast<NxU32>(inner);
	++*reinterpret_cast<unsigned*>(static_cast<unsigned char*>(inner) + 0x74);
	memcpy(&mWordE4, rec + 0x50, sizeof(mWordE4));
	mBase.nxApplyDescriptor(rec);
	return true;
	}

// phys_fn_001265 (0x00025460), PLANE-table slot 12.
void PlaneShape::nxPlaneLoadFromDesc(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	memcpy(&mNormalE0, rec + 0x4c, sizeof(mNormalE0));
	float d = 0.0f;
	memcpy(&d, rec + 0x58, sizeof(d));
	mDistanceEC = -d;
	mBase.nxApplyDescriptor(rec);
	}

void ShapeBase::nxApplyOwnerUpdate(unsigned flags)
	{
	if(mOwner04 == nullptr)
		return;
	(void) flags;
	}

// ---------------------------------------------------------------------------
// Shape-to-name registry. See evidence section 3o for the full decode.

// The global list head at .data 0x10123c0c. Each entry is {shape*, name*}
// (8-byte stride). The list grows via the SDK allocator when capacity is
// exhausted.
static void* gShapeNameList = nullptr;

// phys_fn_000480 (0x000edc0): associate or dissociate a shape with a name.
static const unsigned NX_REG_CAP = 32;
static void* sRegShapes[NX_REG_CAP] = {};
static void* sRegNames[NX_REG_CAP] = {};
static unsigned sRegCount = 0;

bool ShapeBase::nxShapeNameRegistry(void* shape, void* name)
	{
	if(shape == nullptr)
		return false;
	for(unsigned i = 0; i < sRegCount; ++i)
		{
		if(sRegShapes[i] == shape)
			{
			if(name != nullptr)
				{
				sRegNames[i] = name;
				return true;
				}
			if(i < sRegCount - 1)
				{
				sRegShapes[i] = sRegShapes[sRegCount - 1];
				sRegNames[i] = sRegNames[sRegCount - 1];
				}
			--sRegCount;
			return true;
			}
		}
	if(name != nullptr && sRegCount < NX_REG_CAP)
		{
		sRegShapes[sRegCount] = shape;
		sRegNames[sRegCount] = name;
		++sRegCount;
		return true;
		}
	return name == nullptr;
	}

// phys_fn_001347 (0x00027740), BASE-table slot 1. See ObjectModel.h.
bool ShapeBase::nxApplyDescriptor(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	memcpy(&mPose6C, rec + 8, sizeof(mPose6C));			// rep movsd 9 + three words
	NxU16 flagsLo = 0;
	memcpy(&flagsLo, rec + 0x38, 2);					// movzx word [ebp+0x38]
	mHalfwordDE = flagsLo;								// mov [ebx+0xde],cx
	memcpy(&mHalfwordDA, rec + 0x3e, sizeof(mHalfwordDA));
	if(mWord9C != 0)
		{
		unsigned ud = 0;
		memcpy(&ud, rec + 0x40, sizeof(ud));
		unsigned char* col = reinterpret_cast<unsigned char*>(mWord9C);
		memcpy(col + 4, &ud, sizeof(ud));				// guarded store 0x0002778d..90
		}
	NxU16 grp = 0;
	memcpy(&grp, rec + 0x3c, sizeof(grp));				// mov ax,[ebp+0x3c]
	nxApplyGroup(grp);									// validated setter 0x26d90
	// The registry arm (phys_fn_000480 with [desc+0x44]) reduces to a
	// no-op-return-true when the name is NULL and the list is empty -- the
	// drive passes name=NULL; full registry transcription is its own row.
	return true;										// mov al,1
	}

// phys_fn_001263 (0x00025420), PLANE-table slot 0.
void PlaneShape::nxPlaneScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		// destroyed through its own vtable: 0x00025433..37
		}
	mBase.nxBaseDtorOwnerArms();		// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of the base-dtor chain
	(void) flags;							// self-free arm not modeled
	}

// phys_fn_001375 (0x00027c30), SPHERE-table slot 0.
void SphereShape::nxSphereScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		// destroyed through its own vtable by the image: 0x00027c43..47
		}
	mBase.nxBaseDtorOwnerArms();			// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of the base-dtor chain
	(void) flags;							// self-free arm not modeled
	}

// phys_fn_001353 (0x00027850), SPHERE-table slot 12.
void SphereShape::nxSphereLoadFromDesc(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	float r = 0.0f;
	memcpy(&r, rec + 0x4c, sizeof(r));		// fld [edi+0x4c] at 0x00027856
	mRadiusE0 = r;							// fst [esi+0xe0] at 0x0002785b
	// fcomp against the zero literal at 0x00027861: a report fires unless
	// strictly positive (line 0x35). The radius was already stored
	// unconditionally, and the BASE apply-desc tail runs either way.
	if(!(mRadiusE0 > 0.0f))
		nxReport(1, nxSourceFileSphereLoadCpp, 0x35, 0,
			nxMsgSphereLoadRadius);
	mBase.nxApplyDescriptor(record);				// call 0x00027740 at 0x00027895
	}

// ---------------------------------------------------------------------------
// The mass frame and the SPHERE slot-4 row behind it. See evidence section
// 3p for the disassembly walk this transcribes.

// The stored .rdata words participate in the arithmetic, exactly as in
// MassProperties.cpp -- same literals, different rows.
static const NxF32 gMassKFourThirdsPi = 4.1887903f;	// [0x101068d8]
static const NxF32 gMassKTwoFifths = 0.4f;			// [0x101068e4]
static const NxF32 gMassDensitySentinel = 1.0f;		// [0x101041ec]

namespace
	{
	// The process runs x87 at _PC_53, so every register intermediate is a
	// double and only the stores round to 32 bits. Writing the lifetimes as
	// double reproduces the oracle bitwise (same argument as
	// MassProperties.cpp's header comment).
	inline NxF32 mul32(NxF32 a, NxF32 b)
		{
		double p = static_cast<double>(a) * static_cast<double>(b);
		return static_cast<NxF32>(p);
		}
	}

// phys_fn_000843 (0x0001c750), __thiscall ret 8. The image squares and cubes
// the radius on the x87 stack, multiplies by 4pi/3 for the mass, then keeps
// going from the mass value -- r^3 -> r^5 through two more radius multiplies,
// one 2/5 -- for the diagonal inertia. Nine words are zeroed by integer moves
// (+0x04..+0x1c off-diagonal thirds and the whole offset), which is why a
// fresh frame carries exact +0.0f there.
void MassFrame::nxMassFrameBuildSphere(float radius, const void* extra)
	{
	double r3 = static_cast<double>(radius);
	r3 *= static_cast<double>(radius);
	r3 *= static_cast<double>(radius);					// fld/fmul/fmul chain
	double mass = r3 * static_cast<double>(gMassKFourThirdsPi);

	double diag = mass;
	diag *= static_cast<double>(radius);				// back up to r^4
	diag *= static_cast<double>(radius);				// r^5
	diag *= static_cast<double>(gMassKTwoFifths);		// (2/5) m r^2

	NxF32 stored = static_cast<NxF32>(mass);
	mMass = stored;										// fstp [esi+0x30]
	NxF32 inertia = static_cast<NxF32>(diag);
	mInertia[0] = inertia;								// fst [esi] / [esi+0x10] / [esi+0x20]
	mInertia[4] = inertia;
	mInertia[8] = inertia;
	// mov dword ptr [esi+4/+8/+0xc/+0x14/+0x18/+0x1c], 0
	mInertia[1] = mInertia[2] = mInertia[3] = 0.0f;
	mInertia[5] = mInertia[6] = mInertia[7] = 0.0f;
	// and the whole COM offset triple, also integer-zeroed
	mOffset.x = 0.0f; mOffset.y = 0.0f; mOffset.z = 0.0f;

	if(extra != nullptr)
		{
		// push extra; call 0x1bdc0 ; add extra,0x24 ; call 0x1c040 --
		// the parallel-axis pair, mirroring the BOX builder. A zero payload
		// folds the inertia to zero, which is what the oracle's non-null
		// arm produces (see 3z117/3z118).
		nxMassFrameFoldPayload(extra);
		nxMassFrameTranslate(reinterpret_cast<const unsigned char*>(extra) + 0x24);
		}
	}

// phys_fn_000837 (0x0001c5c0), __thiscall ret 4. Ten fld/fmul/fstp triples:
// the nine inertia words and the mass. The offset at +0x24..+0x2c is skipped
// entirely -- scaling by density keeps the center.
void MassFrame::nxMassFrameScale(float s)
	{
	for(unsigned i = 0; i < 9; ++i)
		mInertia[i] = mul32(mInertia[i], s);
	mMass = mul32(mMass, s);
	}

// phys_fn_000839 (0x0001c630), __thiscall ret 4. The weights live at +0x30 on
// both frames; the quotient divides the .rdata 1.0f literal by their sum (a
// real fdiv, then rounded by fstp m32 at 0x1c695 before use). The
// products are formed weight-times-offset per frame first, added pairwise --
// x-side as other+this, y/z sides as this+other, matching the stack order --
// scaled by the quotient and stored z, x, y. Then mass takes the raw sum and
// the nine inertia words accumulate componentwise.
void MassFrame::nxMassFrameMerge(const MassFrame& other)
	{
	double sum = static_cast<double>(mMass) + static_cast<double>(other.mMass);
	const NxF32 q = static_cast<NxF32>(static_cast<double>(gMassDensitySentinel) / sum);

	double ax = static_cast<double>(other.mMass) * static_cast<double>(other.mOffset.x);
	const NxF32 ay = static_cast<NxF32>(static_cast<double>(other.mMass) * other.mOffset.y);
	const NxF32 az = static_cast<NxF32>(static_cast<double>(other.mMass) * other.mOffset.z);
	const NxF32 cx = static_cast<NxF32>(static_cast<double>(mMass) * mOffset.x);
	const NxF32 cy = static_cast<NxF32>(static_cast<double>(mMass) * mOffset.y);
	const NxF32 cz = static_cast<NxF32>(static_cast<double>(mMass) * mOffset.z);
	// Other-x stays in the FPU stack; y/z pairwise sums are stored m32.
	const NxF32 sy = static_cast<NxF32>(static_cast<double>(cy) + ay);
	const NxF32 sz = static_cast<NxF32>(static_cast<double>(cz) + az);

	NxF32 zx = static_cast<NxF32>((ax + cx) * q);
	NxF32 zz = static_cast<NxF32>(static_cast<double>(sz) * q);
	NxF32 zy = static_cast<NxF32>(static_cast<double>(sy) * q);
	mOffset.x = zx;						// fstp [ecx+0x24]
	mOffset.y = zy;						// fstp [ecx+0x28]
	mOffset.z = zz;						// fstp [ecx+0x2c] (stored first in the image)
	mMass = static_cast<NxF32>(sum);	// fstp [ecx+0x30]

	for(unsigned i = 0; i < 9; ++i)
		mInertia[i] = static_cast<NxF32>(
			static_cast<double>(mInertia[i]) + static_cast<double>(other.mInertia[i]));
	}

// phys_fn_000851 (0x0001c930), __thiscall ret 0xc, SPHERE-table slot 4.
// Local frame, optional payload fold, conditional density scale against the
// 1.0f sentinel (fucompp/test ah,0x44/jnp: an unordered density falls through
// and scales, which `!=` reproduces), merge into the destination.
void SphereShape::nxSphereComputeMassFrame(MassFrame* dest, float density,
	float radius, const void* extra)
	{
	MassFrame local;
	local.nxMassFrameBuildSphere(radius, extra);
	if(density != gMassDensitySentinel)
		local.nxMassFrameScale(density);
	dest->nxMassFrameMerge(local);		}

// ---------------------------------------------------------------------------
// The BOX and CAPSULE slot-4 rows behind the same frame. Evidence section 3p.

static const NxF32 gMassKOneThird = (1.0f / 3.0f);	// [0x101068ec] stored 0.33333334
static const NxF32 gMassKEight = 8.0f;				// [0x101068f0]
static const NxF32 gMassKFour = 4.0f;				// [0x101068f4]
static const NxF32 gMassKThree = 3.0f;				// [0x101068f8] (bits 0x40400000)
static const NxF32 gPiLiteral = 3.1415927f;			// [0x101068d0]
static const NxF32 gHalfLiteral = 0.5f;				// [0x101043cc]
static const NxF32 gTwelfthLiteral = 0.083333336f;	// [0x101068e0]

namespace
	{
	// The box/capsule builders open their volume accumulator at the .rdata
	// 1.0f literal and test each extent with an INTEGER word compare, so
	// -0.0f counts as non-zero and replaces/multiplies in. MassProperties.cpp
	// documents the same shipped quirk in the exported kernels.
	inline bool nonZeroWord(float v)
		{
		NxU32 word;
		memcpy(&word, &v, sizeof(word));
		return word != 0;
		}
	}

// phys_fn_000829 (0x0001bd00), __thiscall ret 4. Volume over half-extents:
// the accumulator replaces with the first non-zero extent then multiplies the
// rest, times 8 ([0x101068f0]) -- full extents are twice half-extents. That
// mass value stays live on the x87 stack for the whole function; a copy times
// 1/3 ([0x101068ec]) becomes the diagonal factor F. Squares are taken from
// the raw floats; each diagonal is F times its pairwise sum (xx: y^2+z^2,
// yy: z^2+x^2, zz: x^2+y^2), rounded once at the store.
void MassFrame::nxMassFrameBuildBox(const float* he)
	{
	double acc = static_cast<double>(gMassDensitySentinel);	// fld 1.0f
	if(nonZeroWord(he[0]))
		acc = static_cast<double>(he[0]);					// fstp st(0); fld [eax]
	if(nonZeroWord(he[1]))
		acc *= static_cast<double>(he[1]);
	if(nonZeroWord(he[2]))
		acc *= static_cast<double>(he[2]);

	double m = acc * static_cast<double>(gMassKEight);		// fmul [0x101068f0]
	double f = m * static_cast<double>(gMassKOneThird);		// fld 1/3; fmul st(1)

	double xx = static_cast<double>(he[0]) * static_cast<double>(he[0]);
	double yy = static_cast<double>(he[1]) * static_cast<double>(he[1]);
	double zz = static_cast<double>(he[2]) * static_cast<double>(he[2]);

	mInertia[1] = mInertia[2] = mInertia[3] = 0.0f;			// integer zero stores
	mInertia[5] = mInertia[6] = mInertia[7] = 0.0f;

	double iXX = (zz + yy) * f;								// fadd st(2) chain
	double iYY = (zz + xx) * f;
	double iZZ = (yy + xx) * f;

	mMass = static_cast<NxF32>(m);							// fstp [ecx+0x30]
	mInertia[0] = static_cast<NxF32>(iXX);					// fstp [ecx]
	mInertia[4] = static_cast<NxF32>(iYY);					// via [esp+0x10]
	mInertia[8] = static_cast<NxF32>(iZZ);					// via [esp+0x14]
	mOffset.x = 0.0f; mOffset.y = 0.0f; mOffset.z = 0.0f;
	}

// phys_fn_000845 (0x0001c7c0), __thiscall ret 0xc. A unit-density cylinder of
// radius `radius` and height 2*cylHalfHeight: mass = pi*r^2*2c. The axial
// diagonal carries mass*r^2/2 (through r^2*pi*c*... folded to mr^2/2 by the
// .rdata 0.5f at 0x101043cc); the transverse pair carries the full cylinder
// formula mass*(3r^2+4c^2)/12 ([0x101068f8] = 3, [0x101068f4] = 4, over the
// .rdata 1/12 at 0x101068e0). axisSelector routes the axial term:
// 0 -> +0x00, 1 -> +0x10 (and that path never writes +0x00 -- an image hole
// this transcription reproduces), anything else -> +0x20.
void MassFrame::nxMassFrameBuildCapsule(unsigned axisSelector, float radius,
	float cylHalfHeight)
	{
	mInertia[1] = mInertia[2] = mInertia[3] = 0.0f;
	mInertia[5] = mInertia[6] = mInertia[7] = 0.0f;

	double c2 = static_cast<double>(cylHalfHeight);
	c2 += c2;												// fadd st(0),st(0)
	double m = c2 * static_cast<double>(radius);
	m *= static_cast<double>(radius);
	m *= static_cast<double>(gPiLiteral);					// pi from 0x101068d0

	double axial = m * static_cast<double>(radius);			// fld r; fmul st(1)
	axial *= static_cast<double>(radius);
	axial *= static_cast<double>(gHalfLiteral);				// .rdata 0.5f

	double rr = static_cast<double>(radius) * static_cast<double>(radius);
	double t0 = rr * static_cast<double>(gMassKThree);
	double t1 = static_cast<double>(cylHalfHeight) *
		static_cast<double>(cylHalfHeight);
	t1 *= static_cast<double>(gMassKFour);
	double side = (t0 + t1) * m;							// faddp st(1); fmul st(1)
	side *= static_cast<double>(gTwelfthLiteral);			// 1/12 from 0x101068e0

	mMass = static_cast<NxF32>(m);

	NxF32 sAx = static_cast<NxF32>(axial);
	NxF32 sSide = static_cast<NxF32>(side);
	if(axisSelector == 0)
		{
		mInertia[0] = sAx;
		mInertia[4] = sSide;
		mInertia[8] = sSide;
		}
	else if(axisSelector == 1)
		{
		mInertia[4] = sAx;
		mInertia[8] = sSide;
		// +0x00 stays as the caller left it: the image's selector==1 path
		// never stores it.
		}
	else
		{
		mInertia[0] = sSide;
		mInertia[4] = sSide;
		mInertia[8] = sAx;
		}
	mOffset.x = 0.0f; mOffset.y = 0.0f; mOffset.z = 0.0f;
	}

// phys_fn_000831 (0x0001bdc0), __thiscall ret 4. Straight-line x87 transform
// over payload {Vec3 d; SymMat3 K}. Nine intermediates -- each an faddp chain
// ROUNDED TO FLOAT32 by its fstp m32 store before the rep movsd copy feeds
// them back -- then nine inertia stores, then the offset triple. The mass at
// +0x30 never participates. Formula table in evidence section 3r.
void MassFrame::nxMassFrameFoldPayload(const void* payload)
	{
	const NxF32* p = static_cast<const NxF32*>(payload);
	const double dx = p[0], dy = p[1], dz = p[2];
	const double k00 = p[3], k01 = p[4], k02 = p[5];
	const double k11 = p[6], k12 = p[7], k22 = p[8];

	const double i0 = mInertia[0], i1 = mInertia[1], i2 = mInertia[2];
	const double i3 = mInertia[3], i4 = mInertia[4], i5 = mInertia[5];
	const double i6 = mInertia[6], i7 = mInertia[7], i8 = mInertia[8];

	// 0x0001bdc7..0x0001beb8: nine three-product chains; each fstp m32
	// rounds its result before anything reads it back.
	NxF32 a0 = static_cast<NxF32>((dy * i3 + dz * i6) + dx * i0);
	NxF32 a1 = static_cast<NxF32>((dz * i7 + dy * i4) + dx * i1);
	NxF32 a2 = static_cast<NxF32>((dz * i8 + dx * i2) + dy * i5);
	NxF32 a3 = static_cast<NxF32>((k00 * i0 + k02 * i6) + k01 * i3);
	NxF32 a4 = static_cast<NxF32>((k02 * i7 + k01 * i4) + k00 * i1);
	NxF32 a5 = static_cast<NxF32>((k00 * i2 + k02 * i8) + k01 * i5);
	NxF32 a6 = static_cast<NxF32>((k11 * i0 + k22 * i6) + k12 * i3);
	NxF32 a7 = static_cast<NxF32>((k22 * i7 + k12 * i4) + k11 * i1);
	NxF32 a8 = static_cast<NxF32>((k11 * i2 + k22 * i8) + k12 * i5);

	// 0x0001bebe..0x0001bfc5: six stored results plus two stacked values and
	// the final scalar. After pop edi/pop esi the 0x1bfb1 load reads A[1]
	// (old frame +0x20): the scalar is the quadratic form d' I d.
	double t1 = ((double)a1 * k01 + (double)a2 * k02) + (double)a0 * k00;
	double u1 = ((double)a3 * k00 + (double)a4 * k01) + (double)a5 * k02;
	double u2 = ((double)a3 * k11 + (double)a4 * k12) + (double)a5 * k22;
	double w  = ((double)a6 * dx + (double)a8 * dz) + (double)a7 * dy;
	double v1 = ((double)a6 * k00 + (double)a7 * k01) + (double)a8 * k02;
	double v2 = ((double)a6 * k11 + (double)a7 * k12) + (double)a8 * k22;
	double t2 = ((double)a1 * k12 + (double)a2 * k22) + (double)a0 * k11;
	double q0 = ((double)a3 * dx + (double)a5 * dz) + (double)a4 * dy;
	double s  = ((double)a0 * dx + (double)a2 * dz) + (double)a1 * dy;

	// 0x0001bfa9..0x0001bfe6: inertia overwrite -- integer moves carry the
	// memory temps while the FPU stack yields [edx]/[edx+4]/[edx+8] through
	// one fxch (top becomes T1).
	mInertia[0] = static_cast<NxF32>(s);
	mInertia[1] = static_cast<NxF32>(t1);
	mInertia[2] = static_cast<NxF32>(t2);
	mInertia[3] = static_cast<NxF32>(q0);
	mInertia[4] = static_cast<NxF32>(u1);
	mInertia[5] = static_cast<NxF32>(u2);
	mInertia[6] = static_cast<NxF32>(w);
	mInertia[7] = static_cast<NxF32>(v1);
	mInertia[8] = static_cast<NxF32>(v2);

	// 0x0001bfe9..0x0001c032: offset triple -- fstp order stores the o.d
	// term FIRST, then fxch hands the two o^T K columns to +0x28/+0x2c.
	// Mass untouched.
	const double ox = mOffset.x, oy = mOffset.y, oz = mOffset.z;
	NxF32 nx = static_cast<NxF32>((oz * dz + dy * oy) + dx * ox);	// [edx+0x24]
	NxF32 ny = static_cast<NxF32>((oy * k01 + ox * k00) + oz * k02);	// [edx+0x28]
	NxF32 nz = static_cast<NxF32>((oy * k12 + ox * k11) + oz * k22);	// [edx+0x2c]
	mOffset.x = nx;
	mOffset.y = ny;
	mOffset.z = nz;
	}

// phys_fn_000847 (0x0001c880), __thiscall ret 4. Conditionally zeroes all
// thirteen words when the byte argument is non-zero; leaves the frame
// untouched when it is zero.
void MassFrame::nxMassFrameConditionalZero(unsigned flag)
	{
	if(flag == 0)
		return;
	mInertia[0] = 0.0f; mInertia[1] = 0.0f; mInertia[2] = 0.0f;
	mInertia[3] = 0.0f; mInertia[4] = 0.0f; mInertia[5] = 0.0f;
	mInertia[6] = 0.0f; mInertia[7] = 0.0f; mInertia[8] = 0.0f;
	mOffset.x = 0.0f; mOffset.y = 0.0f; mOffset.z = 0.0f;
	mMass = 0.0f;
	}

// phys_fn_000833 (0x1c040), __thiscall ret 4: translate the frame by
// {d.x,d.y,d.z} at param+0. PROVISIONAL: early-out when d all-zero; else
// d+offset; centered path (new center at origin) vs displaced path.
void MassFrame::nxMassFrameTranslate(const void* param)
	{
	const float* d = static_cast<const float*>(param);
	unsigned w0,w1,w2;
	memcpy(&w0,d+0,4); memcpy(&w1,d+1,4); memcpy(&w2,d+2,4);
	if(w0==0 && w1==0 && w2==0)
		return;								// 0x1c04e..0x1c05e all-zero early-out
	// Move the reference center from old_offset (o) to c = d + o. The
	// inertia about the new reference gains m*(Q(c) - Q(o)) where
	// Q(r)=|r|^2 I - r r^T. The image splits this into a "centered" path
	// (when c lands at the origin, Q(c)=0) and a "displaced" path
	// (generic), but both are the same Delta-Q; we compute the plain delta.
	const float ox = mOffset.x, oy = mOffset.y, oz = mOffset.z;
	const float cx = d[0] + ox, cy = d[1] + oy, cz = d[2] + oz;
	// Q(r) diagonals/off-diagonals as float products, matched to the image's
	// m32 store points.
	const float Qc[9] = {
		cy*cy+cz*cz, -(cx*cy), -(cx*cz),
		-(cx*cy), cx*cx+cz*cz, -(cy*cz),
		-(cx*cz), -(cy*cz), cx*cx+cy*cy };
	const float Qo[9] = {
		oy*oy+oz*oz, -(ox*oy), -(ox*oz),
		-(ox*oy), ox*ox+oz*oz, -(oy*oz),
		-(ox*oz), -(oy*oz), ox*ox+oy*oy };
	// Centered (c == 0) vs displaced path select the same Delta-Q.
	for(unsigned i = 0; i < 9; ++i)
		{
		const float term = Qc[i] - Qo[i];
		const float scaled = static_cast<float>(static_cast<double>(term) * mMass);
		mInertia[i] = static_cast<float>(static_cast<double>(mInertia[i]) + scaled);
		}
	mOffset.x = cx; mOffset.y = cy; mOffset.z = cz;	// shared tail 0x1c578
	}

// Provisional phys_fn_000947, ret 12: three stack DWORDs; the last is unused.
bool BoxShape::nxBoxAccumulateMass(MassFrame* destination, float density, unsigned reserved)
	{
	(void) reserved;
	if(!mBase.nxFlagBitsDE(7))
		nxBoxComputeMassFrame(destination, density, mHull.mDims04, &mBase.mPose6C);
	return true;
	}

// phys_fn_001565 (0x2e5a0, wrapper over the 001552 vptr-ctor):
// the callee installs vptr 0x10107848 and zeroes +0x4/+0x8, then the
// wrapper overrides [+0x10]=arg, vptr 0x1010785c, [+0xc]=0.
void* BoxShape::nxCtorWrap565(unsigned argument)
	{
	*(unsigned*)((unsigned char*)this + 0x00) = 0x1010785cu;
	*(unsigned*)((unsigned char*)this + 0x04) = 0;
	*(unsigned*)((unsigned char*)this + 0x08) = 0;
	*(unsigned*)((unsigned char*)this + 0x0c) = 0;
	*(unsigned*)((unsigned char*)this + 0x10) = argument;
	return this;
	}
void* BoxShape::nxCtorWrap571(unsigned argument)
	{
	*(unsigned*)((unsigned char*)this + 0x00) = 0x1010786cu;
	*(unsigned*)((unsigned char*)this + 0x04) = 0;
	*(unsigned*)((unsigned char*)this + 0x08) = 0;
	*(unsigned*)((unsigned char*)this + 0x0c) = 0;
	*(unsigned*)((unsigned char*)this + 0x10) = argument;
	return this;
	}
void* BoxShape::nxCtorWrap575(unsigned argument)
	{
	*(unsigned*)((unsigned char*)this + 0x00) = 0x10107890u;
	*(unsigned*)((unsigned char*)this + 0x04) = 0;
	*(unsigned*)((unsigned char*)this + 0x08) = 0;
	*(unsigned*)((unsigned char*)this + 0x0c) = 0;
	*(unsigned*)((unsigned char*)this + 0x10) = 0;
	*(unsigned*)((unsigned char*)this + 0x14) = argument;
	return this;
	}

// phys_fn_002065 (0x51ec0): zero the first 0x34 bytes (the 001655-ish init
// zeroes [this+0..0x20], then the 005355 sub-call zeroes [this+0x24..0x30]).
void* BoxShape::nxWrapZero2065()
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(this);
	for(unsigned o = 0; o <= 0x30; o += 4)
		*(unsigned*)(p + o) = 0;
	return this;
	}

// phys_fn_001409 (0x29750): the 001455 sub-ctor sets [this]=0x1010769c and
// zeroes +4..+0x48; the wrapper then overrides vptr to 0x1010767c and zeroes
// +0x64..0x7c.
void* BoxShape::nxWrap1409()
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(this);
	for(unsigned o = 4; o <= 0x48; o += 4)
		*(unsigned*)(p + o) = 0;
	for(unsigned o = 0x64; o <= 0x7c; o += 4)
		*(unsigned*)(p + o) = 0;
	*(unsigned*)(p + 0x00) = 0x1010767cu;
	return this;
	}

// phys_fn_000240 (0xb7c0): out = [*container + index*4] if
//   index < ([container+0xc] - [container+8])>>2 else 0; return [out + 0x6cc].
unsigned BoxShape::nxWrap240(unsigned index)
	{
	void* container = *reinterpret_cast<void**>(
		reinterpret_cast<unsigned char*>(this) + 4);
	const unsigned char* base = reinterpret_cast<const unsigned char*>(container);
	unsigned lo, hi;
	memcpy(&hi, base + 8, 4); memcpy(&lo, base + 0xc, 4);
	signed delta = static_cast<signed>(lo - hi) >> 2;
	unsigned char* hb = reinterpret_cast<unsigned char*>(static_cast<size_t>(hi));
	unsigned char* slot;
	if(index >= static_cast<unsigned>(delta))
		slot = nullptr;
	else
		slot = *reinterpret_cast<unsigned char**>(hb + index*4);
	return *reinterpret_cast<unsigned*>(slot + 0x6cc);
	}

// phys_fn_001255 (0x25090, PLANE vtable slot 9, ret 4): the plane's local
// AABB. An axis-aligned normal selects that axis and writes +/- distance
// there; the other axes keep +/- 0x7effffff. out[2] holds the -0x7effffff
// sentinel (0xfeffffff) unless the normal is -Z, in which case it carries
// the distance (the x87 const is popped and d loaded).
void PlaneShape::nxPlaneLocalAABB1255(float* out) const
	{
	unsigned ax, ay, az, ad;
	memcpy(&ax, &mNormalE0[0], 4);
	memcpy(&ay, &mNormalE0[1], 4);
	memcpy(&az, &mNormalE0[2], 4);
	memcpy(&ad, &mDistanceEC, 4);
	ax &= 0x7fffffffu; ay &= 0x7fffffffu; az &= 0x7fffffffu;
	unsigned w4 = 0x7effffffu, w8 = 0x7effffffu, wc = 0x7effffffu;
	unsigned w10 = 0xfeffffffu, w14 = 0xfeffffffu;
	unsigned w2 = 0xfeffffffu;
	const float negDistance = -mDistanceEC;
	unsigned nd; memcpy(&nd, &negDistance, 4);
	if(ax == 0x3f800000u && ay == 0u && az == 0u)
		{
		if(!(mNormalE0[0] > 0.0f)) w10 = ad;		// jne: nx <= 0 (or unordered)
		else                       w4 = nd;
		}
	else if(ax == 0u && ay == 0x3f800000u && az == 0u)
		{
		if(!(mNormalE0[1] > 0.0f)) w14 = ad;
		else                       w8 = nd;
		}
	else if(ax == 0u && ay == 0u && az == 0x3f800000u)
		{
		if(!(mNormalE0[2] > 0.0f)) w2 = ad;		// const popped, d loaded
		else                       wc = nd;
		}
	unsigned o[6] = { w10, w14, w2, w4, w8, wc };
	memcpy(out, o, sizeof(o));
	}

// phys_fn_001016 (0x22620, CAPSULE vtable slot 9, ret 4): the capsule's world
// AABB. The segment runs along local Y, so each world extent is
// M[i][1]*halfHeight; the row expands the LOWER endpoint (t - e) by +/- radius
// into out[0..2] / out[3..5], then merges the UPPER endpoint's +/- radius
// (A/B below) with min on the low triple and max on the high triple. All
// products accumulate in x87 extended precision and store to float, so a
// double intermediate reproduces the staged rounding.
void CapsuleShape::nxCapsuleWorldAABB1016(float* out) const
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(this);
	float m01, m11, m21, hh, r, t0, t1, t2;
	memcpy(&m01, p + 0x10, 4);
	memcpy(&m11, p + 0x1c, 4);
	memcpy(&m21, p + 0x28, 4);
	memcpy(&hh, p + 0xe4, 4);
	memcpy(&r, p + 0xe0, 4);
	memcpy(&t0, p + 0x30, 4);
	memcpy(&t1, p + 0x34, 4);
	memcpy(&t2, p + 0x38, 4);
	const float e0 = static_cast<float>(static_cast<double>(m01)*hh);
	const float e1 = static_cast<float>(static_cast<double>(m11)*hh);
	const float e2 = static_cast<float>(static_cast<double>(m21)*hh);
	const float mn0 = static_cast<float>(static_cast<double>(t0) - e0);
	const float mn1 = static_cast<float>(static_cast<double>(t1) - e1);
	const float mn2 = static_cast<float>(static_cast<double>(t2) - e2);
	const float mx0 = static_cast<float>(static_cast<double>(t0) + e0);
	const float mx1 = static_cast<float>(static_cast<double>(t1) + e1);
	const float mx2 = static_cast<float>(static_cast<double>(t2) + e2);
	const float a0 = static_cast<float>(static_cast<double>(mx0) - r);
	const float a1 = static_cast<float>(static_cast<double>(mx1) - r);
	const float a2 = static_cast<float>(static_cast<double>(mx2) - r);
	const float b0 = static_cast<float>(static_cast<double>(mx0) + r);
	const float b1 = static_cast<float>(static_cast<double>(mx1) + r);
	const float b2 = static_cast<float>(static_cast<double>(mx2) + r);
	out[0] = static_cast<float>(static_cast<double>(mn0) - r);
	out[1] = static_cast<float>(static_cast<double>(mn1) - r);
	out[2] = static_cast<float>(static_cast<double>(mn2) - r);
	out[3] = static_cast<float>(static_cast<double>(r) + mn0);
	out[4] = static_cast<float>(static_cast<double>(r) + mn1);
	out[5] = static_cast<float>(static_cast<double>(r) + mn2);
	if(!(out[0] <= a0)) out[0] = a0;
	if(!(out[1] <= a1)) out[1] = a1;
	if(!(out[2] <= a2)) out[2] = a2;
	// The +0xc/+0x10 arms use `test ah,5 / jp`, which updates on out <= B
	// (and skips the unordered case) -- a MAX, not an unconditional store.
	if(out[3] <= b0) out[3] = b0;
	if(out[4] <= b1) out[4] = b1;
	if(b2 > out[5]) out[5] = b2;
	}

// phys_fn_001267 (0x25490, PLANE vtable slot 8, ret 4): selects a 6-dword
// record from the table at *([this+0xc4]+0x14), indexed by
// [this+0xa4+0x28], and copies it to out. When [this+0xcc] != 0xffff and
// [this+0xa4+8] lacks bit 2, the 004886 init runs first on the
// [this+0xcc]-indexed record (its only observable being [rec+8] |= 2).
void PlaneShape::nxPlaneIndexed6_1267(unsigned* out) const
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(this);
	unsigned short ax;
	memcpy(&ax, p + 0xcc, 2);
	if(ax != 0xffffu && !(p[0xa4 + 8] & 2))
		{
		unsigned char* inner = *reinterpret_cast<unsigned char* const*>(p + 0xc4);
		unsigned char* base = *reinterpret_cast<unsigned char**>(inner + 0x14);
		unsigned char* rec = base + static_cast<unsigned>(ax) * 24u;
		*(unsigned*)(rec + 8) |= 2u;		// 004886 observable
		}
	unsigned short ax2;
	memcpy(&ax2, p + 0xa4 + 0x28, 2);
	const unsigned char* inner = *reinterpret_cast<unsigned char* const*>(p + 0xc4);
	const unsigned char* base = *reinterpret_cast<const unsigned char* const*>(inner + 0x14);
	const unsigned char* entry = base + static_cast<unsigned>(ax2) * 24u;
	memcpy(out, entry, 24);
	}

// phys_fn_001008 (0x22440, CAPSULE vtable slot 4, ret 0xc): mirror of the
// sphere slot-4 wrapper -- radius from [this+0xe0], cylHalfHeight from
// [this+0xe0]+[this+0xe4], axisSelector 1, pose [this+0x6c].
bool CapsuleShape::nxCapsuleAccumulateMass(MassFrame* destination, float density,
	unsigned reserved)
	{
	(void) reserved;
	if(!mBase.nxFlagBitsDE(7))
		{
		const unsigned char* p = reinterpret_cast<const unsigned char*>(this);
		const float radius = *reinterpret_cast<const float*>(p + 0xe0);
		const float halfHeight =
			radius + *reinterpret_cast<const float*>(p + 0xe4);
		nxCapsuleComputeMassFrame(destination, density, 1u, radius, halfHeight,
			p + 0x6c);
		}
	return true;
	}

// phys_fn_001371 (0x27be0, SPHERE vtable slot 4, ret 0xc): mirror of the BOX
// slot-4 wrapper -- when the low flag bits are clear, run the sphere mass
// helper on the facade radius and pose, then return true.
bool SphereShape::nxSphereAccumulateMass(MassFrame* destination, float density,
	unsigned reserved)
	{
	(void) reserved;
	if(!mBase.nxFlagBitsDE(7))
		{
		const float radius = *reinterpret_cast<const float*>(
			reinterpret_cast<const unsigned char*>(this) + 0xe0);
		nxSphereComputeMassFrame(destination, density, radius,
			reinterpret_cast<const unsigned char*>(this) + 0x6c);
		}
	return true;
	}

// phys_fn_000827 (0x1bcc0, ret 0xc): rep-movsd 9 from arg1 into [this+0],
// then arg2[0..2] into [this+0x24..0x2c] and arg3 into [this+0x30].
void BoxShape::nxPoseCopyWithTail0827(const void* src, const unsigned* extra,
	unsigned x)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(this);
	memcpy(p + 0x00, src, 36);
	memcpy(p + 0x24, extra, 12);
	memcpy(p + 0x30, &x, 4);
	}

// phys_fn_001403 (0x29190, MESH vtable slot 4): copy [[this+0xe0]+0x5c][0..3]
// into out[0..3], then out[0..2] = M * v + t, with the 3x3 matrix at
// [this+0xc..0x2c] (rows) and the translation at [this+0x30..0x38]. The x87
// body accumulates in extended precision and stores to float; a double
// intermediate reproduces the staged rounding.
void BoxShape::nxTransformPoint1403(float* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(this);
	const unsigned char* src =
		*reinterpret_cast<const unsigned char* const*>(p + 0xe0) + 0x5c;
	float v[4];
	memcpy(v, src, sizeof(v));
	float mf[9];
	memcpy(mf, p + 0xc, sizeof(mf));
	float tf[3];
	memcpy(tf, p + 0x30, sizeof(tf));
	const float rx = static_cast<float>(static_cast<double>(mf[0])*v[0] + static_cast<double>(mf[1])*v[1] + static_cast<double>(mf[2])*v[2] + tf[0]);
	const float ry = static_cast<float>(static_cast<double>(mf[3])*v[0] + static_cast<double>(mf[4])*v[1] + static_cast<double>(mf[5])*v[2] + tf[1]);
	const float rz = static_cast<float>(static_cast<double>(mf[6])*v[0] + static_cast<double>(mf[7])*v[1] + static_cast<double>(mf[8])*v[2] + tf[2]);
	out[0] = rx; out[1] = ry; out[2] = rz;
	out[3] = v[3];
	}

// phys_fn_000849 (0x0001c8c0), __thiscall ret 0xc, helper of BOX slot 4.
// Pose support is provisional: centered boxes and the driven finite poses;
// general x87 staging, exceptional inputs and payload aliasing remain open.
void BoxShape::nxBoxComputeMassFrame(MassFrame* dest, float density,
	const float* halfExtents, const void* extra)
	{
	MassFrame local;
	local.nxMassFrameBuildBox(halfExtents);
	if(extra != nullptr)
		{
		// 0x1c8dd: rotate the centered box frame by the first nine pose words.
		local.nxMassFrameFoldPayload(extra);
		// Centered-box specialization of 000833 (0x1c040): the local
		// frame starts with zero center. Add m*(|t|^2 I - t*t^T), then
		// center += t. The image stores mass-scaled terms to float before
		// adding them to inertia (0x1c4a3..0x1c575).
		const float* translation = static_cast<const float*>(extra) + 9;
		const double x = translation[0], y = translation[1], z = translation[2];
		const double terms[9] = { y*y+z*z, -x*y, -x*z,
			-y*x, x*x+z*z, -y*z, -z*x, -z*y, x*x+y*y };
		for(unsigned i = 0; i < 9; ++i)
			{
			const float term = static_cast<float>(terms[i]);
			const float scaled = static_cast<float>(static_cast<double>(term) * local.mMass);
			local.mInertia[i] = static_cast<float>(static_cast<double>(local.mInertia[i]) + scaled);
			}
		local.mOffset.x += translation[0];
		local.mOffset.y += translation[1];
		local.mOffset.z += translation[2];
		}
	if(density != gMassDensitySentinel)
		local.nxMassFrameScale(density);
	dest->nxMassFrameMerge(local);		}

// phys_fn_000853 (0x0001c980), __thiscall ret 0x14, CAPSULE-table slot 4.
void CapsuleShape::nxCapsuleComputeMassFrame(MassFrame* dest, float density,
	unsigned axisSelector, float radius, float cylHalfHeight, const void* extra)
	{
	MassFrame local;
	local.nxMassFrameBuildCapsule(axisSelector, radius, cylHalfHeight);
	if(extra != nullptr)
		{
		// Same parallel-axis pair as the sphere/box builders (see 3z118).
		local.nxMassFrameFoldPayload(extra);
		local.nxMassFrameTranslate(
			reinterpret_cast<const unsigned char*>(extra) + 0x24);
		}
	if(density != gMassDensitySentinel)
		local.nxMassFrameScale(density);
	dest->nxMassFrameMerge(local);		}


// ---------------------------------------------------------------------------
// CapsuleShape. See ObjectModel.h for the row map.

CapsuleShape::CapsuleShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x00021a67..6f
	{
	mFloatE0 = 0.0f;						// mov [esi+0xe0],0 at 0x00021a7a
	mFloatE4 = 0.0f;						// mov [esi+0xe4],0 at 0x00021a84

	// The embedded collision object: a fresh 0x1c-byte block through the SDK
	// allocator (0x00021a8e..9c), built by phys_fn_001123 -- the capsule-family
	// variant of the shared collision-object constructor -- with the capsule
	// stored at BOTH +0x08 and +0x18.
	void* memory = nxGetSdkAllocator()->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObject(this)
		: 0;								// null arm: 0x00021aad
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00021aaf

	mBase.mSentinelD0 = 3;					// NX_SHAPE_CAPSULE: 0x00021ab5
	}

// phys_fn_000991 (0x00021b40), CAPSULE-table slot 13.
bool CapsuleShape::nxCapsuleSaveState(void* record)
	{
	unsigned char* rec = static_cast<unsigned char*>(record);
	memcpy(rec + 0x4c, &mFloatE0, sizeof(mFloatE0));	// mov at 0x00021b4a
	const float height = mFloatE4 + mFloatE4;			// fld; fadd st(0),st(0):
	memcpy(rec + 0x50, &height, sizeof(height));		//   descriptor height = 2 * stored half-height
	// The third saved word lives at +0xe8 (mWordE8) -- real storage the
	// constructor leaves poisoned and nothing has named yet.
	memcpy(rec + 0x54, &mWordE8, sizeof(mWordE8));	// mov [+eax+0x54] at 0x00021b5e
	return mBase.nxBaseSaveState(record);				// jmp 0x000256f0 at 0x00021b65
	}

// phys_fn_000995 (0x00021be0), CAPSULE-table slot 14.
void CapsuleShape::nxCapsuleSetRadius(float radius)
	{
	mFloatE0 = radius;						// mov [ecx+0xe0],eax at 0x00021be6
	// Tail-jumps through BASE slot 6 (owner-notify) with the flag forced to
	// 1: a null-owner no-op on a detached shape.
	}

// phys_fn_001014 (0x000225e0), CAPSULE-table slot 0.
void CapsuleShape::nxCapsuleScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		// destroyed through its own vtable: 0x000225f3..f7
		}
	mBase.nxBaseDtorOwnerArms();		// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of the base-dtor chain
	(void) flags;							// self-free arm not modeled
	}

// phys_fn_001004 (0x00021c80), CAPSULE-table slot 8.
void CapsuleShape::nxCapsuleLocalAABB(float* out) const
	{
	const float r = mFloatE0;				// fld [+0xe0]
	const float reach = mFloatE4 + r;		// fld [+0xe4]; fadd [+0xe0]
	out[0] = -r;
	out[1] = -reach;
	out[2] = -r;
	out[3] = r;
	out[4] = reach;
	out[5] = r;
	}

// phys_fn_000989 (0x00021ad0), CAPSULE-table slot 12.
void CapsuleShape::nxCapsuleLoadFromDesc(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	memcpy(&mFloatE0, rec + 0x4c, sizeof(mFloatE0));			// radius: 0x00021ad8
	float h = 0.0f;
	memcpy(&h, rec + 0x50, sizeof(h));							// desc height
	h *= 0.5f;													// fmul [0x101043cc] at 0x00021ae4
	mFloatE4 = h;												// fstp [+0xe4]
	memcpy(&mWordE8, rec + 0x54, sizeof(mWordE8));				// third word: 0x00021af0
	// The radius is reloaded and fcomp'd against the zero literal at
	// 0x00021aff: a report fires unless strictly positive (line 0x37).
	// Everything was already stored unconditionally above.
	if(!(mFloatE0 > 0.0f))
		nxReport(1, nxSourceFileCapsuleShapeCpp, 0x37, 0,
			nxMsgCapsuleLoadRadius);
	mBase.nxApplyDescriptor(rec);										// call BASE slot 1 at 0x00021b34
	}

// phys_fn_001001 (0x00021c30), CAPSULE-table slot 10.
void CapsuleShape::nxCapsuleCenterRadius(float* out) const
	{
	out[0] = mBase.mPose0C.mTranslation[0];	// +0x30 at 0x00021c30
	out[1] = mBase.mPose0C.mTranslation[1];
	out[2] = mBase.mPose0C.mTranslation[2];
	const float rr = mFloatE4 + mFloatE0;	// fld [+0xe4]; fadd [+0xe0]
	out[3] = rr;
	}

// phys_fn_001003 (0x00021c60), CAPSULE-table slot 11.
void CapsuleShape::nxCapsuleZeroCenterRadius(float* out) const
	{
	out[0] = 0.0f;							// xor edx,edx; three stores
	out[1] = 0.0f;
	out[2] = 0.0f;
	const float rr = mFloatE4 + mFloatE0;
	out[3] = rr;
	}

// ---------------------------------------------------------------------------
// BOX-table slot 10. See ObjectModel.h.

void BoxShape::nxBoxCenterAndDiagonal(float* out) const
	{
	out[0] = mBase.mPose0C.mTranslation[0];	// mov edx,[ecx+0x30] at 0x00020670
	out[1] = mBase.mPose0C.mTranslation[1];	// +0x34
	out[2] = mBase.mPose0C.mTranslation[2];	// +0x38
	const double dx = mHull.mDims04[0];		// fld [+0xe4]
	const double dy = mHull.mDims04[1];		// [+0xe8]
	const double dz = mHull.mDims04[2];		// [+0xec]
	out[3] = static_cast<float>(sqrt(dx * dx + dy * dy + dz * dz));
	}

// phys_fn_000939 (0x000206c0), BOX-table slot 11.
void BoxShape::nxBoxZeroCenterAndDiagonal(float* out) const
	{
	out[0] = 0.0f;							// xor edx,edx; mov [eax],edx ...
	out[1] = 0.0f;							//   (three dword stores, 0x000206c4..cc)
	out[2] = 0.0f;
	const double dx = mHull.mDims04[0];
	const double dy = mHull.mDims04[1];
	const double dz = mHull.mDims04[2];
	out[3] = static_cast<float>(sqrt(dx * dx + dy * dy + dz * dz));	// 0x000206ce..f8
	}

// phys_fn_000927 (0x00020450), BOX-table slot 13.
bool BoxShape::nxBoxSaveState(void* record)
	{
	memcpy(reinterpret_cast<unsigned char*>(record) + 0x4c,
		&mHull.mDims04[0], sizeof(mHull.mDims04));	// 0x00020450..6e
	return mBase.nxBaseSaveState(record);			// jmp 0x000256f0 at 0x00020473
	}

// phys_fn_000941 (0x00020700), BOX-table slot 8.
void BoxShape::nxBoxLocalAABB(float* out) const
	{
	out[0] = -mHull.mDims04[0];				// fld/fchs/fxch chain, 0x00020700..23
	out[1] = -mHull.mDims04[1];
	out[2] = -mHull.mDims04[2];
	out[3] = mHull.mDims04[0];				// raw copies, 0x00020726..41
	out[4] = mHull.mDims04[1];
	out[5] = mHull.mDims04[2];
	}

// phys_fn_000935 (0x000205a0), BOX-table slot 9. Extent rows of pose one's
// rotation: axis 0 uses r0/r2/r1, axis 1 r3/r5/r4, axis 2 r6/r7/r8 -- the
// same |row . dims| pattern as every OBB bounds helper.
void BoxShape::nxBoxWorldAABB(float* out) const
	{
	const float* r = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation[0]);
	const float dx = mHull.mDims04[0];
	const float dy = mHull.mDims04[1];
	const float dz = mHull.mDims04[2];
	const float e0 = fabsf(dx * r[0]) + fabsf(dy * r[2]) + fabsf(dz * r[1]);
	const float e1 = fabsf(dx * r[3]) + fabsf(dy * r[5]) + fabsf(dz * r[4]);
	const float e2 = fabsf(dx * r[6]) + fabsf(dy * r[8]) + fabsf(dz * r[7]);
	const float* t = mBase.mPose0C.mTranslation;
	out[0] = t[0] - e0;						// fsub [esp+4], 0x00020639
	out[1] = t[1] - e1;
	out[2] = t[2] - e2;
	out[3] = t[0] + e0;						// fadd, 0x0002064b..5b
	out[4] = t[1] + e1;
	out[5] = t[2] + e2;
	}

// phys_fn_000979 (0x00021940), BOX-table slot 0. The image's order: vtable
// restore words (masked on both sides), the colobj destruction, then the
// base-dtor chain whose tail destroys the embedded Prunable. The colobj's
// own deleting row is not transcribed yet -- its writes land in an external
// block, so nothing inside the shape buffer depends on it.
void BoxShape::nxBoxScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		// mov ecx,[esi+0x9c]; test; push 1; call [eax] at 0x0002195b..61 --
		// destroyed through its own vtable by the image.
		}
	mBase.nxBaseDtorOwnerArms();		// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of 0x00026bd0: jmp 0xb5640
	(void) flags;							// flags&1 self-free arm: operator
	}										// delete territory, not modeled

// phys_fn_001399 (0x00028e80), MESH-table slot 0.
void MeshShape::nxMeshScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		// destroyed through its own vtable: 0x00028e93..97
		}
	if(mWordE0)
		{
		--*reinterpret_cast<NxU32*>(mWordE0 + 0x74);	// dec [mesh+0x74]: 0x00028ea3
		}
	mBase.nxBaseDtorOwnerArms();		// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of the base-dtor chain
	(void) flags;							// self-free arm not modeled
	}

// ---------------------------------------------------------------------------
// PlaneShape. See ObjectModel.h for the row map.

#include "NxUtilities.h"

PlaneShape::PlaneShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x00024edb..df
	{
	// The embedded collision object: a fresh 0x1c-byte block through the SDK
	// allocator (0x00024eea..f8), built by phys_fn_001159 -- the plane-family
	// variant of the shared collision-object constructor -- with the plane
	// stored at BOTH +0x08 and +0x18.
	void* memory = nxGetSdkAllocator()->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObject(this)
		: 0;								// null arm: 0x00024f09
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00024f0b

	mBase.mSentinelD0 = 0;					// NX_SHAPE_PLANE: 0x00024f24
	mNormalE0[0] = 0.0f;					// 0x00024f2f
	mNormalE0[1] = 1.0f;					// 0x00024f35
	mNormalE0[2] = 0.0f;					// 0x00024f3c
	mDistanceEC = 0.0f;						// 0x00024f44

	// The image calls NxFoundation's NxNormalToTangents through the import at
	// .rdata 0x1010418c (call 0x00024f4e); the transcription reaches the same
	// function through the reconstruction's own export.
	NxNormalToTangents(
		*reinterpret_cast<const NxVec3*>(&mNormalE0[0]),
		*reinterpret_cast<NxVec3*>(&mTangentF0[0]),
		*reinterpret_cast<NxVec3*>(&mBinormalFC[0]));

	mWord108 = 1;							// 0x00024f57
	}

// phys_fn_001251 (0x00024f80), PLANE-table slot 13.
bool PlaneShape::nxPlaneSaveState(void* record)
	{
	unsigned char* rec = static_cast<unsigned char*>(record);
	memcpy(rec + 0x4c, &mNormalE0[0], sizeof(mNormalE0));	// 0x00024f80..9c
	const float negD = -mDistanceEC;		// fld [+0xec]; fchs at 0x00024f9f..a5
	memcpy(rec + 0x58, &negD, sizeof(negD));
	return mBase.nxBaseSaveState(record);	// jmp 0x000256f0 at 0x00024fae
	}

// phys_fn_001257 (0x000251d0), PLANE-table slots 9 and 11.
void PlaneShape::nxPlaneExtentRow(float* out) const
	{
	out[0] = 0.0f;							// xor ecx,ecx; three stores
	out[1] = 0.0f;
	out[2] = 0.0f;
	unsigned big = 0x7f7fffffu;				// +FLT_MAX: the unbounded reach
	memcpy(out + 3, &big, sizeof(big));
	}

// ---------------------------------------------------------------------------
// MeshShape. See ObjectModel.h for the row map.

MeshShape::MeshShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x00027dbb..bf
	{
	mWordE0 = 0;							// mov [esi+0xe0],0 at 0x00027dca
	mWordE4 = 0;							// mov [esi+0xe4],0 at 0x00027dd4

	// The embedded collision object: a fresh 0x1c-byte block through the SDK
	// allocator (0x00027dde..ec), built by phys_fn_001241 -- the mesh-family
	// variant of the shared collision-object constructor -- with the mesh
	// shape stored at BOTH +0x08 and +0x18.
	void* memory = nxGetSdkAllocator()->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObject(this)
		: 0;								// null arm: 0x00027dfd
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00027dff

	mBase.mSentinelD0 = 4;					// NX_SHAPE_MESH: 0x00027e05
	}

// phys_fn_001385 (0x00027e60), MESH-table slot 13.
bool MeshShape::nxMeshSaveState(void* record)
	{
	const unsigned char* mesh = reinterpret_cast<const unsigned char*>(mWordE0);
	unsigned meshWord = *reinterpret_cast<const unsigned*>(mesh + 0xe4);	// 0x00027e60..66
	unsigned char* rec = static_cast<unsigned char*>(record);
	memcpy(rec + 0x4c, &meshWord, sizeof(meshWord));
	memcpy(rec + 0x50, &mWordE4, sizeof(mWordE4));		// 0x00027e73..79
	return mBase.nxBaseSaveState(record);				// jmp 0x000256f0 at 0x00027e80
	}

// phys_fn_001387 (0x00027e90), MESH-table slot 11.
void MeshShape::nxMeshGetWords5C(unsigned* out) const
	{
	const unsigned char* mesh = reinterpret_cast<const unsigned char*>(mWordE0);
	memcpy(out, mesh + 0x5c, 16);						// four dwords, 0x00027e96..b1
	}

// phys_fn_001389 (0x00027ec0), MESH-table slot 8.
void MeshShape::nxMeshGetWords44(unsigned* out) const
	{
	const unsigned char* mesh = reinterpret_cast<const unsigned char*>(mWordE0);
	memcpy(out, mesh + 0x44, 24);						// six dwords, 0x00027ec0..ef
	}

// ---------------------------------------------------------------------------
// The BASE vtable's stub rows. See ObjectModel.h for the slot map.

// phys_fn_001249 (0x00024f70), base-table slot 4.
bool ShapeBase::nxBaseSlot4(void* /*argument1*/, void* /*argument2*/)
	{
	return false;							// xor al,al; ret 0xc
	}

// phys_fn_004812 (0x000b4070), base-table slot 5.
void* ShapeBase::nxBaseSlot5(void* /*argument1*/, void* /*argument2*/,
	void* /*argument3*/, void* /*argument4*/)
	{
	return 0;								// xor eax,eax; ret 0x14
	}

// phys_fn_001035 (0x00022dd0), base-table slot 7.
bool ShapeBase::nxBaseSlot7(void* /*argument1*/)
	{
	return false;							// xor al,al; ret 8
	}

// phys_fn_001277 (0x000256f0), base-table slot 2. Pure data movement:
// pose three, the halfword trio and [colobj+4] go into a descriptor-shaped
// record; every other byte of the record is untouched.
bool ShapeBase::nxBaseSaveState(void* record)
	{
	unsigned char* rec = static_cast<unsigned char*>(record);
	memcpy(rec + 8, &mPose6C, sizeof(mPose6C));		// rep movsd 9 + three words
	unsigned int de = mHalfwordDE;
	memcpy(rec + 0x38, &de, sizeof(de));			// movzx + dword store
	memcpy(rec + 0x3c, &mHalfwordD8, sizeof(mHalfwordD8));
	memcpy(rec + 0x3e, &mHalfwordDA, sizeof(mHalfwordDA));
	unsigned colobj = mWord9C;						// mov edx,[eax+0x9c]
	unsigned word04 = *reinterpret_cast<const unsigned*>(
		reinterpret_cast<const unsigned char*>(colobj) + 4);
	memcpy(rec + 0x40, &word04, sizeof(word04));	// no null test in the image
	return true;									// mov al,1
	}
