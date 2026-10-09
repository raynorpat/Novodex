#ifndef NX_PHYSICS_INTERNAL_TRIANGLE_MESH_H
#define NX_PHYSICS_INTERNAL_TRIANGLE_MESH_H
#include "NxPhysicsBackend.h"
#include "Nxp.h"
#include "Opcode.h"
#include <cstddef>

// Mechanically shared actual receiver declaration from TriangleMesh.h.
struct InternalTriangleMesh
	{
	NxU32					mVertexCount;		//!< +0x00
	NxU32					mTriangleCount;		//!< +0x04
	void*					mVertices;			//!< +0x08, 12 bytes each
	void*					mTriangles;			//!< +0x0c, 12 bytes each, 32-bit indices
	NxU16*					mMaterialIndices;	//!< +0x10, 2 bytes each, optional
	NxU32*					mFaceRemap;			//!< +0x14, 4 bytes each, optional
	void*					mVertexNormals;		//!< +0x18, 12 bytes each
	void*					mTriangleData;		//!< +0x1c, 16 bytes per triangle, allocated by 002079 and released by 002067
	Opcode::BaseModel*		mModel;				//!< +0x20
	Opcode::MeshInterface	mMeshInterface;	//!< +0x24, OPCODE's four-word mesh interface
	void*					mInterfaceAllocation;	//!< +0x34; the TriangleMesh destructor frees it through the Foundation allocator
	};

static_assert(sizeof(InternalTriangleMesh) == 0x38, "actual raw32 internal receiver");
static_assert(offsetof(InternalTriangleMesh, mVertices) == 8, "actual vertex array");
static_assert(offsetof(InternalTriangleMesh, mVertexNormals) == 0x18, "actual normal owner");
static_assert(offsetof(InternalTriangleMesh, mMeshInterface) == 0x24, "actual vendor receiver");

//! The internal mesh's vertex normals (phys_fn_002081, 0x00052240), built on
//! demand (TriangleMeshTopology.cpp, convex-mesh gap Task 2i). Thiscall on the
//! InternalTriangleMesh with no argument; naked, so declared without parameters
//! and called from naked code (001844) or through a register thunk.
#if NX_PHYSICS_USE_X87
void nxMeshComputeVertexNormals();
#else
//002081: actual internal receiver; allocates/stores output before calling the
// smooth-normal export. Allocation/result are historically unchecked.
void nxMeshComputeVertexNormals(InternalTriangleMesh* mesh);
#endif
#endif
