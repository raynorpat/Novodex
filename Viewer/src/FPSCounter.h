#ifndef FPSCOUNTER_H
#define FPSCOUNTER_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

	#include "Nx.h"

	class FPSCounter
	{
		public:
		// Constructor/Destructor
							FPSCounter();
							~FPSCounter();

					void	Update();
		NX_INLINE	NxF32	GetFPS()		const	{ return mFPS;			}
		NX_INLINE	NxF32	GetInstantFPS()	const	{ return mInstantFPS;	}

		private:
					NxF32	mLastTime;
					NxU32	mFrames;
					NxF32	mLastTime2;
					NxU32	mFrames2;
					NxF32	mFPS;
					NxF32	mInstantFPS;
	};

#endif