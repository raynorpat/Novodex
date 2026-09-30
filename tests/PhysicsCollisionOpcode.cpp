// The candidate side's OPCODE objects and the Foundation SDK for
// NxPhysicsCollisionTests' convex-mesh gap Task 2i families
// (contact_convex_heightfield, contact_convex_mesh, mesh_vertex_normals), in a
// translation unit of their own for the reason PhysicsCollisionInflate.cpp
// gives: Opcode.h's translation-unit-wide pragmas must not reach
// PhysicsCollisionTests.cpp. Nothing here returns a float or a double.
//
// - Each side's TriangleMesh image carries an OPCODE Model at +0x28 that the
//   side's own Model::Build built over a MeshInterface of its own: the
//   candidate's here (the vendored class), the oracle's through its constructor
//   and Build rows (called by the harness with the create block built here).
// - Each side's context carries an OBBCollider at +0x110: the candidate's is
//   the vendored class, constructed in place here.
// - The Foundation SDK: 001847 and 001849 report a failed query through
//   FoundationSDK::error after a guard that breaks when no instance exists, and
//   002081 allocates through nxFoundationSDKAllocator, so the families create a
//   Foundation (NxCreateFoundationSDK) with a recording output stream and a
//   recording allocator, and release it after.

#include "Opcode.h"
#include "NxFoundationSDK.h"
#include "NxUserOutputStream.h"
#include "NxUserAllocator.h"

#include <stdlib.h>
#include <string.h>
#include <new>

using namespace Opcode;

unsigned nx2iObbColliderSize()
	{
	return (unsigned) sizeof(OBBCollider);
	}

void nx2iCandidateObbColliderConstruct(void* at)
	{
	new(at) OBBCollider;
	}

void nx2iCandidateLssColliderConstruct(void* at)
	{
	new(at) LSSCollider;
	}

void nx2iCandidateLssColliderDestruct(void* at)
	{
	((LSSCollider*) at)->~LSSCollider();
	}

void nx2iCandidateObbColliderDestruct(void* at)
	{
	((OBBCollider*) at)->~OBBCollider();
	}

void nx2lCandidateAabbTreeColliderConstruct(void* at)
	{
	new(at) AABBTreeCollider;
	}

void nx2lCandidateAabbTreeColliderDestruct(void* at)
	{
	((AABBTreeCollider*) at)->~AABBTreeCollider();
	}

// The collider's flags (+0x04) and touched-primitive Container pointer (+0x10),
// as the rows read them: checked against the vendored class once.
unsigned nx2iObbColliderLayoutOk()
	{
	unsigned char storage[sizeof(OBBCollider) + 16];
	OBBCollider* c = new(storage) OBBCollider;
	c->SetFirstContact(true);
	c->SetTemporalCoherence(true);
	unsigned flags;
	memcpy(&flags, storage + 4, 4);
	const bool flagsOk = (flags & 3u) == 3u;
	c->SetFirstContact(false);
	c->SetTemporalCoherence(false);
	memcpy(&flags, storage + 4, 4);
	const bool clearedOk = (flags & 3u) == 0u;
	c->~OBBCollider();
	return flagsOk && clearedOk ? 1u : 0u;
	}

unsigned nx2iModelSize()
	{
	return (unsigned) sizeof(Model);
	}

// A MeshInterface over the given words (32-bit triangles, float vertices).
void* nx2iMeshInterfaceNew(unsigned nbTris, unsigned nbVerts, const unsigned* tris, const unsigned* verts)
	{
	MeshInterface* iface = new MeshInterface;
	iface->SetNbTriangles(nbTris);
	iface->SetNbVertices(nbVerts);
	iface->SetPointers((const IceMaths::IndexedTriangle*) tris, (const IceMaths::Point*) verts);
	return iface;
	}

void nx2iMeshInterfaceDelete(void* iface)
	{
	delete (MeshInterface*) iface;
	}

// The create block both sides build from: the given interface, the given
// splitting rules (OPCODE's SplittingRules bits), a leaf limit of one, the tree
// kind (bit 0 no-leaf, bit 1 quantized), nothing kept.
static void nx2iCreate(OPCODECREATE& create, void* iface, unsigned rules, unsigned kind)
	{
	create.mIMesh = (MeshInterface*) iface;
	create.mSettings.mRules = rules;
	create.mSettings.mLimit = 1;
	create.mNoLeaf = (kind & 1) != 0;
	create.mQuantized = (kind & 2) != 0;
	create.mKeepOriginal = false;
	create.mCanRemap = false;
	}

void* nx2iCandidateModelBuild(void* iface, unsigned rules, unsigned kind, unsigned* built)
	{
	OPCODECREATE create;
	nx2iCreate(create, iface, rules, kind);
	Model* model = new Model;
	*built = model->Build(create) ? 1u : 0u;
	return model;
	}

void nx2iCandidateModelDelete(void* model)
	{
	delete (Model*) model;
	}

// The oracle's Model in `storage` (its constructor 005362 first), built by its
// Build row (005368) from the same kind of create block.
typedef void* (__thiscall* Nx2iCtorFn)(void*);
typedef bool (__thiscall* Nx2iModelBuildFn)(void*, const OPCODECREATE*);

void nx2iOracleModelBuild(void* storage, const void* ctor, const void* build, void* iface, unsigned rules,
	unsigned kind, unsigned* built)
	{
	OPCODECREATE create;
	nx2iCreate(create, iface, rules, kind);
	((Nx2iCtorFn) ctor)(storage);
	*built = ((Nx2iModelBuildFn) build)(storage, &create) ? 1u : 0u;
	}

// A model's used bytes (its own vtable's slot) and its tree's node count.
void nx2iModelShape(const void* model, unsigned* usedBytes, unsigned* nodes)
	{
	const BaseModel* m = (const BaseModel*) model;
	*usedBytes = m->GetUsedBytes();
	const AABBOptimizedTree* tree = m->GetTree();
	*nodes = tree ? tree->GetNbNodes() : 0u;
	}

// A model's tree as words: its node array's words, each word that points into
// the array replaced by 0x80000000 | its byte offset there (the two sides'
// arrays are at different addresses); the count comes back (at most `max`).
unsigned nx2iModelTreeWords(const void* model, unsigned* out, unsigned max)
	{
	const BaseModel* m = (const BaseModel*) model;
	const AABBOptimizedTree* tree = m->GetTree();
	if(!tree)
		return 0;
	const unsigned char* nodes;
	unsigned bytes;
	if(m->HasLeafNodes())
		{
		if(m->IsQuantized())
			{
			nodes = (const unsigned char*) ((const AABBQuantizedTree*) tree)->GetNodes();
			bytes = ((const AABBQuantizedTree*) tree)->GetUsedBytes();
			}
		else
			{
			nodes = (const unsigned char*) ((const AABBCollisionTree*) tree)->GetNodes();
			bytes = ((const AABBCollisionTree*) tree)->GetUsedBytes();
			}
		}
	else
		{
		if(m->IsQuantized())
			{
			nodes = (const unsigned char*) ((const AABBQuantizedNoLeafTree*) tree)->GetNodes();
			bytes = ((const AABBQuantizedNoLeafTree*) tree)->GetUsedBytes();
			}
		else
			{
			nodes = (const unsigned char*) ((const AABBNoLeafTree*) tree)->GetNodes();
			bytes = ((const AABBNoLeafTree*) tree)->GetUsedBytes();
			}
		}
	unsigned n = 0;
	for(unsigned offset = 0; offset + 4 <= bytes && n < max; offset += 4)
		{
		unsigned word;
		memcpy(&word, nodes + offset, 4);
		const size_t address = (size_t) word;
		if(address >= (size_t) nodes && address < (size_t) nodes + bytes)
			word = 0x80000000u | (unsigned) (address - (size_t) nodes);
		out[n++] = word;
		}
	return n;
	}

// The byte offset of the model's mesh-interface word (a model copied with this
// word zeroed makes OBBCollider::Collide fail at Setup, the "Opcode is not OK."
// arms).
unsigned nx2iModelMeshInterfaceOffset(const void* model, const void* iface)
	{
	const unsigned char* bytes = (const unsigned char*) model;
	for(unsigned offset = 4; offset < sizeof(Model); offset += 4)
		{
		const void* word;
		memcpy(&word, bytes + offset, 4);
		if(word == iface)
			return offset;
		}
	return 0;
	}

// ---------------------------------------------------------------------------
// The Foundation SDK: a recording output stream and a recording allocator.

struct Nx2iReport
	{
	unsigned code, line, messageHash, fileHash;
	};

static Nx2iReport gReports[64];
static unsigned gNbReports = 0;

static unsigned nx2iHash(const char* s)
	{
	unsigned h = 2166136261u;
	for(; s && *s; ++s)
		h = (h ^ (unsigned char) *s) * 16777619u;
	return h;
	}

class Nx2iStream : public NxUserOutputStream
	{
	public:
	virtual void reportError(NxErrorCode code, const char* message, const char* file, int line)
		{
		if(gNbReports < 64)
			{
			Nx2iReport& r = gReports[gNbReports++];
			r.code = (unsigned) code;
			r.line = (unsigned) line;
			r.messageHash = nx2iHash(message);
			r.fileHash = nx2iHash(file);
			}
		}
	virtual NxAssertResponse reportAssertViolation(const char*, const char*, int) { return NX_AR_CONTINUE; }
	virtual void print(const char*) {}
	};

class Nx2iAllocator : public NxUserAllocator
	{
	public:
	unsigned mallocs, frees, lastSize;
	Nx2iAllocator() : mallocs(0), frees(0), lastSize(0) {}
	virtual void* mallocDEBUG(size_t size, const char*, int) { return take(size); }
	virtual void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType) { return take(size); }
	virtual void* malloc(size_t size) { return take(size); }
	virtual void* malloc(size_t size, NxMemoryType) { return take(size); }
	virtual void* realloc(void* memory, size_t size) { return ::realloc(memory, size); }
	virtual void free(void* memory) { if(memory) ++frees; ::free(memory); }
	void* take(size_t size) { ++mallocs; lastSize = (unsigned) size; return ::malloc(size ? size : 1); }
	};

static Nx2iStream gStream;
static Nx2iAllocator gAllocator;
static NxUserAllocator* gPreviousAllocator = 0;
static NxFoundationSDK* gFoundation = 0;

// 1 when this call created the Foundation (none existed).
unsigned nx2iFoundationBegin()
	{
	gPreviousAllocator = nxFoundationSDKAllocator;
	gNbReports = 0;
	gFoundation = NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION, &gStream, &gAllocator);
	return gFoundation && nxFoundationSDKAllocator == &gAllocator ? 1u : 0u;
	}

void nx2iFoundationEnd()
	{
	if(gFoundation)
		gFoundation->release();
	gFoundation = 0;
	nxFoundationSDKAllocator = gPreviousAllocator;
	}

// The reports since the last call, as four words each (code, line, and the
// FNV-1a hashes of the message and the file name); the count comes back.
unsigned nx2iTakeReports(unsigned* out, unsigned max)
	{
	unsigned n = gNbReports < max ? gNbReports : max;
	for(unsigned i = 0; i < n; ++i)
		{
		out[4 * i + 0] = gReports[i].code;
		out[4 * i + 1] = gReports[i].line;
		out[4 * i + 2] = gReports[i].messageHash;
		out[4 * i + 3] = gReports[i].fileHash;
		}
	const unsigned total = gNbReports;
	gNbReports = 0;
	return total;
	}

// A block the Foundation allocator handed out (002081's normals), released.
void nx2iFoundationFree(void* block)
	{
	gAllocator.free(block);
	}

void nx2iFoundationCounts(unsigned* mallocs, unsigned* frees, unsigned* lastSize)
	{
	*mallocs = gAllocator.mallocs;
	*frees = gAllocator.frees;
	*lastSize = gAllocator.lastSize;
	}
