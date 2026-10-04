/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef CLONE_CONTROLLER_H
#define CLONE_CONTROLLER_H

#include "NxArray.h"
#include "FlyController.h"
class NxBody;
class Act;

/**
Controllers are demo-specific behaviors like car driving or shooting gallery.  This is the
sort of thing you would script in a game, but we don't have a scripting language in this
viewer.

This controller is for creating lots of instances of some collection of jointed actors.
*/
class CloneController : public FlyController
	{
	public:
	struct ActorRec
		{
		ActorRec(const char * n) { name = n; templat = 0; }
		const char * name;
		//ActActor
		ODBlock * templat;
		};

	struct JointRec
		{
		JointRec(const char * n) { name = n; templat = 0; }
		const char * name;
		//ActJoint
		ODBlock	* templat;
		};

	NxArray<ActorRec *> actors;
	NxArray<JointRec *> joints;

	CloneController(ODBlock & block);
	virtual ~CloneController();

	virtual void activate(bool on);
	virtual bool input(char c, bool down);	//returns true if the event was handled.
	virtual void tick(float sec, Act & act, bool paused);
	virtual void mouseDrag(int x, int y, int dx, int dy, int button);
	private:
	void load();

	Act * pact;
	bool loaded;
	};
#endif