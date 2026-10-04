/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef ACT_H
#define ACT_H

#include "NxPhysics.h"

#ifdef INCLUDE_SUBSTANCE
#include "NxSubstanceIncludes.h"
#endif

#include "TextSpooler.h"
#include "Graphics.h"

class ODBlock;
class PhysicsDemo;
class ActController;
class ActRecorder;
class ActObject;
class ActActor;
class ActJoint;
class ActEffector;
struct ActorTemplate;


class MyAllocator : public NxUserAllocator
	{
	static unsigned numAllocs;
	static unsigned numFrees;

	void * malloc(NxU32 size);
	void * mallocDEBUG(NxU32 size, const char *fileName, int line);
	void * realloc(void * memory, NxU32 size);
	void free(void * memory);
	};


/**
This is the physics scene class.  It does the following things:
	1) loads a psc.ods scene file:
		- creates the bodies in the physics SDK
		- creates the bounding volumes in the colldet SDK
		- creates the graphical representations in the graphics lib
	2) when tick() gets called, it tells physics to simulate the scene forward in time,
		and updates the information between the physics, collision, and graphics aspects
	3) picking and dragging objects is implemented here.
	4) when the scene's time is up or the user presses the appropriate button, the scene unloads itself.
*/
class Act : public NxUserTriggerReport, public NxUserNotify
	{
	public: //types
	friend class ActActor;
	friend class ActJoint;

	enum Parameters
		{
		FRACT_COLLCANCEL = 0,		//!< Whether on collision with a fractured object the contact normal impulse should be canceled.
		FRACT_DELMINTIME = 1,		//!< Min time to delete small fragments.
		FRACT_DELMAXTIME = 2,		//!< Max time to delete small fragments.
		FRACT_CDTRIGS = 3,			//!< Min number of trigs to do collision detection for fracture pieces
		FRACT_STAYTRIGS = 4,		//!< Min number of trigs for fragment that doesn't get deleted eventually
		COLL_VETO_JOINTED = 5,		//!< Jointed body pair collisions should be vetoed.
		IGNORE_MASS = 6,			//!< mass and inertia tensors of all bodies set to identity.
		PARAMS_NUM_VALUES= 7		//!< This is not a parameter, it just records the current number of parameters
		};

	float params[PARAMS_NUM_VALUES];

	enum SDKParameters
		{
		VISUALIZE_WORLD_AXES,		
		VISUALIZE_BODY_AXES,
		VISUALIZE_BODY_MASS_AXES,
		VISUALIZE_BODY_LIN_VELOCITY,
		VISUALIZE_BODY_ANG_VELOCITY,
		VISUALIZE_BODY_LIN_MOMENTUM, 
		VISUALIZE_BODY_ANG_MOMENTUM,
		VISUALIZE_BODY_LIN_ACCEL,
		VISUALIZE_BODY_ANG_ACCEL,
		VISUALIZE_BODY_LIN_FORCE,
		VISUALIZE_BODY_ANG_FORCE,
		VISUALIZE_BODY_REDUCED,
		VISUALIZE_BODY_JOINT_GROUPS,
		VISUALIZE_BODY_CONTACT_LIST,
		VISUALIZE_BODY_JOINT_LIST,
		VISUALIZE_BODY_DAMPING,
		VISUALIZE_BODY_SLEEP,

		VISUALIZE_JOINT_LOCAL_AXES,
		VISUALIZE_JOINT_WORLD_AXES,
		VISUALIZE_JOINT_LIMITS,
		VISUALIZE_JOINT_ERROR,
		VISUALIZE_JOINT_FORCE,
		VISUALIZE_JOINT_REDUCED,

		VISUALIZE_CONTACT_POINT,
		VISUALIZE_CONTACT_NORMAL,
		VISUALIZE_CONTACT_ERROR,
		VISUALIZE_CONTACT_FORCE,

		VISUALIZE_COLLISION_SHAPES,
		VISUALIZE_COLLISION_AXES,
		VISUALIZE_COLLISION_AABBS,
		VISUALIZE_COLLISION_COMPOUNDS,

		VISUALIZE_ACTOR_AXES,

		BOX_BOX_FAST

		};

	Act(const char * fname,PhysicsDemo * pd);
	~Act();
	ActActor * createActor(unsigned actorNum, ODBlock * block);
	ActJoint * createJoint(unsigned jointNum, ODBlock * block);

	inline static Act * instance() { return gact; }
	bool tick(float sec);		//returns true when finished
	void render();
	void updateGraphicsPoses();
	void toggleSDKParameter(SDKParameters);
	void setVisualizationScale(float scale);
	ActActor * findActor(const char * name);
	NxActor* findBody(const char * name);
	ActJoint * findActJoint(const char * name);

	//interaction
	void linePick(const Vec3 & start, const Vec3 & end);	//pick an object (stab)
	void lineDrag(const Vec3 & start, const Vec3 & end);	//drag object around.
	ActActor * linePick(const NxVec3 & start, const NxVec3 & end);			//subroutine.
	void lineDrag(const NxVec3 & start, const NxVec3 & end);			//drag object around.
	const NxVec3 & getStabPointActorSpace() { return stabPointActorSpace; }
	const NxVec3 getWorldStab();
	void tickDrag();
	void unpick();
	void activatePicked(unsigned code);

	PhysicsDemo * getContainer() const { return container; }

	//virtuals implemented for base classes:
	virtual void onTrigger(NxShape & triggerShape, NxShape & otherShape, NxTriggerFlag status);
	virtual bool onJointBreak(NxReal breakingForce, NxJoint & brokenJoint);

	NX_INLINE	NxScene* getScene()			{ return scene;	}

	private:
	PhysicsDemo * container;
	static Act * gact;
	ODBlock * script;
	TextSpooler	textSpooler;
	SceneGraph::Object * lightObj[3];
	ActRecorder * recorder;		//only not null if its being used for recording or playback.	
	unsigned gridResolution;
	unsigned frameNumber;			//incremented with every tick()
	float timeRemaining;
	bool fractureOK, breakingJointsOK;
	bool doCollisions;
	bool haveInvisibleActors;

	MyAllocator myAllocator;
	NxPhysicsSDK* physicsSDK;
	NxScene* scene;

	/*-------------------------\
	|	End
	\-------------------------*/

	ActActor	* actorList;
	ActJoint	* jointList;
	ActEffector * effectorList;

	//behaviors:
	NxArray<ActActor *> behaviorActors;

	void tickBehaviors(NxReal dt);
	void registerBehavior(ActActor *, bool add);	//else remove

	//picking:
	ActActor * pickedActor;
	NxVec3 stabPointActorSpace, lineStart, lineEnd;	//where picked box was picked, and the two ends of the drag line.  These are only valid if we have a pickedBody.
	NxReal  stabPointCameraDist; //the point where the body is attached to the camera, in camera space.
	float dragForce;
	float maxStretch;
	bool dragging; 
	bool drag; 

	void loadParameters(ODBlock *);
	void createEffector(unsigned effNum,ODBlock * block);
	void addActor(ActActor & ub);

	void addJoint(ActJoint &);
	void deleteJoint(ActJoint & j);
	void deleteActor(ActActor & ub);	//only call outside of physics callbacks.
	void nearestPointOnLine(const NxVec3 & rkPoint, const NxVec3 & start, const NxVec3 & end, NxVec3 & nearest);
	void setParameter(NxU32 paramEnum, float paramValue);
	};
#endif //__ACT_H__
