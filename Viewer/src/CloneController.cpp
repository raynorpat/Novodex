/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ODBlock.h"

#include "CloneController.h"
#include "Act.h"
#include "ActActor.h"
#include "ActJoint.h"
#include "ViewerGraphicsContext.h"
 
extern bool keyDown[256];	//ViewerPlatform.cpp
extern char modeString[32];
extern char helpString[32];

/*
format:

Control
	{
	Type { CloneController; }
	default { 1; }

	Actors
		{
		Head;
		Body;
		Leg;
		}
	Joints
		{
		Neck;
		Shoulder;
		}
	}
*/

CloneController::~CloneController()
	{
	unsigned i;
	for (i=0; i<joints.size(); i++)
		{
		delete joints[i];
		}
	for (i=0; i<actors.size(); i++)
		{
		delete actors[i];
		}
	}

CloneController::CloneController(ODBlock & block)
	{
	pact = 0;
	loaded = false;
	//defaults:
	ODBlock  * pb = block.getBlock("Actors");

	if (pb)
		{
		pb->reset();
		while (pb->moreTerminals())
			{
			actors.pushBack(new ActorRec(pb->nextTerminal()));
			}
		}
	pb = block.getBlock("Joints");
	if (pb)
		{
		pb->reset();
		while (pb->moreTerminals())
			{
			joints.pushBack(new JointRec(pb->nextTerminal()));
			}
		}
	}

void CloneController::activate(bool on)
	{
	FlyController::activate(on);
	if (on)
		sprintf(modeString, "Mode: Clone");
	}

void CloneController::mouseDrag(int x, int y, int dx, int dy, int button)
	{
	FlyController::mouseDrag(x,y,dx,dy,button);
	}



bool CloneController::input(char c, bool down)
	{
	//do the common input handling first
	if (ActController::input(c, down)) return true;

	if (pact && down && c == ' ')
		{
		//clone a copy -- i.e. re-instance objects:
		unsigned i;
		for (i = 0; i < actors.size(); i++)
			{
			ODBlock * b = actors[i]->templat;
			if (b)
				ActActor * actor = pact->createActor(0, b);
			}
		for (i = 0; i < joints.size(); i++)
			{
			ODBlock * b = joints[i]->templat;
			if (b)
				ActJoint * joint = pact->createJoint(0,b);
			}
		}
	return false;
	}


void CloneController::tick(float sec, Act & act, bool paused)
	{
	FlyController::tick(sec, act, paused);
	if (paused)
		return;

	if (!loaded)
		{
		loaded = true;

		pact = &act;

		unsigned i;
		for (i = 0; i < actors.size(); i++)
			{
			ActActor * a = act.findActor(actors[i]->name);
			if (a)
				actors[i]->templat = a->getBlock();		//the clone is taken out of the simulation to serve as a template!
			}
		for (i = 0; i < joints.size(); i++)
			{
			ActJoint * a = act.findActJoint(joints[i]->name);
			if (a)
				joints[i]->templat = a->getBlock();		//the clone is taken out of the simulation to serve as a template!
			}

/*
		Projectile * n = projectiles;
		while(n)
			{
			if (n && n->name && !n->projectile )
				{
				ActActor * ub = act.findActor(n->name);
				}
			n = n->next;
			}
*/
		}
	}