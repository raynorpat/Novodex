/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef __PHYSICSDEMO_H__
#define __PHYSICSDEMO_H__
#include "Act.h"

class ActController;

struct Model
	{
	const char * name;
	SceneGraph::Model	* model;
	ODBlock * block;
	Model * next;	//linked list
	};

struct ActorTemplate
	{
	const char * name;
	ODBlock * block;
	ActorTemplate * next;	//linked list
	};

/**
Physics demo is the class that loads pds.ods files, and cycles through their scenes.
It also stores any information that is shared between scenes, like graphics model data.

It also manages act controllers.  The default flight controller is also implemented in
physicsdemo.cpp
*/
class PhysicsDemo
	{

	Model * modelList;
	ActorTemplate * actorTemplateList;

	ODBlock * acts;				//act list block, call nextTerminal, etc to go to next block
	ODBlock * script;

	Act * currentAct;
	ActController * currentController;	//never NULL.
	ActController * controllerList;		//does not include defaultController.

	ActController * defaultController;


	void loadModel (ODBlock * model);
	void loadActorTemplate (ODBlock * actor);

	SceneGraph::Object * instanceSceneGraph(ODBlock & nodeBlock, SceneGraph::Object & parent, bool topLevel);

	bool goNextAct();			//returns true when finished
	void deleteControllerList();
	public:

	PhysicsDemo();
	void load(char * fileName);
	~PhysicsDemo();
	bool tick(float sec);		//returns true when finished  (delta time passed!)
	void controllerTick(float sec, bool paused);	//don't run act, just fly around if in that mode.
	void updateGraphicsPoses();
	void renderNonSceneExtras();
	Model	* getModel(const char * name);
	ActorTemplate   * getActorTemplate(const char * name);
	SceneGraph::Object * instanceActorTemplateGraphics(ActorTemplate & actor, SceneGraph::Object & parent);	//OK to return NULL. instances the actor with root being its parent.

	ActController * createActController(ODBlock & specialBlock);
	void setActController(ActController * c = NULL);

	ActController * getCurrentController() const;
	ActController * getDefaultController() const;
	ActController * getController(const char * name) const;
	inline Act * getAct();

	void	reset();
	second	pauseElapsed;
	second	perfUpdateElapsed;
	NxF32	remain;

	//interaction
	void linePick(const Vec3 & start, const Vec3 & end);	//pick an object (stab)
	void lineDrag(const Vec3 & start, const Vec3 & end);	//drag object around.
	void unpick();

	bool input(char c, bool down);							//returns true if the event was handled.
	void menuInput(int);
	void mouseDrag(int x, int y, int dx, int dy, int button);
	};


inline Act * PhysicsDemo::getAct()
	{
	return currentAct;
	}
#endif //__PHYSICSDEMO_H__