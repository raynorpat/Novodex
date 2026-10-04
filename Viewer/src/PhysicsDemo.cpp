/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PhysicsDemo.h"
#include <exception>

#include <Graphics.h>
#include <fstream>
#include <ODBlock.h>

#include "FlyController.h"

#include "Sound.h"

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#include <Assert.h>
#elif LINUX
#include <assert.h>
#include <unistd.h>
#else
#error you need to port the directory access stuff to this OS!
#endif

#include "ViewerGraphicsContext.h"

extern void Fatal(const char * m);
extern bool nextSceneSignal;
extern bool gDump; // need new core dump when scene changes

//==================================================

PhysicsDemo::PhysicsDemo()
	{
	defaultController = new FlyController();
	currentController = defaultController;
	currentController->activate(true);
	controllerList = 0;
	currentAct = 0;
	script = 0;
	acts = 0;
	modelList = 0;
	actorTemplateList = 0;
	reset();
	}

void PhysicsDemo::load(char * fileName)
	{
	char def[] = "demo.pds.ods";		//default file to try to open if none specified.
	char buffer[257] = "";
	std::ifstream myFile;

	reset();

	try {
		//1) get the script from the file
		if (!fileName || !*fileName)
			{
			fileName = def;
			myFile.open(fileName);
			if (!myFile.is_open())
				{
				myFile.close();
				myFile.clear();
				//didn't find the default file.  Show a file open dialog
#ifdef WIN32
				OPENFILENAME f;
				memset (&f, 0, sizeof(OPENFILENAME));
				fileName[0] = 0;
				f.lStructSize = sizeof(OPENFILENAME);
				f.hwndOwner = GetActiveWindow();
				f.lpstrFile = (LPSTR)buffer;
				f.nMaxFile = 256;
				f.lpstrFilter =	"Demo Files(*.pds.ods)\0*.pds.ods;\0All Files (*.*)\0*.*\0";
				f.nFilterIndex = 1;
				f.lpstrTitle = "Open";
				f.lpstrInitialDir = "C:\\";
				if (GetOpenFileName(&f))
					fileName = buffer;
				else
					Fatal("No file selected.");
#elif LINUX
	assert(0);
#else
	#error please show an OS specific file open dialog here!
#endif
				}
			}

		if (!myFile.is_open())	//here we have either just selected a file in the file dialog, or one was specified on the command line
			{
			//the specified file may be in a different directory.
			//because it will be referencing other files relative to that dir,
			//we need to switch the current dir.
			//1) extract the dir of the specified file

			unsigned endIndex = strlen(fileName);
			unsigned oldEndIndex = endIndex;
			//search backwards for first path separator
			while (endIndex && fileName[endIndex-1] != '\\' && fileName[endIndex-1] != '/')
				endIndex --;
			//now we either reached endIndex == 0, in which case there was no separator (and thus no dir),
			//or we found one
			if (endIndex)
				{
				//if endIndex did not decrement a single time, there is only a path, but no file name
				if (endIndex == oldEndIndex)
					{
					Fatal("Please specify a physics demo file, not a directory, as the command line parameter!");
					}
				char * dirName = fileName;
				fileName[endIndex-1] = 0;			//separate the two.
				fileName = &(fileName[endIndex]);	//should be the start of the actual file name.
#ifdef WIN32
				if (!SetCurrentDirectory(dirName))
#elif LINUX
				if (chdir(dirName) == 1)
#else
	#error please port the above nonportable code (because mac separators are weird) to your OS of choice!
#endif
					{
					char buffer[257];
					sprintf(buffer, "Can't find the directory: %.200s!", dirName);
					Fatal(buffer);
					}
				}
			}

		myFile.close();
		myFile.clear();

		myFile.open(fileName);
		if (!myFile.is_open())
			{
			sprintf(buffer, "Can't open the demo script file: %s!", fileName);
			Fatal(buffer);
			}
		script = ODBlock::loadScript(myFile); 
		if (!script) Fatal(ODBlock::lastError);
		//2) start iterating the Acts block
		acts = script->getBlock("Acts");
		if (!acts) Fatal("Can't load demo script (no acts block)!");
		acts->reset();
		//3) Load all the models in the Models block
		ODBlock * models = script->getBlock("Models");
		if (models)
			{
			models->reset(); 
			while (models->moreSubBlocks())
				loadModel(models->nextSubBlock());
			}
		ODBlock * actors = script->getBlock("Actors");
		if (actors)
			{
			actors->reset(); 
			while (actors->moreSubBlocks())
				loadActorTemplate(actors->nextSubBlock());
			}

#ifdef INCLUDE_SOUND
		if (script->getBlock("soundSupport") != NULL) Sound::Manager::open();
#endif
		//4) set up the first act to play
		if (goNextAct()) 
			Fatal("No acts!");
		}
	catch (char * c)
		{
		Fatal(c);
		}
	catch (const std::exception & error)
		{
		Fatal(error.what());
		}
	catch (...)
		{
		Fatal("Error during startup!");
		}
	}

PhysicsDemo::~PhysicsDemo()
	{
	//delete owned stuff
	Model * t, * i = modelList;
	while (i)
		{
		t = i;
		i = i->next;
		delete t;
		}
	modelList = 0;

	ActorTemplate * at, * ai = actorTemplateList;
	while (ai)
		{
		at = ai;
		ai = ai->next;
		delete at;
		}
	actorTemplateList = 0;

	deleteControllerList();
	sdelete(defaultController);
	currentController = NULL;
	sdelete(currentAct);

#ifdef INCLUDE_SOUND
	Sound::Manager::close();
#endif
	sdelete(script);
	}

void PhysicsDemo::deleteControllerList()
	{
	currentController = defaultController;
	while (controllerList)
		{
		ActController * t = controllerList;
		controllerList = controllerList->next;
		delete(t);
		}
	}

bool PhysicsDemo::goNextAct()
	{
	string actFN;
	deleteControllerList();
	if (acts->moreTerminals())
		{
		actFN = acts->nextTerminal();
		currentAct = new Act(actFN,this);
		if (currentAct) 
			{
			currentAct->updateGraphicsPoses();
			currentAct->render();	//set the poses for the world objects, otherwise it doesn't happen this frame anymore
			}
		return false;
		}
	else
		return true;
	}

void PhysicsDemo::reset()
	{
	pauseElapsed = 0.0;
	perfUpdateElapsed = 0.0;
	remain = 0.0f;
	}

bool PhysicsDemo::tick(float sec)		//returns true when finished
	{
	//0) update the sound
	#ifdef INCLUDE_SOUND
		if (Sound::Manager::isRunning()) 
			{
			Sound::Listener* listener = Sound::Manager::getListener();
			const NxVec3 oldPosition = listener->position;
			listener->position.set(&ViewGC::camera->position.x);
			listener->velocity = sec > 0 ? (listener->position-oldPosition)/sec : NxVec3(0,0,0);

			listener->listenerOri[0] = -ViewGC::camera->orientation.M16[8];
			listener->listenerOri[1] = -ViewGC::camera->orientation.M16[9];
			listener->listenerOri[2] = -ViewGC::camera->orientation.M16[10];
			
			listener->listenerOri[3] = ViewGC::camera->orientation.M16[4];
			listener->listenerOri[4] = ViewGC::camera->orientation.M16[5];
			listener->listenerOri[5] = ViewGC::camera->orientation.M16[6];
			listener->update();
			}
	#endif

	//1) tick the current act
	try 
		{
		if (currentAct->tick(sec) || nextSceneSignal)
			{
      gDump = true;
			nextSceneSignal = false;
			sdelete(currentAct);

			bool noMoreActs = goNextAct();
			if (noMoreActs)	//loop!
				{
				acts->reset();
				reset();
				return goNextAct();
				}
			else 
				return false;
			}
		else
			return false;
		}
	catch (char * c)
		{
		Fatal(c);
		}
	catch (const std::exception & error)
		{
		Fatal(error.what());
		}
	catch (...)
		{
		Fatal("Error! See console for details!");
		}

	return false;	//never reached.
	}

void PhysicsDemo::controllerTick(float sec, bool paused)	//don't run act, just fly around if in that mode.
	{
	currentController->tick(sec, *currentAct, paused);
	}


void PhysicsDemo::updateGraphicsPoses()
	{
	if (currentAct)
		currentAct->updateGraphicsPoses();
	}

void PhysicsDemo::renderNonSceneExtras()
	{
	if (currentAct)
		currentAct->render();
	}


void PhysicsDemo::loadModel (ODBlock * model)
/*
Model block:

ModelName
	{
	model3File.mod.ods;
	}
*/
	{
	char es [100];
	Model * m = new Model;
	model->reset();
	char * model3FName = model->nextTerminal();
	if (!model)
		{
		Fatal("Empty model block!?");
		}
	std::ifstream str(model3FName);
	if (!str.is_open())		
		{
		sprintf(es,"Cannot open file %s!",model3FName);
		Fatal(es);
		}
	m->model =  new SceneGraph::Model();
	SceneGraph::Scene::getInstance().addModel(*m->model);
	m->model->load(str);

	if (ODBlock::lastError)
		{
		sprintf(es,"Error in %s: %s\n",model3FName,ODBlock::lastError);
		Fatal(es);
		}
	m->name = model->ident();
	m->block = model;

	//add to list:
	m->next = modelList;
	modelList = m;
	}



void PhysicsDemo::loadActorTemplate (ODBlock * actor)
	{
	//load the model and put it in actors array, look for 10+ overflow, use Fatal()

	ActorTemplate * a = new ActorTemplate;
	a->name = actor->ident();
	//don't actually create the graphics or collision objects here, 
	//as they have to be created for each instance.  Instead, just save the block:
	a->block = actor;

	a->next = actorTemplateList;
	actorTemplateList = a;
	}
ActorTemplate * PhysicsDemo::getActorTemplate(const char * name)
	{
	ActorTemplate * i = actorTemplateList;
	while (i)
		{
		if (!strcmp(name,i->name))
			return i;
		i = i->next;
		}

	//also try with legacy 2 char prefix:
	if (name)
		name ++;
	if (name)
		name ++;
	if (!name)
		return NULL;

	i = actorTemplateList;
	while (i)
		{
		if (!strcmp(name,i->name))
			return i;
		i = i->next;
		}

	return NULL;
	}

Model * PhysicsDemo::getModel(const char * name)
	{
	Model * i = modelList;
	while (i)
		{
		if (!strcmp(name,i->name))
			return i;
		i = i->next;
		}
	return 0;
	}

SceneGraph::Object * PhysicsDemo::instanceActorTemplateGraphics(ActorTemplate & actor,SceneGraph::Object &  parent)	//OK to return NULL.
/*
Actor block:

ActorName
	{
	Graphics
		{
		# transform (pos+orient) from group space to body space (COM centered)
		Node
			{
			#the top level node has NO transform.  It will receive the dynamics body's transform that is animated.
			Model {Ball; }	#all nodes may have an optional model.

			# all nodes may have unlimited number of child nodes.

			Node	
				{
				#subnodes may have a transform (pos + orient) relative to the parent node. (Node2Parent transform)
				position {x;y;z;}		
				orientation {w;x;y;z;}

				Model { Rubik; }
				}
			Node
				{
				...
				}
			...
			}
		}
	#... other stuff
	}

*/
	{

	ODBlock * ablock = actor.block;
	assert(ablock);
	ODBlock * gblock = ablock->getBlock("Graphics");
	if (!gblock)
		{
		printf("Warning: actor %s has no Graphics block.\n", actor.name);
		return NULL;
		}

	ODBlock * nblock = gblock->getBlock("Node");
	if (nblock)
		return instanceSceneGraph(*nblock, parent, true);
	else
		return NULL;
	}

SceneGraph::Object * PhysicsDemo::instanceSceneGraph(ODBlock & nodeBlock, SceneGraph::Object &  parent, bool topLevel)
	{
	SceneGraph::Model * model = NULL;
	//is there a model?
	const char * modelName = 0;
	nodeBlock.getBlockString("Model", &modelName);
	if (modelName)
		{
		Model * m = getModel(modelName);
		if (m)
			model = m->model;
		else
			{
			printf("Warning: model %s not found.\n", modelName);
			}
		}
	//check if the model should be hidden
	bool hidden = nodeBlock.getBlock("hidden",false)!=0;


	//create a node with or w/o model:
	SceneGraph::Object * so = parent.createChild(model);
	assert(so);

	//set hidded
	so->bHideModel = hidden;

	//is there a transform?
	if (!topLevel)
		{
		Vec3 pos; 
		Quat rot;
		if (nodeBlock.getBlockFloats("position", &pos.x, 3))
			so->setPosition(&pos);
		if (nodeBlock.getBlockFloats("orientation", &rot.w, 4))
			so->setOrientation(&rot);
		else if (nodeBlock.getBlockFloats("angles", &pos.x, 3))
			{
			Vec3 vX(1.0f,0.0f,0.0f),vY(0.0f,1.0f,0.0f),vZ(0.0f, 0.0f,1.0f);
			Quat RotX((float)pos.x,vX);
			Quat RotY((float)pos.y,vY);
			Quat RotZ((float)pos.z,vZ);
			rot = RotZ * RotY * RotX;
			rot.Normalize();
			so->setOrientation(&rot);
			}
		}

	//are there children?
	nodeBlock.reset();
	while(nodeBlock.moreSubBlocks())
		{
		ODBlock * sub =  nodeBlock.nextSubBlock();
		if (!strcmp(sub->ident(), "Node"))
			{
			instanceSceneGraph(*sub,*so,false);
			}
		}
	return so;
	}

void PhysicsDemo::linePick(const Vec3 & start, const Vec3 & end)	//pick an object (stab)
	{
	if (currentAct)
		currentAct->linePick(start, end);	
	}

void PhysicsDemo::lineDrag(const Vec3 & start, const Vec3 & end)	//drag object around.
	{
	if (currentAct)
		currentAct->lineDrag(start, end);
	}

void PhysicsDemo::unpick()
	{
	if (currentAct)
		currentAct->unpick();
	}

bool PhysicsDemo::input(char c, bool down)
	{
	if (currentController) return currentController->input(c,down);
	return false;
	}

void PhysicsDemo::mouseDrag(int x, int y, int dx, int dy, int button)
	{
	if (currentController)
		currentController->mouseDrag(x,y,dx,dy,button);
	}

ActController * PhysicsDemo::createActController(ODBlock & specialBlock)
	{
	ActController * c = ActController::create(specialBlock);
	//add to our list:
	c->next = controllerList;
	controllerList = c;		

	if (c->defaultController)
		setActController(c);
	return c;
	}

void PhysicsDemo::setActController(ActController * c)
	{
	if (c)
		{
		if (currentController != c)
			{
			currentController->activate(false);
			currentController = c;
			currentController->activate(true);
			}
		}
	else if (currentController != defaultController)
		{
		currentController->activate(false);
		currentController = defaultController;
		currentController->activate(true);
		}
	}

ActController * PhysicsDemo::getCurrentController() const
	{
	return currentController;
	}

ActController * PhysicsDemo::getDefaultController() const
	{
	return defaultController;
	}

ActController * PhysicsDemo::getController(const char * name) const
	{
	for (ActController * cIt = controllerList; cIt != NULL; cIt = cIt->next)
		if (strcmp(cIt->name,name) == 0) return cIt;
	return NULL;
	}

