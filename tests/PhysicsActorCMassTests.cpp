#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxShape.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned bits(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
static unsigned word(const unsigned char* bytes, unsigned offset)
{ unsigned u; memcpy(&u, bytes + offset, 4); return u; }
static void vector(const char* tag, const char* field, const NxVec3& v)
{
	printf("cmass %s %s=%x.%x.%x\n", tag, field, bits(v.x), bits(v.y), bits(v.z));
}
static void matrix(const char* tag, const char* field, const NxMat33& m)
{
	float v[9]; m.getRowMajor(v);
	printf("cmass %s %s=%x.%x.%x.%x.%x.%x.%x.%x.%x\n", tag, field,
		bits(v[0]), bits(v[1]), bits(v[2]), bits(v[3]), bits(v[4]),
		bits(v[5]), bits(v[6]), bits(v[7]), bits(v[8]));
}
static void pose(const char* tag, const char* field, const NxMat34& p)
{
	float v[9]; p.M.getRowMajor(v);
	printf("cmass %s %s=%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x\n", tag, field,
		bits(v[0]), bits(v[1]), bits(v[2]), bits(v[3]), bits(v[4]),
		bits(v[5]), bits(v[6]), bits(v[7]), bits(v[8]),
		bits(p.t.x), bits(p.t.y), bits(p.t.z));
}
static void probe(const char* tag, NxActor* actor)
{
	pose(tag, "local_pose", actor->getCMassLocalPose());
	vector(tag, "local_position", actor->getCMassLocalPosition());
	matrix(tag, "local_orientation", actor->getCMassLocalOrientation());
	pose(tag, "global_pose", actor->getCMassGlobalPose());
	vector(tag, "global_position", actor->getCMassGlobalPosition());
	matrix(tag, "global_orientation", actor->getCMassGlobalOrientation());
}
static void recordState(const char* tag, NxActor* actor)
{
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(actor) + 0x14);
	unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
	unsigned char* aux = *reinterpret_cast<unsigned char**>(record + 0x120);
	unsigned* flags = *reinterpret_cast<unsigned**>(aux + 0x40);
	unsigned id = word(record, 0x11c);
	printf("cmass %s record=%x.%x.%x.%x.%x.%x.%x\n", tag,
		word(record, 0x124), word(record, 0x128),
		word(record, 0x12c), word(record, 0x130),
		word(record, 0x198), word(record, 0x84), flags[id]);
}
static void transformState(const char* tag, NxActor* actor)
{
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(actor) + 0x14);
	unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
	printf("cmass %s transform=%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		tag, word(record, 0x18), word(record, 0x1c), word(record, 0x20),
		word(record, 0x24), word(record, 0x28), word(record, 0x2c), word(record, 0x30),
		word(record, 0x50), word(record, 0x54), word(record, 0x58),
		word(record, 0x5c), word(record, 0x60), word(record, 0x64), word(record, 0x68),
		word(record, 0x100), word(record, 0x104), word(record, 0x108),
		word(record, 0x158), word(record, 0x15c), word(record, 0x160));
}
// Joint-open-items Task 4: rotated bodies. A rotation matrix from a unit
// quaternion (x, y, z, w), formed in the harness and printed as input.
static NxMat33 quatMatrix(float x, float y, float z, float w)
{
	const float inv = 1.0f / sqrtf(x * x + y * y + z * z + w * w);
	x *= inv; y *= inv; z *= inv; w *= inv;
	NxMat33 m;
	m.setRow(0, NxVec3(1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z), 2.0f * (x * z + w * y)));
	m.setRow(1, NxVec3(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x)));
	m.setRow(2, NxVec3(2.0f * (x * z - w * y), 2.0f * (y * z + w * x), 1.0f - 2.0f * (x * x + y * y)));
	return m;
}
// The body-record words the mass-frame refresh (000768) and the pose
// conversions (000801, 000196/000200) write: +0x5c quaternion, +0x124
// mass-frame quaternion, +0x134 mass-frame world 3x3, +0x158 world centre,
// +0x164 world inverse inertia.
static void massFrame(const char* tag, NxActor* actor)
{
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(actor) + 0x14);
	unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
	static const unsigned blocks[5][2] = { { 0x5c, 4 }, { 0x124, 4 }, { 0x134, 9 }, { 0x158, 3 }, { 0x164, 9 } };
	for(unsigned k = 0; k < 5; ++k)
	{
		printf("cmass %s frame off=%x words=", tag, blocks[k][0]);
		for(unsigned w = 0; w < blocks[k][1]; ++w)
			printf("%s%x", w ? "." : "", word(record, blocks[k][0] + 4 * w));
		printf("\n");
	}
}
// Every setter that ends in the mass-frame refresh, over a body created at
// `orientation` with a rotated mass frame; each step prints the frame words.
static void rotatedCase(NxScene* scene, const char* name, const NxMat33& orientation,
	const NxMat33& massRotation, const NxMat33& target)
{
	NxBoxShapeDesc box; box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxBodyDesc body; body.mass = 5.0f;
	body.massSpaceInertia = NxVec3(2.0f, 3.0f, 4.0f);
	body.massLocalPose.M = massRotation;
	body.massLocalPose.t = NxVec3(0.5f, -1.0f, 1.5f);
	NxActorDesc desc; desc.shapes.pushBack(&box); desc.body = &body;
	desc.globalPose.M = orientation;
	desc.globalPose.t = NxVec3(4.0f, -1.0f, 2.0f);
	char tag[96];
	sprintf(tag, "rot_%s", name);
	matrix(tag, "input_orientation", orientation);
	matrix(tag, "input_mass_rotation", massRotation);
	matrix(tag, "input_target", target);
	NxActor* actor = scene->createActor(desc);
	printf("cmass %s created=%u\n", tag, actor ? 1u : 0u);
	if(!actor) return;
	sprintf(tag, "rot_%s_created", name); massFrame(tag, actor);
	NxMat34 targetPose; targetPose.M = target; targetPose.t = NxVec3(-2.0f, 3.0f, 0.5f);
	actor->setGlobalPose(targetPose);
	sprintf(tag, "rot_%s_set_global_pose", name); massFrame(tag, actor);
	actor->setGlobalPosition(NxVec3(1.0f, 2.0f, -3.0f));
	sprintf(tag, "rot_%s_set_global_position", name); massFrame(tag, actor);
	actor->setGlobalOrientation(orientation);
	sprintf(tag, "rot_%s_set_global_orientation", name); massFrame(tag, actor);
	NxQuat quat; quat.x = 0.1f; quat.y = -0.7f; quat.z = 0.1f; quat.w = 0.7f;
	actor->setGlobalOrientationQuat(quat);
	sprintf(tag, "rot_%s_set_global_orientation_quat", name); massFrame(tag, actor);
	actor->setCMassOffsetLocalPosition(NxVec3(-0.25f, 0.75f, 1.25f));
	sprintf(tag, "rot_%s_set_local_position", name); massFrame(tag, actor);
	actor->setCMassOffsetLocalOrientation(target);
	sprintf(tag, "rot_%s_set_local_orientation", name); massFrame(tag, actor);
	NxMat34 localPose; localPose.M = orientation; localPose.t = NxVec3(0.5f, 0.25f, -0.75f);
	actor->setCMassOffsetLocalPose(localPose);
	sprintf(tag, "rot_%s_set_local_pose", name); massFrame(tag, actor);
	actor->setCMassOffsetGlobalPosition(NxVec3(1.5f, 2.5f, -2.5f));
	sprintf(tag, "rot_%s_set_global_offset_position", name); massFrame(tag, actor);
	actor->setCMassOffsetGlobalOrientation(massRotation);
	sprintf(tag, "rot_%s_set_global_offset_orientation", name); massFrame(tag, actor);
	NxMat34 worldPose; worldPose.M = target; worldPose.t = NxVec3(2.0f, -0.5f, 1.0f);
	actor->setCMassOffsetGlobalPose(worldPose);
	sprintf(tag, "rot_%s_set_global_offset_pose", name); massFrame(tag, actor);
	sprintf(tag, "rot_%s_final", name); probe(tag, actor);
	scene->releaseActor(*actor);
}
// NpActor.cpp completion Task 3: the CMass-global setters 000204-000208 over
// single-shape and grouped (two-box) actors with rotated mass frames. Each
// step prints the record words 000789 writes (+0x18, +0x24, +0x50, +0x5c,
// +0x124, +0x164), the world centre and frame it reads, and every shape's
// global pose (the 000004 shape update; for the group, 001018's child loop;
// the pose itself is 001315's composition).
static void globalMassStep(const char* tag, NxActor* actor)
{
	massFrame(tag, actor);
	transformState(tag, actor);
	// The shape's world pose (001315's composition), as exact words.
	NxShape* const* shapes = actor->getShapes();
	for(unsigned i = 0; i < actor->getNbShapes(); ++i)
	{
		char shapeTag[128];
		sprintf(shapeTag, "%s shape%u", tag, i);
		pose(shapeTag, "global_pose", shapes[i]->getGlobalPose());
	}
}
static void globalMassCase(NxScene* scene, const char* name, unsigned shapeCount,
	const NxMat33& massRotation, const NxMat33& first, const NxMat33& second)
{
	NxBoxShapeDesc box0; box0.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxBoxShapeDesc box1; box1.dimensions = NxVec3(0.5f, 0.25f, 1.5f);
	box1.localPose.M = quatMatrix(0.2f, -0.4f, 0.3f, 0.8f);
	box1.localPose.t = NxVec3(1.25f, -0.5f, 2.0f);
	NxBodyDesc body; body.mass = 3.0f;
	body.massSpaceInertia = NxVec3(1.5f, 2.5f, 3.5f);
	body.massLocalPose.M = massRotation;
	body.massLocalPose.t = NxVec3(0.75f, -1.25f, 0.5f);
	NxActorDesc desc; desc.shapes.pushBack(&box0);
	if(shapeCount > 1) desc.shapes.pushBack(&box1);
	desc.body = &body;
	desc.globalPose.M = quatMatrix(0.3f, 0.1f, -0.2f, 0.9f);
	desc.globalPose.t = NxVec3(-3.0f, 2.0f, 1.0f);
	NxActor* actor = scene->createActor(desc);
	char tag[96];
	sprintf(tag, "gm_%s", name);
	printf("cmass %s created=%u shapes=%u\n", tag, actor ? 1u : 0u,
		actor ? actor->getNbShapes() : 0u);
	if(!actor) return;
	matrix(tag, "input_mass_rotation", massRotation);
	matrix(tag, "input_first", first);
	matrix(tag, "input_second", second);
	sprintf(tag, "gm_%s_created", name); globalMassStep(tag, actor);
	NxMat34 target; target.M = first; target.t = NxVec3(1.5f, -2.25f, 3.125f);
	actor->setCMassGlobalPose(target);
	sprintf(tag, "gm_%s_pose", name); globalMassStep(tag, actor);
	actor->setCMassGlobalPosition(NxVec3(-0.375f, 4.5f, -1.75f));
	sprintf(tag, "gm_%s_position", name); globalMassStep(tag, actor);
	actor->setCMassGlobalOrientation(second);
	sprintf(tag, "gm_%s_orientation", name); globalMassStep(tag, actor);
	sprintf(tag, "gm_%s_final", name); probe(tag, actor);
	scene->releaseActor(*actor);
}
int wmain(int argc, wchar_t** argv)
{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH]; HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorCMassTests", pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK missing");
	static NxPageGuardedAllocator allocator;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, 0);
	if(!sdk) return nxFail("SDK creation failed");
	NxSceneDesc sceneDesc; sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");
	NxBoxShapeDesc box; box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxBodyDesc body; body.mass = 5.0f;
	body.massSpaceInertia = NxVec3(2.0f, 3.0f, 4.0f);
	NxActorDesc desc; desc.shapes.pushBack(&box); desc.body = &body;
	for(unsigned variant = 0; variant < 3; ++variant)
	{
		desc.globalPose.id(); body.massLocalPose.id();
		if(variant >= 1)
		{
			desc.globalPose.t = NxVec3(4.0f, 5.0f, 6.0f);
			desc.globalPose.M.setRow(0, NxVec3(0.0f, -1.0f, 0.0f));
			desc.globalPose.M.setRow(1, NxVec3(1.0f, 0.0f, 0.0f));
			desc.globalPose.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
			body.massLocalPose.t = NxVec3(1.0f, 2.0f, 3.0f);
		}
		if(variant >= 2)
		{
			body.massLocalPose.M.setRow(0, NxVec3(1.0f, 0.0f, 0.0f));
			body.massLocalPose.M.setRow(1, NxVec3(0.0f, 0.0f, -1.0f));
			body.massLocalPose.M.setRow(2, NxVec3(0.0f, 1.0f, 0.0f));
		}
		NxActor* actor = scene->createActor(desc);
		printf("cmass variant=%u created=%u\n", variant, actor ? 1u : 0u);
		if(!actor) return nxFail("actor creation failed");
		probe(variant == 0 ? "identity" : variant == 1 ? "offset" : "rotated", actor);
		if(variant == 0)
		{
			actor->wakeUp(0.1f);
			recordState("low_wake_before", actor);
			actor->setCMassOffsetLocalPosition(NxVec3(1.0f, 2.0f, 3.0f));
			probe("low_wake_after", actor);
			recordState("low_wake_after", actor);
		}
		if(variant == 1)
		{
			recordState("offset_initial", actor);
			actor->setCMassOffsetLocalPosition(NxVec3(2.0f, 3.0f, 4.0f));
			probe("set_local_position", actor);
			recordState("set_local_position", actor);
			NxMat33 xRotation(NX_IDENTITY_MATRIX);
			xRotation.setRow(1, NxVec3(0.0f, 0.0f, -1.0f));
			xRotation.setRow(2, NxVec3(0.0f, 1.0f, 0.0f));
			actor->setCMassOffsetLocalOrientation(xRotation);
			probe("set_local_orientation", actor);
			recordState("set_local_orientation", actor);
			NxMat34 localPose;
			localPose.id();
			localPose.M.setRow(0, NxVec3(0.0f, -1.0f, 0.0f));
			localPose.M.setRow(1, NxVec3(1.0f, 0.0f, 0.0f));
			localPose.t = NxVec3(3.0f, 4.0f, 5.0f);
			actor->setCMassOffsetLocalPose(localPose);
			probe("set_local_pose", actor);
			recordState("set_local_pose", actor);
			actor->setCMassOffsetGlobalPosition(NxVec3(7.0f, 8.0f, 9.0f));
			probe("set_global_offset_position", actor);
			pose("set_global_offset_position", "actor_pose", actor->getGlobalPose());
			recordState("set_global_offset_position", actor);
			transformState("set_global_offset_position", actor);
			actor->setCMassOffsetGlobalOrientation(xRotation);
			probe("set_global_offset_orientation", actor);
			pose("set_global_offset_orientation", "actor_pose", actor->getGlobalPose());
			recordState("set_global_offset_orientation", actor);
			NxMat34 worldPose;
			worldPose.id();
			worldPose.M.setRow(0, NxVec3(0.0f, 0.0f, 1.0f));
			worldPose.M.setRow(2, NxVec3(-1.0f, 0.0f, 0.0f));
			worldPose.t = NxVec3(8.0f, 9.0f, 10.0f);
			actor->setCMassOffsetGlobalPose(worldPose);
			probe("set_global_offset_pose", actor);
			pose("set_global_offset_pose", "actor_pose", actor->getGlobalPose());
			recordState("set_global_offset_pose", actor);
			actor->setCMassGlobalPosition(NxVec3(9.0f, 10.0f, 11.0f));
			probe("set_global_mass_position", actor);
			pose("set_global_mass_position", "actor_pose", actor->getGlobalPose());
			recordState("set_global_mass_position", actor);
			transformState("set_global_mass_position", actor);
			actor->setCMassGlobalOrientation(xRotation);
			probe("set_global_mass_orientation", actor);
			pose("set_global_mass_orientation", "actor_pose", actor->getGlobalPose());
			recordState("set_global_mass_orientation", actor);
			transformState("set_global_mass_orientation", actor);
			NxMat34 movedPose;
			movedPose.id();
			movedPose.M.setRow(0, NxVec3(0.0f, -1.0f, 0.0f));
			movedPose.M.setRow(1, NxVec3(1.0f, 0.0f, 0.0f));
			movedPose.t = NxVec3(10.0f, 11.0f, 12.0f);
			actor->setCMassGlobalPose(movedPose);
			probe("set_global_mass_pose", actor);
			pose("set_global_mass_pose", "actor_pose", actor->getGlobalPose());
			recordState("set_global_mass_pose", actor);
			transformState("set_global_mass_pose", actor);
			unsigned char* movedBody = *reinterpret_cast<unsigned char**>(
				reinterpret_cast<unsigned char*>(actor) + 0x14);
			printf("cmass set_global_mass_pose body_rotation=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
				word(movedBody, 0x20), word(movedBody, 0x24), word(movedBody, 0x28),
				word(movedBody, 0x2c), word(movedBody, 0x30), word(movedBody, 0x34),
				word(movedBody, 0x38), word(movedBody, 0x3c), word(movedBody, 0x40));
			const NxMat34& reference = actor->getGlobalPoseReference();
			pose("set_global_mass_pose", "pose_reference", reference);
			printf("cmass set_global_mass_pose reference_identity=%u\n",
				&reference == reinterpret_cast<const NxMat34*>(movedBody + 0x20) ? 1u : 0u);
		}
		scene->releaseActor(*actor);
	}
	desc.body = 0;
	desc.globalPose.id();
	NxActor* staticActor = scene->createActor(desc);
	printf("cmass static_created=%u\n", staticActor ? 1u : 0u);
	if(!staticActor) return nxFail("static actor creation failed");
	probe("static", staticActor);
	const NxMat34& staticReference = staticActor->getGlobalPoseReference();
	pose("static", "pose_reference", staticReference);
	scene->releaseActor(*staticActor);
	// Joint-open-items Task 4: rotated bodies through every setter that ends
	// in the mass-frame refresh. The orientations walk every arm of the
	// matrix-to-quaternion conversions: a general rotation (trace arm), 180
	// degrees about x, y and z (exact, the three pivot arms) and three general
	// rotations whose largest diagonal is x, y and z (non-positive trace).
	const NxMat33 general = quatMatrix(1.0f, 2.0f, 3.0f, 4.0f);
	const NxMat33 mass = quatMatrix(-0.3f, 0.5f, 0.2f, 0.8f);
	const NxMat33 flipX = quatMatrix(1.0f, 0.0f, 0.0f, 0.0f);
	const NxMat33 flipY = quatMatrix(0.0f, 1.0f, 0.0f, 0.0f);
	const NxMat33 flipZ = quatMatrix(0.0f, 0.0f, 1.0f, 0.0f);
	const NxMat33 nearX = quatMatrix(0.95f, 0.2f, 0.1f, 0.2f);
	const NxMat33 nearY = quatMatrix(0.15f, 0.9f, -0.3f, 0.25f);
	const NxMat33 nearZ = quatMatrix(-0.2f, 0.25f, 0.9f, 0.3f);
	rotatedCase(scene, "general", general, mass, nearX);
	rotatedCase(scene, "flip_x", flipX, nearY, nearZ);
	rotatedCase(scene, "flip_y", flipY, nearZ, general);
	rotatedCase(scene, "flip_z", flipZ, flipX, nearY);
	rotatedCase(scene, "near_x", nearX, flipY, flipZ);
	rotatedCase(scene, "near_y", nearY, general, flipX);
	rotatedCase(scene, "near_z", nearZ, nearX, flipY);
	// NpActor.cpp completion Task 3 (000204-000208, 000756, 000789, 000746,
	// 000004/001018). With an identity mass frame the actor rotation 000789
	// converts is the target itself, so the targets walk every arm of both
	// conversions; the rotated frames give general products.
	const NxMat33 identity(NX_IDENTITY_MATRIX);
	globalMassCase(scene, "single_general", 1, mass, general, nearZ);
	globalMassCase(scene, "single_flip_x", 1, identity, flipX, nearY);
	globalMassCase(scene, "group_general", 2, mass, general, nearZ);
	globalMassCase(scene, "group_near_x", 2, identity, nearX, flipZ);
	globalMassCase(scene, "group_near_y", 2, identity, nearY, flipX);
	globalMassCase(scene, "group_near_z", 2, identity, nearZ, flipY);
	globalMassCase(scene, "group_rotated", 2, nearY, nearX, general);
	// Task 3 review (000128): the reference getter moves the position's x and
	// y through the x87 stack (an SNaN comes out quiet) and z as a dword.
	{
		NxBoxShapeDesc snanBox; snanBox.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
		NxBodyDesc snanBodyDesc; snanBodyDesc.mass = 2.0f;
		snanBodyDesc.massSpaceInertia = NxVec3(1.0f, 2.0f, 3.0f);
		NxActorDesc snanDesc; snanDesc.shapes.pushBack(&snanBox); snanDesc.body = &snanBodyDesc;
		NxActor* snanActor = scene->createActor(snanDesc);
		if(snanActor)
		{
			unsigned char* snanBody = *reinterpret_cast<unsigned char**>(
				reinterpret_cast<unsigned char*>(snanActor) + 0x14);
			unsigned char* snanRecord = *reinterpret_cast<unsigned char**>(snanBody + 8);
			unsigned saved[3];
			memcpy(saved, snanRecord + 0x50, sizeof(saved));
			const unsigned snan[3] = { 0x7f800001u, 0xff800002u, 0x7fa00003u };
			memcpy(snanRecord + 0x50, snan, sizeof(snan));
			const NxMat34& ref = snanActor->getGlobalPoseReference();
			unsigned t[3];
			memcpy(t, &ref.t, sizeof(t));
			printf("cmass reference_snan t=%x.%x.%x\n", t[0], t[1], t[2]);
			memcpy(snanRecord + 0x50, saved, sizeof(saved));
			snanActor->getGlobalPoseReference();
			scene->releaseActor(*snanActor);
		}
	}
	sdk->releaseScene(*scene); sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
