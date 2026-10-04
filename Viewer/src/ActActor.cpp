/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ViewerPlatform.h"

#include "ODBlock.h"
#include "Graphics.h"
#include "glm.h"

#include "Act.h"
#include "ActActor.h"
#include "PhysicsDemo.h"
#include "ViewerGraphicsContext.h"
#include "CollisionMeshAdapter.h"
// Descriptors

#define ACT Act::instance()
extern void Fatal(const char * m);

void RenderShape(NxShape* shape);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ActActor::ActActor(NxScene* owner_) : owner(owner_)
	{
	scale = 1.0f;
	sceneObj = NULL;

	actor = NULL;
	controller = NULL;	
	actorTemplate = NULL;

	next = previous = 0;

	ownedModel = 0;
	block = 0;
	behavior = 0;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ActActor::~ActActor()
	{
	//TODO: should only delete actors which are not referenced by Joints or Effectors, or others!!
	if (behavior)
		{
		ACT->registerBehavior(this, false);
		delete behavior;
		behavior = 0;
		}

	//0) get rid of graphics
	if (sceneObj)
		{
		//we may have created special userData stuff for the body.  If so, delete it
		ViewGC::world->deleteChild(sceneObj);
		sceneObj = 0;
		}

#ifdef INCLUDE_SOUND
	for (unsigned i=0; i<soundObjects.size(); i++)
		delete soundObjects[i];
#endif

	if(actor)
		{
		owner->releaseActor(*actor);
		actor = NULL;
		}

	if (ownedModel)
		sdelete(ownedModel->mesh);

	sdelete(ownedModel);
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
creates graphics object, and NxActor using NxBodyDesc, NxActorDesc 
*/
bool ActActor::load(unsigned actorNum, ODBlock * block, ActorTemplate * actr)
	{
	this->block = block;
	actorTemplate = actr;
	//graphical rep
	sceneObj = ACT->container->instanceActorTemplateGraphics(*actr, *ViewGC::world);

	NxActorDesc actorDesc;
	NxBodyDesc bodyDesc;
	NxMaterial actorMaterial;

	//Note: we read in single precision stuff, so we need intermediate vars.
	float mass;
	Vec3 t;
	Vec3 position3(0,0,0);
	Quat q;
	NxVec3 rv;
	NxQuat rq;
	const char * name = 0;

	block->getBlockString("name", &name);

	if (!block->getBlockFloat("mass",&mass) || mass < 0)
		mass = 1;
	if (ACT->params[Act::IGNORE_MASS] != 0)
		mass = 0;

	if (mass != 0)
		{	
		actorDesc.body = &bodyDesc;
		bodyDesc.mass = mass;
		}

	block->getBlockFloat("scale",&scale);
	if (sceneObj)
		sceneObj->scale = scale;


	if (block->getBlockFloats("position",&position3.x,3))
		actorDesc.globalPose.t.set(position3.x,position3.y,position3.z);

	//rotation:
	if (block->getBlockFloats("orientation",&q.w,4))
		{
		rq.setWXYZ(q.w,q.x,q.y,q.z);
		actorDesc.globalPose.M.fromQuat(rq);
		}
	else if (block->getBlockFloats("angles",&t.x,3))
		{
		Vec3 vX(1.0f,0.0f,0.0f),vY(0.0f,1.0f,0.0f),vZ(0.0f, 0.0f,1.0f);
		Quat RotX((float)t.x,vX);
		Quat RotY((float)t.y,vY);
		Quat RotZ((float)t.z,vZ);
		q = RotZ * RotY * RotX;
		q.Normalize();
		rq.setWXYZ(q.w,q.x,q.y,q.z);
		actorDesc.globalPose.M.fromQuat(rq);
		}

	if(actorDesc.body)
		{
		float inertia[6];

		if (ACT->params[Act::IGNORE_MASS])
			{
			//mass and inertia tensors of all bodies set to identity
			actorDesc.density = 0;
			bodyDesc.mass = 1;
			bodyDesc.massSpaceInertia.set(1,1,1);
			}
		else if (block->getBlockFloats("inertia",inertia,6)) //m,I
			{
			NxMat33 inertiaTensorDesc;
			inertiaTensorDesc(0,0)	= inertia[0];
			inertiaTensorDesc(0,1)	= inertiaTensorDesc(1,0)	= inertia[1];
			inertiaTensorDesc(0,2)	= inertiaTensorDesc(2,0)	= inertia[2];
			inertiaTensorDesc(1,1)	= inertia[3];
			inertiaTensorDesc(1,2)	= inertiaTensorDesc(2,1)	= inertia[4];
			inertiaTensorDesc(2,2)	= inertia[5];
			bodyDesc.massSpaceInertia.set(inertiaTensorDesc(0,0), inertiaTensorDesc(1,1), inertiaTensorDesc(2,2));

			//bodyDesc.mass is already set.
			}
		else if (block->getBlockFloat("density", &actorDesc.density)) //d,S
			{
			//use density.
			bodyDesc.mass = 0;
			bodyDesc.massSpaceInertia.zero();
			}
		else	//m,S
			{
			//use mass and shapes to compute a tensor.
			bodyDesc.massSpaceInertia.zero();
			}

		if (block->getBlockFloats("angularVelocity",&t.x,3))//used to be "angMomentum"
			{
			rv.set(t.x,t.y,t.z);
			bodyDesc.angularVelocity = rv;
			}

		if (block->getBlockFloats("linearVelocity",&t.x,3))	//used to be "momentum"
			{
			rv.set(t.x,t.y,t.z);
			bodyDesc.linearVelocity = rv;
			}

		float maxAV;
		if (block->getBlockFloat("maxAngVel",&maxAV))
			{
			bodyDesc.maxAngularVelocity = maxAV;
			}

		int sleepCount = 0;
		if (block->getBlockInt("wake",&sleepCount))
			{
			bodyDesc.wakeUpCounter = sleepCount;
			}

		if (block->getBlockFloat("linDamping", &t.x))
			{
			bodyDesc.linearDamping = t.x;
			}

		if (block->getBlockFloat("angDamping", &t.x))
			{
			bodyDesc.angularDamping = t.x;
			}
		}

	if (!block->getBlockFloat("restitution",		&actorMaterial.restitution))
		actorMaterial.restitution		= 0.0f;
	if (!block->getBlockFloat("friction",		&actorMaterial.dynamicFriction))
		actorMaterial.staticFriction	= 0.5f;
	if (!block->getBlockFloat("staticFriction",	&actorMaterial.staticFriction))
		actorMaterial.dynamicFriction	= 0.5f;

	if (!block->getBlockFloat("frictionV",		&actorMaterial.dynamicFrictionV))				;
	else if (!block->getBlockFloat("staticFrictionV",	&actorMaterial.staticFrictionV))		;
	else if (!block->getBlockFloats("dirOfAnisotropy",	&actorMaterial.dirOfAnisotropy.x, 3))	;
	else //if we get here we have read all 3 aniso friction params
		actorMaterial.flags |=  NX_MF_ANISOTROPIC;

	// Triggers
	// PT: wrong place but kept for compatibility
	int trigger=0;
	if (block->getBlockInt("trigger", &trigger))
		{
		// We found a trigger
		}

	//make colldet shapes:
	ODBlock * collBlock = actr->block->getBlock("Collision");
	if (collBlock)	
		{
		createShapeForBody(collBlock, actorDesc.shapes, actorDesc.globalPose, actorDesc.body!=0);
		if (!sceneObj)
			ACT->haveInvisibleActors = true;		//this is a hack to make nice drawings for XML demos
		}

#ifdef INCLUDE_SOUND
	ODBlock * soundBlock = actr->block->getBlock("Sound");
	if (soundBlock && Sound::Manager::isRunning())
		{
		soundBlock->reset();
		while(soundBlock->moreSubBlocks())
			{
			ODBlock * sourceBlock = soundBlock->nextSubBlock();
			const char * waveFile = 0;
			if (!sourceBlock->getBlockString("wave", &waveFile)) continue;
			const int soundBuffer = Sound::Manager::addBuffer(waveFile);
			if (soundBuffer < 0) continue;
			soundObjects.pushBack(new Sound::Source());
			Sound::Source * source = soundObjects.back();

			source->position.set(0.0f,0.0f,0.0f);
			source->velocity.set(0.0f,0.0f,0.0f);

			sourceBlock->getBlockFloat( "gainDefault", &source->gainDefault);
			sourceBlock->getBlockFloat( "pitchDefault", &source->pitchDefault);
			
			source->gain = source->gainDefault;
			source->pitch = source->pitchDefault;

			source->loop = true;
			source->updateAll();

			source->queueBuffer(static_cast<unsigned>(soundBuffer));
			source->play();
			}
		}
#endif

	bool suppressDefaultShape = (block->getBlock("suppressDefaultShape") != NULL);


	// no default shape if no graphics, for XML file compatibility.
	if(!actorDesc.shapes.size() && !suppressDefaultShape && sceneObj)
		{		
		//default to AABB in local space		
		printf("Warning, no shape found for body.  Using default box.\n");
		NxBoxShapeDesc* boxDesc = new NxBoxShapeDesc;
		if (!block->getBlockFloats("dimensions",&t.x,3))
			{
			//NOTE: this only computes a good box for single models, not entire groups.  
			//If its a group, the unit cube is returned, therefore dimensions should be specified.
			if (sceneObj)
				sceneObj->getBoundingBox(t);

			t*=0.5f;	//halve it -- we need radius, this gives diameter.
			}

		boxDesc->dimensions.set(t.x, t.y, t.z);
//		if(!actorDesc.body)
//			boxDesc->group = NX_GROUP_STATIC;

		actorDesc.shapes.pushBack(boxDesc);
		}

	// Before creating the actor, replicate the trigger flags in all actor shapes
	// PT: only kept for compatibility
	if(trigger)
		{
		NxU32 nbShapes = actorDesc.shapes.size();
		NxShapeDesc*const* desc = actorDesc.shapes.begin();
		while(nbShapes--)
			(*desc++)->shapeFlags |= NX_TRIGGER_ENABLE;
		}

	// Create the new actor
	if (actorDesc.body || actorDesc.shapes.size())	//otherwise the actor is pointless
		{
		actorDesc.name = name;
		//create the actor's material
		NxMaterialIndex actorMaterialIndex = ACT->physicsSDK->addMaterial(actorMaterial);
		//new: assign the material to all the shapes
		NxU32 nbShapes = actorDesc.shapes.size();
		NxShapeDesc** shapes = actorDesc.shapes.begin();
		while(nbShapes--)
			{
			NxShapeDesc* currentDesc = *shapes++;
			currentDesc->materialIndex = actorMaterialIndex;
			}

		actor = owner->createActor(actorDesc);
		actor->userData = this;
		}

	// Then delete shape descriptors allocated elsewhere
	NxU32 nbShapes = actorDesc.shapes.size();
	NxShapeDesc** shapes = actorDesc.shapes.begin();
	while(nbShapes--)
		{
		NxShapeDesc* currentDesc = *shapes++;
		NX_DELETE_SINGLE(currentDesc);
		}

	//com shift:	//TODO: use the desc!!
	//center of mass displacement in Actor space. (the default is zero)
	if (block->getBlockFloats("centerOfMass",&t.x,3))	
		actor->setCMassOffsetLocalPosition(NxVec3(t.x, t.y, t.z));

	//behaviors
	ODBlock * behaviorBlock = block->getBlock("Behaviors");
	if (behaviorBlock)
		loadBehaviors(behaviorBlock);

	//controller:

	ODBlock * specialBlock= block->getBlock("Control");
	if (specialBlock)
		controller = ACT->container->createActController(*specialBlock);
	else
		controller = NULL;
	//set the graphic pose because we won't be continuously updating it
	if(!actorDesc.body)
		updateSceneObjPose(&actorDesc.globalPose);

	return true;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ActActor::loadBehaviors(ODBlock * behBlock)
	{
	NX_ASSERT(actor);
	behavior = new Behavior;
	behavior->typeFlags = 0;
	behavior->posParam = 0;
	behavior->rotParam = 0;
	behavior->posSpeed = 1.0f;
	behavior->rotSpeed = 1.0f;
	ACT->registerBehavior(this, true);

	bool needKin = false;

	ODBlock * translBlock = behBlock->getBlock("translation");
	if (translBlock)
		{
		if (translBlock->getBlockFloats("destination", &(behavior->position[1].x), 3))
			{
			if (translBlock->getBlock("COM"))
				{
				if (actor->isDynamic())
					{
					actor->getCMassLocalPosition(behavior->position[0]);
					behavior->typeFlags |= Behavior::COM_TRANSLATION;
					}
				}
			else
				{
				actor->getGlobalPosition(behavior->position[0]);
				behavior->typeFlags |= Behavior::TRANSLATION;
				needKin = true;
				}
			}
		translBlock->getBlockFloat("speed", &behavior->posSpeed);

		}
	ODBlock * rotationBlock = behBlock->getBlock("rotation");
	if (rotationBlock)
		{
		NxVec3 t;
		if (rotationBlock->getBlockFloats("destinationAngles", &t.x, 3))
			{
			NxVec3 vX(1.0f,0.0f,0.0f),vY(0.0f,1.0f,0.0f),vZ(0.0f, 0.0f,1.0f);
			NxQuat RotX(t.x,vX);
			NxQuat RotY(t.y,vY);
			behavior->orient[1].fromAngleAxis(t.z,vZ);
			behavior->orient[1] *= RotY;
			behavior->orient[1] *= RotX;
			behavior->orient[1].normalize();
			behavior->typeFlags |= Behavior::ROTATION;
			needKin = true;
			actor->getGlobalOrientationQuat(behavior->orient[0]);
			}
		rotationBlock->getBlockFloat("speed", &behavior->rotSpeed);



		}

	if (needKin)
		actor->raiseBodyFlag(NX_BF_KINEMATIC);	//make the actor kinematic

	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ActActor::tickBehaviors(NxReal dt)
	{
	NX_ASSERT(behavior);	//otherwise this should not be called
	NX_ASSERT(actor);
	NxMat34 newPose;

	behavior->posParam += dt * behavior->posSpeed;
	if (behavior->posParam > 1.0f)
		{
		behavior->posParam = 0.999f;
		behavior->posSpeed = - behavior->posSpeed;
		}
	else if (behavior->posParam < 0.0f)
		{
		behavior->posParam = 0.001f;
		behavior->posSpeed = - behavior->posSpeed;
		}


	behavior->rotParam += dt * behavior->rotSpeed;
	//behavior->rotParam = NxMath::mod(behavior->rotParam, 1.0f);//wrap to [0,1]

	if (behavior->rotParam > 1.0f)
		{
		behavior->rotParam = 0.999f;
		behavior->rotSpeed = - behavior->rotSpeed;
		}
	else if (behavior->rotParam < 0.0f)
		{
		behavior->rotParam = 0.001f;
		behavior->rotSpeed = - behavior->rotSpeed;
		}


	if (behavior->typeFlags & Behavior::TRANSLATION || behavior->typeFlags & Behavior::ROTATION)
		{
		actor->getGlobalPose(newPose);
	
		if (behavior->typeFlags & Behavior::TRANSLATION)
			{
			newPose.t = behavior->posParam * behavior->position[1] + (1.0f - behavior->posParam) * behavior->position[0];
/*
//			newPose.t.x += 0.02f;
//			newPose.t.x += 0.1f;
//			newPose.t.x += 1.0f;
			newPose.t.x += 0.01f;
*/
			}

		if (behavior->typeFlags & Behavior::ROTATION)
			{
			//TODO
			NxQuat t;
			t.slerp(behavior->rotParam, behavior->orient[0], behavior->orient[1]);
			newPose.M.fromQuat(t);
			}

		//actor->setGlobalPose(newPose);	//this sort of works too but will be outlawed later --- static motion 
		actor->moveGlobalPose(newPose);		//kinematic motion
		}

	
	if (behavior->typeFlags & Behavior::COM_TRANSLATION)
		{
		NxVec3 t = behavior->posParam * behavior->position[1] + (1.0f - behavior->posParam) * behavior->position[0];
		actor->setCMassOffsetLocalPosition(t);
		}

	//non-dynamics need to be updated now, as they don't get updated automatically.
	if(!actor->isDynamic())
		updateSceneObjPose(getActor2World());

	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ActActor::updateSceneObjPose(const NxMat34	* mat)
	{
	if (sceneObj && mat)
		{
		mat->t.get(&(sceneObj->position.x));
		mat->M.getColumnMajorStride4(&(sceneObj->orientation.M44[0][0]));

		sceneObj->orientation.M44[0][3] =
		sceneObj->orientation.M44[1][3] =
		sceneObj->orientation.M44[2][3] =

		sceneObj->orientation.M44[3][0] =
		sceneObj->orientation.M44[3][1] =
		sceneObj->orientation.M44[3][2] = 0;

		sceneObj->orientation.M44[3][3] = 1;
		
		sceneObj->updateMatrix();			
		}

#ifdef INCLUDE_SOUND
	for (unsigned i=0; i<soundObjects.size(); i++)
		{
		if (mat) soundObjects[i]->position = mat->t;
		else if (sceneObj) soundObjects[i]->position.set(&sceneObj->position.x);
		soundObjects[i]->velocity = actor && actor->isDynamic() ? actor->getLinearVelocity() : NxVec3(0,0,0);
		soundObjects[i]->updateAll();
		}
#endif
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ActActor::loadBoxData(ODBlock* shapeBlock, NxBoxShapeDesc* desc, const NxMat34& pose)
	{
	// Default values
	Vec3 dims;		dims.Set(1.0f,1.0f,1.0f);
	Vec3 pos;		pos.Zero();
	Quat orient;	orient.Zero();
	Vec3 temp;

	// Fetch data
	shapeBlock->getBlockFloats("position",&pos.x, 3);
	if (!shapeBlock->getBlockFloats("orientation",&orient.w, 4))
		if (shapeBlock->getBlockFloats("angles", &temp.x, 3))
			{
			Vec3 vX(1.0f,0.0f,0.0f),vY(0.0f,1.0f,0.0f),vZ(0.0f, 0.0f,1.0f);
			Quat RotX((float)temp.x,vX);
			Quat RotY((float)temp.y,vY);
			Quat RotZ((float)temp.z,vZ);
			orient = RotZ * RotY * RotX;
			orient.Normalize();
			}

	shapeBlock->getBlockFloats("dimensions",&dims.x, 3);

	NxQuat q;
	NxMat33 mat;
	q.setWXYZ(orient.w, orient.x, orient.y, orient.z);
	mat.fromQuat(q);

	// Fill descriptor
	desc->dimensions	= NxVec3(scale*dims.x*0.5f, scale*dims.y*0.5f, scale*dims.z*0.5f);
	desc->localPose.t	= NxVec3(scale*pos.x, scale*pos.y, scale*pos.z);
	desc->localPose.M	= mat;

	if(shapeBlock->getBlockInt("trigger"))
		{
		// We found a trigger
		desc->shapeFlags	|= NX_TRIGGER_ENABLE;
		}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ActActor::loadSphereData(ODBlock* shapeBlock, NxSphereShapeDesc* desc, const NxMat34& pose)
	{
	// Default values
	Vec3 pos;
	pos.Zero();
	float r = 1.0f;

	// Fetch data
	shapeBlock->getBlockFloat("radius",&r);
	shapeBlock->getBlockFloats("position",&pos.x, 3);

	// Fill descriptor
	desc->radius		= scale*r;
	desc->localPose.t	= NxVec3(scale*pos.x, scale*pos.y, scale*pos.z);

	if(shapeBlock->getBlockInt("trigger"))
		{
		// We found a trigger
		desc->shapeFlags	|= NX_TRIGGER_ENABLE;
		}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ActActor::loadCapsuleData(ODBlock* shapeBlock, NxCapsuleShapeDesc* desc, const NxMat34& pose)
	{
	// Default values
	Vec3 pos;		pos.Zero();
	Quat orient;	orient.Zero();
	Vec3 temp;		temp.Zero();
	float r = 1.0f;
	float h = 1.0f;

	// Fetch data
	shapeBlock->getBlockFloat("radius",&r);
	shapeBlock->getBlockFloat("height",&h);
	shapeBlock->getBlockFloats("position",&pos.x, 3);
	if (!shapeBlock->getBlockFloats("orientation",&orient.w, 4))
		if (shapeBlock->getBlockFloats("angles", &temp.x, 3))
			{
			Vec3 vX(1.0f,0.0f,0.0f),vY(0.0f,1.0f,0.0f),vZ(0.0f, 0.0f,1.0f);
			Quat RotX((float)temp.x,vX);
			Quat RotY((float)temp.y,vY);
			Quat RotZ((float)temp.z,vZ);
			orient = RotZ * RotY * RotX;
			orient.Normalize();
			}

	NxQuat q;
	NxMat33 mat;
	q.setWXYZ(orient.w, orient.x, orient.y, orient.z);
	mat.fromQuat(q);

	// Fill descriptor
	desc->radius		= scale*r;
	desc->height		= scale*h;
	desc->localPose.t	= NxVec3(scale*pos.x, scale*pos.y, scale*pos.z);
	desc->localPose.M	= mat;

	if(shapeBlock->getBlockInt("trigger"))
		{
		// We found a trigger
		desc->shapeFlags	|= NX_TRIGGER_ENABLE;
		}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ActActor::loadPlaneData(ODBlock* shapeBlock, NxPlaneShapeDesc* desc, const NxMat34& pose)
	{
	// Default values
	float d = 0;
	Vec3 pos;	pos.Set(0.0f, 1.0f, 0.0f);

	// Fetch data
	shapeBlock->getBlockFloats("normal",&pos.x, 3);
	shapeBlock->getBlockFloat("d",&d);

	// Fill descriptor
	desc->normal	= NxVec3(pos.x, pos.y, pos.z);
	desc->d			= d;

	if(shapeBlock->getBlockInt("trigger"))
		{
		// We found a trigger
		desc->shapeFlags	|= NX_TRIGGER_ENABLE;
		}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ActActor::createShapeForBody(ODBlock* collBlock, NxArray<NxShapeDesc*>& array, const NxMat34& pose, bool body)
/*
Format:
Collision
	{
	ShapeName
		{
		#shape specific
		}
	...more shapes (compound)
	}
*/
	{
	//this is to avoid crash due to having multiple "TriangleMesh" tags, 
	//which is an exporter bug at the moment.
	bool hadMeshAlready = false;	

	collBlock->reset();
	while (collBlock->moreSubBlocks())
		{
		ODBlock * shapeBlock = collBlock->nextSubBlock();
		const char * name = shapeBlock->ident();

		//1) load the next shape into currShape and the visualization it needs into currVis
		if (0 == strcmp("Box", name))
			{
			NxBoxShapeDesc* boxDesc = new NxBoxShapeDesc;
			boxDesc->name = name;
			loadBoxData(shapeBlock, boxDesc, pose);
			array.pushBack(boxDesc);
			}
		else if (0 == strcmp("Sphere", name))
			{
			NxSphereShapeDesc* sphereDesc = new NxSphereShapeDesc;
			sphereDesc->name = name;
			loadSphereData(shapeBlock, sphereDesc, pose);
			array.pushBack(sphereDesc);
			}
		else if (0 == strcmp("Capsule", name))
			{
			NxCapsuleShapeDesc* capsuleDesc = new NxCapsuleShapeDesc;
			capsuleDesc->name = name;
			loadCapsuleData(shapeBlock, capsuleDesc, pose);
			array.pushBack(capsuleDesc);
			}
		else if (0 == strcmp("Plane", name))
			{
			NxPlaneShapeDesc* planeDesc = new NxPlaneShapeDesc;
			planeDesc->name = name;
			loadPlaneData(shapeBlock, planeDesc, pose);
			array.pushBack(planeDesc);
			}
		else if (0 == strcmp("TriangleMesh", name))
			{
			if (!hadMeshAlready)
				{
				hadMeshAlready = true;
				//is it a heightfield?
				//char * hfi;
				NxU32 hfAxis = 0xff;
				NxReal hfVal = 0;
				shapeBlock->getBlockInt("heightFieldInfAxis", (int *)&hfAxis);
				shapeBlock->getBlockFloat("heightFieldInfValue", &hfVal);
				if (hfAxis > 2)
					hfAxis = 0xff;

				// PMAP
				int pmapDensity = 0;
				const char* pmapFile = NULL;
				shapeBlock->getBlockInt("pmapDensity", &pmapDensity);
				shapeBlock->getBlockString("pmapFile", &pmapFile);
				// ~PMAP

				//this guy may load several shapes and load them into the compound, 
				//which it will also instance in that case.
				int ssc = 1, t = 0;
				shapeBlock->getBlockInt("smoothSphereCollisions", &ssc);
				shapeBlock->getBlockInt("trigger", &t);
				addShapesFromGraphic(array, body, pose, hfAxis, hfVal, pmapDensity, pmapFile, ssc != 0, t != 0, shapeBlock->getBlock("convex")!=0, name);
				}
			}
		else
			printf("Unknown collision shape!\n");
			}
		}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ActActor::graphicsUpdate()
	{
	if(actor && actor->isDynamic())
		updateSceneObjPose(getActor2World());
	}

#ifdef INCLUDE_SOUND
Sound::Source * ActActor::getSoundObject(unsigned i)
	{
	if (i >= soundObjects.size()) return NULL;
	return soundObjects[i];
	}
#endif

SceneGraph::Object * ActActor::getVisObject()
	{
	return sceneObj;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ActActor::isCalled(const char * n)
	{
	const char * name = getName();
	if (!name)
		return 0;
	else
		return (!strcmp(name, n));
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ActActor::enablePair(NxScene* scene, ActActor & other, bool enable)
	{
	NxActor* actor0 = getActor();
	NxActor* actor1 =other.getActor();
	if(actor0 && actor1 && scene)
//		scene->enablePair(*actor0, *actor1, enable);
		scene->setActorPairFlags(*actor0, *actor1, enable ? 0 : NX_IGNORE_PAIR);	//Note: this wipes out any other flags that may be set! Good thing this app doesn't use any others.
	else
		printf("Error: Can't disable colldet between actors with no shapes.\n");
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static NxU32 getFileSize(const char* name)
{
	#ifndef SEEK_END
	#define SEEK_END 2
	#endif
	
	FILE* File = fopen(name, "rb");
	if(!File)
		return 0;

	fseek(File, 0, SEEK_END);
	NxU32 eof_ftell = ftell(File);
	fclose(File);
	return eof_ftell;
}

/**
Load pmap from file
*/
void ActActor::loadPMap(const char* pmapFile, int pmapDensity, NxTriangleMesh *triangleMesh)
	{

	FILE* fp = fopen(pmapFile, "rb");
	if(!fp)
		{
		// Create pmap
		NxPMap pmap;
		if(NxCreatePMap(pmap, *triangleMesh, pmapDensity))
			{
			// Save to disk for next time
			FILE* fp = fopen(pmapFile, "wb");
			if(fp)
				{
				fwrite(pmap.data, 1, pmap.dataSize, fp);
				fclose(fp);
				}

			NxReleasePMap(pmap);
			}
		}

	// Try again after possible creation
	fp = fopen(pmapFile, "rb");
	if(fp)
			{
		NxPMap pmap;
		pmap.dataSize	= getFileSize(pmapFile);
		pmap.data		= new NxU8[pmap.dataSize];
		fread(pmap.data, 1, pmap.dataSize, fp);
		fclose(fp);

		bool Status = triangleMesh->loadPMap(pmap);
		NX_DELETE_ARRAY(pmap.data);
		}
	}

void ActActor::addShapesFromGraphic(NxArray<NxShapeDesc*>& array, bool body, const NxMat34& pose, NxU32 hfAxis, NxReal hfVal, int pmapDensity, const char* pmapFile, bool smoothSphereMode, bool trigger, bool convex, const char* name)
	{
	if (!sceneObj)
		return;

	//* for each model in the subtree create an adapter if it doesn't already exist; all the adapters create cd::triangleMeshes
	//* instance all the meshes into shapes
	//* set the shapes'relative transforms to the correct ones in the tree.
	//* if there are multiple shapes create a shape set and add all the shapes

	//iterate through the objects
	SceneGraph::Object * t = sceneObj;

	if (t->scale != 0 && t->scale != 1)
		{
		printf("Error: Scaled meshes not yet supported for collision detection.\n");
		return;
		}

	SceneGraph::Iterator i(t);
	i.setCumulateTransforms(true, false);
	SceneGraph::Object * o;
	SceneGraph::Model * mo;
	SceneGraph::Mesh * me;
	SceneGraph::SubMesh * sme;
	while ((o = i.getNextObject()))
		{
		clMatrix4x4 * x = i.getCumulatedTransform();

		while ((mo = i.getNextModelOfCurrObject() ))
			{
			while ((me = i.getNextMeshOfCurrModel() ))
				{
				while ((sme = i.getNextSubmeshOfCurrMesh()))
					{
					if (sme->nTriangles >= 2 && sme->nVertices >= 4)
						{
						CollisionMeshAdapter* adapter = (CollisionMeshAdapter *)sme->userData;
						if (!adapter)	//no adapter yet.
							{			//create adapter:
							//BUG: if the terrain was previously created with different axis values, 
							//then it will not take over the right ones here.
							sme->userData = adapter = new CollisionMeshAdapter(); 

							NxTriangleMeshDesc meshDesc;
							//meshDesc.userTriangleMesh			= adapter;
							meshDesc.numVertices				= sme->nVertices;
							meshDesc.numTriangles				= sme->nTriangles;
							meshDesc.pointStrideBytes			= sizeof(float) * (3 + 3 + 2);
							meshDesc.triangleStrideBytes		= sizeof(unsigned) * 3;
							meshDesc.points						= sme->vertexList;
							meshDesc.triangles					= sme->indexList;							
							meshDesc.flags						= convex ? NX_MF_CONVEX|NX_MF_COMPUTE_CONVEX : 0;


							//meshDesc.state						= NX_MESHSTATE_STATIC;
							meshDesc.heightFieldVerticalAxis	= (NxHeightFieldAxis)hfAxis;
							meshDesc.heightFieldVerticalExtent	= hfVal;
							//meshDesc.convex						= convex;
							NxTriangleMesh * triangleMesh = ACT->physicsSDK->createTriangleMesh(meshDesc);
							if (!triangleMesh)
								Fatal("Failure to create TrigMesh!");

							adapter->setTriangleMesh(*triangleMesh);

							if(pmapFile)
								loadPMap(pmapFile, pmapDensity, triangleMesh);
							}

						NxTriangleMeshShapeDesc* triangleMeshShapeDesc = new NxTriangleMeshShapeDesc;
//						if (!body)
//							triangleMeshShapeDesc->group = NX_GROUP_STATIC;

						if(smoothSphereMode)
							triangleMeshShapeDesc->meshFlags |= NX_MESH_SMOOTH_SPHERE_COLLISIONS;
						else
							triangleMeshShapeDesc->meshFlags &= ~NX_MESH_SMOOTH_SPHERE_COLLISIONS;

						if(trigger)
							triangleMeshShapeDesc->shapeFlags |= NX_TRIGGER_ENABLE;

						triangleMeshShapeDesc->meshData = adapter->getTriangleMesh();
						triangleMeshShapeDesc->name = name;

						//set the mesh2body transform for the shape
						NxVec3 p;
						NxMat33 m;
						if (x || (t->scale != 0 && t->scale != 1))
							{
							if (x)
								{
								p.set(x->M16[12], x->M16[13], x->M16[14]);
								m.setColumnMajorStride4(&x->M16[0]);
								}
							else
								p.zero();

							triangleMeshShapeDesc->localPose.t = p;
							triangleMeshShapeDesc->localPose.M = m;
							}

						array.pushBack(triangleMeshShapeDesc);
						}
					else
						printf("warning: mesh empty or too small for colldet!\n");
					}			
				}
			}
		}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void ActActor::wakeUp()
	{
	if(actor)	actor->wakeUp();
	}
