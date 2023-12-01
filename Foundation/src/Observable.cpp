/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "Nxf.h"
#include "Observable.h"
#include "NxAssert.h"

namespace NxFoundation
	{

void Observable::addObserver(Observer &o)
	{
	observers.push_back(&o);
	}

void Observable::removeObserver(Observer &o)
	{
	unsigned j = observers.size();
	for (unsigned i=0; i<j; i++)
		{
		if (observers[i] == &o)
			{
			if (i < j - 1)	//not last one?
				observers[i] = observers[j-1];
			observers.pop_back();
			
			if (observers.size() == 0)
				event(OE_LAST_OBSERVER_REMOVED, *this);	//don't access this anymore after this call, we may have deleted ourselves.
			return;
			}
		}
	NX_ASSERT(0);	//not found
	}

void Observable::notifyObservers(NxU32 e)
	{
	unsigned j = observers.size();
	for (unsigned i=0; i<j; i++)
		observers[i]->event(e, *this);
	}

unsigned Observable::getNumObservers()
	{
	return observers.size();
	}

void Observable::event(NxU32, Observable &)	//default implementation
	{
	}

	}