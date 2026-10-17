/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The actor, body and contact visualisation rows (scene-raycast block, Task 4,
// visualisation sub-area; the contract is
// docs/reconstruction/novodex-physics/units/scene-raycast-contract.md).
// 000766, 000869 and 000907 are in the gap after SceneRaycast.cpp; 000020 is
// in the gap before Actor.cpp. The call chain from NxScene::visualize is in
// SceneVisualize.h.
//
// PRECISION. NxScene::visualize is an API-time call, so every row here runs
// under the control word 0x027f (53-bit precision, round to nearest); none is
// reached from the simulation step. The x87 register lifetimes are written
// `double` and the float spills `float`; SSE2 double arithmetic is the x87's
// at 53 bits, so this file keeps the default architecture. The square roots
// go through X87Sqrt.h and the one `fistp` through a naked helper here.
//
// PARAMETERS. The rows read the SDK's live parameter array at .data
// 0x10123b18 (PhysicsSDK.cpp's gParameter) directly, element 4 * index; here
// they are read through PhysicsSDK::getParameter, as the joint rows do
// (revolute-contract.md open issue 8), which returns the same element.

#include "SceneVisualize.h"
#include "ContactPairManager.h"
#include "Containers.h"
#include "PhysicsSDK.h"
#include "NxDebugRenderable.h"
#include "NxBox.h"
#include "NxMat33.h"
#include "NxVec3.h"
#include "NxBodyDesc.h"
#include "NpActorDynamicMath.h"
#include "IcePrunable.h"
#include "IcePruner.h"
#include "Opcode.h"
#include "X87Sqrt.h"

#include <string.h>
#include <vector>
#include <stddef.h>
#include <windows.h>

// phys_fn_001937 (0x0004b9f0): submit each free-pruner world AABB to the
// PhysicsSDK's cached renderable in blue. The record is 24 bytes, matching
// NxBounds3's six contiguous float words.
static __declspec(noinline) unsigned char nxVisualizeFreePrunerWorldBoxes(Pruner* pruner)
	{
	const PruningPool& pool = pruner->mPool;
	const unsigned count = pool.mNbTotal;
	if(!count)
		return 1;
	NxDebugRenderable* const renderable = PhysicsSDK::instance->getDebugRenderable();
	for(unsigned i = 0; i < count; ++i)
		{
		NxBounds3 bounds;
		memcpy(&bounds, &pool.mWorldBoxes[i], sizeof(bounds));
		renderable->addAABB(bounds, 0xff0000ffu, false);
		}
	return 1;
	}

// The parameter indices the rows use, pinned against the element addresses the
// listings read (0x10123b18 + 4 * index).
static_assert(NX_VISUALIZATION_SCALE == 13, "0x10123b4c");
static_assert(NX_VISUALIZE_WORLD_AXES == 14, "0x10123b50");
static_assert(NX_VISUALIZE_BODY_AXES == 15, "0x10123b54");
static_assert(NX_VISUALIZE_BODY_MASS_AXES == 16, "0x10123b58");
static_assert(NX_VISUALIZE_BODY_LIN_VELOCITY == 17, "0x10123b5c");
static_assert(NX_VISUALIZE_BODY_ANG_VELOCITY == 18, "0x10123b60");
static_assert(NX_VISUALIZE_BODY_JOINT_GROUPS == 26, "0x10123b80");
static_assert(NX_VISUALIZE_CONTACT_POINT == 37, "0x10123bac");
static_assert(NX_VISUALIZE_CONTACT_NORMAL == 38, "0x10123bb0");
static_assert(NX_VISUALIZE_CONTACT_ERROR == 39, "0x10123bb4");
static_assert(NX_VISUALIZE_CONTACT_FORCE == 40, "0x10123bb8");
static_assert(NX_VISUALIZE_ACTOR_AXES == 41, "0x10123bbc");
// The ones phys_fn_000657 (Scene.cpp) tests.
static_assert(NX_VISUALIZE_COLLISION_AABBS == 42, "0x10123bc0");
static_assert(NX_VISUALIZE_COLLISION_SHAPES == 43, "0x10123bc4");
static_assert(NX_VISUALIZE_COLLISION_AXES == 44, "0x10123bc8");
static_assert(NX_VISUALIZE_COLLISION_COMPOUNDS == 45, "0x10123bcc");
static_assert(NX_VISUALIZE_COLLISION_SPHERES == 48, "0x10123bd8");

static NxReal nxVisSdkParameter(NxParameter parameter)
	{
	const PhysicsSDK* const sdk = PhysicsSDK::instance;
	return sdk ? sdk->getParameter(parameter) : 0.0f;
	}

// `fistp qword ptr`: the register rounded to an integer by the live control
// word (to nearest under 0x027f), returned in edx:eax. A C cast would
// truncate. Naked so the double reaches the FPU unchanged.
static __declspec(naked) NxI64 __cdecl nxVisFistpQword(double /*x*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		sub		esp, 8
		fistp	qword ptr [esp]
		pop		eax
		pop		edx
		ret
		}
	}

template <class T> static inline T& nxVisAt(const void* record, NxU32 offset)
	{
	return *reinterpret_cast<T*>(const_cast<unsigned char*>(static_cast<const unsigned char*>(record)) + offset);
	}

// ---------------------------------------------------------------------------
// Row 004163 (0x0009aca0, 56 B), inline here. The island object's member
// vector at +0x10/+0x14: slot +0x18 of every element with the renderable,
// the count re-read from the vector after every call, as the listing does.
// The row itself is claimed by ObjectModel.cpp's slot-loop kernel
// (nxVectorVirtualLoop4163), which calls a fixed member function rather than
// the element's own table slot, so it cannot serve here.

class NxVisIslandMember
	{
	public:
	virtual void slot0() = 0;
	virtual void slot1() = 0;
	virtual void slot2() = 0;
	virtual void slot3() = 0;
	virtual void slot4() = 0;
	virtual void slot5() = 0;
	virtual void visualize(NxDebugRenderable& renderable) = 0;		// +0x18
	};

static __declspec(noinline) void nxVisIslandMembers(void* island, NxDebugRenderable& renderable)
	{
	for(NxU32 i = 0; i < static_cast<NxU32>(
		static_cast<NxI32>(nxVisAt<char*>(island, 0x14) - nxVisAt<char*>(island, 0x10)) >> 2); i++)
		nxVisAt<NxVisIslandMember**>(island, 0x10)[i]->visualize(renderable);
	}

// ---------------------------------------------------------------------------
// The two velocity arrows of 000766 share one instruction pattern
// (0x17c61-0x17ce5 for +0x34, 0x17ce7-0x17d7b for +0x1a0, 0x17db7-0x17e38 for
// +0x40, 0x17e3a-0x17ece for +0x1ac): |v| = fsqrt((z z + y y) + x x), stored
// to a float; when that float is not 0.0f (a NaN passes) the reciprocal
// 1.0f / |v| stays in a register and the unit direction is (x r, y r, z r),
// each rounded to float. Inlined in the image, a function here.
static inline void nxVisVectorArrow(NxDebugRenderable& renderable, const NxVec3& position, const float* v,
	NxReal scale, NxU32 colour)
	{
	const double x = v[0];
	const float y = v[1];
	const double z = v[2];
	const float length = static_cast<float>(x87FsqrtDot3(z, z, y, y, x, x));
	if(length != 0.0f)
		{
		const double reciprocal = 1.0 / length;
		NxVec3 direction;
		direction.x = static_cast<float>(reciprocal * x);
		direction.y = static_cast<float>(y * reciprocal);
		direction.z = static_cast<float>(z * reciprocal);
		renderable.addArrow(position, direction, length, scale, colour);
		}
	}

// phys_fn_000766 (0x000179a0, 1377 B)
// Body visualisation, __thiscall on the dynamic record, `ret 4`. Nothing
// unless the body flags (+0x10c) have NX_BF_VISUALIZATION (bit 8). Each part
// is gated by its parameter being non-zero (fucompp against 0.0f; a NaN
// draws) and scaled by NX_VISUALIZATION_SCALE:
// - NX_VISUALIZE_BODY_AXES (0x179b5-0x17b00): the three columns of the mass
//   frame orientation (+0x134, row-major) times scale * param, as lines from
//   the world centre of mass (+0x158) in 0xff0000, 0xff00, 0xff. Every
//   product is stored to float before the add except column 0's y (+0x140),
//   which stays in the register.
// - NX_VISUALIZE_BODY_MASS_AXES (0x17b03-0x17c24): the inertia box. With
//   k = 6.0f / mass (+0x188) and the principal moments I (+0x18c..+0x194),
//   the half extents are fsqrt(((Iz + Iy) - Ix) k), fsqrt(((Ix - Iy) + Iz) k)
//   (both to float) and fsqrt(((Ix + Iy) - Iz) k) (kept), each times the
//   float h = (param * scale) * 0.5f; the box is at the centre of mass with
//   the mass frame orientation, drawn by addOBB (slot +0x28) with frame
//   false and a grey whose level is fistp(+0x4c * 2.5000002f * 255.0f).
// - NX_VISUALIZE_BODY_LIN_VELOCITY (0x17c27-0x17d7b): arrows of the linear
//   velocity (+0x34) in 0xffffff and of +0x1a0 in 0x8000, scale the float
//   param * scale, by addArrow (slot +0x30).
// - NX_VISUALIZE_BODY_ANG_VELOCITY (0x17d7d-0x17ece): the same for the
//   angular velocity (+0x40) in 0 and +0x1ac in 0x5f1fe0.
// - NX_VISUALIZE_BODY_JOINT_GROUPS (0x17ed0-0x17ef3): the island object at
//   +0x1e0, when there is one, through phys_fn_004163.
void NxBodyVisualRecord::visualize(NxDebugRenderable& renderable)
	{
	if(!(nxVisAt<NxU32>(this, 0x10c) & NX_BF_VISUALIZATION))
		return;
	const float* frame = &nxVisAt<float>(this, 0x134);
	const NxVec3& centre = nxVisAt<NxVec3>(this, 0x158);

	if(nxVisSdkParameter(NX_VISUALIZE_BODY_AXES) != 0.0f)
		{
		const double scale = static_cast<double>(nxVisSdkParameter(NX_VISUALIZATION_SCALE)) *
			nxVisSdkParameter(NX_VISUALIZE_BODY_AXES);
		const float x0 = static_cast<float>(frame[0] * scale);
		const double y0 = frame[3] * scale;
		const float z0 = static_cast<float>(frame[6] * scale);
		const float x1 = static_cast<float>(frame[1] * scale);
		const float y1 = static_cast<float>(frame[4] * scale);
		const float z1 = static_cast<float>(frame[7] * scale);
		const float x2 = static_cast<float>(frame[2] * scale);
		const float y2 = static_cast<float>(frame[5] * scale);
		const float z2 = static_cast<float>(frame[8] * scale);
		NxVec3 end;
		end.x = static_cast<float>(static_cast<double>(x0) + centre.x);
		end.y = static_cast<float>(y0 + centre.y);
		end.z = static_cast<float>(static_cast<double>(z0) + centre.z);
		renderable.addLine(centre, end, 0xff0000);
		end.x = static_cast<float>(static_cast<double>(x1) + centre.x);
		end.y = static_cast<float>(static_cast<double>(y1) + centre.y);
		end.z = static_cast<float>(static_cast<double>(z1) + centre.z);
		renderable.addLine(centre, end, 0xff00);
		end.x = static_cast<float>(static_cast<double>(x2) + centre.x);
		end.y = static_cast<float>(static_cast<double>(y2) + centre.y);
		end.z = static_cast<float>(static_cast<double>(z2) + centre.z);
		renderable.addLine(centre, end, 0xff);
		}

	if(nxVisSdkParameter(NX_VISUALIZE_BODY_MASS_AXES) != 0.0f)
		{
		NxBox box;
		memcpy(&box.center, &centre, sizeof(NxVec3));
		const double k = 6.0 / nxVisAt<float>(this, 0x188);
		const double ix = nxVisAt<float>(this, 0x18c);
		const double iy = nxVisAt<float>(this, 0x190);
		const double iz = nxVisAt<float>(this, 0x194);
		const float e0 = static_cast<float>(x87Fsqrt(((iz + iy) - ix) * k));
		const float e1 = static_cast<float>(x87Fsqrt(((ix - iy) + iz) * k));
		const double e2 = x87Fsqrt(((ix + iy) - iz) * k);
		const float half = static_cast<float>((static_cast<double>(nxVisSdkParameter(NX_VISUALIZE_BODY_MASS_AXES)) *
			nxVisSdkParameter(NX_VISUALIZATION_SCALE)) * 0.5f);
		box.extents.z = static_cast<float>(e2 * half);
		box.extents.x = static_cast<float>(static_cast<double>(e0) * half);
		box.extents.y = static_cast<float>(static_cast<double>(e1) * half);
		memcpy(&box.rot, frame, 9 * sizeof(float));
		const NxU32 grey = static_cast<NxU32>(nxVisFistpQword(
			(static_cast<double>(nxVisAt<float>(this, 0x4c)) * 2.5000002f) * 255.0f));
		renderable.addOBB(box, (((grey | 0xffffff00u) << 8) | grey) << 8 | grey, false);
		}

	if(nxVisSdkParameter(NX_VISUALIZE_BODY_LIN_VELOCITY) != 0.0f)
		{
		const NxReal scale = static_cast<float>(static_cast<double>(nxVisSdkParameter(NX_VISUALIZE_BODY_LIN_VELOCITY)) *
			nxVisSdkParameter(NX_VISUALIZATION_SCALE));
		nxVisVectorArrow(renderable, centre, &nxVisAt<float>(this, 0x34), scale, 0xffffff);
		nxVisVectorArrow(renderable, centre, &nxVisAt<float>(this, 0x1a0), scale, 0x8000);
		}

	if(nxVisSdkParameter(NX_VISUALIZE_BODY_ANG_VELOCITY) != 0.0f)
		{
		const NxReal scale = static_cast<float>(static_cast<double>(nxVisSdkParameter(NX_VISUALIZE_BODY_ANG_VELOCITY)) *
			nxVisSdkParameter(NX_VISUALIZATION_SCALE));
		nxVisVectorArrow(renderable, centre, &nxVisAt<float>(this, 0x40), scale, 0);
		nxVisVectorArrow(renderable, centre, &nxVisAt<float>(this, 0x1ac), scale, 0x5f1fe0);
		}

	if(nxVisSdkParameter(NX_VISUALIZE_BODY_JOINT_GROUPS) != 0.0f)
		{
		void* island = nxVisAt<void*>(this, 0x1e0);
		if(island)
			nxVisIslandMembers(island, renderable);
		}
	}

// The actor's global pose as 000020 forms it twice, inline (0x100015cf-
// 0x100016b4 and 0x100016ef-0x100017d4): with a body (+0x08), the rotation of
// the body's quaternion (+0x24, w last) in the listing's spill pattern
// (nxNpActorComposeRotation) and the body's position (+0x18); without one,
// the actor's own 3x4 pose at +0x20.
static inline void nxVisActorGlobalPose(const void* actor, float* pose)
	{
	const unsigned char* body = nxVisAt<unsigned char*>(actor, 0x08);
	if(body)
		{
		nxNpActorComposeRotation(reinterpret_cast<const float*>(body + 0x24), pose);
		memcpy(pose + 9, body + 0x18, 3 * sizeof(float));
		}
	else
		memcpy(pose, &nxVisAt<float>(actor, 0x20), 12 * sizeof(float));
	}

// phys_fn_000020 (0x00001560, 731 B)
// Actor visualisation, __thiscall on the actor record, `ret 4`. When
// NX_VISUALIZE_ACTOR_AXES is non-zero (a NaN draws) it draws the global
// pose's basis through addBasis (slot +0x34): lengths (1, 1, 1), scale the
// raw parameter (not times NX_VISUALIZATION_SCALE), colours 0xffff0000,
// 0xff00ff00, 0xff0000ff. The pose is formed twice; the listing passes the
// first copy's rotation and the second copy's position. Then, whatever the
// parameter, the body (+0x08), when there is one, visualises itself
// (phys_fn_000766).
void NxActorVisualRecord::visualize(NxDebugRenderable& renderable)
	{
	if(nxVisSdkParameter(NX_VISUALIZE_ACTOR_AXES) != 0.0f)
		{
		NxVec3 lengths;
		lengths.x = 1.0f;
		lengths.y = 1.0f;
		lengths.z = 1.0f;
		NxU32 colours[3] = { 0xffff0000u, 0xff00ff00u, 0xff0000ffu };
		float rotationPose[12];
		float positionPose[12];
		nxVisActorGlobalPose(this, rotationPose);
		nxVisActorGlobalPose(this, positionPose);
		NxMat33 columns;
		memcpy(&columns, rotationPose, 9 * sizeof(float));
		NxVec3 position;
		memcpy(&position, positionPose + 9, sizeof(NxVec3));
		renderable.addBasis(position, columns, lengths, nxVisSdkParameter(NX_VISUALIZE_ACTOR_AXES), colours);
		}
	NxBodyVisualRecord* body = nxVisAt<NxBodyVisualRecord*>(this, 0x08);
	if(body)
		body->visualize(renderable);
	}

// The stream 000869 reads at +0x40 is the entries pointer of the actor pair's
// SdkContainer at +0x38 (ContactPairManager.h), which 000873/000875 fill.
static_assert(offsetof(SdkContainer, mEntries) == 8, "actor pair +0x40");

// phys_fn_000869 (0x0001d2d0, 699 B)
// Contact visualisation, __thiscall on the actor pair (the pair node's +0x14),
// `ret 4`. Walks the contact stream the emitters 000873/000875 write (the
// container at +0x38, entries at +0x40: the pair count word, then the pairs):
// - a patch header is 12 bytes; its third word holds the normal count in the
//   low half and the flags in bits 16-23. A header with flag 2 is skipped
//   (the next header follows at once).
// - each normal is 16 bytes: the normal (3 floats) and its point count;
// - each point is 16 bytes: the position (3 floats) and the separation, then
//   4 more bytes when flag 1 is set, and 4 (separation >= 0) or 8 (sign bit
//   set) more when flag 4 is set.
// Per point, one line from the point along the normal, of length
// NX_VISUALIZE_CONTACT_FORCE * scale in 0xff0000 when that parameter is set
// (the length is the parameter, not the contact's force), else
// NX_VISUALIZE_CONTACT_NORMAL * scale in 0xff, else
// |-|separation| * NX_VISUALIZE_CONTACT_ERROR * scale| in 0xffff00, drawn only
// when the length is not 0 (a NaN draws); then, when NX_VISUALIZE_CONTACT_POINT
// is set, a three-line cross of half size d = float(float(param * scale) *
// 0.1f) in 0xff whose second and third lines re-add and re-subtract d from the
// first's rounded coordinates (0x1d489-0x1d578).
__declspec(noinline) void NxActorPair::row000869(NxDebugRenderable& renderable)
	{
	const float pointScale = static_cast<float>(static_cast<double>(nxVisSdkParameter(NX_VISUALIZE_CONTACT_POINT)) *
		nxVisSdkParameter(NX_VISUALIZATION_SCALE));
	const unsigned char* stream = reinterpret_cast<const unsigned char*>(at<SdkContainer>(0x38).mEntries);
	NxU32 patches;
	if(stream)
		{
		patches = *reinterpret_cast<const NxU32*>(stream);
		stream += 4;
		}
	else
		patches = 0;

	for(;;)
		{
		if(patches == 0)
			return;
		NxU32 header;
		NxU16 normals;
		NxU8 flags;
		for(;;)
			{
			header = *reinterpret_cast<const NxU32*>(stream + 8);
			stream += 12;
			normals = static_cast<NxU16>(header);
			flags = static_cast<NxU8>(header >> 16);
			patches--;
			if(!(flags & 2))
				break;
			if(patches == 0)
				return;
			}

		while(normals-- != 0)
			{
			const float* normal = reinterpret_cast<const float*>(stream);
			NxU32 points = *reinterpret_cast<const NxU32*>(stream + 0xc);
			stream += 0x10;
			while(points-- != 0)
				{
				const NxVec3& point = *reinterpret_cast<const NxVec3*>(stream);
				const NxU32 separationWord = *reinterpret_cast<const NxU32*>(stream + 0xc);
				stream += 0x10;
				const NxU32 negative = separationWord & 0x80000000u;
				const NxU32 negatedWord = separationWord | 0x80000000u;
				if(flags & 1)
					stream += 4;
				if(flags & 4)
					stream += negative ? 8 : 4;

				NxU32 colour = 0xff;
				double length = 0.0;
				bool draw = true;
				if(nxVisSdkParameter(NX_VISUALIZE_CONTACT_FORCE) != 0.0f)
					{
					colour = 0xff0000;
					length = static_cast<double>(nxVisSdkParameter(NX_VISUALIZE_CONTACT_FORCE)) *
						nxVisSdkParameter(NX_VISUALIZATION_SCALE);
					}
				else if(nxVisSdkParameter(NX_VISUALIZE_CONTACT_NORMAL) != 0.0f)
					length = static_cast<double>(nxVisSdkParameter(NX_VISUALIZE_CONTACT_NORMAL)) *
						nxVisSdkParameter(NX_VISUALIZATION_SCALE);
				else if(nxVisSdkParameter(NX_VISUALIZE_CONTACT_ERROR) != 0.0f)
					{
					float negated;
					memcpy(&negated, &negatedWord, 4);
					colour = 0xffff00;
					const double product = (static_cast<double>(negated) * nxVisSdkParameter(NX_VISUALIZE_CONTACT_ERROR)) *
						nxVisSdkParameter(NX_VISUALIZATION_SCALE);
					length = product < 0.0 ? -product : product;
					}
				else
					draw = false;

				if(draw && length != 0.0)
					{
					const float nx = static_cast<float>(length * normal[0]);
					const float ny = static_cast<float>(length * normal[1]);
					const double z = length * normal[2] + point.z;
					NxVec3 end;
					end.x = static_cast<float>(static_cast<double>(nx) + point.x);
					end.y = static_cast<float>(static_cast<double>(ny) + point.y);
					end.z = static_cast<float>(z);
					renderable.addLine(point, end, colour);
					}

				if(nxVisSdkParameter(NX_VISUALIZE_CONTACT_POINT) != 0.0f)
					{
					const float d = static_cast<float>(static_cast<double>(pointScale) * 0.1f);
					NxVec3 a;
					NxVec3 b;
					a.x = static_cast<float>(static_cast<double>(point.x) - d);
					a.y = point.y;
					a.z = point.z;
					b.x = static_cast<float>(static_cast<double>(point.x) + d);
					b.y = point.y;
					b.z = point.z;
					renderable.addLine(a, b, 0xff);
					a.x = static_cast<float>(static_cast<double>(a.x) + d);
					b.x = static_cast<float>(static_cast<double>(b.x) - d);
					a.y = static_cast<float>(static_cast<double>(a.y) - d);
					b.y = static_cast<float>(static_cast<double>(b.y) + d);
					renderable.addLine(a, b, 0xff);
					a.y = static_cast<float>(static_cast<double>(a.y) + d);
					b.y = static_cast<float>(static_cast<double>(b.y) - d);
					a.z = static_cast<float>(static_cast<double>(a.z) - d);
					b.z = static_cast<float>(static_cast<double>(b.z) + d);
					renderable.addLine(a, b, 0xff);
					}
				}
			}
		}
	}

// phys_fn_000907 (0x0001fda0, 8 B)
// `add ecx, 0x14; jmp 0x1001d2d0`: the node's actor pair visualises its
// contacts (phys_fn_000869). Called by phys_fn_000657 for every pair node on
// the Scene's list whose stamp (+0x104) is the Scene's (+0x540).
void NxPairNode::row000907(NxDebugRenderable& renderable)
	{
	pair()->row000869(renderable);
	}

// ---------------------------------------------------------------------------
// Placeholders for the remaining rows phys_fn_000657 calls that are not
// written. They carry no stable-ID line:
// - 000638 (0x000128e0, 428 B, cdecl (engine, renderable, colour, compounds)):
//   the world boxes of the engine's objects (NX_VISUALIZE_COLLISION_AABBS,
//   0xffffff00; NX_VISUALIZE_COLLISION_COMPOUNDS, 0xffff00ff).
// - 000581 (0x00010a50, 29 B + 000583's 62 B, cdecl (engine, renderable)):
//   slot +0xc of every engine object (the shapes' own visualisation;
//   NX_VISUALIZE_COLLISION_SHAPES, _AXES or _SPHERES).
// - 003639 (0x00089dc0, 52 B, thiscall on the fluid manager at Scene +0x61c,
//   `ret 4`), which the candidate never creates.
// ---------------------------------------------------------------------------

static __declspec(noinline) bool nxSceneVisualizePrunerDispatch(void* engine,
	bool visualizeStatic, bool visualizeDynamic, bool visualizeFree)
	{
	if(!engine)
		return false;
	const auto drawTree = [](AABBTree* tree, NxU32 colour)
		{
		if(!tree)
			return;
		NxDebugRenderable* const prunerRenderable = PhysicsSDK::instance
			? PhysicsSDK::instance->getDebugRenderable() : 0;
		if(!prunerRenderable)
			return;
		struct WalkContext
			{
			NxDebugRenderable* renderable;
			NxU32 colour;
			};
		WalkContext context = { prunerRenderable, colour };
		const auto drawNode = [](const AABBTreeNode* node, udword, void* userData) -> bool
			{
			WalkContext* const context = static_cast<WalkContext*>(userData);
			NxBounds3 bounds;
			memcpy(&bounds, node->GetAABB(), sizeof(bounds));
			context->renderable->addAABB(bounds, context->colour, false);
			return true;
			};
		tree->Walk(drawNode, &context);
		};

	unsigned char* const bytes = static_cast<unsigned char*>(engine);
	Pruner** const pruners = reinterpret_cast<Pruner**>(bytes + 0x1c);
	if(visualizeStatic)
		{
		StaticPruner* const pruner = static_cast<StaticPruner*>(pruners[0]);
		if(pruner)
			{
			pruner->BuildTree();
			drawTree(pruner->mTree, 0xffff0000u);
			}
		}
	if(visualizeDynamic)
		{
		BoundedDynamicPruner* const pruner = static_cast<BoundedDynamicPruner*>(pruners[1]);
		if(pruner)
			{
			pruner->BuildVisualizationTree();
			const udword count = pruner->mPool.mNbObjects[0] + pruner->mPool.mNbObjects[1];
			if(count)
				{
				NxDebugRenderable* const prunerRenderable = PhysicsSDK::instance
					? PhysicsSDK::instance->getDebugRenderable() : 0;
				if(prunerRenderable)
					{
					struct WalkContext
						{
						NxDebugRenderable* renderable;
						};
					WalkContext context = { prunerRenderable };
					const auto drawNode = [](const AABB& node, void* userData)
						{
						WalkContext* const context = static_cast<WalkContext*>(userData);
						NxBounds3 bounds;
						Point min, max;
						node.GetMin(min);
						node.GetMax(max);
						bounds.set(NxVec3(min.x, min.y, min.z), NxVec3(max.x, max.y, max.z));
						context->renderable->addAABB(bounds, 0xffff0000u, false);
						};
					pruner->WalkVisualizationTree(drawNode, &context);
					}
				NxDebugRenderable* const boundsRenderable = PhysicsSDK::instance
					? PhysicsSDK::instance->getDebugRenderable() : 0;
				if(boundsRenderable)
					{
					NxBounds3 bounds;
					Point min, max;
					pruner->mBounds.GetMin(min);
					pruner->mBounds.GetMax(max);
					bounds.set(NxVec3(min.x, min.y, min.z), NxVec3(max.x, max.y, max.z));
					boundsRenderable->addAABB(bounds, 0xffff00ffu, false);
					}
				}
			}
		Type3StaticPruner* const type3Pruner = static_cast<Type3StaticPruner*>(pruners[3]);
		if(type3Pruner)
			{
			if(!type3Pruner->mTree)
				type3Pruner->BuildTree();
			drawTree(type3Pruner->mTree, 0xffff0000u);
			}
		}
	if(visualizeFree)
		{
		Pruner* const pruner = pruners[2];
		if(pruner)
			nxVisualizeFreePrunerWorldBoxes(pruner);
		}
	return true;
	}

void nxSceneVisualizeCollisionPruners(void* engine, NxDebugRenderable* renderable)
	{
	const bool visualizeSap = PhysicsSDK::instance->getParameter(NX_VISUALIZE_COLLISION_SAP) != 0.0f;
	const bool visualizeStatic = PhysicsSDK::instance->getParameter(NX_VISUALIZE_COLLISION_STATIC) != 0.0f;
	const bool visualizeDynamic = PhysicsSDK::instance->getParameter(NX_VISUALIZE_COLLISION_DYNAMIC) != 0.0f;
	const bool visualizeFree = PhysicsSDK::instance->getParameter(NX_VISUALIZE_COLLISION_FREE) != 0.0f;
	if(!visualizeSap && !visualizeStatic && !visualizeDynamic && !visualizeFree)
		return;
	if(!nxSceneVisualizePrunerDispatch(engine, visualizeStatic, visualizeDynamic, visualizeFree))
		return;

	unsigned char* const bytes = static_cast<unsigned char*>(engine);
	if(visualizeSap)
		{
		void* const sap = *reinterpret_cast<void**>(bytes + 0x2c);
		if(sap)
			{
			const unsigned char* const sapBytes = static_cast<const unsigned char*>(sap);
			const unsigned count = *reinterpret_cast<const unsigned*>(sapBytes + 0x18);
			const unsigned char* const entries = *reinterpret_cast<const unsigned char* const*>(sapBytes + 0x1c);
			for(unsigned i = 0; entries && i < count; ++i)
				{
				const unsigned char* const entry = entries + i * 24;
				NxBounds3 bounds;
				unsigned words[6];
				for(unsigned axis = 0; axis < 6; ++axis)
					{
					const unsigned* const word = *reinterpret_cast<const unsigned* const*>(entry + axis * 4);
					words[axis] = *word;
					}
				memcpy(&bounds, words, sizeof(bounds));
				renderable->addAABB(bounds, 0xff00ffffu, false);
				}
			}
		}
	}
void nxSceneVisualizeCollisionBounds(void* engine, NxDebugRenderable* renderable, NxU32 colour, bool compounds)
	{
	// phys_fn_000638 (0x000128e0, 428 bytes) walks sections 1 and 2 of the
	// static pruner at engine+0x1c, then the selected dynamic pruner at
	// engine+0x1c+selectedType*4. Each pool entry's owner is the shape; flag
	// bit 3 at shape+0xde gates drawing, and compounds mode restricts type
	// shape+0xd0 to the internal compound-group type 5. Its world AABB is
	// resolved from the Prunable handle and submitted with the chosen color.
	// The caller ignores the draw call's return register, matching the source
	// helper's void interface.
	const auto visualizePruner = [renderable, colour, compounds](Pruner* pruner)
		{
		if(!pruner)
			return;
		const PruningPool& pool = pruner->mPool;
		const unsigned count = pool.mNbObjects[1] + pool.mNbObjects[2];
		for(unsigned index = 0; index < count; ++index)
			{
			Prunable* const prunable = pool.mObjects[pool.mNbObjects[0] + index];
			const unsigned char* const shape = static_cast<const unsigned char*>(prunable->mOwner);
			unsigned short flags;
			unsigned type;
			memcpy(&flags, shape + 0xde, sizeof(flags));
			if((flags & 8u) == 0)
				continue;
			memcpy(&type, shape + 0xd0, sizeof(type));
			if(compounds && type != 5u)
				continue;
			NxBounds3 bounds;
			memcpy(&bounds, prunable->GetWorldAABB(), sizeof(bounds));
			renderable->addAABB(bounds, colour, false);
			}
		};

	unsigned char* const bytes = static_cast<unsigned char*>(engine);
	Pruner* const staticPruner = *reinterpret_cast<Pruner**>(bytes + 0x1c);
	visualizePruner(staticPruner);
	const unsigned selectedType = *reinterpret_cast<unsigned*>(bytes + 0x70);
	Pruner* const dynamicPruner = *reinterpret_cast<Pruner**>(bytes + 0x1c + selectedType * 4);
	visualizePruner(dynamicPruner);
	}

void nxSceneVisualizeCollisionShapes(void* engine, NxDebugRenderable* renderable)
	{
	// phys_fn_000581 walks section 1 and section 2 of the static pruner;
	// phys_fn_000583 continues over the same sections of the engine-selected dynamic
	// pruner. Each pool entry is a Prunable; its owner at +4 is the ShapeBase
	// object, whose virtual slot 3 draws the shape into the Scene renderable.
	const auto visualizePruner = [renderable](Pruner* pruner)
		{
		const PruningPool& pool = pruner->mPool;
		const unsigned count = pool.mNbObjects[1] + pool.mNbObjects[2];
		for(unsigned index = 0; index < count; ++index)
			{
			Prunable* const prunable = pool.mObjects[pool.mNbObjects[0] + index];
			void* const shape = prunable->mOwner;
			void** const vtable = *reinterpret_cast<void***>(shape);
			typedef void (__thiscall* NxShapeVisualizeFn)(void*, NxDebugRenderable*);
			(reinterpret_cast<NxShapeVisualizeFn>(vtable[3]))(shape, renderable);
			}
		};

	unsigned char* const bytes = static_cast<unsigned char*>(engine);
	Pruner* const staticPruner = *reinterpret_cast<Pruner**>(bytes + 0x1c);
	visualizePruner(staticPruner);
	const unsigned selectedType = *reinterpret_cast<unsigned*>(bytes + 0x70);
	Pruner* const dynamicPruner = *reinterpret_cast<Pruner**>(bytes + 0x1c + selectedType * 4);
	visualizePruner(dynamicPruner);
	}

extern "C" void* __cdecl nxNpFluidEmitterStoreGlobalPose(void* internal);
extern "C" float __cdecl nxNpFluidEmitterGetDimensionX(void* internal);
extern "C" float __cdecl nxNpFluidEmitterGetDimensionY(void* internal);
extern "C" void __cdecl nxNpFluidEmitterVisualizeRecord(void* internal,
	unsigned, unsigned, void*, NxDebugRenderable* renderable);

typedef float (__cdecl *NxVisualizeFluidGetCellSizeFn)(void*, void*);
typedef void* (__cdecl *NxVisualizeFluidGetParticlesFn)(void*, void*);

static void nxVisualizeFluidRecord(void* internal, NxDebugRenderable* renderable)
	{
	if(!internal || !renderable)
		return;
	unsigned char* const fluid = static_cast<unsigned char*>(internal);
	if((*reinterpret_cast<unsigned*>(fluid + 0x58) & 1u) == 0)
		return;

	unsigned* emitters = *reinterpret_cast<unsigned**>(fluid + 4);
	unsigned* const emittersEnd = *reinterpret_cast<unsigned**>(fluid + 8);
	if(emitters && emittersEnd && emittersEnd >= emitters)
		for(; emitters != emittersEnd; ++emitters)
			nxNpFluidEmitterVisualizeRecord(
				reinterpret_cast<void*>(*emitters), 0, 0, 0, renderable);

	unsigned char* const manager = *reinterpret_cast<unsigned char**>(fluid + 0x7c);
	void* const backendManager = manager
		? *reinterpret_cast<void**>(manager + 0x30) : 0;
	void* const backendFluid = *reinterpret_cast<void**>(fluid + 0x80);
	HMODULE const fluidModel = GetModuleHandleA("FluidModel.DLL");
	NxVisualizeFluidGetCellSizeFn getCellSize = fluidModel
		? reinterpret_cast<NxVisualizeFluidGetCellSizeFn>(
			GetProcAddress(fluidModel, "FluidGetCellSize")) : 0;
	NxVisualizeFluidGetParticlesFn getParticles = fluidModel
		? reinterpret_cast<NxVisualizeFluidGetParticlesFn>(
			GetProcAddress(fluidModel, "FluidGetParticles")) : 0;
	if(getCellSize)
		(void)getCellSize(backendManager, backendFluid);
	if(getParticles)
		(void)getParticles(backendManager, backendFluid);

	// The oracle submits the cached fluid AABB in red when the bounds parameter
	// is enabled. Particle-position, velocity, and kernel-radius branches are
	// reconstructed separately from this shared traversal.
	if(nxVisSdkParameter(NX_VISUALIZE_FLUID_BOUNDS) > 0.0f)
		{
		NxBounds3 bounds;
		memcpy(&bounds, fluid + 0x5c, sizeof(bounds));
		renderable->addAABB(bounds, 0xffff0000u, false);
		}
	}

void nxSceneVisualizeFluids(void* fluids, NxDebugRenderable* renderable)
	{
	if(!fluids || !renderable)
		return;
	unsigned char* const manager = static_cast<unsigned char*>(fluids);
	void** fluid = *reinterpret_cast<void***>(manager + 4);
	void** const fluidsEnd = *reinterpret_cast<void***>(manager + 8);
	while(fluid && fluidsEnd && fluid < fluidsEnd)
		nxVisualizeFluidRecord(*fluid++, renderable);
	}

static NxVec3 nxFluidEmitterVisualPlanePoint(const float* pose, double x, double y)
	{
	const double px = (double)pose[0] * x + (double)pose[1] * y + pose[9];
	const double py = (double)pose[3] * x + (double)pose[4] * y + pose[10];
	const double pz = (double)pose[6] * x + (double)pose[7] * y + pose[11];
	return NxVec3((NxReal)px, (NxReal)py, (NxReal)pz);
	}

extern "C" void __cdecl nxNpFluidEmitterVisualizeRecord(void* internal,
	unsigned, unsigned, void*, NxDebugRenderable* renderable)
	{
	if(!internal || !renderable
		|| (*reinterpret_cast<unsigned char*>(static_cast<unsigned char*>(internal) + 0x10) & 1) == 0)
		return;
	const NxReal* const parameters = nxPhysicsSDKParameters();
	const NxReal scale = parameters[NX_VISUALIZATION_SCALE]
		* parameters[NX_VISUALIZE_FLUID_EMITTERS];
	if(scale == 0.0f)
		return;

	float pose[12];
	if(!nxNpFluidEmitterStoreGlobalPose(internal))
		return;
	memcpy(pose, static_cast<unsigned char*>(internal) + 0x48, sizeof(pose));
	const NxReal dimensionX = nxNpFluidEmitterGetDimensionX(internal);
	const NxReal dimensionY = nxNpFluidEmitterGetDimensionY(internal);
	unsigned char* const fluid = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(internal) + 4);
	unsigned char* const backend = *reinterpret_cast<unsigned char**>(fluid + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(backend + 0x30);
	const unsigned fluidHandle = *reinterpret_cast<unsigned*>(fluid + 0x80);
	typedef float (__cdecl *RestDistanceFn)(unsigned, unsigned, ...);
	HMODULE fluidModel = GetModuleHandleA("FluidModel.DLL");
	RestDistanceFn getRestDistance = reinterpret_cast<RestDistanceFn>(
		GetProcAddress(fluidModel, "FluidGetRestParticleDistance"));
	if(!getRestDistance)
		return;
	const NxReal restDistance = getRestDistance(context, fluidHandle);
	NxMat33 emitterRotation;
	memcpy(&emitterRotation, pose, sizeof(emitterRotation));
	const NxVec3 emitterPosition(pose[9], pose[10], pose[11]);
	const NxVec3 emitterLengths(scale, scale, scale + scale);
	NxU32 emitterColors[3] = { 0xffff0000u, 0xff00ff00u, 0xff0000ffu };
	renderable->addBasis(emitterPosition, emitterRotation, emitterLengths,
		1.0f, emitterColors);
	const NxVec3 p0 = nxFluidEmitterVisualPlanePoint(pose, dimensionX, dimensionY);
	const NxVec3 p1 = nxFluidEmitterVisualPlanePoint(pose, dimensionX, -dimensionY);
	const NxVec3 p2 = nxFluidEmitterVisualPlanePoint(pose, -dimensionX, -dimensionY);
	const NxVec3 p3 = nxFluidEmitterVisualPlanePoint(pose, -dimensionX, dimensionY);
	const NxU32 lineColor = 0xff0000ffu;
	renderable->addLine(p0, p1, lineColor);
	renderable->addLine(p1, p2, lineColor);
	renderable->addLine(p2, p3, lineColor);
	renderable->addLine(p3, p0, lineColor);

	NxMat33 frameRotation;
	frameRotation.id();
	NxVec3 framePosition(0.0f, 0.0f, 0.0f);
	unsigned char* const frameActor = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(internal) + 0x14);
	if(frameActor)
		{
		float frameWords[12];
		unsigned char* const body = *reinterpret_cast<unsigned char**>(frameActor + 8);
		if(body)
			{
			const float x = *reinterpret_cast<float*>(body + 9 * sizeof(float));
			const float y = *reinterpret_cast<float*>(body + 10 * sizeof(float));
			const float z = *reinterpret_cast<float*>(body + 11 * sizeof(float));
			const float w = *reinterpret_cast<float*>(body + 12 * sizeof(float));
			const float ySquareSum = static_cast<float>(
				static_cast<double>(y) * y + static_cast<double>(y) * y);
			const double zSquareSum = static_cast<double>(z) * z
				+ static_cast<double>(z) * z;
			const double xSquareSum = static_cast<double>(x) * x
				+ static_cast<double>(x) * x;
			const double ySquareWide = static_cast<double>(y) * y
				+ static_cast<double>(y) * y;
			frameWords[0] = static_cast<float>(1.0 - ySquareSum - zSquareSum);
			frameWords[1] = (x * y + x * y) - (z * w + z * w);
			frameWords[2] = (x * z + x * z) + (y * w + y * w);
			frameWords[3] = (x * y + x * y) + (z * w + z * w);
			frameWords[4] = static_cast<float>(1.0 - xSquareSum - zSquareSum);
			frameWords[5] = (y * z + y * z) - (x * w + x * w);
			frameWords[6] = (x * z + x * z) - (y * w + y * w);
			frameWords[7] = (y * z + y * z) + (x * w + x * w);
			const float xSquareRounded = static_cast<float>(xSquareSum);
			const float ySquareRounded = static_cast<float>(ySquareWide);
			frameWords[8] = (1.0f - xSquareRounded) - ySquareRounded;
			framePosition.set(*reinterpret_cast<float*>(body + 6 * sizeof(float)),
				*reinterpret_cast<float*>(body + 7 * sizeof(float)),
				*reinterpret_cast<float*>(body + 8 * sizeof(float)));
			}
		else
			{
			memcpy(frameWords, frameActor + 0x20, sizeof(frameWords));
			framePosition.set(frameWords[9], frameWords[10], frameWords[11]);
			}
		memcpy(&frameRotation, frameWords, sizeof(frameRotation));
		}
	const NxVec3 frameLengths(1.0f, 1.0f, 1.0f);
	NxU32 frameColors[3] = { 0xffffff00u, 0xff00ffffu, 0xffff00ffu };
	renderable->addBasis(framePosition, frameRotation, frameLengths, 1.0f,
		frameColors);

	if(restDistance <= 0.0f)
		return;
	const NxReal rowSpacing = static_cast<NxReal>(sqrt(3.0) * restDistance * 0.5);
	const unsigned columns = 2 * static_cast<unsigned>(dimensionX / restDistance + 0.5);
	const unsigned rows = 2 * static_cast<unsigned>(dimensionY / rowSpacing + 0.5f);
	if(columns == 0 || rows == 0 || columns > 4096 || rows > 4096)
		return;
	const double rowFactor = static_cast<double>(rows - 1);
	const double columnFactor = static_cast<double>(columns) - 0.5;
	const NxReal xBaseFirst = static_cast<NxReal>(pose[0] * columnFactor
		* restDistance * 0.5);
	const NxReal xBase = (pose[9] - xBaseFirst)
		- static_cast<NxReal>(pose[1] * rowFactor * rowSpacing * 0.5);
	const NxReal yBaseFirst = static_cast<NxReal>(pose[3] * columnFactor);
	const NxReal yBase = (pose[10] - yBaseFirst * restDistance * 0.5f)
		- static_cast<NxReal>(pose[4] * rowFactor * rowSpacing * 0.5);
	const NxReal zBaseFirst = static_cast<NxReal>(pose[6] * columnFactor);
	const NxReal zBase = (pose[11] - zBaseFirst * restDistance * 0.5f)
		- static_cast<NxReal>(pose[7] * rowFactor * rowSpacing * 0.5);
	const NxReal radius = restDistance * 0.25f;
	const NxU32 particleColor = 0xff0000ffu;
	for(unsigned row = 0; row < rows; ++row)
		{
		const NxReal stagger = (row & 1) ? restDistance * 0.5f : 0.0f;
		const NxReal rowDistance = static_cast<NxReal>(row) * rowSpacing;
		for(unsigned column = 0; column < columns; ++column)
			{
			const double columnDistance = static_cast<double>(column)
				* restDistance + stagger;
			const NxReal rowX = static_cast<NxReal>(pose[1] * rowDistance);
			const NxReal rowY = static_cast<NxReal>(pose[4] * rowDistance);
			const NxReal rowZ = static_cast<NxReal>(pose[7] * rowDistance);
			const NxVec3 position(
				static_cast<NxReal>(pose[0] * columnDistance + xBase + rowX),
				static_cast<NxReal>(pose[3] * columnDistance + yBase + rowY),
				static_cast<NxReal>(pose[6] * columnDistance + zBase + rowZ));
		float circleWords[12];
		memcpy(circleWords, pose, sizeof(circleWords));
		circleWords[9] = position.x;
		circleWords[10] = position.y;
		circleWords[11] = position.z;
		NxMat34 circleMatrix;
		memcpy(&circleMatrix, circleWords, sizeof(circleWords));
		renderable->addCircle(10, circleMatrix, particleColor, radius, false);
		}
		}
	}
