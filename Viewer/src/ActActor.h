#ifndef ACTACTOR_H
#define ACTACTOR_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
class ShapeVisualizer;
class ODBlock;
class NxSubstance;

#include "Sound.h"

/**
An actor.  This is a physics actor + a graphics object.
*/
class ActActor
	{
	public:

	ActActor(NxScene*);
	virtual ~ActActor();

	/**
	load from a "body" block.
	returns false on failure.
	*/
	bool load(unsigned actorNum, ODBlock * actor, ActorTemplate * actr);

	/**
	updates the graphice representation of the object.
	*/
	void graphicsUpdate();

	/**
	does behavior computation
	*/
	void tickBehaviors(NxReal dt);

#ifdef INCLUDE_SOUND
	/**
	returns the i'th source belonging to the actor. If i invalid, it returns NULL
	*/
	Sound::Source * getSoundObject(unsigned i);
#endif	

	/**
	returns the scenegraph object belonging to this actor. 
	*/
	SceneGraph::Object * getVisObject();

	NxActor* getActor();

	/**
	returns the name
	*/
	const char * getName();

	/**
	returns actor's controller code, if there is one.
	*/
	ActController * getController();

	/**
	checks the name, returns true if its the same.
	*/
	bool isCalled(const char * name);

	/**
	all non-dynamic bodies are considered to be sleeping
	*/
	bool isSleeping();

	/**
	returns true if the actor wants its contacts to call notifyContact()
	*/
	bool needsContactNotify();

	/**
	enable / disable colldet between these two actors.
	*/
	void enablePair(NxScene* scene, ActActor & other, bool enable);

	void wakeUp();

	//for linked list in Act:
	ActActor * previous;
	ActActor * next;

	NX_INLINE const NxMat34 * getActor2World();
	ODBlock * getBlock();

	protected:

	SceneGraph::Object * sceneObj;		//graphical representation (NULL for static stuff or tetra meshes at this point)

#ifdef INCLUDE_SOUND
	NxArray<Sound::Source *> soundObjects; //owned sources
#endif

	NxActor*	actor;			//optional -- we have purely graphical actActors too!
	NxScene*	owner;
	ActController * controller;	//stuff like car driving logic, etc.  usually NULL.  Not owned!
	ActorTemplate * actorTemplate; //the actor in PhysicsDemo.	Not owned!
	float scale;				   //scale loaded from script

	void createShapeForBody(ODBlock* collBlock, NxArray<NxShapeDesc*>& array, const NxMat34& pose, bool body);
	private:
	void loadBoxData		(ODBlock* shapeBlock, NxBoxShapeDesc* desc,		const NxMat34& pose);
	void loadSphereData		(ODBlock* shapeBlock, NxSphereShapeDesc* desc,	const NxMat34& pose);
	void loadCapsuleData	(ODBlock* shapeBlock, NxCapsuleShapeDesc* desc,	const NxMat34& pose);
	void loadPlaneData		(ODBlock* shapeBlock, NxPlaneShapeDesc* desc,	const NxMat34& pose);

	void changeStateRigid2Soft();
	void changeStateSoft2Rigid();
	void changeStateRigid2Static();
	void changeStateRigid2Brittle();

	void loadBehaviors(ODBlock * behBlock);
	void loadPMap(const char* pmapFile, int pmapDensity, NxTriangleMesh *triangleMesh);
	void addShapesFromGraphic(NxArray<NxShapeDesc*>& array, bool body, const NxMat34& pose, NxU32 hfAxis = 0xff, NxReal hfVal = 0, int pmapDensity=0, const char* pmapFile=NULL, bool smoothSphereMode=false, bool trigger=false, bool convex=false, const char* name=NULL);

	void updateSceneObjPose(const NxMat34 *);

	ODBlock * block;			//actor got loaded from here.
	SceneGraph::Model * ownedModel;		//used if the sceneObj's model is not from physicsDemo's shared model list, but a custom one.

	//just a simplistic behavior implementation for now:
	struct Behavior
		{
		enum Type { TRANSLATION = 1, COM_TRANSLATION = 1<<1, ROTATION = 1 << 2 };
		NxU32 typeFlags;
		NxVec3 position[2];
		NxQuat orient[2];
		NxReal posParam, rotParam;
		NxReal posSpeed, rotSpeed;
		};
	Behavior * behavior;		
	};

NX_INLINE NxActor* ActActor::getActor() 
	{ 
	return actor;
	}

NX_INLINE const char * ActActor::getName() 
	{ 
	return actor ? actor->getName() : 0; 
	}


NX_INLINE ActController * ActActor::getController() 
	{
	return controller; 
	}

NX_INLINE bool ActActor::isSleeping()
	{
	if (!actor) 
		return true;
	return actor->isSleeping();
	}

NX_INLINE bool ActActor::needsContactNotify()
	{
	return 0;
	}

NX_INLINE const NxMat34 * ActActor::getActor2World()
	{
	if (!actor) 
		return 0;
	return &actor->getGlobalPoseReference();
	}

NX_INLINE ODBlock * ActActor::getBlock()
	{
	return block;
	}

#endif
