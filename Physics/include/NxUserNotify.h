#ifndef NX_PHYSICS_NXUSERNOTIFY
#define NX_PHYSICS_NXUSERNOTIFY
/*----------------------------------------------------------------------------*\
|
|						Public Interface to NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include "Nxp.h"

/**
 An interface class that the user can implement in order to receive simulation events.
*/
class NxUserNotify
	{
	public:
	/**
	This is called when a breakable joint breaks.
	*/
	virtual void onJointBreak(NxReal breakingForce, NxJoint & brokenJoint) = 0;
	};
#endif