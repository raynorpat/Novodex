/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
/**
This file has the methods which are called from the graphics framework:

void appStartUp();
void appShutDown();
void appPrintHelp();
char * appGetTitle();
void appRenderTargSized(unsigned w,unsigned h);
void appRender();
void appTick();
void appKey(char, bool down = true);
void appClick(int x, int y, int button, bool up);	//1 = left, 2 = right, 3 = middle.
void appDrag(int x, int y, int dx, int dy, int button);

Below we basically do the following:

  1) initialize the graphics library on appStartUp()
  2) create a PhysicsDemo object.  (see PhysicsDemo.h)
  3) as time passes, keep on calling the tick() method of the physics demo.
  4) feed user input (mouse, keyboard) to the physics demo.
  5) clean up on quit.
*/

#include <fstream>

#ifdef WIN32
#include <crtdbg.h>				//debug heap

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX				//suppress windows' global min,max macros.
#include <windows.h>
#include <commdlg.h>
#include "resource.h"
#endif

#include "ViewerPlatform.h"
#include <float.h>

//graphics lib:
#include "Graphics.h"
#include "ODBlock.h"
#include "ViewerPlatform.h"
#include "ViewerClock.h"

//demo specific:
#include "PhysicsDemo.h"
#include "ViewerGraphicsContext.h"
#include "PerfDisplay.h"
#include "NxProfiler.h"
#include "FPSCounter.h"
#include "ShapeVis.h"


//#define NODEBUG				//to remove debug keys
/*-------------------------\
|	Demo     Variables
\-------------------------*/

extern bool bFramerateOn;
enum
	{
	PAUSED,
	RUN,		//as fast as machine can process simulation
	STEP,		//take one step, then pause
	TONEXTCOLL,
	REALTIME,	//run maximum at realtime speed
	CATCHUP_REALTIME	//like realtime, but if sim 'owes' time by going slower before, then it can speed up to make up for it (Diablo)
	} pauseState;

/*-------------------------\
|	User input
\-------------------------*/
extern bool keyDown[256];	//in ViewerPlatform.cpp

/*-------------------------\
|	Graphics Variables
\-------------------------*/
SceneGraph::Object * visualization;
bool drawSkin = false;
bool nextSceneSignal = false;
bool sleepOK = false;			//if we do frame rate limiting to avoid sucking all the proc power.
unsigned debugDrawLevel = 1;
/*-------------------------\
|	Profiling
\-------------------------*/

unsigned NumCDHit = 0;
unsigned NumCDTest = 0;
float fpsLimit = 0.01f;//0.02f; //0.033f;	//inverse fps ceiling.
float physicsDt	= 0.02f; //inverse physics Hz.  note: is changed by Act class.
float timeToStart = 0;

unsigned numFrames = 0;	//number of graphics frames rendered so far.

PerfDisplay perfDisp;
bool perfOn = false;
bool perfUpdate = true;
NxProfiler::DefineZone perfAppRender("Rendering");
NxProfiler::DefineZone perfAppInput("Input");
NxProfiler::DefineZone perfTickPhys("Physics");

unsigned DEBUGnumTicksThisFrame = 0;
second DEBUGTicksThisFrame = 0;

/*-------------------------\
|	Drawn strings
\-------------------------*/
char fpsString[12] = " ";
char perfString[16] = "[**********]";
char modeString[32] = "Mode: none";
char helpString[1024] = "Keys: ";
static bool gShowHelp = false;

static FPSCounter gFPS;

static void fpsUpdate();			//called every second to update the fps
/*-------------------------\
|	demo vars
\-------------------------*/
PhysicsDemo * pdemo = NULL;
static bool physicsTestConfigured = false;
static const char * physicsTestActorName = NULL;
static float physicsTestMinY = 0.0f;
static float physicsTestMaxY = 0.0f;
static unsigned physicsTestStepsRemaining = 0;
static int physicsTestResult = -1;

void viewerConfigurePhysicsTest(const char * actorName, float minY, float maxY, unsigned steps)
	{
	physicsTestConfigured = true;
	physicsTestActorName = actorName;
	physicsTestMinY = minY;
	physicsTestMaxY = maxY;
	physicsTestStepsRemaining = steps;
	physicsTestResult = -1;
	}

int viewerPhysicsTestResult()
	{
	return physicsTestResult;
	}
ViewerClock cloTickTimer;
static bool loadSceneRequested = false;

void appRenderTargSized(unsigned ww, unsigned hh);

void Fatal(const char * m)
	{
#ifdef WIN32
	if (viewerHiddenWindow()) fprintf(stderr, "Fatal Error: %s\n", m);
	else MessageBox(NULL,m, "Fatal Error",MB_ICONSTOP);
	exit(1);
#elif LINUX
	printf("Fatal Error: %s", m);
	exit(1);
#endif
	}

char * chewedCommandLine(int argc, char** argv)
	{
	// The C runtime already removes argument quotes and preserves path spaces.
	return argc > 1 ? argv[1] : 0;
	}


void visToggleMenuProc(int id)
	{
	if (pdemo)
		{
		Act * act = pdemo->getAct();
		act->toggleSDKParameter((Act::SDKParameters) id);
		}
	}

void menuProc(int id)
	{
	switch (id)
		{
		//viewer vis
		case 6://wireframe
			{
			static bool gWire = false;
			gWire = !gWire;
			if(gWire)	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			else		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			break;
		case 9:	//hide all
			ViewGC::world->bHideSubtree = !ViewGC::world->bHideSubtree;
			break;

		case 41:
			break;
		case 42:
			break;
		case 43:
			break;

		case 81:
			break;
		case 82:
			break;
		case 83:
			break;
		//profiler
		case 101:
			perfOn = false;
			break;
		case 102:
			perfOn = true;
			NxProfiler::setMode(NX_SELF_TIME);
			break;
		case 103:
			perfOn = true;
			NxProfiler::setMode(NX_SELF_STDEV);
			break;
		case 104:
			perfOn = true;
			NxProfiler::setMode(NX_HIERARCHICAL_TIME);
			break;
		case 105:
			perfOn = true;
			NxProfiler::setMode(NX_HIERARCHICAL_STDEV);
			break;
		case 106:
			perfUpdate = !perfUpdate;
			break;
		}
	}


static void releaseScene()
	{
	sdelete(pdemo);
	SceneGraph::Scene::getInstance().shutDown();
	ShapeVis::release();
	ViewGC::camera = NULL;
	ViewGC::world = NULL;
	ViewGC::pickLine = NULL;
	visualization = NULL;
	}

static void initializeScene(char * filename)
	{
	nextSceneSignal = false;
	ViewGC::cameraEulers.Set(0, 0, 0);
	ViewGC::visualizationScale = 1.0f;
	ViewGC::fov = 45.0f;
	physicsDt = 0.02f;
	timeToStart = 0;
	numFrames = 0;
	DEBUGnumTicksThisFrame = 0;
	DEBUGTicksThisFrame = 0;
	// Scripted lights change persistent GL state; restore the startup light.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	const GLfloat direction[] = {0.0f, 0.0f, 1.0f, 0.0f};
	const GLfloat diffuse[] = {0.5f, 0.5f, 0.5f, 1.0f};
	const GLfloat specular[] = {0.4f, 0.4f, 0.4f, 1.0f};
	glLightfv(GL_LIGHT0, GL_POSITION, direction);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 0);
	for (unsigned i = 1; i < 8; ++i) glDisable(GL_LIGHT0 + i);
	glEnable(GL_LIGHT0);
	glClearColor(0.2f, 0.35f, 0.55f, 1.0f);

	SceneGraph::Scene & scene = SceneGraph::Scene::getInstance();
	ViewGC::camera = new SceneGraph::Object();
	ViewGC::camera->bHideSubtree = true;
	scene.addObject(*ViewGC::camera);
	ViewGC::camera->setPosition(0, 10.0f, 28.0f);
	scene.setCamera(ViewGC::camera);
	ViewGC::world = new SceneGraph::Object();
	ViewGC::world->scale = 1;
	scene.addObject(*ViewGC::world);

	pdemo = new PhysicsDemo();
	pdemo->load(filename);
	pdemo->updateGraphicsPoses();
	ShapeVis::init();
	ViewGC::pickLine = new SceneGraph::Line(Vec3(0,0,0), Vec3(0,0,0));
	ViewGC::world->primitive = ViewGC::pickLine;
	appRenderTargSized(0, 0);
	pauseState = CATCHUP_REALTIME;
	cloTickTimer.GetElapsedSeconds();
	}

bool appLoadScene(char * filename)
	{
	// Cancelling the picker must leave the running scene intact.
	if (!filename || !*filename) return false;
	releaseScene();
	initializeScene(filename);
	return true;
	}

void fileMenuProc(int id)
	{
	if (id == 1) loadSceneRequested = true;
	else if (id == 2) viewerRequestClose();
	}

static void loadSceneFromDialog()
	{
	char filename[MAX_PATH] = {};
	OPENFILENAMEA dialog = {};
	dialog.lStructSize = sizeof(dialog);
	dialog.hwndOwner = GetActiveWindow();
	dialog.lpstrFile = filename;
	dialog.nMaxFile = sizeof(filename);
	dialog.lpstrFilter = "Scene Files (*.pds.ods)\0*.pds.ods\0All Files (*.*)\0*.*\0";
	dialog.nFilterIndex = 1;
	dialog.lpstrTitle = "Load Scene";
	dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetOpenFileNameA(&dialog)) appLoadScene(filename);
	else if (CommDlgExtendedError())
		MessageBoxA(dialog.hwndOwner, "The scene file picker could not be opened.", "Load Scene", MB_OK | MB_ICONERROR);
	// Ignore time spent in the modal picker, including when it was cancelled.
	cloTickTimer.GetElapsedSeconds();
	}

void createMenus()
	{
	int mainMenuID, fileMenuID, oldVisMenuID, perfMenuID, visMenuID;//, optionsMenuID;
	mainMenuID = ViewerUi::createMenu(menuProc);
	fileMenuID = ViewerUi::createMenu(fileMenuProc);
	ViewerUi::addMenuEntry("Load Scene...", 1);
	ViewerUi::addMenuEntry("Exit", 2);
	oldVisMenuID = ViewerUi::createMenu(menuProc);
	ViewerUi::addMenuEntry("wireframe", 6);
	ViewerUi::addMenuEntry("hide", 9);

	visMenuID = ViewerUi::createMenu(visToggleMenuProc);
	ViewerUi::addMenuEntry("World Axes", Act::VISUALIZE_WORLD_AXES);
	ViewerUi::addMenuEntry("Actor Axes", Act::VISUALIZE_ACTOR_AXES);

	ViewerUi::addMenuEntry("Body Axes", Act::VISUALIZE_BODY_AXES);
	ViewerUi::addMenuEntry("Body Inertia", Act::VISUALIZE_BODY_MASS_AXES);
	ViewerUi::addMenuEntry("Body L.Velocity", Act::VISUALIZE_BODY_LIN_VELOCITY);
	ViewerUi::addMenuEntry("Body A.Velocity", Act::VISUALIZE_BODY_ANG_VELOCITY);
	ViewerUi::addMenuEntry("Body Reduced", Act::VISUALIZE_BODY_REDUCED);
	ViewerUi::addMenuEntry("Body JointGroups", Act::VISUALIZE_BODY_JOINT_GROUPS);
	ViewerUi::addMenuEntry("Body Sleep", Act::VISUALIZE_BODY_SLEEP);

	ViewerUi::addMenuEntry("Joint World Axes", Act::VISUALIZE_JOINT_WORLD_AXES);
	ViewerUi::addMenuEntry("Joint Local Axes", Act::VISUALIZE_JOINT_LOCAL_AXES);
	ViewerUi::addMenuEntry("Joint Limits", Act::VISUALIZE_JOINT_LIMITS);

	ViewerUi::addMenuEntry("Contact Points", Act::VISUALIZE_CONTACT_POINT);
	ViewerUi::addMenuEntry("Contact Normals", Act::VISUALIZE_CONTACT_NORMAL);
	ViewerUi::addMenuEntry("Contact Forces", Act::VISUALIZE_CONTACT_FORCE);
	ViewerUi::addMenuEntry("Contact Error", Act::VISUALIZE_CONTACT_ERROR);

	ViewerUi::addMenuEntry("Collision Shapes", Act::VISUALIZE_COLLISION_SHAPES);
	ViewerUi::addMenuEntry("Collision AABBs", Act::VISUALIZE_COLLISION_AABBS);

	perfMenuID = ViewerUi::createMenu(menuProc);
		ViewerUi::addMenuEntry("none", 101);
		ViewerUi::addMenuEntry("self", 102);
		ViewerUi::addMenuEntry("self-stddev", 103);
		ViewerUi::addMenuEntry("hier", 104);
		ViewerUi::addMenuEntry("hier-stddev", 105);
		ViewerUi::addMenuEntry("toggle pause", 106);

	ViewerUi::setMenu(mainMenuID);
	ViewerUi::addSubMenu("File", fileMenuID);
	ViewerUi::addSubMenu("ViewerVisualize", oldVisMenuID);
	ViewerUi::addSubMenu("SDKVisualize", visMenuID);
	ViewerUi::addSubMenu("Profiler", perfMenuID);

	ViewerUi::setMainMenu(mainMenuID);
	}

void appStartUp(int argc, char** argv)
	{
	printf("\nNovodex Dynamics Demo \nCopyright 2004 Novodex AG.\n\n");

	createMenus();
	loadSceneRequested = false;

#ifdef WIN32
	typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);

	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapIntervalEXT)
		{
		printf("Disabled vsync.\n");
		wglSwapIntervalEXT(0);
		}
	else
		{
		printf("Warning: wglSwapIntervalEXT not supported.\n");
		}
#elif LINUX
#endif

	try
		{
	//optional debug heap:
#ifdef _DEBUG
#ifdef WIN32
		/*
         * Set the debug-heap flag to keep freed blocks in the
         * heap's linked list - This will allow us to catch any
         * inadvertent use of freed memory
		 */
        int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
		//tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;	//agressive mode, SLOW!  Don't turn on.
        _CrtSetDbgFlag(tmpDbgFlag);
#elif LINUX
#endif

#endif
/*-------------------------\
|	Graphics
\-------------------------*/
		initializeScene(chewedCommandLine(argc, argv));
		}
	catch (char * x)
		{
		Fatal(x);
		}

	pauseState = CATCHUP_REALTIME; //  //REALTIME; //PAUSED;
	bFramerateOn = true;
	cloTickTimer.GetElapsedSeconds();

	//draw all lines fat:
	glLineWidth(3);

	}

void appShutDown()
	{
	printf("shutDown\n");
	releaseScene();
	ViewerUi::shutDown();
	}

void appPrintHelp()
	{
	}

char * appGetTitle()
	{
	static char title[] = "Novodex Dynamics Demo";
	return title;
	}

void appRenderTargSized(unsigned ww,unsigned hh)
	{
	static unsigned w;
	static unsigned h;
	// Act can change fieldOfView while startup is still loading the first scene.
	if (!w || !h)
		{
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		w = viewport[2] > 0 ? viewport[2] : 1;
		h = viewport[3] > 0 ? viewport[3] : 1;
		}

	if (ww != 0)
		w = ww;
	if (hh != 0)
		h = hh;

	SceneGraph::Scene::getInstance().setProjection(1.0f, 10000.0f, ViewGC::fov, ((float)w)/h);
	}

void tickPhysics(second elapsed);


void appTick()
	{
	// Apply menu requests after menu traversal, while the GL/ImGui frame is alive.
	if (loadSceneRequested)
		{
		loadSceneRequested = false;
		loadSceneFromDialog();
		}
	if (physicsTestConfigured)
		{
		if (!pdemo || !pdemo->getAct())
			{
			physicsTestResult = 1;
			printf("Viewer physics test failed: scene did not load.\n");
			viewerRequestClose();
			return;
			}
		if (physicsTestStepsRemaining)
			{
			pdemo->controllerTick(physicsDt, true);
			pdemo->tick(physicsDt);
			pdemo->updateGraphicsPoses();
			--physicsTestStepsRemaining;
			return;
			}

		NxActor * actor = pdemo->getAct()->findBody(physicsTestActorName);
		if (!actor)
			{
			physicsTestResult = 2;
			printf("Viewer physics test failed: actor '%s' was not found.\n", physicsTestActorName);
			}
		else
			{
			const float y = actor->getGlobalPosition().y;
			physicsTestResult = y >= physicsTestMinY && y <= physicsTestMaxY ? 0 : 3;
			printf("Viewer physics test %s: actor '%s' y=%.4f expected [%.4f, %.4f].\n",
				physicsTestResult == 0 ? "passed" : "failed", physicsTestActorName, y,
				physicsTestMinY, physicsTestMaxY);
			}
		viewerRequestClose();
		return;
		}
	 tickPhysics(0);
	}

//#define MAX_NB_ITER	16
#define MAX_NB_ITER	8

void tickPhysics(second)
	{
	if (!pdemo)	return;

	second delta = cloTickTimer.GetElapsedSeconds();
//	printf("%f\n", delta);

//	static second pauseElapsed = 0.0;
//  FPS stuff
//	static second perfUpdateElapsed = 0.0;

	pdemo->perfUpdateElapsed += delta;
	if (pdemo->perfUpdateElapsed > 1.0f)	//update the perf data every second
		{
		fpsUpdate();
		DEBUGnumTicksThisFrame = 0;
		DEBUGTicksThisFrame = 0;
		pdemo->perfUpdateElapsed = 0;
		}

	// Number of updates can be limited, to prevent retroaction loops

	if (pauseState != PAUSED)
		{
		if (pauseState == RUN || pauseState == STEP)	// "as fast as possible" == framerate-dependent run
			{
				DEBUGnumTicksThisFrame ++;
				DEBUGTicksThisFrame += physicsDt;
				pdemo->controllerTick(physicsDt, pauseState == PAUSED);
				NxProfiler::SetCurrentZone xx(perfTickPhys);
				pdemo->tick(physicsDt);
			if (pauseState == STEP)
				pauseState = PAUSED;
			}
		else
			{
			NxU32 limit = MAX_NB_ITER;

//			static NxF32 remain = 0.0f;
			pdemo->remain += delta;
			while(pdemo->remain>=physicsDt && limit)
				{
				DEBUGnumTicksThisFrame ++;
				DEBUGTicksThisFrame += physicsDt;
				pdemo->controllerTick(physicsDt, pauseState == PAUSED);
				NxProfiler::SetCurrentZone xx(perfTickPhys);
				pdemo->tick(physicsDt);
				pdemo->remain -= physicsDt;
				limit--;
				}
			if(pdemo->remain>physicsDt)	pdemo->remain = physicsDt;
			}
		}
	else
		{
		pdemo->pauseElapsed += delta;
		if (pdemo->pauseElapsed >= physicsDt)
			{
			pdemo->controllerTick(physicsDt, pauseState == PAUSED);					//this will only do user input handling for the flying camera mode.
			pdemo->pauseElapsed = 0;
			}
		}
	// GLFW renders continuously in viewerRun().
	}

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

void appRender()
	{
	gFPS.Update();

	NxProfiler::SetCurrentZone xx(perfAppRender);
	static ViewerClock GrxTimer;
	GrxTimer.GetElapsedSeconds();					//profile

	glMatrixMode(GL_MODELVIEW);
	SceneGraph::Scene & s = SceneGraph::Scene::getInstance();
	s.clear();
	glEnable(GL_CULL_FACE);

//int Time;
//StartProfile(Time);
	s.render();
//EndProfile(Time);
//printf("%d\n", Time);
	glDisable(GL_TEXTURE_2D);	//this gets left on
	if (pdemo)
		pdemo->renderNonSceneExtras();

	// font colors
	static float colorYellow[4] = {1,1,0,1};
	static float colorWhite[4] = {1,1,1,1};

	ViewerUi::start();
	char Buffer[256];
	sprintf(Buffer, "%.2f fps", gFPS.GetFPS());
	ViewerUi::draw(1, 20, -0.96, 0.9, Buffer, colorYellow);	//"FPS: 100"

	ViewerUi::draw(1,24, -0.96, 0.83, modeString, colorWhite);	//"Mode: Fly"

	if(!gShowHelp)
		{
		ViewerUi::draw(1,24, -0.96, 0.76, "Press H for help", colorWhite);
		}
	else
		{
		ViewerUi::draw(1,24, -0.96, 0.76, "Press H to hide help", colorWhite);
		char helpLine[1024];

		float y=0.6;
		bool moreLines = true;
		int i=0;
		while(moreLines)
			{
			char* dest=helpLine;

			while(helpString[i] && helpString[i]!='\n')
				{
				*dest++ = helpString[i++];
				}
			moreLines = helpString[i++]!=0;
			*dest++=0;	// Close the string

			ViewerUi::draw(1,24, -0.96, y, helpLine);
			y-=0.1f;
			}
		}

	ViewerUi::end();


	if (perfOn && perfUpdate)
		NxProfiler::update();
	if (perfOn)
		perfDisp.display();


//	NxProfiler::SetCurrentZone xx2(perfSwapBuffers);
	numFrames ++;
	}


void appKey(char c, bool down)
	{
//	cloTickTimer.GetElapsedTime();		//ignore time spent with I/O, esp when a lot of time gathered as we were paused.
	NxProfiler::SetCurrentZone xx(perfAppInput);

	if (pdemo->input(c, down))			//pdemo handled the event.
		return;

	if(down)	//lower case
		{
		if (c == 27)	//temp, get rid if this when modules work.
			{
			viewerRequestClose();
			return;
			}
#ifndef NODEBUG
		switch (c)
			{
			case 'h':
				gShowHelp = !gShowHelp;
				break;
			case '1':
				pauseState = PAUSED;
				timeToStart = 0;
				break;
			case '2':
				pauseState = STEP;
				timeToStart = 0;
				break;
			case '3':
				pauseState = REALTIME;
				break;
			case '4':
				pauseState = CATCHUP_REALTIME;
				break;
			case '5':
				pauseState = RUN;
				break;
			case '6':
				debugDrawLevel ++;
				break;
			case '7':
				if (debugDrawLevel >= 2)
					debugDrawLevel --;
				break;
			case '8':
				NxProfiler::setMode(NX_SELF_TIME);
				break;
			case '9':
				NxProfiler::setMode(NX_HIERARCHICAL_TIME);
				break;
			case '*':
				NxProfiler::setMode(NX_SELF_STDEV);
				break;
			case '(':
				NxProfiler::setMode(NX_HIERARCHICAL_STDEV);
				break;
			case '0':
				perfOn = !perfOn;
				break;
			case ')':
				perfUpdate = !perfUpdate;
				break;
			case 'b':
				visToggleMenuProc(Act::VISUALIZE_COLLISION_SHAPES);
				break;
			case 'x':
				visToggleMenuProc(Act::VISUALIZE_CONTACT_POINT);
				break;
			case 'n':
				nextSceneSignal = true;
				break;
			case 'k':
				printf("orientation {%f;%f;%f;%f;}\n",ViewGC::camera->qOrientation.w, ViewGC::camera->qOrientation.x,ViewGC::camera->qOrientation.y, ViewGC::camera->qOrientation.z);
				printf("position {%f;%f;%f;}\n", ViewGC::camera->position.x,ViewGC::camera->position.y, ViewGC::camera->position.z);
				printf("angles {%f;%f;%f;}\n", ViewGC::cameraEulers.x,ViewGC::cameraEulers.y, ViewGC::cameraEulers.z);
				printf("scale {%f;}\n", ViewGC::world->scale);
				break;
			case 'q':
				ViewGC::visualizationScale *= 1.1;
				if (pdemo)
					{
					Act * act = pdemo->getAct();
					act->setVisualizationScale(ViewGC::visualizationScale);
					}
				break;
			case 'z':
				ViewGC::visualizationScale *= 0.9;
				if (pdemo)
					{
					Act * act = pdemo->getAct();
					act->setVisualizationScale(ViewGC::visualizationScale);
					}
				break;
			case 'u':
				menuProc(6);
				break;
			}
#else
		if (c == 12)
			nextSceneSignal = true;
#endif
		}
	}

static void window2world(int x, int y, Vec3 & v, Vec3 & w)
	{
   GLint viewport[4];
   GLdouble projmatrix[16], mvmatrix[16];
	GLint realy;  //  OpenGL y coordinate position
	GLdouble wx, wy, wz;  //  returned world x, y, z coords

    glGetIntegerv (GL_VIEWPORT, viewport);
	glMatrixMode(GL_MODELVIEW); //switch to model/world/view xforms for remainder of session.
	glLoadIdentity();
	if (ViewGC::camera)
		ViewGC::camera->renderAsCamera();
    glGetDoublev (GL_MODELVIEW_MATRIX, mvmatrix);
    glGetDoublev (GL_PROJECTION_MATRIX, projmatrix);
	// note viewport[3] is height of window in pixels
    realy = viewport[3] - (GLint) y - 1;
	// printf ("Coordinates at cursor are (%4d, %4d)\n", x, realy);
    gluUnProject ((GLdouble) x, (GLdouble) realy, 0.0, mvmatrix, projmatrix, viewport, &wx, &wy, &wz);
	v.Set(wx,wy,wz);

	v -= ViewGC::world->position;
	ViewGC::world->qOrientation.InverseRotate(v);
	if (ViewGC::world->scale)
		v *= 1/ViewGC::world->scale;

	// printf ("World coords at z=0.0 are (%f, %f, %f)\n", v.x, v.y, v.z);
    gluUnProject ((GLdouble) x, (GLdouble) realy, 1.0,mvmatrix, projmatrix, viewport, &wx, &wy, &wz);
	w.Set(wx,wy,wz);

	w -= ViewGC::world->position;
	ViewGC::world->qOrientation.InverseRotate(w);
	if (ViewGC::world->scale)
		w *= 1/ViewGC::world->scale;
	// printf ("World coords at z=1.0 are (%f, %f, %f)\n", w.x, w.y, w.z);
	}

void appClick(int x, int y, int button, bool down)
	{
	NxProfiler::SetCurrentZone xx(perfAppInput);
	Vec3 line_start, line_end;
	if (button == 2)//Right mouse hardwired pick + drag.
		{
		if (down)
			{
			window2world(x, y, line_start, line_end);
			if (pdemo)
				pdemo->linePick(line_start,line_end);
			}
		else if (pdemo)
			pdemo->unpick();
		}
	}

void appDrag(int x, int y, int dx, int dy, int button)
	{
	NxProfiler::SetCurrentZone xx(perfAppInput);
	Vec3 line_start, line_end;
	if (button == 2)//Right mouse hardwired pick + drag.
		{
		window2world(x, y, line_start, line_end);
		if (pdemo)
			pdemo->lineDrag(line_start,line_end);
		}
	else
		if (pdemo)
			pdemo->mouseDrag(x,y,dx,dy,button);
	}

static void fpsUpdate()
	{
	static unsigned oldNumFrames = 0;
	unsigned dnumFrames = numFrames - oldNumFrames;

	sprintf(fpsString, "%.2f * realTime",DEBUGTicksThisFrame);
	//limit grx if physics is below desired rate:
	oldNumFrames = numFrames;
	}
