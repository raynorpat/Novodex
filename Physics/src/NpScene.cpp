/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The NxScene wrapper. Its layout is measured from phys_fn_000285 (0x0000c310);
// see NpScene.h for the offsets.
//
// The Scene lock links and condition object follow the oracle's measured
// layouts. Public wrapper methods recover those protocols individually; the
// worker condition is reconstructed in the helpers below.
//
// The forwarding slots are the point of this class. phys_fn_000293 (0x0000c490)
// is the shape of all of them, and it is transcribed:
//
//     uVar3 = tryLock(scene->mWriteLock);          // +0xc
//     if (uVar3) {
//         piVar1 = scene->mWriteLock;
//         piVar5 = FUN_10011730(param_1);           // the Scene-side call
//         if (piVar5 != 0) { iVar4 = *piVar5; unlock(piVar1); return iVar4; }
//         unlock(piVar1);
//         return 0;
//     }
//     error(2, "NpScene.cpp", 0x69,
//           "PhysicsSDK: WriteLock is still aquired. Procedure call skipped to
//            avoid a deadlock!");
//     return 0;

#include "NpScene.h"
#include "NpSceneGuard.h"
#if !NX_PHYSICS_USE_X87
#include "ReadWriteLockLifetime.h"
#endif
#include <stdio.h>
#include <string.h>

#include "Scene.h"
#include "PhysicsSDK.h"
#include "NpPhysicsSDK.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxTriangle.h"
#include "NxJointDesc.h"
#include "NxJoint.h"
#include "NxUserContactReport.h"
#include "ContactPairManager.h"
#include "FoundationSDK.h"
#include "core/Joint.h"
#include "core/SpringAndDamperEffector.h"
#include "core/NpSpringAndDamperEffector.h"
#include "ContactPairManager.h"

// ---------------------------------------------------------------------------
// Lock and condition-object helpers.
// ---------------------------------------------------------------------------
#if NX_PHYSICS_USE_X87
static void* nxLockConstruct(void* memory);
#endif
static bool nxLockTryLock(void* lock);
static bool nxLockUnlock(void* lock);
static void* nxConditionConstruct(void* memory, void* a, void* b, void* c);
static bool nxConditionStart(void* condition);
static void nxConditionStop(void* condition);
// The deadlock report, defined in Scene.cpp.
void nxSceneDeadlockReport();

// The oracle emits each compatibility warning once per module lifetime.
static bool gStartRunWarningEmitted = false;
static bool gFinishRunWarningEmitted = false;
static bool gRunForWarningEmitted = false;
static bool gWaitWarningEmitted = false;

NpScene::NpScene(NxSceneInternal* scene)
	{
#if !NX_PHYSICS_USE_X87
	static_assert(alignof(NpScene)==4,"actual Scene wrapper alignment");
	static_assert(offsetof(NpScene,mWriteLock)==0x0c,"original write link placement");
	static_assert(offsetof(NpScene,mReadLock)==0x10,"original read link placement");
	static_assert(offsetof(NpScene,mLockA)==0x14 && offsetof(NpScene,mLockB)==0x18,"original event placement");
	static_assert(offsetof(NpScene,mCondition)==0x1c && offsetof(NpScene,mScene)==0x24,"original condition/owner placement");
#endif
	mWriteLock = 0;
	mReadLock = 0;
	mCondition = 0;
	mFlag = 0;
	mScene = scene;

#if NX_PHYSICS_USE_X87
	// The inner lock at +8, then the two locks at +0xc and +0x10 through
	// phys_fn_0005b6a0, then the two at +0x14 and +0x18, then the 0x18-byte object
	// at +0x1c linked to them.
	//
	// Each field points to a 4-byte link, whose word points to a 0x20-byte lock
	// block. The actor wrapper copies these link pointers into +0x0c/+0x10.
	// Allocating only the four-byte link without its inner block was the old
	// constructor overrun; allocating only the block made the public alias's
	// allocation size wrong.
	static const NxU32 kNpSceneLockBlock = 0x20;

	mWriteLock = nxFoundationSDKAllocator->malloc(4, NX_MEMORY_PERSISTENT);
	if(mWriteLock)
		{
		void* block = nxFoundationSDKAllocator->malloc(kNpSceneLockBlock, NX_MEMORY_PERSISTENT);
		*static_cast<void**>(mWriteLock) = block ? nxLockConstruct(block) : 0;
		}

	mReadLock = nxFoundationSDKAllocator->malloc(4, NX_MEMORY_PERSISTENT);
	if(mReadLock)
		{
		void* block = nxFoundationSDKAllocator->malloc(kNpSceneLockBlock, NX_MEMORY_PERSISTENT);
		*static_cast<void**>(mReadLock) = block ? nxLockConstruct(block) : 0;
		}

	mLockA[0] = mLockA[1] = mLockA[2] = mLockA[3] = 0;
	mLockB[0] = mLockB[1] = mLockB[2] = mLockB[3] = 0;
	*reinterpret_cast<HANDLE*>(mLockA) = ::CreateEventA(0, TRUE, FALSE, 0);
	*reinterpret_cast<HANDLE*>(mLockB) = ::CreateEventA(0, TRUE, FALSE, 0);
#else
	// Original C310 constructs events14/18 before the real write/read links.
	*reinterpret_cast<HANDLE*>(mLockA) = ::CreateEventA(0, TRUE, FALSE, 0);
	*reinterpret_cast<HANDLE*>(mLockB) = ::CreateEventA(0, TRUE, FALSE, 0);
	mWriteLock = nxSceneLockCreate();
	mReadLock = nxSceneLockCreate();
#endif

	mCondition = nxGetSdkAllocator()->malloc(0x18, NX_MEMORY_PERSISTENT);
	if(mCondition)
		{
		mCondition = nxConditionConstruct(mCondition, mLockB, mLockA, mScene);
		if(mCondition)
			nxConditionStart(mCondition);
		}
	}

NpScene::~NpScene()
	{
	if(mCondition)
		{
		nxConditionStop(mCondition);
		nxGetSdkAllocator()->free(*reinterpret_cast<void**>(
			static_cast<unsigned char*>(mCondition) + 4));
		nxFoundationSDKAllocator->free(mCondition);
		}
#if NX_PHYSICS_USE_X87
	if(*reinterpret_cast<HANDLE*>(mLockA))
		::CloseHandle(*reinterpret_cast<HANDLE*>(mLockA));
	if(*reinterpret_cast<HANDLE*>(mLockB))
		::CloseHandle(*reinterpret_cast<HANDLE*>(mLockB));
	if(mReadLock)
		{
		if(*static_cast<void**>(mReadLock))
			::DeleteCriticalSection(static_cast<CRITICAL_SECTION*>(
				*static_cast<void**>(mReadLock)));
		nxFoundationSDKAllocator->free(*static_cast<void**>(mReadLock));
		nxFoundationSDKAllocator->free(mReadLock);
		}
	if(mWriteLock)
		{
		if(*static_cast<void**>(mWriteLock))
			::DeleteCriticalSection(static_cast<CRITICAL_SECTION*>(
				*static_cast<void**>(mWriteLock)));
		nxFoundationSDKAllocator->free(*static_cast<void**>(mWriteLock));
		nxFoundationSDKAllocator->free(mWriteLock);
		}
#else
	// Original D970: condition ends first, write then read, events18 then14.
	nxSceneLockDestroy(mWriteLock);
	nxSceneLockDestroy(mReadLock);
	if(*reinterpret_cast<HANDLE*>(mLockB))
		::CloseHandle(*reinterpret_cast<HANDLE*>(mLockB));
	if(*reinterpret_cast<HANDLE*>(mLockA))
		::CloseHandle(*reinterpret_cast<HANDLE*>(mLockA));
#endif
	}

// phys_fn_000293 (0x0000c490): the forwarding shape every slot in this class has.
// The oracle's `this + 0xc` is mWriteLock, and its `this + 0x24` is mScene.
NxActor* NpScene::createActor(const NxActorDescBase& desc)
	{
	if(!mWriteLock || !nxLockTryLock(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x69, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return 0;
		}

	NxActor* actor = mScene->createActor(desc);

	nxLockUnlock(mWriteLock);
	return actor;
	}

// phys_fn_000295 at 0x0000c500: the same lock, then Scene::releaseActor.
void NpScene::releaseActor(NxActor& actor)
	{
	if(!mWriteLock || !nxLockTryLock(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x70, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}

	void* body = *reinterpret_cast<void**>(
		reinterpret_cast<unsigned char*>(&actor) + 0x14);
	if(body)
		mScene->releaseActor(body);
	nxLockUnlock(mWriteLock);
	}

void NpScene::release()
	{
	// The oracle's NpScene release forwards to the Scene's release, which is a
	// Phase 3 row and not reconstructed.
	}

// ---------------------------------------------------------------------------
// Lock and condition implementations.
// ---------------------------------------------------------------------------

#if NX_PHYSICS_USE_X87
static void* nxLockConstruct(void* memory)
	{
	// phys_fn_0005b6a0 zeroes the writer flag at +0x18 and initializes
	// the CRITICAL_SECTION occupying the first 0x18 bytes.
	memset(memory, 0, 0x20);
	::InitializeCriticalSection(static_cast<CRITICAL_SECTION*>(memory));
	return memory;
	}
#endif

static bool nxLockTryLock(void* lock)
	{
	return nxNpSceneGuardWriteTry(lock);
	}

static bool nxLockUnlock(void* lock)
	{
	nxNpSceneGuardLeave(lock);
	return true;
	}

static void* nxConditionConstruct(void* memory, void* a, void* b, void* c)
	{
	// The condition object is 0x18 bytes: vptr, allocated worker state, work
	// event, completion event, scene pointer, and SDK lock pointer.
	memset(memory, 0, 0x18);
	void* state = nxFoundationSDKAllocator->malloc(0x14, NX_MEMORY_PERSISTENT);
	if(state) memset(state, 0, 0x14);
	unsigned char* bytes = static_cast<unsigned char*>(memory);
	*reinterpret_cast<void**>(bytes + 4) = state;
	*reinterpret_cast<HANDLE*>(bytes + 8) = *reinterpret_cast<HANDLE*>(a);
	*reinterpret_cast<HANDLE*>(bytes + 12) = *reinterpret_cast<HANDLE*>(b);
	*reinterpret_cast<NxSceneInternal**>(bytes + 16) = static_cast<NxSceneInternal*>(c);
	ReadWriteLock* sdkLock = 0;
	if(PhysicsSDK::instance && PhysicsSDK::instance->getNp())
		sdkLock = &PhysicsSDK::instance->getNp()->mLock;
	*reinterpret_cast<ReadWriteLock**>(bytes + 20) = sdkLock;
	return memory;
	}

// Worker state is laid out at condition+4: thread handle, state (0 stopped,
// 1 running, 2 exited), and exit flag. The remaining bytes are reserved by the
// original 0x14-byte allocation.
static DWORD WINAPI nxSceneWorker(void* parameter)
	{
	unsigned char* condition = static_cast<unsigned char*>(parameter);
	HANDLE work = *reinterpret_cast<HANDLE*>(condition + 8);
	HANDLE done = *reinterpret_cast<HANDLE*>(condition + 12);
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(condition + 16);
	ReadWriteLock* sdkLock = *reinterpret_cast<ReadWriteLock**>(condition + 20);
	unsigned char* state = *reinterpret_cast<unsigned char**>(condition + 4);

	for(;;)
		{
		if(::WaitForSingleObject(work, INFINITE) != WAIT_OBJECT_0)
			return 0;
		::ResetEvent(work);
		if(*reinterpret_cast<volatile LONG*>(state + 8))
			{
			*reinterpret_cast<LONG*>(state + 4) = 2;
			return 0;
			}

		if(sdkLock)
			sdkLock->lock();
		NpScene* wrapper = scene
			? reinterpret_cast<NpScene*>(scene->at<void*>(0x6cc)) : 0;
		const bool sceneLocked = wrapper && wrapper->writeLink()
			&& nxNpSceneGuardWriteTry(wrapper->writeLink());

		if(sceneLocked && scene)
			scene->simulateFrame();

		if(sceneLocked)
			nxNpSceneGuardLeave(wrapper->writeLink());
		if(sdkLock)
			sdkLock->unlock();
		::SetEvent(done);
		}
	}

static bool nxConditionStart(void* condition)
	{
	unsigned char* bytes = static_cast<unsigned char*>(condition);
	unsigned char* state = *reinterpret_cast<unsigned char**>(bytes + 4);
	if(!state || *reinterpret_cast<LONG*>(state + 4) != 0)
		return false;
	*reinterpret_cast<volatile LONG*>(state + 8) = 0;
	HANDLE thread = ::CreateThread(0, 0, nxSceneWorker, condition, 0, 0);
	if(!thread)
		return false;
	*reinterpret_cast<HANDLE*>(state) = thread;
	*reinterpret_cast<LONG*>(state + 4) = 1;
	return true;
	}

static void nxConditionStop(void* condition)
	{
	unsigned char* bytes = static_cast<unsigned char*>(condition);
	unsigned char* state = *reinterpret_cast<unsigned char**>(bytes + 4);
	if(!state || *reinterpret_cast<LONG*>(state + 4) != 1)
		return;
	*reinterpret_cast<volatile LONG*>(state + 8) = 1;
	::SetEvent(*reinterpret_cast<HANDLE*>(bytes + 8));
	HANDLE thread = *reinterpret_cast<HANDLE*>(state);
	if(thread)
		{
		::WaitForSingleObject(thread, INFINITE);
		::CloseHandle(thread);
		}
	*reinterpret_cast<LONG*>(state + 4) = 2;
	}

// ---------------------------------------------------------------------------
// The remaining NxScene virtuals. Unimplemented methods are explicit stubs;
// reconstructed methods are annotated at their definitions below.
// ---------------------------------------------------------------------------


// phys_fn_000291 (0x0000c460, 39 B): read-lock the Scene gravity copy.
void NpScene::getGravity(NxVec3& gravity)
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	mScene->getGravity(gravity);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000299 (0x0000c5d0, 87 B, phase 7): NxScene::releaseJoint. The
// write lock at +0xc through phys_fn_002364; on failure the Foundation
// instance test with int3 and the deadlock report (code 2, NpScene.cpp line
// 0x7f). Otherwise Scene::releaseJoint (phys_fn_000653) on the joint's
// appData word (+0x08, the internal Joint), then phys_fn_002366 on the link
// value loaded before the call.
void NpScene::releaseJoint(NxJoint& joint)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x7f, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	void* link = mWriteLock;
	mScene->releaseJoint(static_cast<Joint*>(joint.appData));
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000301 (0x0000c630, 140 B, phase 7):
// NxScene::createSpringAndDamperEffector. The write lock at +0xc through
// phys_fn_002364; on failure the deadlock report (code 2, NpScene.cpp line
// 0x87) and 0. Otherwise Scene::createSpringAndDamperEffector
// (phys_fn_000587). A null internal effector returns 0. An internal effector
// with a public object gets the scene's read link (+0x10) at np+0x10 and its
// write link (+0xc) at np+0xc and the public object is returned; one whose
// public object is null (its allocation failed) is released again through
// Scene::releaseEffector (phys_fn_000594) and 0 is returned. The unlock is
// on the link value loaded before the call.
NxSpringAndDamperEffector* NpScene::createSpringAndDamperEffector(const NxSpringAndDamperEffectorDesc& desc)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x87, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return 0;
		}
	void* link = mWriteLock;
	SpringAndDamperEffector* effector = mScene->createSpringAndDamperEffector(desc);
	if(effector)
		{
		NpSpringAndDamperEffector* np = effector->mPublicObject;
		if(np)
			{
			np->mWord08 = reinterpret_cast<NxU32>(mReadLock);
			np->mWord04 = reinterpret_cast<NxU32>(mWriteLock);
			nxNpSceneGuardLeave(link);
			return np;
			}
		mScene->releaseEffector(effector);
		}
	nxNpSceneGuardLeave(link);
	return 0;
	}

// phys_fn_000303 (0x0000c6c0, 92 B, phase 7): NxScene::releaseEffector. The
// write lock at +0xc; on failure the deadlock report (code 2, NpScene.cpp
// line 0x9a). Otherwise the internal effector at the public object's +0x14
// (phys_fn_003952, called on the argument as the spring-and-damper
// wrapper, the only effector type) goes to Scene::releaseEffector
// (phys_fn_000594), then the unlock on the link value loaded before it.
void NpScene::releaseEffector(NxEffector& effector)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x9a, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	void* link = mWriteLock;
	mScene->releaseEffector(static_cast<NpSpringAndDamperEffector&>(effector).getInternal());
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000305 (0x0000c720): direct Scene::createController forward. This
// slot is intentionally not write-locked in the pinned NpScene implementation.
NxController* NpScene::createController(const NxControllerDesc& desc)
	{
	return mScene ? mScene->createController(desc) : 0;
	}

// phys_fn_000307 (0x0000c730): direct Scene::releaseController forward.
void NpScene::releaseController(NxController& controller)
	{
	if(mScene)
		mScene->releaseController(controller);
	}

// phys_fn_000309 (0x0000c740): guarded forward to Scene::setActorPairFlags.
void NpScene::setActorPairFlags(NxActor& actor0, NxActor& actor1, NxU32 nxContactPairFlag)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0xab, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	void* link = mWriteLock;
	NxU8* body0 = *reinterpret_cast<NxU8**>(reinterpret_cast<NxU8*>(&actor0) + 0x14);
	NxU8* body1 = *reinterpret_cast<NxU8**>(reinterpret_cast<NxU8*>(&actor1) + 0x14);
	NxU8* shape0 = body0 ? *reinterpret_cast<NxU8**>(body0 + 0x10) : 0;
	NxU8* shape1 = body1 ? *reinterpret_cast<NxU8**>(body1 + 0x10) : 0;
	if(mScene && shape0 && shape1 && shape0 != shape1)
		cpmSetShapePairFlags(mScene, shape0, shape1, nxContactPairFlag);
	nxNpSceneGuardLeave(link);
	}

// Return the flags stored by setActorPairFlags.
NxU32 NpScene::getActorPairFlags(NxActor& actor0, NxActor& actor1) const
	{
	return mScene ? cpmGetActorPairFlags(mScene, &actor0, &actor1) : 0;
	}

// phys_fn_000313 (0x0000c7f0): write-locked forward to Scene::setShapePairFlags.
void NpScene::setShapePairFlags(NxShape& shape0, NxShape& shape1, NxU32 nxContactPairFlag)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0xb8, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	void* link = mWriteLock;
	NxU8* const internalShape0 = *reinterpret_cast<NxU8**>(reinterpret_cast<NxU8*>(&shape0) + 8);
	NxU8* const internalShape1 = *reinterpret_cast<NxU8**>(reinterpret_cast<NxU8*>(&shape1) + 8);
	if(mScene)
		mScene->setShapePairFlags(internalShape0, internalShape1, nxContactPairFlag);
	nxNpSceneGuardLeave(link);
	}

// Return the flags stored by setShapePairFlags.
NxU32 NpScene::getShapePairFlags(NxShape& shape0, NxShape& shape1) const
	{
	if(!mScene) return 0;
	const NxU8* const internalShape0 = *reinterpret_cast<NxU8* const*>(reinterpret_cast<const NxU8*>(&shape0) + 8);
	const NxU8* const internalShape1 = *reinterpret_cast<NxU8* const*>(reinterpret_cast<const NxU8*>(&shape1) + 8);
	return cpmGetShapePairFlags(mScene, internalShape0, internalShape1);
	}

// Read-lock while forwarding the active contact-pair count from Scene.
NxU32 NpScene::getNbPairs() const
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	const NxU32 count = mScene->getNbPairs();
	nxNpSceneGuardLeave(link);
	return count;
	}

// Read-lock while forwarding the active pair flags from Scene.
bool NpScene::getPairFlagArray(NxPairFlag* userArray, NxU32 numPairs) const
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	const bool result = mScene->getPairFlagArray(userArray, numPairs);
	nxNpSceneGuardLeave(link);
	return result;
	}

// The Scene actor array begins at internal +0x55c. The wrapper forwards the
// count as the distance from its first pointer to its last pointer.
NxU32 NpScene::getNbActors() const
	{
	const NxActor* const* first = mScene->at<NxActor**>(0x55c);
	const NxActor* const* last = mScene->at<NxActor**>(0x560);
	return first ? static_cast<NxU32>(last - first) : 0;
	}

// The public pointer is the Scene's existing contiguous actor array.
NxActor** NpScene::getActors()
	{
	return mScene->at<NxActor**>(0x55c);
	}

// phys_fn_000321 (0x0000c910, 36 B, phase 7): NxScene::getNbJoints. The
// read lock at +0x10 (phys_fn_002362 / phys_fn_002366, the link value loaded
// once) around Scene::getNbJoints (phys_fn_000559).
NxU32 NpScene::getNbJoints() const
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	const NxU32 count = mScene->getNbJoints();
	nxNpSceneGuardLeave(link);
	return count;
	}

// phys_fn_000323 (0x0000c940, 31 B, phase 7): NxScene::resetJointIterator,
// the same lock around Scene::resetJointIterator (phys_fn_000563); the
// unlock is a tail jump.
void NpScene::resetJointIterator()
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	mScene->resetJointIterator();
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000325 (0x0000c960, 55 B, phase 7): NxScene::getNextJoint, the
// same lock around Scene::getNextJoint (phys_fn_000567); a joint is returned
// as its public object ([internal+0x48], 0xc97a), the end as 0.
NxJoint * NpScene::getNextJoint()
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	Joint* joint = mScene->getNextJoint();
	NxJoint* result = joint ? static_cast<NxJoint*>(joint->mPublicObject) : 0;
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000327 (0x0000c9a0, 36 B, phase 7): NxScene::getNbEffectors. The
// read lock at +0x10 (phys_fn_002362 / phys_fn_002366, the link value loaded
// once) around Scene::getNbEffectors (phys_fn_000561).
NxU32 NpScene::getNbEffectors() const
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	const NxU32 count = mScene->getNbEffectors();
	nxNpSceneGuardLeave(link);
	return count;
	}

// phys_fn_000329 (0x0000c9d0, 31 B, phase 7): NxScene::resetEffectorIterator,
// the same lock around Scene::resetEffectorIterator (phys_fn_000565); the
// unlock is a tail jump.
void NpScene::resetEffectorIterator()
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	mScene->resetEffectorIterator();
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000331 (0x0000c9f0, 55 B, phase 7): NxScene::getNextEffector, the
// same lock around Scene::getNextEffector (phys_fn_000569); an effector is
// returned as its public object ([internal+0x20], 0xca0a), the end as 0.
NxEffector * NpScene::getNextEffector()
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	Effector* effector = mScene->getNextEffector();
	NxEffector* result = effector ? effector->mPublicObject : 0;
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000333 (oracle RVA 0x0000ca30): load the scene pointer at +0x24 and
// tail-jump to NxFluidAssert. The pinned NxFluidAssert is a one-byte `retn`, so
// this public slot has no observable work in the shipped binary.
void NpScene::flushStream()
	{
	
	}

// phys_fn_000335 (0x0000ca40): warn once, then use the public simulate slot.
void NpScene::startRun(NxReal elapsedTime)
	{
	if(!gStartRunWarningEmitted)
		{
		gStartRunWarningEmitted = true;
		NxFoundation::FoundationSDK::getInstance().error(NXE_DB_PRINT,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x114, 0,
			"Warning: deprecated method: \nScene::startRun(). Use the new  simulate() instead!\n\n");
		}
	simulate(elapsedTime);
	}

// phys_fn_000336 (0x0000ca90): warn once, then fetch the finished rigid-body run.
void NpScene::finishRun()
	{
	if(!gFinishRunWarningEmitted)
		{
		gFinishRunWarningEmitted = true;
		NxFoundation::FoundationSDK::getInstance().error(NXE_DB_PRINT,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x11a, 0,
			"Warning: deprecated method: \nScene::finishRun(). Use the new  fetchResults() instead!\n\n");
		}
	fetchResults(NX_RIGID_BODY_FINISHED, true);
	}

// phys_fn_000338 (0x0000cae0): write-lock, then forward the timing triplet.
void NpScene::setTiming(NxReal maxTimestep, NxU32 maxIter, NxTimeStepMethod method)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x120, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	void* link = mWriteLock;
	mScene->setTiming(maxTimestep, maxIter, static_cast<NxU32>(method));
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000340 (0x0000cb50): read-lock, then copy the timing triplet out.
void NpScene::getTiming(NxReal & maxTimestep, NxU32 & maxIter, NxTimeStepMethod & method) const
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	NxU32 methodValue;
	mScene->getTiming(maxTimestep, maxIter, methodValue);
	method = static_cast<NxTimeStepMethod>(methodValue);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000342 (0x0000cb90): the legacy sequence is setTiming, simulate,
// flushStream, and blocking fetchResults, with its deprecation warning once.
void NpScene::runFor(NxReal elapsedTime, NxReal maxTimestep, NxU32 maxIter, NxTimeStepMethod method)
	{
	if(!gRunForWarningEmitted)
		{
		gRunForWarningEmitted = true;
		NxFoundation::FoundationSDK::getInstance().error(NXE_DB_PRINT,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x12e, 0,
			"Warning: deprecated method: \nScene::runFor. Use the new setTiming(), simulate(), flushStream(), fetchResults() sequence instead!\n\n");
		}
	setTiming(maxTimestep, maxIter, method);
	simulate(elapsedTime);
	flushStream();
	fetchResults(NX_RIGID_BODY_FINISHED, true);
	}

// phys_fn_000344 (0x0000cc10, 77 B)
// NxScene::visualize (slot 31 of the table at .rdata:0x10105a98). The write
// lock at +0xc (phys_fn_002364); on failure the deadlock report (code 2, line
// 0x13c) and a plain return, no unlock. Otherwise the link is kept, the Scene
// row on +0x24 runs (phys_fn_000657), and the unlock (phys_fn_002366, a tail
// jump in the image) is on the kept link. Scene-raycast block Task 4.
void NpScene::visualize()
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x13c, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	void* link = mWriteLock;
	mScene->visualize();
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000346 (0x0000cc60): read-lock around Scene::getSceneStats.
NxSceneStats* NpScene::getSceneStats()
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	NxSceneStats* const stats = mScene->getSceneStats();
	nxNpSceneGuardLeave(link);
	return stats;
	}

// phys_fn_000348 (0x0000cc90): read-lock around Scene::getLimits.
void NpScene::getLimits(NxSceneLimits& limits) const
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	mScene->getLimits(limits);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000350: store the callback consumed by joint-break events.
void NpScene::setUserNotify(NxUserNotify* callback)
	{
	if(mScene)
		mScene->at<NxUserNotify*>(0x6ac) = callback;
	}

// phys_fn_000352: read the callback stored at Scene+0x6ac.
NxUserNotify* NpScene::getUserNotify() const
	{
	return mScene ? mScene->at<NxUserNotify*>(0x6ac) : 0;
	}

// phys_fn_000354: store the trigger callback consumed by phys_fn_000640.
void NpScene::setUserTriggerReport(NxUserTriggerReport* callback)
	{
	if(mScene)
		mScene->at<NxUserTriggerReport*>(0x6b0) = callback;
	}

// phys_fn_000356: read the callback stored at Scene+0x6b0.
NxUserTriggerReport* NpScene::getUserTriggerReport() const
	{
	return mScene ? mScene->at<NxUserTriggerReport*>(0x6b0) : 0;
	}

// phys_fn_000358 (0x0000cde0): Scene+0x6b4 stores the callback pointer.
void NpScene::setUserContactReport(NxUserContactReport* callback)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxSceneDeadlockReport();
		return;
		}
	void* link = mWriteLock;
	if(mScene) mScene->at<void*>(0x6b4) = callback;
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000360 (0x0000ce40): Scene+0x6b4 getter.
NxUserContactReport* NpScene::getUserContactReport() const
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	NxUserContactReport* report = mScene ? mScene->at<NxUserContactReport*>(0x6b4) : 0;
	nxNpSceneGuardLeave(link);
	return report;
	}

// phys_fn_000362 (0x0000ce70). The pinned SDK exports this entry but fluid
// contact reporting is disabled; retain the callback-independent warning.
void NpScene::setUserFluidContactReport(NxUserFluidContactReport* callback)
	{
	(void)callback;
	NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
		"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x179, 0,
		"NxFluid::setUserFluidContactReport(): Feature not available!");
	}

// phys_fn_000364 (0x0000cea0). The unavailable getter warns and returns null.
NxUserFluidContactReport* NpScene::getUserFluidContactReport() const
	{
	NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
		"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x17f, 0,
		"NxFluid::getUserFluidContactReport(): Feature not available!");
	return 0;
	}

// The six NxScene raycasts (slots 42-47 of the NpScene table at
// .rdata:0x10105a98). Each takes the write lock at +0xc (phys_fn_002364); on
// failure it reports the deadlock (code 2, this file's line) and returns a
// null result without unlocking. Otherwise the lock link is kept, maxDist must
// be greater than zero (the fcomp against 0.0f, 0x1000cf11: a NaN fails),
// else error 1 on the next line; the Scene row on +0x24 runs, then the unlock
// (phys_fn_002366) on the kept link. Scene-raycast block Task 3.
static const char* const kNpSceneFile = "\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp";
static const char* const kNpSceneDeadlock =
	"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!";

static inline void nxNpSceneRaycastError(NxErrorCode code, int line, const char* message)
	{
	NxFoundation::FoundationSDK::getInstance().error(code, kNpSceneFile, line, 0, message);
	}

// phys_fn_000366 (0x0000ced0, 181 B)
bool NpScene::raycastAnyBounds(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups, NxReal maxDist) const
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxNpSceneRaycastError(NXE_INVALID_OPERATION, 0x18b, kNpSceneDeadlock);
		return false;
		}
	void* link = mWriteLock;
	if(!(maxDist > 0.0f))
		{
		nxNpSceneRaycastError(NXE_INVALID_PARAMETER, 0x18c,
			"Scene::raycastAnyBounds: The maximum distance must be greater than zero!");
		nxNpSceneGuardLeave(link);
		return false;
		}
	const bool result = mScene->raycastAnyBounds(worldRay, shapesType, groups, maxDist);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000368 (0x0000cf90, 181 B)
bool NpScene::raycastAnyShape(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups, NxReal maxDist) const
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxNpSceneRaycastError(NXE_INVALID_OPERATION, 0x197, kNpSceneDeadlock);
		return false;
		}
	void* link = mWriteLock;
	if(!(maxDist > 0.0f))
		{
		nxNpSceneRaycastError(NXE_INVALID_PARAMETER, 0x198,
			"Scene::raycastAnyShape: The maximum distance must be greater than zero!");
		nxNpSceneGuardLeave(link);
		return false;
		}
	const bool result = mScene->raycastAnyShape(worldRay, shapesType, groups, maxDist);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000370 (0x0000d050, 189 B)
NxU32 NpScene::raycastAllBounds(const NxRay& worldRay, NxUserRaycastReport& report, NxShapesType shapesType, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxNpSceneRaycastError(NXE_INVALID_OPERATION, 0x1a2, kNpSceneDeadlock);
		return 0;
		}
	void* link = mWriteLock;
	if(!(maxDist > 0.0f))
		{
		nxNpSceneRaycastError(NXE_INVALID_PARAMETER, 0x1a3,
			"Scene::raycastAllBounds: The maximum distance must be greater than zero!");
		nxNpSceneGuardLeave(link);
		return 0;
		}
	const NxU32 result = mScene->raycastAllBounds(worldRay, report, shapesType, groups, maxDist, hintFlags);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000372 (0x0000d110, 189 B)
NxU32 NpScene::raycastAllShapes(const NxRay& worldRay, NxUserRaycastReport& report, NxShapesType shapesType, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxNpSceneRaycastError(NXE_INVALID_OPERATION, 0x1ad, kNpSceneDeadlock);
		return 0;
		}
	void* link = mWriteLock;
	if(!(maxDist > 0.0f))
		{
		nxNpSceneRaycastError(NXE_INVALID_PARAMETER, 0x1ae,
			"Scene::raycastAllShapes: The maximum distance must be greater than zero!");
		nxNpSceneGuardLeave(link);
		return 0;
		}
	const NxU32 result = mScene->raycastAllShapes(worldRay, report, shapesType, groups, maxDist, hintFlags);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000374 (0x0000d1d0, 210 B)
// maxDist is checked, then the Scene row gets FLT_MAX and hint flags
// 0xffffffff in their place (0x1000d263, 0x1000d265). A hit returns the
// shape's public object (+0x9c).
NxShape* NpScene::raycastClosestBounds(const NxRay& worldRay, NxShapesType shapeType, NxRaycastHit& hit, NxU32 groups, NxReal maxDist, NxU32 /*hintFlags*/) const
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxNpSceneRaycastError(NXE_INVALID_OPERATION, 0x1b9, kNpSceneDeadlock);
		return 0;
		}
	void* link = mWriteLock;
	if(!(maxDist > 0.0f))
		{
		nxNpSceneRaycastError(NXE_INVALID_PARAMETER, 0x1ba,
			"Scene::raycastClosestBounds: The maximum distance must be greater than zero!");
		nxNpSceneGuardLeave(link);
		return 0;
		}
	void* shape = mScene->raycastClosestBounds(worldRay, shapeType, hit, groups, NX_MAX_F32, 0xffffffff);
	NxShape* result = shape
		? *reinterpret_cast<NxShape**>(static_cast<unsigned char*>(shape) + 0x9c)
		: 0;
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000376 (0x0000d2b0, 213 B)
NxShape* NpScene::raycastClosestShape(const NxRay& worldRay, NxShapesType shapeType, NxRaycastHit& hit, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxNpSceneRaycastError(NXE_INVALID_OPERATION, 0x1c6, kNpSceneDeadlock);
		return 0;
		}
	void* link = mWriteLock;
	if(!(maxDist > 0.0f))
		{
		nxNpSceneRaycastError(NXE_INVALID_PARAMETER, 0x1c7,
			"Scene::raycastClosestShape: The maximum distance must be greater than zero!");
		nxNpSceneGuardLeave(link);
		return 0;
		}
	void* shape = mScene->raycastClosestShape(worldRay, shapeType, hit, groups, maxDist, hintFlags);
	NxShape* result = shape
		? *reinterpret_cast<NxShape**>(static_cast<unsigned char*>(shape) + 0x9c)
		: 0;
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000378 (0x0000d390): read-lock around the Scene's sphere overlap collection.
NxU32 NpScene::overlapSphereShapes(const NxSphere& worldSphere, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback)
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	const NxU32 result = mScene->overlapSphereShapes(worldSphere, shapeType, nbShapes, shapes, callback);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000380 (0x0000d400): read-lock around the Scene's AABB overlap collection.
NxU32 NpScene::overlapAABBShapes(const NxBounds3& worldBounds, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback)
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	const NxU32 result = mScene->overlapAABBShapes(worldBounds, shapeType, nbShapes, shapes, callback);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000382 (0x0000d480): read-lock around the Scene's plane culling query.
NxU32 NpScene::cullShapes(NxU32 nbPlanes, const NxPlane* worldPlanes, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback)
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	const NxU32 result = mScene->cullShapes(nbPlanes, worldPlanes, shapeType, nbShapes, shapes, callback);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000384 (0x0000d500): read-lock, run the sphere overlap query, and
// release the lock.
bool NpScene::checkOverlapSphere(const NxSphere& worldSphere, NxShapesType shapeType)
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	const bool result = mScene->checkOverlapSphere(worldSphere, shapeType);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000386 (0x0000d580): read-lock, run the pruner-backed AABB query,
// and release the same read lock on the way out.
bool NpScene::checkOverlapAABB(const NxBounds3& worldBounds, NxShapesType shapeType)
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	const bool result = mScene->checkOverlapAABB(worldBounds, shapeType);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000388 (0x0000d5c0): read-lock around the Scene-side AABB triangle query.
NxU32 NpScene::overlapAABBTriangles(const NxBounds3& worldBounds, NxArraySDK<NxTriangle>& worldTriangles)
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	const NxU32 result = mScene->overlapAABBTriangles(worldBounds, worldTriangles);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000400 (0x0000d7c0): take the write lock, lazily create the internal
// fluid manager through Scene, then return the public fluid object (null while
// the shipped build's fluid backend is disabled).
NxFluid* NpScene::createFluid(const NxFluidDesc& desc)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x266, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return 0;
		}
	void* link = mWriteLock;
	NxFluid* fluid = mScene->createFluid(desc);
	nxNpSceneGuardLeave(link);
	return fluid;
	}

// phys_fn_000402 (0x0000d840): release through Scene using the NpFluid
// internal pointer at +0x14, under the scene write lock.
void NpScene::releaseFluid(NxFluid& fluid)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x26d, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	void* link = mWriteLock;
	void* fluidInternal = *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(&fluid) + 0x14);
	if(mScene)
		mScene->releaseFluid(fluidInternal);
	nxNpSceneGuardLeave(link);
	}

// (unimplemented) getNbFluids
NxU32 NpScene::getNbFluids() const
	{
	return 0;
	}

// (unimplemented) getFluids
NxFluid** NpScene::getFluids()
	{
	return 0;
	}

// phys_fn_000404 (0x0000d8a0). Implicit meshes are unavailable in the pinned
// build; preserve the exported warning and null return.
NxImplicitMesh* NpScene::createImplicitMesh(const NxImplicitMeshDesc& desc)
	{
	(void)desc;
	NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
		"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x281, 0,
		"NxScene::createImplicitMesh(): Feature not available!");
	return 0;
	}

// phys_fn_000406 (0x0000d8d0). The unavailable release operation warns and
// does not inspect or retain the supplied object.
void NpScene::releaseImplicitMesh(NxImplicitMesh& mesh)
	{
	(void)mesh;
	NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
		"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x287, 0,
		"NxScene::releaseImplicitMesh(): Feature not available!");
	}

// phys_fn_000408 (0x0000d900). The unavailable count operation warns and
// returns zero.
NxU32 NpScene::getNbImplicitMeshes() const
	{
	NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
		"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x28d, 0,
		"NxScene::getNbImplicitMeshes(): Feature not available!");
	return 0;
	}

// phys_fn_000410 (0x0000d930). The unavailable enumeration warns and returns
// a null array.
NxImplicitMesh** NpScene::getImplicitMeshes()
	{
	NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
		"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x293, 0,
		"NxScene::getImplicitMeshes(): Feature not available!");
	return 0;
	}

// phys_fn_000390 (0x0000d600): warn once, then forward the run-finished fence
// to the same checkResults virtual slot used by the public replacement API.
bool NpScene::wait(NxStandardFences, bool block)
	{
	if(!gWaitWarningEmitted)
		{
		gWaitWarningEmitted = true;
		NxFoundation::FoundationSDK::getInstance().error(NXE_DB_PRINT,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x204, 0,
			"Warning: deprecated method: \nScene::wait(). Use the new  checkResults() instead!\n\n");
		}
	return checkResults(NX_RIGID_BODY_FINISHED, block);
	}

// phys_fn_000392 (0x0000d660): a successful write-lock probe is immediately
// released; failure reports the scene as non-writable.
bool NpScene::isWritable()
	{
	void* link = mWriteLock;
	if(!nxNpSceneGuardWriteTry(link))
		return false;
	nxNpSceneGuardLeave(link);
	return true;
	}

void NpScene::simulate(NxReal elapsedTime)
	{
	if(!(elapsedTime >= 0.0f))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x21c, 0,
			"Scene::simulate: The elapsed time must be nonnegative!");
		return;
		}

	// phys_fn_000394 probes +0x10 under the scene lock, releases it before
	// writing Scene+0x544 through phys_fn_000538, then reacquires it to mark the
	// run pending and signal the work event through phys_fn_002373.
	void* readLink = mReadLock;
	nxNpSceneGuardEnter(readLink);
	const bool alreadyRunning = mFlag != 0;
	nxNpSceneGuardLeave(readLink);
	if(alreadyRunning)
		return;

	mScene->at<NxReal>(0x544) = elapsedTime;

	nxNpSceneGuardEnter(readLink);
	mFlag = 1;
	::SetEvent(*reinterpret_cast<HANDLE*>(mLockB));
	nxNpSceneGuardLeave(readLink);
	}

// (unimplemented) checkResults
bool NpScene::checkResults(NxSimulationStatus status, bool block )
	{
	if(!(static_cast<NxU32>(status) & 1))
		return true;
	if(!mCondition)
		return false;
	HANDLE done = *reinterpret_cast<HANDLE*>(
		static_cast<unsigned char*>(mCondition) + 12);
	return ::WaitForSingleObject(done, block ? INFINITE : 0) == WAIT_OBJECT_0;
	}

// phys_fn_000398 fetch-side path: wait for the worker, dispatch queued trigger/break/contact reports, refresh body gravity/snapshots, then clear the completion event.
bool NpScene::fetchResults(NxSimulationStatus status, bool block )
	{
	if(!checkResults(status, block))
		return false;
	if(mScene)
		{
		nxNpSceneGuardEnter(mReadLock);
		mScene->processSimulationCallbacks();
		mScene->finishSimulation();
		nxNpSceneGuardLeave(mReadLock);
		}
	if(mFlag)
		{
		mFlag = 0;
		::ResetEvent(*reinterpret_cast<HANDLE*>(
			static_cast<unsigned char*>(mCondition) + 12));
		}
	return true;
	}


// phys_fn_000289 (0x0000c400, 84 B): try the write lock, forward the three
// gravity words to Scene+0x520, then release the same lock link.
void NpScene::setGravity(const NxVec3& gravity)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x5b, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	void* link = mWriteLock;
	mScene->setGravity(gravity);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000297's shape: the write lock, the forward, the release.
NxJoint* NpScene::createJoint(const NxJointDesc& desc)
	{
	if(!mWriteLock || !nxLockTryLock(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x78, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return 0;
		}

	NxJoint* joint = mScene->createJoint(desc);

	nxLockUnlock(mWriteLock);
	return joint;
	}

// nxSceneDeadlockReport is defined in Scene.cpp, beside the other scene diagnostics.
