#ifndef ACTEFFECTOR_H
#define ACTEFFECTOR_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
class ODBlock;
class NxEffector;
class NxScene;

/**
An actor.  This is a rigid body + its shape + its graphics.
*/
class ActEffector
	{
	public:

	ActEffector();
	~ActEffector();

	/**
	load from an "effector" block.
	returns false on failure.
	*/
	bool load(unsigned effNum, ODBlock * block, NxScene* scene);

	/**
	invoked as a request for debug visualization
	*/
	void visualize();

	//for linked list in Act:
	ActEffector * previous;
	ActEffector * next;

	private:
	NxEffector * effector;
	};

#endif