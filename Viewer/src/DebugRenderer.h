/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef DEBUGRENDER_H
#define DEBUGRENDER_H

#include "NxFoundation.h"

class DebugRenderer : public NxUserDebugRenderer
	{
	public:
	NX_INLINE void setupColor(NxU32 color) const;
	virtual void renderData(const NxDebugRenderable& data) const;
	};

#endif