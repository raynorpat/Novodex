/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef FLY_CONTROLLER_H
#define FLY_CONTROLLER_H
#include "ActController.h"
class NxBody;

/**
Controllers are demo-specific behaviors like car driving or shooting gallery.  This is the
sort of thing you would script in a game, but we don't have a scripting language in this
viewer.

This controller is for flying around in the scene.
*/
class FlyController : public ActController
	{
	public:
	FlyController();

	virtual void activate(bool on);
	virtual bool input(char c, bool down);
	virtual void tick(float sec, Act & act, bool paused);
	virtual void mouseDrag(int x, int y, int dx, int dy, int button);

	private:
	float timeSinceLastInput;
	int mouseDxSinceLastInput;
	int mouseDySinceLastInput;

	int mouseDxAvg;
	int mouseDyAvg;

	static int helpIndex;
	static float helpTime;
	static float helpSpeed;
	static char * helpDest;
	static int helpCopySize;
	static char helpText[];
	};

#endif