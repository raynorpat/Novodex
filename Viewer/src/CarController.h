/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef CAR_CONTROLLER_H
#define CAR_CONTROLLER_H

#include "NxPhysics.h"
#include "3dMath.h"
#include "ActController.h"

class NxActor;

/**
Controllers are demo-specific behaviors like car driving or shooting gallery.  This is the
sort of thing you would script in a game, but we don't have a scripting language in this
viewer.
This controller is for the car in the monster truck demo.  Caution: the below code has
a bunch of parameters which are specific to that particular truck.
*/
class CarController : public ActController
	{
	public:
	CarController(ODBlock & block);
	virtual ~CarController();
	virtual void activate(bool on);
	virtual bool input(char c, bool down);	//returns true if the event was handled.
	virtual void tick(float sec, Act & act, bool paused);
	virtual void mouseDrag(int x, int y, int dx, int dy, int button);

	private:

	//Camera 
	//	{
	//	position { 0.0; 0.0; -15; }
	//	camRise { 5.0; }
	//	speedFOVMin { 45.0; }
	//	speedFOVMax { 80.0; } 
	//	}	
	struct Camera
		{
		Camera(): position(0.0f,0.0f,-20.0f), lookAt(0.0f,0.0f,0.0f), 
				  risePos(15.0f), riseLookAt(2.0f),
				  fixedPos(true), fixedLookAt(true),
				  speedFOVMin(30.0f), speedFOVMax(100.0f) {}
		Camera(ODBlock & block)
			{
			block.getBlockFloats("position",&position.x,3);
			block.getBlockFloats("lookAt",&lookAt.x,3);
			block.getBlockFloat("risePos",&risePos);
			block.getBlockFloat("riseLookAt",&riseLookAt);
			block.getBlockFloat("speedFOVMin",&speedFOVMin);
			block.getBlockFloat("speedFOVMax",&speedFOVMax);
			fixedPos = block.getBlock("fixedPos") != 0;  
			fixedLookAt = block.getBlock("fixedLookAt") != 0;
			}
		Camera(const Camera & c) { copy(c); }
		Camera & operator=(const Camera & c) { copy(c); return *this; }

		bool fixedPos;
		bool fixedLookAt;
		NxVec3 position;
		NxVec3 lookAt;
		float risePos;
		float riseLookAt;
        float speedFOVMin;
		float speedFOVMax;

		private:
	
		void copy(const Camera & c)
			{
			position = c.position;
			lookAt = c.lookAt;
			risePos = c.risePos;
			riseLookAt = c.riseLookAt;
			speedFOVMin = c.speedFOVMin;
			speedFOVMax = c.speedFOVMax;
			fixedPos = c.fixedPos;
			fixedLookAt = c.fixedLookAt;
			}
		};



	bool allCarPartNamesAvailable() const;
	void resetCar();
	void updateCamera(float sec);
	static float feedbackFilter(float val, float& memory, float sharpness);

	enum DriveState { DS_FORWARD, DS_BACKWARD, DS_NEUTRAL };
	enum SteerState { SS_LEFT, SS_RIGHT, SS_NEUTRAL };
	DriveState accState;
	SteerState steerState;
	float driveStateTime;
	float steerStateTime;

	float driveVelocity;
	float steerVelocity;

	//constant params:
	float maxAcc;			//maximum acceleration of the system (any positive number)
	float maxAccTime;		//time it takes for system to reach max acceleration from standing (any positive number)
	float accHoldTime;	//time the maximum acceleration can be maintained after it being reached (any positive number)
	float accStopTime;	//time the system can maintain any acceleration at all after it stopped maintaining max acc (hold time reached) (any positive number)
	float slowdown;


	NxActor * driveWheelL;
	NxActor * driveWheelR;
	NxActor * driveWheelFrontL;
	NxActor * driveWheelFrontR;

	NxActor * steerBodyL;
	NxActor * steerBodyR;

	NxActor * suspFrontL;
	NxActor * suspFrontR;
	NxActor * suspRearL;
	NxActor * suspRearR;

	NxActor * carbody;

	NxRevoluteJoint * hinges[6];
	NxPrismaticJoint * prismatics[4];


	const char * driveWheelLName;
	const char * driveWheelRName;
	const char * driveWheelFrontLName;	//4wd
	const char * driveWheelFrontRName;	//4wd

	const char * steerBodyLName;
	const char * steerBodyRName;

	const char * suspFrontLName;
	const char * suspFrontRName;
	const char * suspRearLName;
	const char * suspRearRName;

	const char * carBodyName;

	const char * hingeNames[6];
	const char * prismNames[4];

	bool error;
	bool loaded;

	unsigned camIndex;
    NxArray<Camera> cameras;
	Quat camDirBackup;
	Vec3 camPosBackup;
	Vec3 oldCameraLookAt;

	float acc(float t);
	};
#endif
