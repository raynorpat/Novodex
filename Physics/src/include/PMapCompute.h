#ifndef NX_PHYSICS_PMAP_COMPUTE
#define NX_PHYSICS_PMAP_COMPUTE
/*----------------------------------------------------------------------------*|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
//! The nearest-face collider PenetrationMap::create queries (phys_fn_005328 ctor, 005329/005331 dtors,
//! 005353 the query): 0x3c bytes, with the closest squared distance at +0x2c and the closest face at +0x38.
//! The rows are naked transcriptions (PMapCompute.cpp); these members enter them with the registers the
//! oracle's callers set.
struct NxPMapNearest
	{
	unsigned char raw[0x3c];
	void init();
	void fini();
	//! phys_fn_005331, the scalar deleting destructor: flags bit 0 asks for the object to be freed (never
	//! set on an object that is not on the heap). Returns the object.
	void* destroy(unsigned flags);
	//! point: three floats; model: the OPCODE model; world: a 4x4 or null; hint: a start face in/out cell
	//! (0xffffffff for none) or null. False when the model has no mesh.
	bool query(const float* point, const void* model, const void* world, unsigned* hint);
	float distanceSquared() const { return *(const float*) (raw + 0x2c); }
	unsigned face() const { return *(const unsigned*) (raw + 0x38); }
	};

//! phys_fn_002019 (0x0004e540): a ray from `origin` along `direction` through the mesh's OPCODE model
//! (mesh + 0x28); the count of intersections (the caller keeps the parity).
unsigned nxPMapRayParity(const float* direction, const float* origin, const void* mesh);
//! phys_fn_002021 (0x0004e5d0): whether `point` lies inside `mesh`; mode 0, 1, 2 an axis ray, 3 one random
//! ray, any other three random rays by majority.
bool nxPMapClassifyPoint(unsigned mode, const float* point, const void* mesh);
//! phys_fn_002025 (0x0004e870): the grid corners of every cell that touches a surface seed get their
//! nearest face; returns the number of such cells.
unsigned nxPMapMarkCorners(void* penetrationMap, const void* model);

#endif
