/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef ACT_CONTROLLER_H
#define ACT_CONTROLLER_H

class ODBlock;
class Act;

/**
Controllers are demo-specific behaviors like car driving or shooting gallery.  This is the
sort of thing you would script in a game, but we don't have a scripting language in this
viewer.

ActController is the base class for all controllers.
*/
class ActController
	{
	public:
	static ActController * create(ODBlock & block);
	virtual ~ActController() {} 
	virtual void activate(bool on) = 0;
	virtual bool input(char c, bool down);	//returns true if the event was handled.
	virtual void tick(float sec, Act & act, bool paused) = 0;
	virtual void mouseDrag(int x, int y, int dx, int dy, int button) = 0;

	ActController * next;
	bool defaultController;
	const char * name;
	protected:
	ActController() { next = 0; defaultController = true; }
	};

// Viewer input codes (translated from GLFW in ViewerPlatform.cpp):
#define KEY_ENTER			13
#define KEY_LEFT			20
#define KEY_UP				21
#define KEY_RIGHT			22
#define KEY_DOWN			23
#define KEY_PAGE_UP			24
#define KEY_PAGE_DOWN		25
#define KEY_ESCAPE			27
#define KEY_SPACE			32

#endif