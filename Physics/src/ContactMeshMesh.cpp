// ContactMeshMesh.cpp, sub-unit N of units/convex-mesh-gap-contract.md.

#include "NxPhysicsSDK.h"
#include "ContactGeneration.h"

extern "C" void __cdecl nxMeshHeightfieldContact(const NxCollisionShape*,
	const NxCollisionShape*, NxContactSink*, void*);
#include "NxMeshContactHelpers.h"
#include "NxUtilities.h"
#include "Opcode.h"

#include <float.h>
#include <math.h>
#include <string.h>

extern "C" void nxMeshMeshCallAabbTreeCollide();
#pragma comment(linker, "/alternatename:_nxMeshMeshCallAabbTreeCollide=?Collide@AABBTreeCollider@Opcode@@QAE_NAAUBVTCache@2@PBVMatrix4x4@IceMaths@@1@Z")

// phys_fn_001870 (0x00046550, 394 B)
// Matrix-B [MESH][MESH] entry. It prepares two world OBBs in the scene's
// OBBCache and calls the vendored OBBCollider::Collide with each mesh's model.
extern "C" __declspec(naked) bool __cdecl nxOverlapMeshMesh(const void*, const void*, void*)
{
	__asm
	{
		sub esp, 80h
		push esi
		mov esi, dword ptr [esp + 90h]
		push edi
		mov edi, dword ptr [esi + 330h]
		or edi, 1
		mov dword ptr [esi + 330h], edi
		mov edx, edi
		and edx, 0FFFFFFFDh
		mov dword ptr [esi + 330h], edx
		mov eax, edx
		lea ecx, [esi + 32Ch]
		and eax, 0FFFFFFEFh
		mov dword ptr [ecx + 4], eax
		mov eax, dword ptr [esp + 8Ch]
		mov edx, dword ptr [eax + 0Ch]
		mov dword ptr [esp + 48h], edx
		mov edx, dword ptr [eax + 10h]
		mov dword ptr [esp + 58h], edx
		mov edx, dword ptr [eax + 14h]
		mov dword ptr [esp + 68h], edx
		mov edx, dword ptr [eax + 18h]
		mov dword ptr [esp + 4Ch], edx
		mov edx, dword ptr [eax + 1Ch]
		mov dword ptr [esp + 5Ch], edx
		mov edx, dword ptr [eax + 20h]
		mov dword ptr [esp + 6Ch], edx
		mov edx, dword ptr [eax + 24h]
		mov dword ptr [esp + 50h], edx
		mov edx, dword ptr [eax + 28h]
		mov dword ptr [esp + 60h], edx
		mov edx, dword ptr [eax + 2Ch]
		mov dword ptr [esp + 70h], edx
		mov edx, dword ptr [eax + 30h]
		mov dword ptr [esp + 78h], edx
		mov edx, dword ptr [eax + 34h]
		mov dword ptr [esp + 7Ch], edx
		mov edx, dword ptr [eax + 38h]
		mov dword ptr [esp + 80h], edx
		mov edx, dword ptr [esp + 90h]
		mov edi, dword ptr [edx + 0Ch]
		mov dword ptr [esp + 8], edi
		mov edi, dword ptr [edx + 10h]
		mov dword ptr [esp + 18h], edi
		mov edi, dword ptr [edx + 14h]
		mov dword ptr [esp + 28h], edi
		mov edi, dword ptr [edx + 18h]
		mov eax, dword ptr [eax + 0E0h]
		mov dword ptr [esp + 0Ch], edi
		mov edi, dword ptr [edx + 1Ch]
		mov dword ptr [esp + 1Ch], edi
		mov edi, dword ptr [edx + 20h]
		mov dword ptr [esp + 2Ch], edi
		mov edi, dword ptr [edx + 24h]
		mov dword ptr [esp + 10h], edi
		mov edi, dword ptr [edx + 28h]
		mov dword ptr [esp + 20h], edi
		mov edi, dword ptr [edx + 2Ch]
		mov dword ptr [esp + 30h], edi
		mov edi, dword ptr [edx + 30h]
		mov dword ptr [esp + 38h], edi
		mov edi, dword ptr [edx + 34h]
		mov dword ptr [esp + 3Ch], edi
		mov edi, dword ptr [edx + 38h]
		mov dword ptr [esp + 74h], 0
		mov dword ptr [esp + 64h], 0
		mov dword ptr [esp + 54h], 0
		mov dword ptr [esp + 84h], 3F800000h
		mov dword ptr [esp + 40h], edi
		mov dword ptr [esp + 34h], 0
		mov dword ptr [esp + 24h], 0
		mov dword ptr [esp + 14h], 0
		mov dword ptr [esp + 44h], 3F800000h
		mov eax, dword ptr [eax + 28h]
		mov dword ptr [esi + 448h], eax
		mov edx, dword ptr [edx + 0E0h]
		mov eax, dword ptr [edx + 28h]
		lea edx, [esp + 8]
		mov dword ptr [esi + 44Ch], eax
		push edx
		lea eax, [esp + 4Ch]
		push eax
		lea edx, [esi + 440h]
		push edx
		call nxMeshMeshCallAabbTreeCollide
		 test al, al
		je nxMeshMeshOverlap_false
		test byte ptr [esi + 330h], 4
		je nxMeshMeshOverlap_false
		pop edi
		mov al, 1
		pop esi
		add esp, 80h
		ret
	nxMeshMeshOverlap_false:
		pop edi
		xor al, al
		pop esi
		add esp, 80h
		ret
	}
}


// Transform pointers installed by the caller before phys_fn_001874.
extern "C" unsigned* nxTask2lSphereMatrixA = 0;
extern "C" unsigned* nxTask2lSphereMatrixB = 0;
static const float nxTask2lSphereOne = 1.0f;
static const float nxTask2lSphereEpsilon = 0.00001f;
extern "C" void nxTask2lCallSphereCtor(void*, unsigned);
extern "C" void nxTask2lCallSphereSetRadius(float);
extern "C" void nxTask2lCallSphereDtor();
#pragma comment(linker, "/alternatename:_nxTask2lCallSphereCtor=??0SphereShape@@QAE@PAXI@Z")
#pragma comment(linker, "/alternatename:_nxTask2lCallSphereSetRadius=?nxSphereSetRadius@SphereShape@@QAEXM@Z")
#pragma comment(linker, "/alternatename:_nxTask2lCallSphereDtor=?nxSphereCallbackDtor@SphereShape@@QAEXXZ")
extern "C" void __stdcall nxMeshContactAccumulate(unsigned, unsigned, unsigned, const unsigned*, const float*);

// phys_fn_001874 (0x00046780, 801 B)
// Mesh/mesh sphere callback, transcribed from the pinned x86 listing.
extern "C" __declspec(naked) bool __cdecl nxMeshMeshSphereCallback(const float*, const float*)
{
    __asm {
        sub esp, 0x1f4
        push esi
        push 0xffffff
        push 0
        lea ecx, [esp + 0x38]
        call nxTask2lCallSphereCtor
        push -1
        push 0
        lea ecx, [esp + 0x11c]
        call nxTask2lCallSphereCtor
        mov esi, dword ptr [esp + 0x200]
        mov ecx, dword ptr [esp + 0x1fc]
        mov eax, dword ptr [esi]
        fld dword ptr [ecx]
        fld dword ptr [ecx + 4]
        mov dword ptr [esp + 8], eax
        mov eax, dword ptr [esi + 8]
        fld dword ptr [ecx + 8]
        fld st(0)
        mov dword ptr [esp + 0x10], eax
        mov eax, dword ptr [nxTask2lSphereMatrixA]
        fmul dword ptr [eax + 0x14]
        mov edx, dword ptr [esi + 4]
        fld st(2)
        mov dword ptr [esp + 0xc], edx
        fmul dword ptr [eax + 0x10]
        faddp st(1), st(0)
        fld st(3)
        fmul dword ptr [eax + 0xc]
        faddp st(1), st(0)
        fstp dword ptr [esp + 0x14]
        fld st(0)
        fmul dword ptr [eax + 0x20]
        fld st(2)
        fmul dword ptr [eax + 0x1c]
        faddp st(1), st(0)
        fld st(3)
        fmul dword ptr [eax + 0x18]
        faddp st(1), st(0)
        fstp dword ptr [esp + 0x18]
        fmul dword ptr [eax + 0x2c]
        fxch st(1)
        fmul dword ptr [eax + 0x28]
        faddp st(1), st(0)
        fxch st(1)
        fmul dword ptr [eax + 0x24]
        faddp st(1), st(0)
        fld dword ptr [esp + 0x14]
        fadd dword ptr [eax + 0x30]
        fld dword ptr [esp + 0x18]
        fadd dword ptr [eax + 0x34]
        fxch st(2)
        fadd dword ptr [eax + 0x38]
        mov eax, dword ptr [nxTask2lSphereMatrixB]
        fstp dword ptr [esp + 0x1c]
        mov edx, dword ptr [esp + 0x1c]
        mov dword ptr [esp + 0x68], edx
        fstp dword ptr [esp + 0x60]
        fstp dword ptr [esp + 0x64]
        fld dword ptr [esp + 0x10]
        fmul dword ptr [eax + 0x14]
        fld dword ptr [esp + 0xc]
        fmul dword ptr [eax + 0x10]
        faddp st(1), st(0)
        fld dword ptr [esp + 8]
        fmul dword ptr [eax + 0xc]
        faddp st(1), st(0)
        fld dword ptr [esp + 0x10]
        fmul dword ptr [eax + 0x20]
        fld dword ptr [esp + 0xc]
        fmul dword ptr [eax + 0x1c]
        faddp st(1), st(0)
        fld dword ptr [esp + 8]
        fmul dword ptr [eax + 0x18]
        faddp st(1), st(0)
        fstp dword ptr [esp + 0x18]
        fld dword ptr [esp + 0x10]
        fmul dword ptr [eax + 0x2c]
        fld dword ptr [esp + 0xc]
        fmul dword ptr [eax + 0x28]
        faddp st(1), st(0)
        fld dword ptr [esp + 8]
        fmul dword ptr [eax + 0x24]
        mov edx, dword ptr [ecx + 0xc]
        mov dword ptr [esp + 4], edx
        faddp st(1), st(0)
        fstp dword ptr [esp + 0x1c]
        fadd dword ptr [eax + 0x30]
        fld dword ptr [esp + 0x18]
        fadd dword ptr [eax + 0x34]
        fld dword ptr [esp + 0x1c]
        fadd dword ptr [eax + 0x38]
        fstp dword ptr [esp + 0x1c]
        mov eax, dword ptr [esp + 0x1c]
        fxch st(1)
        mov dword ptr [esp + 0x14c], eax
        fstp dword ptr [esp + 0x144]
        fstp dword ptr [esp + 0x148]
        fld dword ptr [ecx + 0x10]
        fcomp dword ptr [esp + 4]
        fnstsw ax
        test ah, 0x41
        jne L_100468e0
        mov eax, dword ptr [ecx + 0x10]
        mov dword ptr [esp + 4], eax
L_100468e0:
        fld dword ptr [ecx + 0x14]
        fcomp dword ptr [esp + 4]
        fnstsw ax
        test ah, 0x41
        jne L_100468f5
        mov ecx, dword ptr [ecx + 0x14]
        mov dword ptr [esp + 4], ecx
L_100468f5:
        mov edx, dword ptr [esp + 4]
        push edx
        lea ecx, [esp + 0x34]
        call nxTask2lCallSphereSetRadius
        fld dword ptr [esi + 0x10]
        mov eax, dword ptr [esi + 0xc]
        mov dword ptr [esp + 4], eax
        fcomp dword ptr [esp + 4]
        fnstsw ax
        test ah, 0x41
        jne L_1004691f
        mov ecx, dword ptr [esi + 0x10]
        mov dword ptr [esp + 4], ecx
L_1004691f:
        fld dword ptr [esi + 0x14]
        fcomp dword ptr [esp + 4]
        fnstsw ax
        test ah, 0x41
        jne L_10046934
        mov edx, dword ptr [esi + 0x14]
        mov dword ptr [esp + 4], edx
L_10046934:
        mov eax, dword ptr [esp + 4]
        push eax
        lea ecx, [esp + 0x118]
        call nxTask2lCallSphereSetRadius
        fld dword ptr [esp + 0x144]
        fsub dword ptr [esp + 0x60]
        pop esi
        fstp dword ptr [esp + 4]
        fld dword ptr [esp + 0x144]
        fsub dword ptr [esp + 0x60]
        fstp dword ptr [esp + 8]
        fld dword ptr [esp + 0x148]
        fsub dword ptr [esp + 0x64]
        fst dword ptr [esp + 0xc]
        fmul dword ptr [esp + 0xc]
        fld dword ptr [esp + 8]
        fmul dword ptr [esp + 8]
        faddp st(1), st(0)
        fld dword ptr [esp + 4]
        fmul dword ptr [esp + 4]
        faddp st(1), st(0)
        fstp dword ptr [esp]
        fld dword ptr [esp + 0x10c]
        fadd dword ptr [esp + 0x1f0]
        fst dword ptr [esp + 0x1c]
        fmul dword ptr [esp + 0x1c]
        fcomp dword ptr [esp]
        fnstsw ax
        test ah, 0x41
        jne L_10046a83
        fld dword ptr [esp]
        fcomp dword ptr [nxTask2lSphereEpsilon]
        fnstsw ax
        test ah, 0x41
        jp L_100469e0
        lea ecx, [esp + 0x110]
        call nxTask2lCallSphereDtor
        lea ecx, [esp + 0x2c]
        call nxTask2lCallSphereDtor
        xor al, al
        add esp, 0x1f4
        ret
L_100469e0:
        fld dword ptr [esp]
        mov eax, dword ptr [esp + 0xc8]
        fsqrt
        lea ecx, [esp + 4]
        push ecx
        lea edx, [esp + 0x14]
        push edx
        push ecx
        mov ecx, dword ptr [esp + 0x1b8]
        fld dword ptr [nxTask2lSphereOne]
        fdiv st(0), st(1)
        fld dword ptr [esp + 0x10]
        fmul st(0), st(1)
        fstp dword ptr [esp + 0x10]
        fld dword ptr [esp + 0x14]
        fmul st(0), st(1)
        fstp dword ptr [esp + 0x14]
        fld dword ptr [esp + 0x18]
        fmul st(0), st(1)
        fstp dword ptr [esp + 0x18]
        fstp st(0)
        fld dword ptr [esp + 0x118]
        fmul dword ptr [esp + 0x10]
        fld dword ptr [esp + 0x118]
        fmul dword ptr [esp + 0x14]
        fld dword ptr [esp + 0x118]
        fmul dword ptr [esp + 0x18]
        fstp dword ptr [esp + 0x34]
        fld dword ptr [esp + 0x68]
        fadd st(0), st(2)
        fstp dword ptr [esp + 0x1c]
        fld dword ptr [esp + 0x6c]
        fadd st(0), st(1)
        fstp dword ptr [esp + 0x20]
        fstp st(0)
        fstp st(0)
        fld dword ptr [esp + 0x70]
        fadd dword ptr [esp + 0x34]
        fstp dword ptr [esp + 0x24]
        fsub dword ptr [esp + 0x28]
        fstp dword ptr [esp]
        push eax
        push ecx
        mov ecx, dword ptr [nxTask2lSphereMatrixA]
        call nxMeshContactAccumulate
L_10046a83:
        lea ecx, [esp + 0x110]
        call nxTask2lCallSphereDtor
        lea ecx, [esp + 0x2c]
        call nxTask2lCallSphereDtor
        mov al, 1
        add esp, 0x1f4
        ret
    }
}

extern "C" unsigned nxMeshContactCount;
extern "C" float nxMeshContactSums[3];
extern "C" float nxMeshContactVertices[96];
extern "C" float nxMeshContactNormals[96];
extern "C" void __stdcall nxMeshContactAccumulate(unsigned, unsigned, unsigned, const unsigned*, const float*);

static void nxMeshMeshSetWorld(const NxCollisionShape* shape, IceMaths::Matrix4x4& world)
	{
	world.Set(
		shape->rotation[0], shape->rotation[3], shape->rotation[6], 0.0f,
		shape->rotation[1], shape->rotation[4], shape->rotation[7], 0.0f,
		shape->rotation[2], shape->rotation[5], shape->rotation[8], 0.0f,
		shape->translation[0], shape->translation[1], shape->translation[2], 1.0f);
	}

static void nxMeshMeshTriangleBounds(const Opcode::MeshInterface* mesh, unsigned triangle, float bounds[6])
	{
	Opcode::VertexPointers vertices;
	mesh->GetTriangle(vertices, triangle);
	for(unsigned axis = 0; axis < 3; ++axis)
		{
		bounds[axis] = (*vertices.Vertex[0])[axis];
		bounds[axis + 3] = bounds[axis];
		for(unsigned vertex = 1; vertex < 3; ++vertex)
			{
			const float value = (*vertices.Vertex[vertex])[axis];
			if(value < bounds[axis]) bounds[axis] = value;
			if(value > bounds[axis + 3]) bounds[axis + 3] = value;
			}
		}
	for(unsigned axis = 0; axis < 3; ++axis)
		{
			const float lo = bounds[axis], hi = bounds[axis + 3];
			bounds[axis] = (lo + hi) * 0.5f;
			bounds[axis + 3] = (hi - lo) * 0.5f;
		}
	}

static bool nxMeshMeshObbOverlap(const NxCollisionShape* shape0, const float bounds0[6],
	const NxCollisionShape* shape1, const float bounds1[6])
	{
	float center[2][3], extent[2][3], axis[2][3][3];
	const NxCollisionShape* shapes[2] = { shape0, shape1 };
	const float* bounds[2] = { bounds0, bounds1 };
	for(unsigned side = 0; side < 2; ++side)
		{
		for(unsigned i = 0; i < 3; ++i)
			{
			center[side][i] = shapes[side]->translation[i];
			for(unsigned j = 0; j < 3; ++j)
				center[side][i] += shapes[side]->rotation[3 * i + j] * bounds[side][j];
			}
		for(unsigned i = 0; i < 3; ++i)
			{
			extent[side][i] = bounds[side][i + 3];
			for(unsigned j = 0; j < 3; ++j)
				axis[side][i][j] = shapes[side]->rotation[3 * j + i];
			}
		}
	float relative[3][3], absolute[3][3], translation[3];
	for(unsigned i = 0; i < 3; ++i)
		for(unsigned j = 0; j < 3; ++j)
			{
			relative[i][j] = axis[0][i][0] * axis[1][j][0] +
				axis[0][i][1] * axis[1][j][1] + axis[0][i][2] * axis[1][j][2];
			absolute[i][j] = fabsf(relative[i][j]);
			}
	for(unsigned i = 0; i < 3; ++i)
		{
		const float delta[3] = { center[1][0] - center[0][0], center[1][1] - center[0][1], center[1][2] - center[0][2] };
		translation[i] = delta[0] * axis[0][i][0] + delta[1] * axis[0][i][1] + delta[2] * axis[0][i][2];
		const float radius1 = extent[1][0] * absolute[i][0] + extent[1][1] * absolute[i][1] + extent[1][2] * absolute[i][2];
		if(fabsf(translation[i]) > extent[0][i] + radius1) return false;
		}
	for(unsigned j = 0; j < 3; ++j)
		{
		const float projected = fabsf(translation[0] * relative[0][j] +
			translation[1] * relative[1][j] + translation[2] * relative[2][j]);
		const float radius0 = extent[0][0] * absolute[0][j] +
			extent[0][1] * absolute[1][j] + extent[0][2] * absolute[2][j];
		if(projected > radius0 + extent[1][j]) return false;
		}
	for(unsigned i = 0; i < 3; ++i)
		for(unsigned j = 0; j < 3; ++j)
			{
		const unsigned i1 = (i + 1) % 3, i2 = (i + 2) % 3;
		const unsigned j1 = (j + 1) % 3, j2 = (j + 2) % 3;
		const float projected = fabsf(translation[i2] * relative[i1][j] - translation[i1] * relative[i2][j]);
		const float radius0 = extent[0][i1] * absolute[i2][j] + extent[0][i2] * absolute[i1][j];
		const float radius1 = extent[1][j1] * absolute[i][j2] + extent[1][j2] * absolute[i][j1];
		if(projected > radius0 + radius1) return false;
		}
	return true;
	}

struct NxMeshMeshNodeRef
	{
	const Opcode::AABBNoLeafNode* node;
	bool leaf;
	unsigned triangle;
	};

struct NxMeshMeshTriBoxProbe : Opcode::AABBTreeCollider
	{
	bool overlap(const IceMaths::Point& v0, const IceMaths::Point& v1, const IceMaths::Point& v2,
		const IceMaths::Point& center, const IceMaths::Point& extents)
		{
		mLeafVerts[0] = v0;
		mLeafVerts[1] = v1;
		mLeafVerts[2] = v2;
		return TriBoxOverlap(center, extents) != FALSE;
		}
	bool trianglesOverlap(const IceMaths::Point& a0, const IceMaths::Point& a1, const IceMaths::Point& a2,
		const IceMaths::Point& b0, const IceMaths::Point& b1, const IceMaths::Point& b2)
		{
		mLeafVerts[0] = a0;
		mLeafVerts[1] = a1;
		mLeafVerts[2] = a2;
		return TriTriOverlap(a0, a1, a2, b0, b1, b2) != FALSE;
		}
	};

static IceMaths::Point nxMeshMeshTransformPoint(const NxCollisionShape* source,
	const NxCollisionShape* destination, const IceMaths::Point& point)
	{
	float world[3];
	for(unsigned row = 0; row < 3; ++row)
		world[row] = source->translation[row]
			+ source->rotation[3 * row] * point.x
			+ source->rotation[3 * row + 1] * point.y
			+ source->rotation[3 * row + 2] * point.z;
	IceMaths::Point local;
	local.x = destination->rotation[0] * (world[0] - destination->translation[0])
		+ destination->rotation[3] * (world[1] - destination->translation[1])
		+ destination->rotation[6] * (world[2] - destination->translation[2]);
	local.y = destination->rotation[1] * (world[0] - destination->translation[0])
		+ destination->rotation[4] * (world[1] - destination->translation[1])
		+ destination->rotation[7] * (world[2] - destination->translation[2]);
	local.z = destination->rotation[2] * (world[0] - destination->translation[0])
		+ destination->rotation[5] * (world[1] - destination->translation[1])
		+ destination->rotation[8] * (world[2] - destination->translation[2]);
	return local;
	}

static bool nxMeshMeshTriangleBoxOverlap(const NxCollisionShape* triangleShape,
	const Opcode::Model* triangleModel, unsigned triangle, const NxCollisionShape* boxShape,
	const Opcode::AABBNoLeafNode* boxNode, NxMeshMeshTriBoxProbe& probe)
	{
	Opcode::VertexPointers vertices;
	triangleModel->GetMeshInterface()->GetTriangle(vertices, triangle);
	const IceMaths::Point v0 = nxMeshMeshTransformPoint(triangleShape, boxShape, *vertices.Vertex[0]);
	const IceMaths::Point v1 = nxMeshMeshTransformPoint(triangleShape, boxShape, *vertices.Vertex[1]);
	const IceMaths::Point v2 = nxMeshMeshTransformPoint(triangleShape, boxShape, *vertices.Vertex[2]);
	return probe.overlap(v0, v1, v2, boxNode->mAABB.mCenter, boxNode->mAABB.mExtents);
	}

static bool nxMeshMeshTrianglesOverlap(const NxCollisionShape* shape0, const Opcode::Model* model0, unsigned triangle0,
	const NxCollisionShape* shape1, const Opcode::Model* model1, unsigned triangle1,
	NxMeshMeshTriBoxProbe& probe)
	{
	Opcode::VertexPointers vertices0, vertices1;
	model0->GetMeshInterface()->GetTriangle(vertices0, triangle0);
	model1->GetMeshInterface()->GetTriangle(vertices1, triangle1);
	const IceMaths::Point a0 = *vertices0.Vertex[0];
	const IceMaths::Point a1 = *vertices0.Vertex[1];
	const IceMaths::Point a2 = *vertices0.Vertex[2];
	const IceMaths::Point b0 = nxMeshMeshTransformPoint(shape1, shape0, *vertices1.Vertex[0]);
	const IceMaths::Point b1 = nxMeshMeshTransformPoint(shape1, shape0, *vertices1.Vertex[1]);
	const IceMaths::Point b2 = nxMeshMeshTransformPoint(shape1, shape0, *vertices1.Vertex[2]);
	return probe.trianglesOverlap(a0, a1, a2, b0, b1, b2);
	}

static NxMeshMeshNodeRef nxMeshMeshChild(const NxMeshMeshNodeRef& parent, bool positive)
	{
	NxMeshMeshNodeRef child;
	const bool leaf = positive ? parent.node->HasPosLeaf() : parent.node->HasNegLeaf();
	child.leaf = leaf != 0;
	child.node = child.leaf ? 0 : (positive ? parent.node->GetPos() : parent.node->GetNeg());
	child.triangle = child.leaf ? (positive ? parent.node->GetPosPrimitive() : parent.node->GetNegPrimitive()) : 0;
	return child;
	}

static bool nxMeshMeshTraverseNoLeaf(const NxCollisionShape* shape0, const NxCollisionShape* shape1,
	const Opcode::Model* model0, const Opcode::Model* model1,
	const NxMeshMeshNodeRef& node0, const NxMeshMeshNodeRef& node1, int depth,
	bool& contactFound, NxMeshMeshTriBoxProbe& probe)
	{
	if(depth < -64) return false;
	float bounds0[6], bounds1[6];
	if(node0.leaf) nxMeshMeshTriangleBounds(model0->GetMeshInterface(), node0.triangle, bounds0);
	else
		{
		const Opcode::CollisionAABB& box = node0.node->mAABB;
		bounds0[0] = box.mCenter.x; bounds0[1] = box.mCenter.y; bounds0[2] = box.mCenter.z;
		bounds0[3] = box.mExtents.x; bounds0[4] = box.mExtents.y; bounds0[5] = box.mExtents.z;
		}
	if(node1.leaf) nxMeshMeshTriangleBounds(model1->GetMeshInterface(), node1.triangle, bounds1);
	else
		{
		const Opcode::CollisionAABB& box = node1.node->mAABB;
		bounds1[0] = box.mCenter.x; bounds1[1] = box.mCenter.y; bounds1[2] = box.mCenter.z;
		bounds1[3] = box.mExtents.x; bounds1[4] = box.mExtents.y; bounds1[5] = box.mExtents.z;
		}
	if(node0.leaf && node1.leaf)
		{
		if(nxMeshMeshTrianglesOverlap(shape0, model0, node0.triangle, shape1, model1, node1.triangle, probe))
			contactFound = true;
		return depth == 0 && contactFound && nxMeshMeshSphereCallback(bounds0, bounds1);
		}
	if(!nxMeshMeshObbOverlap(shape0, bounds0, shape1, bounds1)) return false;
	if(node0.leaf && !node1.leaf
		&& nxMeshMeshTriangleBoxOverlap(shape0, model0, node0.triangle, shape1, node1.node, probe))
		contactFound = true;
	if(!node0.leaf && node1.leaf
		&& nxMeshMeshTriangleBoxOverlap(shape1, model1, node1.triangle, shape0, node0.node, probe))
		contactFound = true;
	const unsigned count0 = node0.leaf ? 1 : 2;
	const unsigned count1 = node1.leaf ? 1 : 2;
	for(unsigned i = 0; i < count0; ++i)
		for(unsigned j = 0; j < count1; ++j)
			{
			const NxMeshMeshNodeRef child0 = node0.leaf ? node0 : nxMeshMeshChild(node0, i == 0);
			const NxMeshMeshNodeRef child1 = node1.leaf ? node1 : nxMeshMeshChild(node1, j == 0);
			if(nxMeshMeshTraverseNoLeaf(shape0, shape1, model0, model1, child0, child1,
				depth - 1, contactFound, probe))
				return true;
			}
	return depth == 0 && contactFound && nxMeshMeshSphereCallback(bounds0, bounds1);
	}

// phys_fn_001876 (0x00046ab0), matrix A [MESH][MESH]. For the common
// unquantized no-leaf format, traverse the transformed trees, mark a hit only
// after triangle/triangle or triangle/node SAT succeeds, then invoke 001874 on
// the root bounds as the oracle does when the contact bit reaches depth zero.
// Other Opcode tree layouts use the vendored collider's primitive pairs. The
// accumulated callback data is reduced to the four extremal points before emission.
void __cdecl NxContactMeshMesh(const NxCollisionShape* shape0,
	const NxCollisionShape* shape1, NxContactSink* sink, void* context)
	{
	(void) context;
	if(!shape0 || !shape1 || !sink)
		return;
	const unsigned char* image0 = *(const unsigned char* const*) &shape0->geometry[0];
	const unsigned char* image1 = *(const unsigned char* const*) &shape1->geometry[0];
	if(!image0 || !image1)
		return;
	const bool convex0 = *(const void* const*) (image0 + 0xa0) != 0;
	const bool convex1 = *(const void* const*) (image1 + 0xa0) != 0;
	const bool terrain0 = *(const unsigned*) (image0 + 0x7c) != 0xff;
	const bool terrain1 = *(const unsigned*) (image1 + 0x7c) != 0xff;
	if(convex0 && convex1)
		{
		NxContactConvexConvex(shape0, shape1, sink, context);
		return;
		}
	if(terrain0 || terrain1)
		{
		if(terrain0)
			{
			if(convex1) NxContactConvexHeightfield(shape1, shape0, sink, context);
			else nxMeshHeightfieldContact(shape1, shape0, sink, context);
			}
		else
			{
			if(convex0) NxContactConvexHeightfield(shape0, shape1, sink, context);
			else nxMeshHeightfieldContact(shape0, shape1, sink, context);
			}
		return;
		}
	if(convex0)
		{
		NxContactConvexMesh(shape0, shape1, sink, context);
		return;
		}
	if(convex1)
		{
		NxContactConvexMesh(shape1, shape0, sink, context);
		return;
		}
	const Opcode::Model* model0 = *(const Opcode::Model* const*) (image0 + 0x28);
	const Opcode::Model* model1 = *(const Opcode::Model* const*) (image1 + 0x28);
	if(!model0 || !model1 || !model0->GetMeshInterface() || !model1->GetMeshInterface())
		return;

	IceMaths::Matrix4x4 world0, world1;
	nxMeshMeshSetWorld(shape0, world0);
	nxMeshMeshSetWorld(shape1, world1);
	// 001874 reads rotation and translation from the original NxCollisionShape
	// layout (+0x0c and +0x30), while the tree collider uses its copied 4x4s.
	nxTask2lSphereMatrixA = (unsigned*) shape0;
	nxTask2lSphereMatrixB = (unsigned*) shape1;

	nxMeshContactCount = 0;
	memset(nxMeshContactSums, 0, sizeof(nxMeshContactSums));
	memset(nxMeshContactVertices, 0, sizeof(nxMeshContactVertices));
	memset(nxMeshContactNormals, 0, sizeof(nxMeshContactNormals));
	if(!model0->HasLeafNodes() && !model1->HasLeafNodes()
		&& !model0->IsQuantized() && !model1->IsQuantized())
		{
		const Opcode::AABBNoLeafTree* tree0 = (const Opcode::AABBNoLeafTree*) model0->GetTree();
		const Opcode::AABBNoLeafTree* tree1 = (const Opcode::AABBNoLeafTree*) model1->GetTree();
		if(tree0->GetNbNodes() && tree1->GetNbNodes())
			{
			NxMeshMeshNodeRef root0 = { tree0->GetNodes(), false, 0 };
			NxMeshMeshNodeRef root1 = { tree1->GetNodes(), false, 0 };
			NxMeshMeshTriBoxProbe probe;
			bool contactFound = false;
			nxMeshMeshTraverseNoLeaf(shape0, shape1, model0, model1, root0, root1, 0, contactFound, probe);
			}
		}
	else
		{
		Opcode::BVTCache cache;
		cache.Model0 = model0;
		cache.Model1 = model1;
		Opcode::AABBTreeCollider collider;
		if(!collider.Collide(cache, &world0, &world1))
			return;
		const Pair* pairs = collider.GetPairs();
		for(unsigned pair = 0; pair < collider.GetNbPairs(); ++pair)
			{
			float bounds0[6], bounds1[6];
			nxMeshMeshTriangleBounds(model0->GetMeshInterface(), pairs[pair].id0, bounds0);
			nxMeshMeshTriangleBounds(model1->GetMeshInterface(), pairs[pair].id1, bounds1);
			if(nxMeshMeshSphereCallback(bounds0, bounds1))
				break;
			}
		}
	if(!nxMeshContactCount)
		return;

	NxVec3 normal(nxMeshContactSums[0], nxMeshContactSums[1], nxMeshContactSums[2]);
	const float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
	if(length != 0.0f)
		{
		normal.x /= length; normal.y /= length; normal.z /= length;
		}
	NxVec3 tangent0, tangent1;
	NxNormalToTangents(normal, tangent0, tangent1);
	unsigned extrema[4] = { 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu };
	float projections[4] = { FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX };
	for(unsigned i = 0; i < nxMeshContactCount; ++i)
		{
		const float* point = nxMeshContactVertices + 3 * i;
		const float pn = point[0] * normal.x + point[1] * normal.y + point[2] * normal.z;
		const float pt = point[0] * tangent0.x + point[1] * tangent0.y + point[2] * tangent0.z;
		const float pb = point[0] * tangent1.x + point[1] * tangent1.y + point[2] * tangent1.z;
		const float values[4] = { pn, pn, pt, pb };
		for(unsigned axis = 0; axis < 4; ++axis)
			if(((axis & 1) == 0 && values[axis] < projections[axis]) ||
				((axis & 1) != 0 && values[axis] > projections[axis]))
				{ projections[axis] = values[axis]; extrema[axis] = i; }
		}
	for(unsigned i = 0; i < 4; ++i)
		{
		bool duplicate = false;
		for(unsigned j = 0; j < i; ++j) duplicate |= extrema[i] == extrema[j];
		if(duplicate || extrema[i] >= nxMeshContactCount) continue;
		const float* point = nxMeshContactVertices + 3 * extrema[i];
		const float* contactNormal = nxMeshContactNormals + 3 * extrema[i];
		NxVec3 p(point[0], point[1], point[2]);
		NxVec3 n(contactNormal[0], contactNormal[1], contactNormal[2]);
		NxEmitContact(sink, shape1->collisionObject, shape0->collisionObject,
			0, &p, &n, 0xffff, 0xffff);
		}
	}
