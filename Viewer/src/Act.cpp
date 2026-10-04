/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

#define NOMINMAX				//suppress windows' global min,max macros.
#ifdef _DEBUG
#ifdef WIN32
#include <crtdbg.h>				//debug heap
#endif
#endif

#include <iostream>
#include <fstream>
#include <assert.h>
#include "ViewerPlatform.h"

#include "ODBlock.h"
#include "SysTime.h"
#include "gltx.h"

#include "Act.h"
#include "PhysicsDemo.h"
#include "ActRecorder.h"

#include "ActActor.h"
#include "ActJoint.h"
#include "ActEffector.h"
#include "ViewerGraphicsContext.h"
#include "CollisionMeshAdapter.h"
#include "DebugRenderer.h"
#include "ShapeVis.h"			//just for XML demos

//most of these are defined in corpusDemoGraphic.cpp
extern void appRenderTargSized(unsigned ww,unsigned hh);
extern void Fatal(const char * m);
extern second TimeFracture;
extern second TimeColldet;
extern float physicsDt;
extern float timeToStart;
bool staticsChanged = false;	//when a static tetra shape fractures we have to readd the static stuff into the grid.
static DebugRenderer gDebugRenderer;
Act * Act::gact = 0;
/*-------------------------\
|	Foundation   Variables
\-------------------------*/
unsigned MyAllocator::numAllocs = 0;
unsigned MyAllocator::numFrees = 0;

bool gDump=true;

static int  gDumpCount=1;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void * MyAllocator::malloc(NxU32 size)
	{
	numAllocs ++;
	return ::malloc(size);
	}

void * MyAllocator::mallocDEBUG(NxU32 size, const char *fileName, int line)
	{
	numAllocs ++;
#ifdef _DEBUG
	#ifdef WIN32
	return ::_malloc_dbg(size, _NORMAL_BLOCK, fileName, line);
	#elif LINUX
	return ::malloc(size);
	#endif
#else
	return ::malloc(size);
#endif
	}

void * MyAllocator::realloc(void * memory, NxU32 size)
	{
	return ::realloc(memory,size);
	}

void MyAllocator::free(void * memory)
	{
	numFrees ++;
	::free(memory);
	}

class MyErrorStream : public NxUserOutputStream
	{
	public:
	void reportError(NxErrorCode e, const char * message, const char *file, int line)
		{
		std::cout << file << '(' << line << ") : ";
		switch (e)
			{
			case NXE_INVALID_PARAMETER:
				std::cout << "invalid parameter";
				break;
			case NXE_INVALID_OPERATION:
				std::cout << "invalid operation";
				break;
			case NXE_OUT_OF_MEMORY:
				std::cout << "out of memory";
				break;
			case NXE_DB_INFO:
				std::cout << "info";
				break;
			case NXE_DB_WARNING:
				std::cout << "warning";
				break;
			default:
				std::cout << "unknown error";
			}

		std::cout << " : " << message << std::endl;

		//this is to retain previous behavior of SDK with exception handling:
		if (e < 100)
			throw "SDK error";	
		}

	NxAssertResponse reportAssertViolation(const char * message, const char *file, int line)
		{
		std::cout << "access violation : " << message << " (" << file << " line " << line << ')' << std::endl;
#ifdef WIN32
		switch (MessageBox(0, message, "AssertViolation, see console for details.", MB_ABORTRETRYIGNORE))
			{
			case IDRETRY:
				return NX_AR_CONTINUE;
			case IDIGNORE:
				return NX_AR_IGNORE;
			case IDABORT:
			default:
				return NX_AR_BREAKPOINT;
			}
#elif LINUX
	assert(0);
#endif
		}

	void print(const char * message)
		{
		std::cout << message;
		}

	} 
myErrorStream;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*-------------------------\
|	Act
\-------------------------*/

Act::Act(const char * fname, PhysicsDemo * pd):
	physicsSDK(NULL),
	scene(NULL),
	doCollisions(true),
	haveInvisibleActors(false),
	container	(pd),
	pickedActor	(0),
	dragForce	(50),
	dragging	(0),
	frameNumber	(0),
	recorder	(0)
	{
	params[FRACT_COLLCANCEL] = 1.0f;
	params[FRACT_DELMINTIME] = 1.0f;
	params[FRACT_DELMAXTIME] = 5.0f;
	params[FRACT_CDTRIGS] = 0;
	params[FRACT_STAYTRIGS] = 0;
	params[COLL_VETO_JOINTED] = 1.0f;
	params[IGNORE_MASS] = 0.0f;

	gact = this; 

  gDump      = true;

	actorList = 0;
	jointList = 0;
	effectorList = 0;

	physicsSDK = NxCreatePhysicsSDK(NX_PHYSICS_SDK_VERSION, &myAllocator, &myErrorStream);
	if(!physicsSDK)
		Fatal("Physics SDK failed to initialize! Wrong DLL version?");

	unsigned i;
	script = NULL;
	fractureOK = true;
	breakingJointsOK = true;
	maxStretch = 1;
	gridResolution = 15;

	for (i=0; i<3; i++)
		lightObj[i] = 0;

	timeRemaining = 1e10;
	timeToStart = 0;
	physicsDt	= 0.02f;

	NxSceneDesc sceneDesc;

	Vec3 t;

	//load stuff from file
	std::ifstream myFile(fname);

	if (!myFile.is_open()) 
		Fatal("Can't open demo script file!");

	script = ODBlock::loadScript(myFile); 
	if (!script) Fatal(ODBlock::lastError);


	ODBlock * pblock = script->getBlock("Parameters");
	if (pblock)
		loadParameters(pblock);


	int recSetting = 0;
	if (script->getBlockInt("recorder", &recSetting))
		if (recSetting)
			{
			recorder = new ActRecorder((recSetting == 1), frameNumber);
			if (!recorder->getStatus())
				{
				printf("Warning: recorder failure!\n");
				delete recorder;
				recorder = 0;
				}
			}

	NxVec3 grav;
	if (script->getBlockFloats("gravity",&t.x,3))
		grav.set(t.x, t.y, t.z);
	else
		grav.set(0,-9.8f,0); //default is to have gravity

	sceneDesc.gravity = grav;

	if(!script->getBlockFloat("simStepSize", &physicsDt))
		{
		if(!script->getBlockFloat("timeStep", &physicsDt))
			{
			// Try with frequency instead
			if(script->getBlockFloat("frequency", &physicsDt))
				physicsDt = 1.0f / physicsDt;
			}
		}

	if (physicsDt <= 0.0f || physicsDt >= 1.0f)
		{
		physicsDt = 0.02f;
		printf("Warning: simStepSize out of range; ignored.");
		}

	script->getBlockFloat("PhysDuration",&(timeRemaining));

	script->getBlockFloat("PhysStart",&(timeToStart));
	script->getBlockFloat("DragForce",&dragForce);
	script->getBlockFloat("DragStretch",&maxStretch);
	if (script->getBlockFloat("fieldOfView",&ViewGC::fov))
		appRenderTargSized(0,0);

	ODBlock * textBlock= script->getBlock("Text");
	if (textBlock)
		textSpooler.load(textBlock);

//	sceneDesc.broadPhase = NX_BROADPHASE_FULL;
	sceneDesc.broadPhase = NX_BROADPHASE_COHERENT;

	//legacy way to turn off colldet:
		{
	int i = 1;
	script->getBlockInt("collDet", &i);
	doCollisions = (i != 0);
		}

	// Create new scene
	sceneDesc.collisionDetection = doCollisions;
	sceneDesc.userTriggerReport = this;
	sceneDesc.userNotify = this;
	sceneDesc.timeStepMethod = NX_TIMESTEP_VARIABLE;
	scene = physicsSDK->createScene(sceneDesc);
  gDump = true;

	if (!script->getBlock("noGroundPlane"))
		{
		NxActorDesc actorDesc;

		NxMaterial actorMaterial;
		actorMaterial.restitution		= 0.0f;
		actorMaterial.staticFriction	= 0.5f;
		actorMaterial.dynamicFriction	= 0.5f;

		static const char groundPlaneName[] = "viewer ground plane";
		NxPlaneShapeDesc planeDesc;
		planeDesc.materialIndex = physicsSDK->addMaterial(actorMaterial);
		actorDesc.shapes.pushBack(&planeDesc);
		scene->createActor(actorDesc);
		}

	//SS: TODO document.
	ODBlock * controlBlock = script->getBlock("Controls");
	if (controlBlock)
		{
		controlBlock->reset();
		while(controlBlock->moreSubBlocks())
			container->createActController( *controlBlock->nextSubBlock());
			}

	ODBlock * bodyBlock= script->getBlock("Bodies");
	unsigned actNum = 0;
	if (bodyBlock)
		{
		bodyBlock->reset();
		while(bodyBlock->moreSubBlocks())
			createActor(actNum++, bodyBlock->nextSubBlock());
		}

	ODBlock * jointBlock= script->getBlock("Joints");
	unsigned jo = 0;
	if (jointBlock)
		{
		jointBlock->reset();
		while(jointBlock->moreSubBlocks())
			createJoint(jo++,jointBlock->nextSubBlock());
		}

	ODBlock * effectorBlock = script->getBlock("Effectors");
	unsigned effNum = 0;
	if (effectorBlock)
		{
		effectorBlock->reset();
		while(effectorBlock->moreSubBlocks())
			createEffector(effNum, effectorBlock->nextSubBlock());
		}

	ODBlock * lightBlock= script->getBlock("Lights");
	if (lightBlock)
		{
		SceneGraph::Scene & s = SceneGraph::Scene::getInstance();
		Vec3 light;
		if (lightBlock->getBlockFloats("1",&light.x,3))
			{
			lightObj[0] = new SceneGraph::Object();
			//s.addObject(*lightObj[0]);
			lightObj[0]->setPosition(&light);
			lightObj[0]->bHideModel = true;
			s.setLight(0,lightObj[0]);
			}
		if (lightBlock->getBlockFloats("2",&light.x,3))
			{
			lightObj[1] = new SceneGraph::Object();
			//s.addObject(*lightObj[1]);
			lightObj[1]->setPosition(&light);
			lightObj[1]->bHideModel = true;
			s.setLight(1,lightObj[1]);
			}
		if (lightBlock->getBlockFloats("3",&light.x,3))
			{
			lightObj[2] = new SceneGraph::Object();
			//s.addObject(*lightObj[2]);
			lightObj[2]->setPosition(&light);
			lightObj[2]->bHideModel = true;
			s.setLight(2,lightObj[2]);
			}
		}

	ODBlock * camBlock= script->getBlock("Camera");
	if (camBlock)
		{
		Quat q;
		Vec3 t;
		if (camBlock->getBlockFloats("angles", &ViewGC::cameraEulers.x, 3))
			{
			if (ViewGC::cameraEulers.z < -90)
				ViewGC::cameraEulers.z = -90;
			if (ViewGC::cameraEulers.z > 90)
				ViewGC::cameraEulers.z = 90;

			q.fromEulerAngles(0, Quat::deg_to_rad(ViewGC::cameraEulers.z), Quat::deg_to_rad(ViewGC::cameraEulers.y));  //this doesn't work when looking downward either.
			ViewGC::camera->setOrientation(&q);
			}
		if (camBlock->getBlockFloats("position",&t.x,3))
			ViewGC::camera->setPosition(&t);
		}

	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Act::~Act()
	{
	sdelete(recorder);

	ActEffector * teffector, * effector = effectorList;
	while (effector)
		{
		teffector = effector;
		effector = effector->next;
		delete teffector;
		}
	effectorList = 0;

	ActJoint	* tjoint, * joint = jointList;
	while (joint)
		{
		tjoint = joint;
		joint = joint->next;
		delete tjoint;
		}
	jointList = 0;

	ActActor	* tactor, * actor = actorList;
	while (actor)
		{
		tactor = actor;
		actor = actor->next;
		delete tactor;
		}
	actorList = 0;

		{
	for (unsigned i=0; i<3; i++)
		if (lightObj[i])
			{
			delete lightObj[i];
			lightObj[i] = NULL;
			}
		}

	//free all the mesh infos after the shapes have been freed:
	SceneGraph::Scene & s = SceneGraph::Scene::getInstance();
	for (unsigned i = 0; i < s.meshList.size(); i++)
		{
		SceneGraph::Mesh * mesh = s.meshList[i];
		if (mesh)
			{
			for (SceneGraph::Mesh::SubMeshList::iterator i = mesh->subMeshList.begin(); i != mesh->subMeshList.end(); i++)
				{
				SceneGraph::SubMesh * s = *i;
				if (s->userData)
					{
					CollisionMeshAdapter * adapter = (CollisionMeshAdapter *)s->userData;
					adapter->releaseTriangleMesh(*physicsSDK);
					delete adapter;	//releases triangle mesh
					s->userData = 0;
					}
				}
			}
		}

	physicsSDK->releaseScene(*scene);
	physicsSDK->release();
	physicsSDK = NULL;

	sdelete(script);
	container = 0;
	gact = 0;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//bool gSceneLock = false;


	//!	This function starts recording the number of cycles elapsed.
	//!	\param		val		[out] address of a 32 bits value where the system should store the result.
	//!	\see		EndProfile
	//!	\see		InitProfiler
	__forceinline void	StartProfile(int& val)
	{
		__asm{
			cpuid
			rdtsc
			mov		ebx, val
			mov		[ebx], eax
		}
	}

	//!	This function ends recording the number of cycles elapsed.
	//!	\param		val		[out] address to store the number of cycles elapsed since the last StartProfile.
	//!	\see		StartProfile
	//!	\see		InitProfiler
	__forceinline void	EndProfile(int& val)
	{
		__asm{
			cpuid
			rdtsc
			mov		ebx, val
			sub		eax, [ebx]
			mov		[ebx], eax
		}
//		val-=GetBaseTime();
	}

bool Act::tick(float sec)
	{
	textSpooler.tick(sec);
	
	if (timeToStart > 0)
		{
		timeToStart -=sec;
		return false;
		}
	else
		{
		tickDrag();			//user interaction --> external forces.

    if ( gDump )
    {
      char scratch[512];
      sprintf(scratch,"dump%d", gDumpCount );
      physicsSDK->coreDump(scratch, false );
      gDumpCount++;
      gDump = false;
    }

	scene->simulate(sec);
	scene->flushStream();
	scene->fetchResults(NX_RIGID_BODY_FINISHED, true);

	//scene->runFor(sec, 0.0f, 0, NX_TIMESTEP_VARIABLE);	// We already do the timestep logic in the viewer...


		//
/*		if(1)
			{
			// PT: test !
			NxU32 nbActors = scene->getNbActors();
			const NxActor** actors scene->getActors();
			while(nbActors--)
				{

				}
			}
*/


//gSceneLock = false;

		tickBehaviors(sec);
//int Time;
//StartProfile(Time);
		updateGraphicsPoses();
//EndProfile(Time);
//printf("%d\n", Time);
		timeRemaining-=sec;
		if (timeRemaining <= 0)
			return true;
		else
			return false;
		}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::tickBehaviors(NxReal dt)
	{
	for (unsigned i = 0; i< behaviorActors.size(); i ++)
		behaviorActors[i]->tickBehaviors(dt);
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::registerBehavior(ActActor * a, bool add)
	{
	if (add)
		{
		behaviorActors.pushBack(a);
		}
	else
		for (unsigned i = 0; i< behaviorActors.size(); i ++)
			if (behaviorActors[i] == a)
				{
				behaviorActors.replaceWithLast(i);
				return;
				}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Act::onJointBreak(NxReal breakingForce, NxJoint & brokenJoint)
	{
	//delete the joint and actJoint!
	printf("joint broken!\n");
	ActJoint * j = (ActJoint *)(brokenJoint.userData);
	NX_ASSERT(j);
	NX_ASSERT(j->getJoint()->getState() == NX_JS_BROKEN);
	// The SDK releases the joint after this callback returns true. Detach it
	// before deleting the viewer wrapper so its destructor cannot release it.
	j->detachBrokenJoint();
	brokenJoint.userData = 0;
	deleteJoint(*j);
	return true;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::onTrigger(NxShape & triggerShape, NxShape & otherShape, NxTriggerFlag status)
	{

	ActActor * triggerActor = (ActActor *)(triggerShape.getActor().userData);
	ActActor * otherActor = (ActActor *)(otherShape.getActor().userData);

	const char* triggerName = triggerActor ? triggerActor->getName() : NULL;
	const char* otherName = otherActor ? otherActor->getName() : NULL;

	// Test name manager
//	triggerName = triggerShape.getName();
//	otherName = otherShape.getName();

	switch(status)
		{
		case NX_TRIGGER_ON_ENTER:
			printf("%s entered trigger %s\n", otherName, triggerName);
			break;
		case NX_TRIGGER_ON_STAY:
			printf("%s touches trigger %s\n", otherName, triggerName);
			break;
		case NX_TRIGGER_ON_LEAVE:
			printf("%s left trigger %s\n", otherName, triggerName);
			break;
		}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::updateGraphicsPoses()
	{
	ActActor	* i = actorList;
	while (i)
		{
		i->graphicsUpdate();
		i = i->next;
		}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::render()
	{
	glPushMatrix();		//add it under the world object so we can rotate it
	glMultMatrixf(ViewGC::world->model2World.M16);	
	if (ViewGC::world->scale != 0)
		glScalef(ViewGC::world->scale,ViewGC::world->scale,ViewGC::world->scale);

	if (haveInvisibleActors)
		{
		ActActor	* i = actorList;
		while (i)
			{
			if (!i->getVisObject())
				{
				NxActor * actor = i->getActor();
				if (actor)
					{
					NxShape ** shapes = actor->getShapes();
					NxU32 nShapes = actor->getNbShapes();
					while (nShapes--)
						ShapeVis::renderShape(shapes[nShapes]);
					}
				}
			i = i->next;
			}
		}
		
	glPopMatrix();

	// Render debug data
//NX_ASSERT(!gSceneLock);
	if(scene)		scene->visualize();
	if(physicsSDK)	physicsSDK->visualize(gDebugRenderer);

	textSpooler.render();
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::deleteActor(ActActor & ub)	//only call outside of physics callbacks.
	{
//gSceneLock = true;
	ActActor * a = &ub;
	ActActor * prev = a->previous;
	if (prev)
		{
		assert(a->previous->next == a);
		a->previous->next = a->next;
		}
	else //a is the root.
		actorList = a->next;

	if (a->next)
		a->next->previous = prev;
	a->next = 0;
	delete a;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ActActor * Act::createActor(unsigned actorNum, ODBlock * block)
	{
	ActorTemplate * actr = container->getActorTemplate(block->ident());
	if (!actr)
		{
		printf("Error: ActorTemplate not found: %s\n", block->ident());
		Fatal("Unknown ActorTemplate! (see console)"); 
		return 0; 
		}
	ActActor * a;
	bool success;
	
	NX_ASSERT(scene);
	a = new ActActor(scene);
	success = a->load(actorNum, block, actr);
	

	if (!success)
		{
		delete a;
		a = 0;
		}
	else
		addActor(*a);

	return a;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ActJoint * Act::createJoint(unsigned jointNum, ODBlock * block)
	{
	ActJoint * a = new ActJoint();
	if (!a->load(jointNum, block, scene))
		{
		delete a;
		a = 0;
		}
	else
		addJoint(*a);
	return a;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::createEffector(unsigned effNum, ODBlock * block)
	{
	ActEffector * a = new ActEffector();
	if (!a->load(effNum, block, scene))
		delete a;
	else
		{
		a->previous = 0;
		a->next = effectorList;
		if (effectorList)
			{
			assert(effectorList->previous == 0);
			effectorList->previous = a;
			}
		effectorList = a;
		}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// interaction


void Act::addActor(ActActor & a)
	{
	a.previous = 0;
	a.next = actorList;
	if (actorList)
		{
		assert(actorList->previous == 0);
		actorList->previous = &a;
		}
	actorList = &a;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::addJoint(ActJoint & a)
	{
	a.previous = 0;
	a.next = jointList;
	if (jointList)
		{
		assert(jointList->previous == 0);
		jointList->previous = &a;
		}
	jointList = &a;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::deleteJoint(ActJoint & j)
	{
	ActJoint * a = &j;
	ActJoint * prev = a->previous;
	if (prev)
		{
		assert(a->previous->next == a);
		a->previous->next = a->next;
		}
	else //a is the root.
		jointList = a->next;
	if (a->next)
		a->next->previous = prev;
	a->next = 0;
	delete a;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ActActor * Act::findActor(const char * name)
	{
	ActActor	* i = actorList;
	while (i)
		{
		if (i->isCalled(name))
			return i;
		i = i->next;
		}
	return 0;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NxActor* Act::findBody(const char * name)
	{
	ActActor * a= findActor(name);
	if (a)
		return a->getActor();
	else
		return 0;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ActJoint * Act::findActJoint(const char * name)
	{
	ActJoint	* i = jointList;
	while (i)
		{
		if (i->isCalled(name))
			return i;
		i = i->next;
		}
	return 0;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::loadParameters(ODBlock * block)
	{
	/*
	Parameters
		{
		RigidBodySDK
			{
			MIN_SEPARATION_FOR_PENALTY {}
			MAX_SEPARATION_FOR_PENALTY {}
			...
			}
		CollisionSDK
			{
			MESH_MESH_LEVEL 
			MESH_BOX_LEVEL
			}
		}
	*/
	ODBlock * rb = block->getBlock("RigidBodySDK");
	float x;
	if (rb)
		{
		if (rb->getBlockFloat("PENALTY_FORCE",  &x))
			physicsSDK->setParameter(NX_PENALTY_FORCE, x);
		if (rb->getBlockFloat("MIN_SEPARATION_FOR_PENALTY",  &x))
			physicsSDK->setParameter(NX_MIN_SEPARATION_FOR_PENALTY, x);

		if (rb->getBlockFloat("SLEEP_LIN_VEL_SQUARED",  &x))
			physicsSDK->setParameter(NX_DEFAULT_SLEEP_LIN_VEL_SQUARED, x);
		if (rb->getBlockFloat("SLEEP_ANG_VEL_SQUARED",  &x))
			physicsSDK->setParameter(NX_DEFAULT_SLEEP_ANG_VEL_SQUARED, x);

		if (rb->getBlockFloat("BOUNCE_TRESHOLD",  &x))
			physicsSDK->setParameter(NX_BOUNCE_TRESHOLD, x);

		if (rb->getBlockFloat("DYN_FRICT_SCALING",  &x))
			physicsSDK->setParameter(NX_DYN_FRICT_SCALING, x);
		if (rb->getBlockFloat("STA_FRICT_SCALING",  &x))
			physicsSDK->setParameter(NX_STA_FRICT_SCALING, x);

		if (rb->getBlockFloat("MAX_ANGULAR_VELOCITY",  &x))
			physicsSDK->setParameter(NX_MAX_ANGULAR_VELOCITY, x);
		}

	rb = block->getBlock("CollisionSDK");
	if (rb)
		{
		if (rb->getBlockFloat("MESH_MESH_LEVEL",  &x))
			physicsSDK->setParameter(NX_MESH_MESH_LEVEL, x);
		if (rb->getBlockFloat("ENABLE_MESH_DEBUG",  &x))
			physicsSDK->setParameter(NX_ENABLE_MESH_DEBUG, x);
		}

	rb = block->getBlock("Viewer");
	if (rb)
		{
		if (rb->getBlockFloat("FRACT_COLLCANCEL",  &x))
			setParameter(FRACT_COLLCANCEL, x);
		if (rb->getBlockFloat("FRACT_DELMINTIME",  &x))
			setParameter(FRACT_DELMINTIME, x);
		if (rb->getBlockFloat("FRACT_DELMAXTIME",  &x))
			setParameter(FRACT_DELMAXTIME, x);
		if (rb->getBlockFloat("FRACT_CDTRIGS",  &x))
			setParameter(FRACT_CDTRIGS, x);
		if (rb->getBlockFloat("FRACT_STAYTRIGS",  &x))
			setParameter(FRACT_STAYTRIGS, x);
		if (rb->getBlockFloat("COLL_VETO_JOINTED",  &x))
			setParameter(COLL_VETO_JOINTED, x);
		if (rb->getBlockFloat("IGNORE_MASS",  &x))
			setParameter(IGNORE_MASS, x);
		}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::setParameter(NxU32 paramEnum, float paramValue)
	{
	params[paramEnum] = paramValue;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const NxVec3 Act::getWorldStab()
	{
	NxVec3 worldStab;
	const NxMat34 * xf = pickedActor->getActor2World();
	if (xf)
		xf->multiply(stabPointActorSpace, worldStab);
	return worldStab;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::toggleSDKParameter(SDKParameters param)
	{
	NxReal p;

	//turn on vis now whenever this is called.  TODO should add GUI controls to toggle this manually.
	setVisualizationScale(ViewGC::visualizationScale);	

	#define RB_PAR_CASE(x)	case x: if (physicsSDK) { p = physicsSDK->getParameter(NX_##x); physicsSDK->setParameter(NX_##x, p > 0 ? 0 : 1.0f); } break;

	switch (param)
		{
		RB_PAR_CASE(VISUALIZE_WORLD_AXES)
		RB_PAR_CASE(VISUALIZE_ACTOR_AXES)
		RB_PAR_CASE(VISUALIZE_BODY_AXES)
		RB_PAR_CASE(VISUALIZE_BODY_MASS_AXES)
		RB_PAR_CASE(VISUALIZE_BODY_LIN_VELOCITY)
		RB_PAR_CASE(VISUALIZE_BODY_ANG_VELOCITY)
		RB_PAR_CASE(VISUALIZE_BODY_LIN_MOMENTUM) 
		RB_PAR_CASE(VISUALIZE_BODY_ANG_MOMENTUM)
		RB_PAR_CASE(VISUALIZE_BODY_LIN_ACCEL)
		RB_PAR_CASE(VISUALIZE_BODY_ANG_ACCEL)
		RB_PAR_CASE(VISUALIZE_BODY_LIN_FORCE)
		RB_PAR_CASE(VISUALIZE_BODY_ANG_FORCE)
		RB_PAR_CASE(VISUALIZE_BODY_REDUCED)
		RB_PAR_CASE(VISUALIZE_BODY_JOINT_GROUPS)
		RB_PAR_CASE(VISUALIZE_BODY_CONTACT_LIST)
		RB_PAR_CASE(VISUALIZE_BODY_JOINT_LIST)
		RB_PAR_CASE(VISUALIZE_BODY_DAMPING)
		RB_PAR_CASE(VISUALIZE_BODY_SLEEP)

		RB_PAR_CASE(VISUALIZE_JOINT_LOCAL_AXES)
		RB_PAR_CASE(VISUALIZE_JOINT_WORLD_AXES)
		RB_PAR_CASE(VISUALIZE_JOINT_LIMITS)
		RB_PAR_CASE(VISUALIZE_JOINT_ERROR)
		RB_PAR_CASE(VISUALIZE_JOINT_FORCE)
		RB_PAR_CASE(VISUALIZE_JOINT_REDUCED)

		RB_PAR_CASE(VISUALIZE_CONTACT_POINT)
		RB_PAR_CASE(VISUALIZE_CONTACT_NORMAL)
		RB_PAR_CASE(VISUALIZE_CONTACT_ERROR)
		RB_PAR_CASE(VISUALIZE_CONTACT_FORCE)

		RB_PAR_CASE(VISUALIZE_COLLISION_SHAPES)
		RB_PAR_CASE(VISUALIZE_COLLISION_AXES)
		RB_PAR_CASE(VISUALIZE_COLLISION_AABBS)
		RB_PAR_CASE(VISUALIZE_COLLISION_COMPOUNDS)
		}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Act::setVisualizationScale(float scale)
	{
	if(physicsSDK)
		physicsSDK->setParameter(NX_VISUALIZATION_SCALE, scale);
	}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
