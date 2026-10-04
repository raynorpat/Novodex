/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ODBlock.h"
#include "CarController.h"
#include "Act.h"
#include "ActJoint.h"
#include "ViewerGraphicsContext.h"
void appRenderTargSized(unsigned ww,unsigned hh);

#include <Graphics.h>

#undef SS_LEFT
#undef SS_RIGHT
extern bool keyDown[256];	//ViewerPlatform.cpp

extern char modeString[32];	//TODO: move to base class...
extern char helpString[32];

namespace 
	{
	char helpText[] = "- arrow keys to drive\n- ESC to leave car\n- 'K' changes camera modes\n- 'R' resets flipped car\n- '5' to go FAST.";
	}; 

CarController::CarController(ODBlock & block)
	{
	camIndex = 0;
	error = false;
	loaded = false;

	//defaults:
	maxAcc = 0.1f;		//maximum acceleration of the system (any positive number)
	maxAccTime = 0.4f;	//time it takes for system to reach max acceleration from standing (any positive number)
	accHoldTime = 2.0f;	//time the maximum acceleration can be maintained after it being reached (any positive number)
	accStopTime = 0.5f;	//time the system can maintain any acceleration at all after it stopped maintaining max acc (hold time reached) (any positive number)
	slowdown = 0.1f;
	oldCameraLookAt.Set(0.0f,0.0f,0.0f);


	block.getBlockFloat("driveMaxAcc", &maxAcc);
	block.getBlockFloat("driveMaxAccTime", &maxAccTime);
	block.getBlockFloat("driveAccHoldTime", &accHoldTime);
	block.getBlockFloat("driveAccStopTime", &accStopTime);
	block.getBlockFloat("driveDrag", &slowdown);

	ODBlock * camerasBlock = block.getBlock("Cameras");
	if (camerasBlock) 
		{
		camerasBlock->reset();
		while(camerasBlock->moreSubBlocks())
			{
			ODBlock * cameraBlock = camerasBlock->nextSubBlock();
			if (cameraBlock)
				cameras.pushBack(Camera(*cameraBlock));
			}
		}
	else cameras.pushBack(Camera());


	driveWheelLName=NULL;
	driveWheelRName=NULL;
	steerBodyLName=NULL;
	steerBodyRName=NULL;
	carBodyName=NULL;
	driveWheelFrontLName = NULL;
	driveWheelFrontRName = NULL;
	suspFrontLName = NULL;
	suspFrontRName = NULL;
	suspRearLName = NULL;
	suspRearRName = NULL;

	driveWheelL = NULL;
	driveWheelR = NULL;
	driveWheelFrontL = NULL;
	driveWheelFrontR = NULL;
	carbody = NULL;

	steerBodyL = NULL;
	steerBodyR = NULL;

	suspFrontL = NULL;
	suspFrontR = NULL;
	suspRearL = NULL;
	suspRearR = NULL;

	unsigned i;
	for (i=0; i < 6; i++)
		hingeNames[i] = 0;

	for (i=0; i < 4; i++)
		prismNames[i] = 0;

	block.getBlockString("driveWheelL", &driveWheelLName);
	block.getBlockString("driveWheelR", &driveWheelRName);

	block.getBlockString("driveWheelFL", &driveWheelFrontLName);
	block.getBlockString("driveWheelFR", &driveWheelFrontRName);


	block.getBlockString("steerBodyL", &steerBodyLName);
	block.getBlockString("steerBodyR", &steerBodyRName);
	block.getBlockString("carBody", &carBodyName);

	block.getBlockString("frontSuspBarL", &suspFrontLName);
	block.getBlockString("frontSuspBarR", &suspFrontRName);
	block.getBlockString("rearSuspBarL", &suspRearLName);
	block.getBlockString("rearSuspBarR", &suspRearRName);

	block.getBlockString("frontSuspBarL", &suspFrontLName);
	block.getBlockString("frontSuspBarR", &suspFrontRName);
	block.getBlockString("rearSuspBarL", &suspRearLName);
	block.getBlockString("rearSuspBarR", &suspRearRName);

	block.getBlockString("hinge1", &hingeNames[0]);
	block.getBlockString("hinge2", &hingeNames[1]);
	block.getBlockString("hinge3", &hingeNames[2]);
	block.getBlockString("hinge4", &hingeNames[3]);
	block.getBlockString("hinge5", &hingeNames[4]);
	block.getBlockString("hinge6", &hingeNames[5]);

	block.getBlockString("prism1", &prismNames[0]);
	block.getBlockString("prism2", &prismNames[1]);
	block.getBlockString("prism3", &prismNames[2]);
	block.getBlockString("prism4", &prismNames[3]);

	if ( !allCarPartNamesAvailable() ) error = true;

	accState = DS_NEUTRAL;
	steerState = SS_NEUTRAL;
	driveStateTime = 0;
	steerStateTime = 0;

	driveVelocity = 0;
	steerVelocity = 0;
	}

CarController::~CarController()
	{
	//nothing
	}

bool CarController::input(char c, bool down)
	{

	//do the common input handling first
	if (ActController::input(c, down)) return true;

	if (down)
	switch (c)
		{
		case 'k':
			//cycle through cam modes:
			if (++camIndex == cameras.size()) camIndex = 0;
			break;
		case 'r':
			resetCar();
			break;
		break;
		}
	return false;
	}

float CarController::acc(float t)
	{
	assert(t >= 0);

	if (t < maxAccTime) 
		return maxAcc*t/maxAccTime;
	else if (t < (maxAccTime + accHoldTime))
		return maxAcc;
	else if (t < (maxAccTime + accHoldTime + accStopTime))
		return maxAcc - maxAcc*(t - (maxAccTime + accHoldTime)) / accStopTime;
	else
		return 0;
	}

void CarController::activate(bool on)
	{
	if (on)
		{
		loaded = false;
		camDirBackup = ViewGC::camera->qOrientation;
		camPosBackup = ViewGC::camera->position;
		sprintf(modeString, "Mode: Drive");
		sprintf(helpString, helpText);
		}
	else
		{
		//put back the camera the way we found it, so that the fly euler angles will still be valid:
		ViewGC::camera->setPosition(camPosBackup.x, camPosBackup.y, camPosBackup.z);
		ViewGC::camera->setOrientation(&camDirBackup);
		}
	}

void CarController::mouseDrag(int x, int y, int dx, int dy, int button)
	{
	}

void CarController::tick(float sec, Act & act, bool paused)
	{
	if (paused)
		return;
		int speed = 0;
		
		if (carbody)
			{
			NxVec3 carvel = carbody->getLinearVelocity();
			speed = (int)(carvel.magnitude() * 3.6f);
			}
		sprintf(modeString, "Mode: Drive at %d km/h", speed);

	if (!loaded)
		{
		if (!error)
			{
			driveWheelL = act.findBody(driveWheelLName);
			driveWheelR = act.findBody(driveWheelRName);
			driveWheelFrontL = act.findBody(driveWheelFrontLName);
			driveWheelFrontR = act.findBody(driveWheelFrontRName);
			steerBodyL = act.findBody(steerBodyLName);
			steerBodyR = act.findBody(steerBodyRName);
			suspFrontL = act.findBody(suspFrontLName);
			suspFrontR = act.findBody(suspFrontRName);
			suspRearL = act.findBody(suspRearLName);
			suspRearR = act.findBody(suspRearRName);
			carbody = act.findBody(carBodyName);

			unsigned i;
			for (i=0; i < 6; i++)
				{
				ActJoint * aj = act.findActJoint(hingeNames[i]);
				NxJoint * j = aj->getJoint();
				hinges[i] = j ? j->isRevoluteJoint() : 0;
				}

			for (i=0; i < 4; i++)
				{
				ActJoint * aj = act.findActJoint(prismNames[i]);
				NxJoint * j = aj->getJoint();
				prismatics[i] = j ? j->isPrismaticJoint() : 0;
				}

			if (!carbody)
				{
				error = true;
				return;
				}
			loaded = true;
			}
		else
			return;
		}

	assert(carbody);

	//camera
	updateCamera(sec);

	bool forwardDown = keyDown['w'] || keyDown[KEY_UP];
	bool backDown = keyDown['s'] || keyDown[KEY_DOWN];

	bool leftDown = keyDown['a'] || keyDown[KEY_LEFT];
	bool rightDown = keyDown['d'] || keyDown[KEY_RIGHT];

	//engine:
	DriveState oldAccState = accState;

	if (forwardDown && !backDown)
		accState = DS_FORWARD;
	else if (!forwardDown && backDown)
		accState = DS_BACKWARD;
	else
		accState = DS_NEUTRAL;

	if (oldAccState != accState)	//state change
		driveStateTime = 0;
	else
		driveStateTime += sec;	//time we're already in this state


	//steering stuff:
	SteerState oldSteerState = steerState;

	if (leftDown && !rightDown)
		steerState = SS_LEFT;
	else if (!leftDown && rightDown)
		steerState = SS_RIGHT;
	else
		steerState = SS_NEUTRAL;

	if (oldSteerState != steerState)	//state change
		{
		steerStateTime = 0;
		}
	else
		steerStateTime += sec;	//time we're already in this state

	switch (accState)
		{
		case DS_FORWARD:
			if (driveVelocity < 0)	//backward vel stops immediately
				driveVelocity = 0;
			
			driveVelocity	= 	acc(driveStateTime);
			break;
		case DS_BACKWARD:
			if (driveVelocity > 0)	//backward vel stops immediately
				driveVelocity = 0;
		
			driveVelocity = - acc(driveStateTime);
			break;
		default: //DS_NEUTRAL
			//stop right away when key released
			//slowdown due to air resistance or friction or whatever.  
			//This makes for a discontinuous acceleration, but is simple.
			driveVelocity = 0;
		}

	switch (steerState)
		{
		case SS_LEFT:
			steerVelocity = -1;
		break;
		case SS_RIGHT:
			steerVelocity = 1;
		break;
		default:
			steerVelocity = 0;
		}

	const NxReal steerSpeed = 4.0f;	//2.0f;
	const NxReal maxSteerAngle = 33.0f;
	const NxReal sangle = NxMath::degToRad(NxMath::min(steerStateTime*steerSpeed, 1.0f) * steerVelocity * maxSteerAngle);

	if (hinges[4])		
		{
		//printf("currAng: %f target: %f\n", NxMath::radToDeg(hinges[4]->getAngle()), steerVelocity);
		hinges[4]->setSpring(NxSpringDesc(10000, 500, sangle));
		}

	if (hinges[5])		
		{
		hinges[5]->setSpring(NxSpringDesc(10000, 500, sangle));
		}

//	if (hinges[0])
//		printf("torque: %f vel: %f\n", driveVelocity, hinges[0]->getVelocity());

	const NxReal absMaxVel = 60.0f;//was 20
	const NxReal motorfek = 15.0f;
	//no motorfek in the rear so that we don't flip over at jump touchdown
	const NxReal damping = driveVelocity == 0 ? motorfek : 0;
	const NxReal maxvel = NxMath::sign(driveVelocity) * absMaxVel;
	//rear wheel drive:
	if (hinges[0])		
		{
		hinges[0]->setMotor(NxMotorDesc(maxvel, NxMath::abs(driveVelocity), false));
		hinges[0]->setSpring(NxSpringDesc(0, damping, 0));
		}
	if (hinges[1])		
		{
		hinges[1]->setMotor(NxMotorDesc(maxvel, NxMath::abs(driveVelocity), false));
		hinges[1]->setSpring(NxSpringDesc(0, damping, 0));
		}
	//front wheels:
	if (hinges[2])		
		{
		hinges[2]->setMotor(NxMotorDesc(maxvel, NxMath::abs(driveVelocity), false));
		hinges[2]->setSpring(NxSpringDesc(0, damping, 0));	
		}
	if (hinges[3])		
		{
		hinges[3]->setMotor(NxMotorDesc(maxvel, NxMath::abs(driveVelocity), false));
		hinges[3]->setSpring(NxSpringDesc(0, damping, 0));
		}

	// speed-dependent fov
	NxF32 speedCoeff = driveVelocity / absMaxVel;
	NxF32 fov;

	static const float fovMin = 50.0f;
	static const float fovMax = 90.0f;
	if(speedCoeff<0.0f)	fov = fovMin;
	else
		{
		speedCoeff /= 0.75f;
		fov = fovMin*(1.0f-speedCoeff) + fovMax*speedCoeff;
		}

	static float memory=60.0f;
	ViewGC::fov = feedbackFilter(fov, memory, 0.02f);
	appRenderTargSized(0,0);
	}

float CarController::feedbackFilter(float val, float& memory, float sharpness)
	{
	if		  (sharpness<0.0f)	sharpness = 0.0f;
	else	if(sharpness>1.0f)	sharpness = 1.0f;
	return memory = val * sharpness + memory * (1.0f - sharpness);
	}

bool CarController::allCarPartNamesAvailable() const
	{	
	if (!(	driveWheelLName			&&	
			driveWheelRName			&&
			driveWheelFrontLName	&&
			driveWheelFrontRName	&&
			steerBodyLName			&&
			steerBodyRName			&&
			carBodyName				&&
			suspFrontLName			&&
			suspFrontRName			&&
			suspRearLName			&&
			suspRearRName))
		{
		printf("CarController Error: Mandatory body references not found!\n");
		return false;
		}

	unsigned i;	
	for (i=0; i < 6; i++)
		if (!hingeNames[i])
			{
			printf("CarController Error: Mandatory joint references not found!\n");
			return false;
			}

	for (i=0; i < 4; i++)
		if (!prismNames[i])
			{
			printf("CarController Error: Mandatory joint references not found!\n");
			return false;
			}

	return true;
	}

/**
This method resets the car to its original position. At the moment the pose of the car is hardwired.
*/
void CarController::resetCar()
	{
	accState = DS_NEUTRAL;
	steerState = SS_NEUTRAL;
	driveStateTime = 0;
	steerStateTime = 0;

	driveVelocity = 0;
	steerVelocity = 0;

	if (driveWheelL && driveWheelR && driveWheelFrontL && driveWheelFrontR && carbody && 
		suspFrontL && suspFrontR && suspRearL && suspRearR && steerBodyL && steerBodyR	)
		{
		NxQuat tq;
		NxQuat zquat;
		zquat.zero();
		NxVec3 zero, z(0,0,1), y(0,1,0);
		zero.zero();
		const NxReal height = NxReal(15.0) ;
		// Force and torque accumulators are reset by this SDK every timestep;
		// the old setForce/setTorque API is no longer exposed. 
		driveWheelL->setGlobalPosition(NxVec3( NxReal(-1.707297), NxReal(1.058553)		+height, NxReal(-2.230793) ));
		driveWheelL->setGlobalOrientationQuat(zquat);
		driveWheelL->setLinearMomentum(zero);
		driveWheelL->setAngularMomentum(zero);

		driveWheelR->setGlobalPosition(NxVec3(NxReal(-1.707300), NxReal(1.058561)		+height, NxReal(1.546263) ));
		driveWheelR->setGlobalOrientationQuat(zquat);
		driveWheelR->setLinearMomentum(zero);
		driveWheelR->setAngularMomentum(zero);

		driveWheelFrontL->setGlobalPosition(NxVec3( NxReal(1.735397), NxReal(1.058553)		+height, NxReal(-2.230793) ));
		driveWheelFrontL->setGlobalOrientationQuat(zquat);
		driveWheelFrontL->setLinearMomentum(zero);
		driveWheelFrontL->setAngularMomentum(zero);

		driveWheelFrontR->setGlobalPosition(NxVec3( NxReal(1.735394), NxReal(1.058559)		+height, NxReal(1.546264) ));
		driveWheelFrontR->setGlobalOrientationQuat(zquat);
		driveWheelFrontR->setLinearMomentum(zero);
		driveWheelFrontR->setAngularMomentum(zero);

		suspFrontL->setGlobalPosition(NxVec3( NxReal(1.735397), NxReal(1.058553)		+height, NxReal(-2.230793)));
		tq.setWXYZ( NxReal(0.477713), NxReal(-0.477712), NxReal(0.521333), NxReal(-0.521338));
		suspFrontL->setGlobalOrientationQuat(tq);
		suspFrontL->setLinearMomentum(zero);
		suspFrontL->setAngularMomentum(zero);

		suspFrontR->setGlobalPosition(NxVec3( NxReal(1.735394), NxReal(1.058559)		+height, NxReal(1.546264) ));
		tq.setWXYZ(NxReal(-0.521335), NxReal(0.521335), NxReal(0.477712), NxReal(-0.477714) );
		suspFrontR->setGlobalOrientationQuat(tq); 
		suspFrontR->setLinearMomentum(zero);
		suspFrontR->setAngularMomentum(zero);

		suspRearL->setGlobalPosition(NxVec3(NxReal(-1.666163), NxReal(1.055847)		+height, NxReal(-1.490285) ));
		tq.setWXYZ(NxReal(0.477713), NxReal(-0.477712), NxReal(0.521333), NxReal(-0.521338) );
		suspRearL->setGlobalOrientationQuat(tq); 
		suspRearL->setLinearMomentum(zero);
		suspRearL->setAngularMomentum(zero);

		suspRearR->setGlobalPosition(NxVec3( NxReal(-1.666164), NxReal(1.055850)		+height, NxReal(0.805752) ));
		tq.setWXYZ(NxReal(-0.521335), NxReal(0.521335), NxReal(0.477712), NxReal(-0.477714));
		suspRearR->setGlobalOrientationQuat(tq); 
		suspRearR->setLinearMomentum(zero);
		suspRearR->setAngularMomentum(zero);

		carbody->setGlobalPosition(NxVec3( NxReal(-0.090812), NxReal(0.119964)		+height, NxReal(-0.342994) ));
		tq.setWXYZ(NxReal(-0.707107), NxReal(0.000000), NxReal(-0.707107), NxReal(0.000000) );
		carbody->setGlobalOrientationQuat(tq); 
		carbody->setLinearMomentum(zero);
		carbody->setAngularMomentum(zero);

		steerBodyL->setGlobalPosition(NxVec3( NxReal(1.735397), NxReal(1.058553)		+height, NxReal(-2.230793) ));
		steerBodyL->setGlobalOrientationQuat(zquat); 
		steerBodyL->setLinearMomentum(zero);
		steerBodyL->setAngularMomentum(zero);

		steerBodyR->setGlobalPosition(NxVec3( NxReal(1.735394), NxReal(1.058559)		+height, NxReal(1.546264) ));
		steerBodyR->setGlobalOrientationQuat(zquat); 
		steerBodyR->setLinearMomentum(zero);
		steerBodyR->setAngularMomentum(zero);

		//ok, do the same to the joints:
		if (hinges[0])
			{
			hinges[0]->getJoint().setGlobalAnchor(NxVec3(NxReal(1.735397), NxReal(1.058553)+height, NxReal(-2.230793)));
			hinges[0]->getJoint().setGlobalAxis(z);//axlefrontl
			}

		if (hinges[1])
			{
			hinges[1]->getJoint().setGlobalAnchor(NxVec3(NxReal(1.735394), NxReal(1.058559)+height, NxReal(1.546264)));
			hinges[1]->getJoint().setGlobalAxis(z);//axlefrontr
			}

		if (hinges[2])
			{
			hinges[2]->getJoint().setGlobalAnchor(NxVec3(NxReal(-1.707297), NxReal(1.058553)+height, NxReal(-2.230793)));
			hinges[2]->getJoint().setGlobalAxis(z);//axleRearL
			}

		if (hinges[3])
			{
			hinges[3]->getJoint().setGlobalAnchor(NxVec3(NxReal(-1.707300), NxReal(1.058561)+height, NxReal(1.546263)));
			hinges[3]->getJoint().setGlobalAxis(z);//axleRearR
			}

		if (hinges[4])
			{
			hinges[4]->getJoint().setGlobalAnchor(NxVec3(NxReal(1.735397), NxReal(1.058553)+height, NxReal(-2.230793)));
			hinges[4]->getJoint().setGlobalAxis(y);//steeringl
			}

		if (hinges[5])
			{
			hinges[5]->getJoint().setGlobalAnchor(NxVec3(NxReal(1.735394), NxReal(1.058559)+height, NxReal(1.546264)));
			hinges[5]->getJoint().setGlobalAxis(y);//steeringr
			}


		if (prismatics[0])
			{
			prismatics[0]->getJoint().setGlobalAnchor(NxVec3(NxReal(1.735397), NxReal(1.058553)+height, NxReal(-2.230793)));
			prismatics[0]->getJoint().setGlobalAxis(y);//sprintFrontL
			}

		if (prismatics[1])
			{
			prismatics[1]->getJoint().setGlobalAnchor(NxVec3(NxReal(1.735394), NxReal(1.058559)+height, NxReal(1.546264)));
			prismatics[1]->getJoint().setGlobalAxis(y);//sprintFrontR
			}

		if (prismatics[2])
			{
			prismatics[2]->getJoint().setGlobalAnchor(NxVec3(NxReal(-1.707297), NxReal(1.058553)+height, NxReal(-2.230793)));
			prismatics[2]->getJoint().setGlobalAxis(y);//springRearL
			}

		if (prismatics[3])
			{
			prismatics[3]->getJoint().setGlobalAnchor(NxVec3(NxReal(-1.707300), NxReal(1.058561)+height, NxReal(1.546263)));
			prismatics[3]->getJoint().setGlobalAxis(y);//springRearR
			}
		}
	}


void CarController::updateCamera(float sec)
	{
	Camera & cam = cameras[camIndex];

	assert(carbody);
	NxVec3 carBodyPos = carbody->getGlobalPosition();

	NxVec3 cameraPos;
	NxVec3 cameraLookAtPos;

	//set camera position
	cameraPos = carbody->getGlobalPoseReference().M * cam.position;
	cameraPos.set(	carBodyPos.x + cameraPos.x, 
					carBodyPos.y + cam.risePos,
					carBodyPos.z + cameraPos.z);

	//set camera lookatpoint
	cameraLookAtPos = carbody->getGlobalPoseReference().M * cam.lookAt;
	cameraLookAtPos.set(	carBodyPos.x + cameraLookAtPos.x, 
				        	carBodyPos.y + cam.riseLookAt,
					        carBodyPos.z + cameraLookAtPos.z);

	float t = 0.7f * sec * 10.0f;
	if (cam.fixedPos)
		ViewGC::camera->setPosition(cameraPos.x, cameraPos.y, cameraPos.z);
	else
		{
		Vec3 tmp(cameraPos.x, cameraPos.y, cameraPos.z);
		Vec3 camPos = ViewGC::camera->position * (1 - t) + tmp * t;
		ViewGC::camera->setPosition(&camPos);
		}

	Vec3 up(0,1,0);
	Vec3 lookAt;
	if (cam.fixedLookAt)
		lookAt.Set(cameraLookAtPos.x,cameraLookAtPos.y,cameraLookAtPos.z);
	else
		{
		Vec3 tmp(cameraLookAtPos.x, cameraLookAtPos.y, cameraLookAtPos.z);
		lookAt = oldCameraLookAt * (1 - t) + tmp * t;
		}
	ViewGC::camera->lookAt(&lookAt, &up);
	oldCameraLookAt.Set(cameraLookAtPos.x,cameraLookAtPos.y,cameraLookAtPos.z);
	}
