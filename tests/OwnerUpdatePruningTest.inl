// This fixture drives the listing-confirmed 001315 arm where the shape's
// pruning collection lacks it: append the shape to the collection's +0x78
// SdkContainer, clear Prunable::mFlags bit 1, and dispatch pruner slot 3.
struct NxOwnerUpdateProbe
	{
	unsigned hits;
	unsigned shapeId;
	unsigned flagsAtCall;
	};

static NxOwnerUpdateProbe* gOwnerUpdateProbe;

static bool __fastcall nxOwnerUpdatePrunerStub(void* self, void*, void* prunable)
	{
	(void)self;
	unsigned char* bytes = static_cast<unsigned char*>(prunable);
	void* owner = 0;
	memcpy(&owner, bytes + 4, sizeof(owner));
	if(gOwnerUpdateProbe)
		{
		++gOwnerUpdateProbe->hits;
		memcpy(&gOwnerUpdateProbe->shapeId,
			static_cast<unsigned char*>(owner) + 0xd4, 4);
		memcpy(&gOwnerUpdateProbe->flagsAtCall, bytes + 8, 4);
		}
	return true;
	}

struct NxOwnerUpdateSceneFixture
	{
	unsigned char scene[0x700];
	unsigned char body[0x50];
	unsigned entries[2];
	void* prunerVtable[4];
	void* pruner[1];
	NxOwnerUpdateProbe probe;
	};

static void nxInitOwnerUpdateSceneFixture(NxOwnerUpdateSceneFixture& fixture)
	{
	memset(&fixture, 0, sizeof(fixture));
	fixture.probe.flagsAtCall = 0xffffffffu;
	fixture.prunerVtable[3] = reinterpret_cast<void*>(&nxOwnerUpdatePrunerStub);
	fixture.pruner[0] = fixture.prunerVtable;
	void* pruner = fixture.pruner;
	memcpy(fixture.scene + 0x640, &pruner, sizeof(pruner));
	const unsigned stamp = 0x2468ace0u;
	memcpy(fixture.scene + 0x540, &stamp, sizeof(stamp));
	void* scene = fixture.scene;
	memcpy(fixture.body + 4, &scene, sizeof(scene));
	const float identity[9] =
		{ 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	memcpy(fixture.body + 0x20, identity, sizeof(identity));
	const float translation[3] = { 3.0f, -2.0f, 5.0f };
	memcpy(fixture.body + 0x44, translation, sizeof(translation));
	// The SdkContainer at collection+0x78 has spare capacity. Growth is a
	// separate row; this fixture isolates the append and virtual update.
	unsigned* container = reinterpret_cast<unsigned*>(fixture.scene + 0x69c);
	container[0] = 1;
	container[1] = 0;
	container[2] = static_cast<unsigned>(reinterpret_cast<size_t>(fixture.entries));
	const float growth = 2.0f;
	memcpy(container + 3, &growth, sizeof(growth));
	}

static void nxPrepareOwnerUpdateShape(unsigned char* shape,
	NxOwnerUpdateSceneFixture& fixture, unsigned shapeId)
	{
	void* owner = fixture.body;
	void* pruning = fixture.scene + 0x624;
	const unsigned stamp = 0x2468ace0u;
	memcpy(shape + 4, &owner, sizeof(owner));
	memcpy(shape + 8, &stamp, sizeof(stamp));
	memcpy(shape + 0xa0, &pruning, sizeof(pruning));
	const unsigned flags = 2;
	memcpy(shape + 0xac, &flags, sizeof(flags));
	const unsigned short handle = 0;
	memcpy(shape + 0xcc, &handle, sizeof(handle));
	const unsigned char pruningType = 0;
	memcpy(shape + 0xce, &pruningType, sizeof(pruningType));
	memcpy(shape + 0xd4, &shapeId, sizeof(shapeId));
	const unsigned short updateFlags = 0;
	memcpy(shape + 0xdc, &updateFlags, sizeof(updateFlags));
	}

static void nxReadOwnerUpdateList(const NxOwnerUpdateSceneFixture& fixture,
	unsigned& count, unsigned& shapeId)
	{
	const unsigned* container = reinterpret_cast<const unsigned*>(fixture.scene + 0x69c);
	count = container[1];
	shapeId = 0;
	if(count)
		{
		const unsigned* entries = reinterpret_cast<const unsigned*>(container[2]);
		const unsigned char* shape = reinterpret_cast<const unsigned char*>(entries[0]);
		memcpy(&shapeId, shape + 0xd4, 4);
		}
	}

static unsigned nxTestOwnerUpdatePruning(const unsigned char* base)
	{
	const unsigned shapeId = 0x00315a51u;
	typedef void (__thiscall* NxOwnerUpdateFn)(void*, unsigned);
	NxOwnerUpdateFn oracleOwnerUpdate = reinterpret_cast<NxOwnerUpdateFn>(
		const_cast<unsigned char*>(base) + 0x000266a0);
	typedef void (__thiscall* NxSphereCtorFn)(void*, void*, unsigned);
	NxSphereCtorFn oracleSphereCtor = reinterpret_cast<NxSphereCtorFn>(
		const_cast<unsigned char*>(base) + 0x000277c0);

	NxOwnerUpdateSceneFixture oracleFixture;
	nxInitOwnerUpdateSceneFixture(oracleFixture);
	unsigned char oracleShape[0xe4];
	memset(oracleShape, 0xcd, sizeof(oracleShape));
	oracleSphereCtor(oracleShape, 0, shapeId);
	nxPrepareOwnerUpdateShape(oracleShape, oracleFixture, shapeId);
	gOwnerUpdateProbe = &oracleFixture.probe;
	oracleOwnerUpdate(oracleShape, 1);
	gOwnerUpdateProbe = 0;
	unsigned oracleCount = 0, oracleListId = 0;
	nxReadOwnerUpdateList(oracleFixture, oracleCount, oracleListId);
	unsigned oracleDC = 0, oracleFlags = 0;
	memcpy(&oracleDC, oracleShape + 0xdc, 2);
	memcpy(&oracleFlags, oracleShape + 0xac, 4);

	NxOwnerUpdateSceneFixture candidateFixture;
	nxInitOwnerUpdateSceneFixture(candidateFixture);
	unsigned char candidateBytes[0xe4];
	memset(candidateBytes, 0xcd, sizeof(candidateBytes));
	SphereShape& candidateShape = *new(candidateBytes) SphereShape(0, shapeId);
	nxPrepareOwnerUpdateShape(candidateBytes, candidateFixture, shapeId);
	gOwnerUpdateProbe = &candidateFixture.probe;
	candidateShape.mBase.nxApplyOwnerUpdate(1);
	gOwnerUpdateProbe = 0;
	unsigned candidateCount = 0, candidateListId = 0;
	nxReadOwnerUpdateList(candidateFixture, candidateCount, candidateListId);
	const unsigned candidateDC = candidateShape.mBase.mHalfwordDC;
	const unsigned candidateFlags = candidateShape.mBase.mPrunable.mFlags;

	const bool oracleOk = oracleCount == 1 && oracleListId == shapeId
		&& oracleFixture.probe.hits == 1
		&& oracleFixture.probe.shapeId == shapeId
		&& oracleFixture.probe.flagsAtCall == 0
		&& oracleDC == 2 && oracleFlags == 0;
	const bool candidateOk = candidateCount == 1 && candidateListId == shapeId
		&& candidateFixture.probe.hits == 1
		&& candidateFixture.probe.shapeId == shapeId
		&& candidateFixture.probe.flagsAtCall == 0
		&& candidateDC == 2 && candidateFlags == 0;
	const unsigned mismatches = oracleOk == candidateOk ? 0u : 1u;
	printf("ownerupd pruning oracle_ok=%u candidate_ok=%u oracle_list=%u/%08x candidate_list=%u/%08x oracle_update=%u/%08x/%08x candidate_update=%u/%08x/%08x mismatches=%u\n",
		oracleOk ? 1u : 0u, candidateOk ? 1u : 0u,
		oracleCount, oracleListId, candidateCount, candidateListId,
		oracleFixture.probe.hits, oracleFixture.probe.shapeId,
		oracleFixture.probe.flagsAtCall,
		candidateFixture.probe.hits, candidateFixture.probe.shapeId,
		candidateFixture.probe.flagsAtCall, mismatches);
	return oracleOk && candidateOk ? 0u : 1u;
	}
