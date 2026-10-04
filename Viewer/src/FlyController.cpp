/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ViewerGraphicsContext.h"

#include <stdio.h>
#include <string.h>
#include "3DMath.h"

#include "FlyController.h"
#include "Act.h"
 
extern bool keyDown[256];	//ViewerPlatform.cpp
extern char modeString[32];
extern char helpString[32];

char FlyController::helpText[] = "* w,a,s,d or arrows to fly horizontally.\n* e,c or pageUp, pageDown to fly up/down.\n* Drag left mouse button to mouselook."
"\n* Right drag to move simple items.\n* Right mouse click on car to drive.\n*0 for performance info.\n* Middle mouse for menu.\n* n goes to next/reset scene.";

FlyController::FlyController()
	{
	timeSinceLastInput = 0;
	mouseDxSinceLastInput = 0;
	mouseDySinceLastInput = 0;
	mouseDxAvg = 0;
	mouseDyAvg = 0;
	}

void FlyController::activate(bool on)
	{
	if (!on) return;
		sprintf(modeString, "Mode: Fly");
		sprintf(helpString, helpText);
	}
	
bool FlyController::input(char c, bool down)
	{
	//do the common input handling first
	if (ActController::input(c, down)) return true;

	Act * act = Act::instance();

	if (c == 127 && down)//del -> quick delete key support
		{
		act->activatePicked(0);
		return true;
		}
	else if (c == 28 && down)// insert -> nudge
		{
		act->activatePicked(1);
		return true;
		}

	//nothing.
	return false;
	}


//handle mouse input immediately.  The below timer based approach didn't work.
void FlyController::mouseDrag(int x, int y, int dx, int dy, int button)
	{
	if (button == 1)	//we do mouselook on left drag.
		{
		Quat q;
		ViewGC::cameraEulers.y -= dx * 1;
		ViewGC::cameraEulers.z -= dy * 1;
		q.fromEulerAngles(0, Quat::deg_to_rad(ViewGC::cameraEulers.z), Quat::deg_to_rad(ViewGC::cameraEulers.y));
		ViewGC::camera->setOrientation(&q);
		}
	}


void FlyController::tick(float sec, Act & act, bool paused)
	{
	if (sec > 0.1f)
		sec = 0.1f;	//don't jump around.

	timeSinceLastInput += sec;

//	if (timeSinceLastInput > 0.02f)		// I need to remove this to test the character controller
		{
		Vec3 delta(0,0,0);
		float speed = 30.0f * timeSinceLastInput;
		float rotSpeed = 10.0f * timeSinceLastInput;
		//do input handling.
		if (keyDown['a'] || keyDown[KEY_LEFT])
			delta.x = -speed;
		else if (keyDown['d'] || keyDown[KEY_RIGHT])
			delta.x = speed;
		if (keyDown['e'] || keyDown[KEY_PAGE_UP])
			delta.y = speed;
		else if (keyDown['c'] || keyDown[KEY_PAGE_DOWN])
			delta.y = -speed;
		if (keyDown['w'] || keyDown[KEY_UP])
			delta.z = -speed;
		else if (keyDown['s'] || keyDown[KEY_DOWN])
			delta.z = speed;

		//translate the camera:
		Vec3 camX(ViewGC::camera->orientation.M16[0], ViewGC::camera->orientation.M16[1], ViewGC::camera->orientation.M16[2]);		//ViewGC::camera x axis
		Vec3 camZ(ViewGC::camera->orientation.M16[8], ViewGC::camera->orientation.M16[9], ViewGC::camera->orientation.M16[10]);		//camera z axis
		camX *= delta.x;	//left/right
		camZ *= delta.z;//*50.0f;	//forward/back

		camX += camZ;
		camX.y += delta.y;	//up/down in world space.
		camX += ViewGC::camera->position;

#ifdef PIERRE_KINEMATIC_TEST
		if(0)
			{
			Vec3 currentCamPos = ViewGC::camera->position;

			NxVec3 direction(	camX.x - currentCamPos.x,
								camX.y - currentCamPos.y,
								camX.z - currentCamPos.z);

			NxVec3 center(currentCamPos.x, currentCamPos.y, currentCamPos.z);
			NxScene* scene = act.getScene();
//			if(NxSweepBoxCollide(scene, center, NxVec3(0.0f, 0.0f, 0.0f), direction))
			if(NxSweepBoxCollide(scene, center, NxVec3(1.0f, 3.5f, 1.0f), direction))
//			if(NxSweepBoxCollide(scene, center, NxVec3(5.0f, 5.0f, 5.0f), direction))
//			if(NxSweepPointCollide(scene, center, direction))
//			if(NxSweepSphereCollide(scene, center, 5.0f, direction))
				{
				ViewGC::camera->setPosition(center.x, center.y, center.z);
				}
			}
		else
#endif
			{
			ViewGC::camera->setPosition(camX.x, camX.y, camX.z);
			}

		timeSinceLastInput = 0;
		mouseDxSinceLastInput = 0;
		mouseDySinceLastInput = 0;
		}
	}


