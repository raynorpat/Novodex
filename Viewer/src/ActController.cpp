/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include <ODBlock.h>
#include "ActController.h"
 
#include "CarController.h"
#include "CloneController.h"

#include "PhysicsDemo.h"

extern void Fatal(const char * m);

ActController * ActController::create(ODBlock & block)
	{
	ActController * r = NULL;
	const char * name = NULL;
	const char * type;
	block.getBlockString("Type", &type);

	int d = 0;
	block.getBlockInt("default", &d);

	if (type) 
		{
		if (!strcmp(type, "CarController"))
			{
			r = new CarController(block);
			r->defaultController = (d != 0);
			r->name = name;
			}
		else if (!strcmp(type, "CloneController"))
			{
			r = new CloneController(block);
			r->defaultController = (d != 0);
			r->name = name;
			}
		else 
			{
			char message[255];
			sprintf(message,"Unknown Controller type: %s",type);
			Fatal(message);
			}
		}
	//add other instancers here.

	return r;
	}


bool ActController::input(char c, bool down)
	{
	Act * act = Act::instance();

	//handle escape, which escapes from the current controller:
	if (c == 27 && down)	//escape
		{
		if (!defaultController)
			{
			act->getContainer()->setActController();
			return true;
			}
		else
			return false;
		}

	return false;
	}
