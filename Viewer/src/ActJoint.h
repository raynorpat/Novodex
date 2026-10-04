#ifndef ACTJOINT_H
#define ACTJOINT_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
class NxScene;

class ODBlock;
/**
A joint.
*/
class ActJoint
	{
	public:
	ActJoint();
	~ActJoint();

	/**
	load from a "joint" block.
	returns false on failure
	*/
	bool load(unsigned jointNum, ODBlock * joint, NxScene* scene);

	/**
	returns the actors this joint connects, if any
	*/
	void getActors(ActActor **, ActActor **);

	/**
	this gets called in sync with physics, and the object can do special behaviors here.
	*/
	inline bool isCalled(const char * name);
	inline NxJoint * getJoint();
	void detachBrokenJoint() { joint = 0; }
	inline ODBlock * getBlock();

	//for linked list in Act:
	ActJoint * previous;
	ActJoint * next;

	private:
	enum JointType { JT_NONE, JT_SPHERICAL, JT_CYLINDRICAL, JT_PRISMATIC, JT_REVOLUTE, JT_POINT_ON_LINE, JT_POINT_IN_PLANE, JT_DIRECTION_SYNC, JT_ORIENTATION_SYNC, JT_COLLISION_VETO, JT_FIXED};
	NxI32 type;

	const char * name;
	NxJoint * joint;			// may be NULL for a broken joint that was removed.
	ODBlock * block;
	};

inline bool ActJoint::isCalled(const char * n)
	{
	return (name && !strcmp(name, n));
	}

inline NxJoint * ActJoint::getJoint()
	{
	return joint;
	}

inline ODBlock * ActJoint::getBlock()
	{
	return block;
	}

#endif
