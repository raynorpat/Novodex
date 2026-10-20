/*----------------------------------------------------------------------------*\
|
|							NovodeX Technology
|
|                             www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ContactGeneration.h"
#include "NxIntersectionRaySphere.h"
#include "NxIntersectionSegmentCapsule.h"
#include "NxIntersectionRayPlane.h"
#include "NxPlane.h"
#include "NxPhysicsBackend.h"
#include "NxInternalTriangleMesh.h"
#include "OPC_RayCollider.h"
#include "TriangleMesh.h"
#include <math.h>

// The original row executes x87 fsqrt. Its result must follow the live
// control word, including the simulation step's 0x0f7f setting.
static double nxSqrt(double value)
	{
#if NX_PHYSICS_USE_X87
	double result;
	__asm
		{
		fld value
		fsqrt
		fstp result
		}
	return result;
#else
	return sqrt(value);
#endif
	}

// phys_fn_001377 at 0x00027c70: what a SPHERE shape puts in vtable slot 5.
//
// Its only callee is NxRaySphereIntersect at 0x00036e80 (phys_fn_001710), a
// Task 2 export. Read alongside the plane's copy the two are not the same
// function with a different intersector:
//
//   * there is no facing test and no "ahead of the origin" test here, only the
//     distance limit, so a sphere behind the ray origin is whatever
//     NxRaySphereIntersect says it is;
//   * hit.distance is written *before* that limit is tested (`fst` at
//     0x00027cc9, `fcomp` at 0x00027ccc), so a raycast rejected for being too
//     far still leaves the distance in the caller's hit record;
//   * the comparison uses the wide value still in st(0) while the hit gets the
//     narrowed copy, and those are not the same number;
//   * the normal is computed and normalised rather than copied out of the
//     shape, and a zero-length one is left unnormalised with the
//     NX_RAYCAST_NORMAL bit still set (0x00027d94).
const NxCollisionShape* __fastcall NxShapeRaycastSphere(const NxCollisionShape* sphere,
	void* edxUnused, const NxRay* worldRay, NxReal maxDistance, NxU32 unread,
	NxU32 hintFlags, NxRaycastHit* hit)
	{
	(void) edxUnused;
	(void) unread;
	const NxVec3* centre = (const NxVec3*) sphere->translation;

	if(!NxRaySphereIntersect(worldRay->orig, worldRay->dir, *centre,
			sphere->geometry[0], &hit->worldImpact))
		return 0;

	// |origin - impact|, from the register copies of the three differences and
	// accumulated z, y, x -- 0x00027ca1..0x00027cc1.
	const double toImpactX = (double) worldRay->orig.x - hit->worldImpact.x;
	const double toImpactY = (double) worldRay->orig.y - hit->worldImpact.y;
	const double toImpactZ = (double) worldRay->orig.z - hit->worldImpact.z;
	const double distance = nxSqrt((toImpactZ * toImpactZ + toImpactY * toImpactY)
		+ toImpactX * toImpactX);

	hit->distance = (NxReal) distance;
	if(distance > (double) maxDistance)
		return 0;

	hit->shape = (NxShape*) sphere->collisionObject;
	hit->faceID = 0;
	hit->u = 0.0f;
	hit->v = 0.0f;
	hit->flags = NX_RAYCAST_SHAPE | NX_RAYCAST_IMPACT | NX_RAYCAST_DISTANCE;
	if(hintFlags & NX_RAYCAST_NORMAL)
		{
		// impact - centre, stored into the hit and read back: the length is
		// formed from the narrowed components at 0x00027d32..0x00027d38 and not
		// from the registers that produced them, and its terms run x, y, z
		// where the distance above runs z, y, x.
		hit->worldNormal.x = (NxReal) ((double) hit->worldImpact.x - centre->x);
		hit->worldNormal.y = (NxReal) ((double) hit->worldImpact.y - centre->y);
		hit->worldNormal.z = (NxReal) ((double) hit->worldImpact.z - centre->z);
		const double length = nxSqrt(((double) hit->worldNormal.x * hit->worldNormal.x
			+ (double) hit->worldNormal.y * hit->worldNormal.y)
			+ (double) hit->worldNormal.z * hit->worldNormal.z);
		// `fucompp` against the 0.0f at 0x101041f0 with `jnp`: equal skips the
		// normalisation, unordered takes it.
		if(length != 0.0)
			{
			const double inverse = 1.0 / length;
			hit->worldNormal.x = (NxReal) (inverse * hit->worldNormal.x);
			hit->worldNormal.y = (NxReal) (inverse * hit->worldNormal.y);
			hit->worldNormal.z = (NxReal) (inverse * hit->worldNormal.z);
			}
		hit->flags |= NX_RAYCAST_NORMAL;
		}
	return sphere;
	}

// phys_fn_001010 at 0x00022480, slot 5 of a CAPSULE shape's vtable
// (0x00106b20 + 0x14). Same ABI note as the two above.
//
// A third convention across three slot-5 rows, and the one that matters: the
// plane's copies the plane geometry into hit.worldNormal under a flag test, the
// sphere's computes and normalises one under the same test, and this one has no
// normal branch at all. `flags` is the literal 0x13 at 0x000225b0 -- SHAPE,
// IMPACT and DISTANCE -- with no `|4` anywhere, and nothing in the 321 bytes
// writes hit+0x10..+0x18 under any hint flags. That is not an oversight to fix
// here; phys_fn_001775 depends on it, in the worst way. See the comment on
// NxContactCapsuleCapsule.
const NxCollisionShape* __fastcall NxShapeRaycastCapsule(const NxCollisionShape* capsule,
	void* edxUnused, const NxRay* worldRay, NxReal maxDistance, NxU32 unread,
	NxU32 hintFlags, NxRaycastHit* hit)
	{
	(void) edxUnused;
	(void) unread;
	(void) hintFlags;

	const NxReal* m = capsule->rotation;
	const NxReal* translation = capsule->translation;
	const NxReal halfHeight = capsule->geometry[1];

	// The same column-1 axis, the same narrowing of the x half-axis and the same
	// narrowed -axisZ as the three capsule entries already closed.
	const NxReal axisX = (NxReal) ((double) m[1] * halfHeight);
	const double axisY = (double) m[4] * halfHeight;
	const double axisZ = (double) m[7] * halfHeight;
	const NxReal negatedAxisZ = (NxReal) (-axisZ);

	NxCapsule body;
	body.p0.x = (NxReal) (-(double) axisX + translation[0]);
	body.p0.y = (NxReal) (-axisY + translation[1]);
	body.p0.z = (NxReal) ((double) negatedAxisZ + translation[2]);
	body.p1.x = (NxReal) ((double) axisX + translation[0]);
	body.p1.y = (NxReal) (axisY + translation[1]);
	body.p1.z = (NxReal) (axisZ + translation[2]);
	// An integer move at 0x000224a4/0x000224d8, so the radius arrives exactly as
	// the shape holds it.
	body.radius = capsule->geometry[0];

	NxReal roots[2];
	const NxU32 count = NxRayCapsuleIntersect(worldRay->orig, worldRay->dir, body, roots);
	if(count == 0)
		return 0;

	// Three ways, not two: one root is taken directly and anything else compares
	// the pair and takes the smaller, with the unordered case taking the second.
	double distance;
	if(count == 1)
		distance = roots[0];
	else
		distance = roots[0] < roots[1] ? roots[0] : roots[1];

	// `test ah,0x41` + `jne`: less, equal or unordered all continue.
	if(distance > maxDistance)
		return 0;

	const NxReal scaledY = (NxReal) (distance * worldRay->dir.y);
	const NxReal scaledZ = (NxReal) (distance * worldRay->dir.z);
	hit->worldImpact.x = (NxReal) (distance * worldRay->dir.x + worldRay->orig.x);
	hit->worldImpact.y = (NxReal) ((double) scaledY + worldRay->orig.y);
	hit->worldImpact.z = (NxReal) ((double) scaledZ + worldRay->orig.z);
	hit->distance = (NxReal) distance;
	hit->shape = (NxShape*) capsule->collisionObject;
	hit->faceID = 0;
	hit->u = 0.0f;
	hit->v = 0.0f;
	hit->flags = 0x13;
	// hit->worldNormal is deliberately not written. See above.
	return capsule;
	}

// Listing transcription of phys_fn_001405 at 0x00029230 (MESH's slot-5 raycast): the scene's RayCollider (Scene+0x450)
// collects the closest stabbed face into a one-entry external buffer, and the result is expanded to a world-space hit
// record with the mesh's optional face remap and lazily-built smooth vertex normals. Compare script:
// docs/reconstruction/novodex-physics/evidence/phase3-mesh-raycast-listing-compare.py.
extern "C" void nxMRCallContainerCtor();
extern "C" void nxMRCallSetExternal();
extern "C" void nxMRCallEmpty();
extern "C" void nxMRCallRayCollide();
extern "C" void nxMRCallVertexNormals();
#pragma comment(linker, "/alternatename:_nxMRCallContainerCtor=??0SdkContainer@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxMRCallSetExternal=?setExternalBuffer@SdkContainer@@QAEXIPAI@Z")
#pragma comment(linker, "/alternatename:_nxMRCallEmpty=?empty@SdkContainer@@QAEXXZ")
#pragma comment(linker, "/alternatename:_nxMRCallRayCollide=?Collide@RayCollider@Opcode@@QAE_NABVRay@IceMaths@@ABVModel@2@PBVMatrix4x4@4@PAI@Z")
#pragma comment(linker, "/alternatename:_nxMRCallVertexNormals=?nxMeshComputeVertexNormals@@YAXXZ")
static const unsigned nxMRK101041ec_4 = 0x3f800000;
static const unsigned nxMRK101041f0_4 = 0x0;
// phys_fn_001405 (0x00029230, 978 B), 306 instructions
__declspec(naked) const NxCollisionShape* __fastcall NxShapeRaycastMesh(const NxCollisionShape* shape,
	void* edxUnused, const NxRay* worldRay, NxReal maxDistance, NxU32 unread,
	NxU32 hintFlags, NxRaycastHit* hit)
	{
	__asm
		{
		sub esp, 0x7c		// 0x00029230
		push ebx		// 0x00029233
		push ebp		// 0x00029234
		push esi		// 0x00029235
		mov esi, ecx		// 0x00029236
		push edi		// 0x00029238
		lea ecx, [esp + 0x2c]		// 0x00029239
		call nxMRCallContainerCtor		// 0x0002923d
		lea eax, [esp + 0x3c]		// 0x00029242
		push eax		// 0x00029246
		mov ebx, 1		// 0x00029247
		push ebx		// 0x0002924c
		lea ecx, [esp + 0x34]		// 0x0002924d
		call nxMRCallSetExternal		// 0x00029251
		mov ecx, dword ptr [esi + 4]		// 0x00029256
		mov ecx, dword ptr [ecx + 4]		// 0x00029259
		mov ebp, dword ptr [ecx + 0x454]		// 0x0002925c
		mov edx, dword ptr [esp + 0x94]		// 0x00029262
		mov dword ptr [ecx + 0x4d4], edx		// 0x00029269
		mov byte ptr [ecx + 0x4dc], bl		// 0x0002926f
		mov byte ptr [ecx + 0x4dd], bl		// 0x00029275
		add ecx, 0x450		// 0x0002927b
		mov edi, 0xfffffffc		// 0x00029281
		lea eax, [esp + 0x2c]		// 0x00029286
		mov dword ptr [ecx + 0x5c], eax		// 0x0002928a
		and ebp, edi		// 0x0002928d
		mov dword ptr [ecx + 4], ebp		// 0x0002928f
		mov edx, dword ptr [esi + 0xe0]		// 0x00029292
		mov eax, dword ptr [edx + 0x70]		// 0x00029298
		mov dword ptr [ecx + 0x88], eax		// 0x0002929b
		mov edx, dword ptr [esi + 0xc]		// 0x000292a1
		mov eax, dword ptr [esi + 0x10]		// 0x000292a4
		mov dword ptr [esp + 0x4c], edx		// 0x000292a7
		mov edx, dword ptr [esi + 0x14]		// 0x000292ab
		mov dword ptr [esp + 0x5c], eax		// 0x000292ae
		mov eax, dword ptr [esi + 0x18]		// 0x000292b2
		mov dword ptr [esp + 0x6c], edx		// 0x000292b5
		mov edx, dword ptr [esi + 0x1c]		// 0x000292b9
		mov dword ptr [esp + 0x50], eax		// 0x000292bc
		mov eax, dword ptr [esi + 0x20]		// 0x000292c0
		mov dword ptr [esp + 0x60], edx		// 0x000292c3
		mov edx, dword ptr [esi + 0x24]		// 0x000292c7
		mov dword ptr [esp + 0x70], eax		// 0x000292ca
		mov eax, dword ptr [esi + 0x28]		// 0x000292ce
		mov dword ptr [esp + 0x54], edx		// 0x000292d1
		mov edx, dword ptr [esi + 0x2c]		// 0x000292d5
		mov dword ptr [esp + 0x64], eax		// 0x000292d8
		mov eax, dword ptr [esi + 0x30]		// 0x000292dc
		mov dword ptr [esp + 0x74], edx		// 0x000292df
		mov edx, dword ptr [esi + 0x34]		// 0x000292e3
		mov dword ptr [esp + 0x7c], eax		// 0x000292e6
		mov eax, dword ptr [esi + 0x38]		// 0x000292ea
		mov dword ptr [esp + 0x80], edx		// 0x000292ed
		mov edx, dword ptr [esi + 0xe0]		// 0x000292f4
		mov dword ptr [esp + 0x84], eax		// 0x000292fa
		mov dword ptr [esp + 0x78], 0		// 0x00029301
		mov dword ptr [esp + 0x68], 0		// 0x00029309
		mov dword ptr [esp + 0x58], 0		// 0x00029311
		mov dword ptr [esp + 0x88], 0x3f800000		// 0x00029319
		mov eax, dword ptr [edx + 0x28]		// 0x00029324
		push 0		// 0x00029327
		lea edx, [esp + 0x50]		// 0x00029329
		push edx		// 0x0002932d
		push eax		// 0x0002932e
		mov eax, dword ptr [esp + 0x9c]		// 0x0002932f
		push eax		// 0x00029336
		call nxMRCallRayCollide		// 0x00029337
		test al, al		// 0x0002933c
		je L29367		// 0x0002933e
		test dword ptr [esp + 0x30], edi		// 0x00029340
		je L29367		// 0x00029344
		mov ecx, dword ptr [esp + 0x34]		// 0x00029346
		fld dword ptr [ecx + 4]		// 0x0002934a
		mov edi, dword ptr [esp + 0xa0]		// 0x0002934d
		fld st(0)		// 0x00029354
		fstp dword ptr [edi + 0x20]		// 0x00029356
		fcomp dword ptr [esp + 0x94]		// 0x00029359
		fnstsw ax		// 0x00029360
		test ah, 0x41		// 0x00029362
		jne L2937c		// 0x00029365
L29367:
		lea ecx, [esp + 0x2c]		// 0x00029367
		call nxMRCallEmpty		// 0x0002936b
		pop edi		// 0x00029370
		pop esi		// 0x00029371
		pop ebp		// 0x00029372
		xor eax, eax		// 0x00029373
		pop ebx		// 0x00029375
		add esp, 0x7c		// 0x00029376
		ret 0x14		// 0x00029379
L2937c:
		mov ecx, dword ptr [ecx]		// 0x0002937c
		fld dword ptr [nxMRK101041ec_4]		// 0x0002937e
		mov eax, dword ptr [esi + 0xe0]		// 0x00029384
		mov edx, dword ptr [eax + 0x14]		// 0x0002938a
		lea ecx, [ecx + ecx*2]		// 0x0002938d
		mov ebx, dword ptr [edx + ecx*4 + 8]		// 0x00029390
		lea ebp, [edx + ecx*4]		// 0x00029394
		mov ecx, dword ptr [ebp]		// 0x00029397
		mov edx, dword ptr [ebp + 4]		// 0x0002939a
		lea ebx, [ebx + ebx*2]		// 0x0002939d
		mov dword ptr [esp + 0x1c], eax		// 0x000293a0
		mov eax, dword ptr [eax + 0x10]		// 0x000293a4
		lea ecx, [ecx + ecx*2]		// 0x000293a7
		lea ecx, [eax + ecx*4]		// 0x000293aa
		lea edx, [edx + edx*2]		// 0x000293ad
		lea edx, [eax + edx*4]		// 0x000293b0
		lea eax, [eax + ebx*4]		// 0x000293b3
		mov ebx, dword ptr [esp + 0x34]		// 0x000293b6
		mov ebx, dword ptr [ebx + 8]		// 0x000293ba
		mov dword ptr [esp + 0x14], ebx		// 0x000293bd
		fsub dword ptr [esp + 0x14]		// 0x000293c1
		mov ebx, dword ptr [esp + 0x34]		// 0x000293c5
		mov ebx, dword ptr [ebx + 0xc]		// 0x000293c9
		mov dword ptr [esp + 0x10], ebx		// 0x000293cc
		fsub dword ptr [esp + 0x10]		// 0x000293d0
		fst dword ptr [esp + 0x18]		// 0x000293d4
		fld st(0)		// 0x000293d8
		fmul dword ptr [ecx]		// 0x000293da
		fld dword ptr [esp + 0x14]		// 0x000293dc
		fmul dword ptr [edx]		// 0x000293e0
		faddp st(1), st(0)		// 0x000293e2
		fld dword ptr [esp + 0x10]		// 0x000293e4
		fmul dword ptr [eax]		// 0x000293e8
		faddp st(1), st(0)		// 0x000293ea
		fld st(1)		// 0x000293ec
		fmul dword ptr [ecx + 4]		// 0x000293ee
		fld dword ptr [esp + 0x10]		// 0x000293f1
		fmul dword ptr [eax + 4]		// 0x000293f5
		faddp st(1), st(0)		// 0x000293f8
		fld dword ptr [esp + 0x14]		// 0x000293fa
		fmul dword ptr [edx + 4]		// 0x000293fe
		faddp st(1), st(0)		// 0x00029401
		fxch st(2)		// 0x00029403
		fmul dword ptr [ecx + 8]		// 0x00029405
		fld dword ptr [esp + 0x10]		// 0x00029408
		fmul dword ptr [eax + 8]		// 0x0002940c
		faddp st(1), st(0)		// 0x0002940f
		fld dword ptr [esp + 0x14]		// 0x00029411
		fmul dword ptr [edx + 8]		// 0x00029415
		faddp st(1), st(0)		// 0x00029418
		fld st(0)		// 0x0002941a
		fmul dword ptr [esi + 0x14]		// 0x0002941c
		fld st(3)		// 0x0002941f
		fmul dword ptr [esi + 0x10]		// 0x00029421
		faddp st(1), st(0)		// 0x00029424
		fld st(2)		// 0x00029426
		fmul dword ptr [esi + 0xc]		// 0x00029428
		faddp st(1), st(0)		// 0x0002942b
		fstp dword ptr [esp + 0x20]		// 0x0002942d
		fld st(0)		// 0x00029431
		fmul dword ptr [esi + 0x20]		// 0x00029433
		fld st(3)		// 0x00029436
		fmul dword ptr [esi + 0x1c]		// 0x00029438
		faddp st(1), st(0)		// 0x0002943b
		fld st(2)		// 0x0002943d
		fmul dword ptr [esi + 0x18]		// 0x0002943f
		faddp st(1), st(0)		// 0x00029442
		fstp dword ptr [esp + 0x24]		// 0x00029444
		fmul dword ptr [esi + 0x2c]		// 0x00029448
		fxch st(2)		// 0x0002944b
		fmul dword ptr [esi + 0x28]		// 0x0002944d
		faddp st(2), st(0)		// 0x00029450
		fmul dword ptr [esi + 0x24]		// 0x00029452
		faddp st(1), st(0)		// 0x00029455
		fld dword ptr [esp + 0x20]		// 0x00029457
		fadd dword ptr [esi + 0x30]		// 0x0002945b
		fld dword ptr [esp + 0x24]		// 0x0002945e
		fadd dword ptr [esi + 0x34]		// 0x00029462
		fxch st(2)		// 0x00029465
		fadd dword ptr [esi + 0x38]		// 0x00029467
		fstp dword ptr [esp + 0x28]		// 0x0002946a
		mov eax, dword ptr [esp + 0x28]		// 0x0002946e
		fstp dword ptr [edi + 4]		// 0x00029472
		mov dword ptr [edi + 0xc], eax		// 0x00029475
		fstp dword ptr [edi + 8]		// 0x00029478
		mov ecx, dword ptr [esi + 0xe0]		// 0x0002947b
		mov eax, dword ptr [ecx + 0x1c]		// 0x00029481
		test eax, eax		// 0x00029484
		mov edx, dword ptr [esi + 0x9c]		// 0x00029486
		mov dword ptr [edi], edx		// 0x0002948c
		je L2949b		// 0x0002948e
		mov ecx, dword ptr [esp + 0x34]		// 0x00029490
		mov edx, dword ptr [ecx]		// 0x00029494
		mov eax, dword ptr [eax + edx*4]		// 0x00029496
		jmp L294a1		// 0x00029499
L2949b:
		mov eax, dword ptr [esp + 0x34]		// 0x0002949b
		mov eax, dword ptr [eax]		// 0x0002949f
L294a1:
		mov ecx, dword ptr [esp + 0x14]		// 0x000294a1
		mov edx, dword ptr [esp + 0x10]		// 0x000294a5
		mov dword ptr [edi + 0x1c], eax		// 0x000294a9
		test byte ptr [esp + 0x9c], 4		// 0x000294ac
		mov dword ptr [edi + 0x24], ecx		// 0x000294b4
		mov dword ptr [edi + 0x28], edx		// 0x000294b7
		mov dword ptr [edi + 0x2c], 0x3b		// 0x000294ba
		je L295ed		// 0x000294c1
		mov ebx, dword ptr [esp + 0x1c]		// 0x000294c7
		mov eax, dword ptr [ebx + 0x20]		// 0x000294cb
		add ebx, 8		// 0x000294ce
		test eax, eax		// 0x000294d1
		jne L294dc		// 0x000294d3
		mov ecx, ebx		// 0x000294d5
		call nxMRCallVertexNormals		// 0x000294d7
L294dc:
		mov eax, dword ptr [ebx + 0x18]		// 0x000294dc
		fld dword ptr [esp + 0x18]		// 0x000294df
		fld st(0)		// 0x000294e3
		mov ecx, dword ptr [ebp]		// 0x000294e5
		mov edx, dword ptr [ebp + 4]		// 0x000294e8
		mov ebp, dword ptr [ebp + 8]		// 0x000294eb
		lea ecx, [ecx + ecx*2]		// 0x000294ee
		fmul dword ptr [eax + ecx*4]		// 0x000294f1
		lea ecx, [eax + ecx*4]		// 0x000294f4
		fld dword ptr [esp + 0x10]		// 0x000294f7
		lea edx, [edx + edx*2]		// 0x000294fb
		lea edx, [eax + edx*4]		// 0x000294fe
		lea ebx, [ebp + ebp*2]		// 0x00029501
		fmul dword ptr [eax + ebx*4]		// 0x00029505
		lea eax, [eax + ebx*4]		// 0x00029508
		faddp st(1), st(0)		// 0x0002950b
		fld dword ptr [esp + 0x14]		// 0x0002950d
		fmul dword ptr [edx]		// 0x00029511
		faddp st(1), st(0)		// 0x00029513
		fstp dword ptr [esp + 0x20]		// 0x00029515
		fld st(0)		// 0x00029519
		fmul dword ptr [ecx + 4]		// 0x0002951b
		fld dword ptr [esp + 0x10]		// 0x0002951e
		fmul dword ptr [eax + 4]		// 0x00029522
		faddp st(1), st(0)		// 0x00029525
		fld dword ptr [esp + 0x14]		// 0x00029527
		fmul dword ptr [edx + 4]		// 0x0002952b
		faddp st(1), st(0)		// 0x0002952e
		fstp dword ptr [esp + 0x24]		// 0x00029530
		fmul dword ptr [ecx + 8]		// 0x00029534
		fld dword ptr [esp + 0x10]		// 0x00029537
		fmul dword ptr [eax + 8]		// 0x0002953b
		faddp st(1), st(0)		// 0x0002953e
		fld dword ptr [esp + 0x14]		// 0x00029540
		fmul dword ptr [edx + 8]		// 0x00029544
		faddp st(1), st(0)		// 0x00029547
		fld st(0)		// 0x00029549
		fmul dword ptr [esi + 0x20]		// 0x0002954b
		fld dword ptr [esp + 0x24]		// 0x0002954e
		fmul dword ptr [esi + 0x1c]		// 0x00029552
		faddp st(1), st(0)		// 0x00029555
		fld dword ptr [esp + 0x20]		// 0x00029557
		fmul dword ptr [esi + 0x18]		// 0x0002955b
		faddp st(1), st(0)		// 0x0002955e
		fld st(1)		// 0x00029560
		fmul dword ptr [esi + 0x2c]		// 0x00029562
		fld dword ptr [esp + 0x24]		// 0x00029565
		fmul dword ptr [esi + 0x28]		// 0x00029569
		faddp st(1), st(0)		// 0x0002956c
		fld dword ptr [esp + 0x20]		// 0x0002956e
		fmul dword ptr [esi + 0x24]		// 0x00029572
		faddp st(1), st(0)		// 0x00029575
		fxch st(2)		// 0x00029577
		fmul dword ptr [esi + 0x14]		// 0x00029579
		fld dword ptr [esp + 0x24]		// 0x0002957c
		fmul dword ptr [esi + 0x10]		// 0x00029580
		faddp st(1), st(0)		// 0x00029583
		fld dword ptr [esp + 0x20]		// 0x00029585
		fmul dword ptr [esi + 0xc]		// 0x00029589
		faddp st(1), st(0)		// 0x0002958c
		fstp dword ptr [edi + 0x10]		// 0x0002958e
		fstp dword ptr [edi + 0x14]		// 0x00029591
		fstp dword ptr [edi + 0x18]		// 0x00029594
		fld dword ptr [edi + 0x18]		// 0x00029597
		fld dword ptr [edi + 0x14]		// 0x0002959a
		fld dword ptr [edi + 0x10]		// 0x0002959d
		fld st(0)		// 0x000295a0
		fmul st(0), st(1)		// 0x000295a2
		fld st(2)		// 0x000295a4
		fmul st(0), st(3)		// 0x000295a6
		faddp st(1), st(0)		// 0x000295a8
		fld st(3)		// 0x000295aa
		fmul st(0), st(4)		// 0x000295ac
		faddp st(1), st(0)		// 0x000295ae
		fsqrt		// 0x000295b0
		fstp st(3)		// 0x000295b2
		fstp st(0)		// 0x000295b4
		fstp st(0)		// 0x000295b6
		fld dword ptr [nxMRK101041f0_4]		// 0x000295b8
		fld st(1)		// 0x000295be
		fucompp		// 0x000295c0
		fnstsw ax		// 0x000295c2
		test ah, 0x44		// 0x000295c4
		jnp L295e7		// 0x000295c7
		fdivr dword ptr [nxMRK101041ec_4]		// 0x000295c9
		fld st(0)		// 0x000295cf
		fmul dword ptr [edi + 0x10]		// 0x000295d1
		fstp dword ptr [edi + 0x10]		// 0x000295d4
		fld st(0)		// 0x000295d7
		fmul dword ptr [edi + 0x14]		// 0x000295d9
		fstp dword ptr [edi + 0x14]		// 0x000295dc
		fmul dword ptr [edi + 0x18]		// 0x000295df
		fstp dword ptr [edi + 0x18]		// 0x000295e2
		jmp L295e9		// 0x000295e5
L295e7:
		fstp st(0)		// 0x000295e7
L295e9:
		or dword ptr [edi + 0x2c], 4		// 0x000295e9
L295ed:
		lea ecx, [esp + 0x2c]		// 0x000295ed
		call nxMRCallEmpty		// 0x000295f1
		pop edi		// 0x000295f6
		mov eax, esi		// 0x000295f7
		pop esi		// 0x000295f9
		pop ebp		// 0x000295fa
		pop ebx		// 0x000295fb
		add esp, 0x7c		// 0x000295fc
		ret 0x14		// 0x000295ff
		}
	}


// phys_fn_001261 at 0x00025350: what a PLANE shape puts in vtable slot 5.
//
// It is a segment/plane raycast whose only callee is NxRayPlaneIntersect at
// 0x00036bb0, a Task 2 export. Three gates in order -- the ray must run into
// the plane, the hit must be ahead of the origin, and it must be within the
// distance limit -- and every one of them lets an unordered comparison through,
// so a NaN anywhere in the ray or the plane reaches the hit record rather than
// being rejected.
//
// `hit.shape` is written from `shape->[0x9c]` (0x000253d8), the collision
// object, not from the shape itself. Whether that object is what the public API
// hands out as an NxShape is a Phase 5 question; this row only records which
// pointer the oracle stores.
const NxCollisionShape* __fastcall NxShapeRaycastPlane(const NxCollisionShape* plane,
	void* edxUnused, const NxRay* worldRay, NxReal maxDistance, NxU32 unread,
	NxU32 hintFlags, NxRaycastHit* hit)
	{
	(void) edxUnused;
	(void) unread;
	const NxReal* n = plane->geometry;

	// 0x00025357..0x00025377, accumulated z, y, x and left in st(0).
	const double facing = ((double) n[2] * worldRay->dir.z + (double) n[1] * worldRay->dir.y)
		+ (double) n[0] * worldRay->dir.x;
	// `test ah,1; jne` at 0x00025381 reads C0 alone, which is set for "less"
	// and for "unordered" alike, so a NaN dot product continues.
	if(facing >= 0.0)
		return 0;

	// The oracle passes the address of its own first argument slot as `dist`
	// (0x00025396) -- a store into the caller's frame, dead by then.
	NxReal distance;
	if(!NxRayPlaneIntersect(*worldRay, *(const NxPlane*) n, distance, hit->worldImpact))
		return 0;

	// 0x000253bb and 0x000253cc, both `test ah,0x41` over C3 and C0 but with
	// opposite branches: strictly ahead of the origin, and not past the limit.
	// Both let the unordered case through.
	if(distance <= 0.0f)
		return 0;
	if(distance > maxDistance)
		return 0;

	// Integer moves, not float stores: an fld/fstp pair would quiet a
	// signalling NaN, and these are `mov` at 0x000253d1 and 0x000253ff.
	memcpy(&hit->distance, &distance, 4);
	hit->shape = (NxShape*) plane->collisionObject;
	hit->faceID = 0;
	hit->u = 0.0f;
	hit->v = 0.0f;
	hit->flags = NX_RAYCAST_SHAPE | NX_RAYCAST_IMPACT | NX_RAYCAST_DISTANCE;
	if(hintFlags & NX_RAYCAST_NORMAL)
		{
		memcpy(&hit->worldNormal.x, &n[0], 4);
		memcpy(&hit->worldNormal.y, &n[1], 4);
		memcpy(&hit->worldNormal.z, &n[2], 4);
		hit->flags |= NX_RAYCAST_NORMAL;
		}
	return plane;
	}
