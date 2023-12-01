#ifndef NX_FOUNDATION_TIME
#define NX_FOUNDATION_TIME
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

/**
Time class.  Not sure how portable this is, but we only need it in the SDK for 
profiling anyway.
TODO: change to 64 bit ints from double.  On the other hand, returning milliseconds
is more intuitive because it doesn't depend on the frequency.

The best thing to do is to track time as 64 bit ticks, but return millisecond double deltas.
*/
namespace NxFoundation
	{

typedef double second;

class NXF_DLL_EXPORT Time
	{
	second  lstTimeS;
	public:
	Time();
	/**
	returns milliseconds elapsed since last call.
	*/
	second GetElapsedSeconds();
	second PeekElapsedSeconds();	 
	private:
	static double GetTimeTicks();
	static second GetClockFrequency();
	};
	}
#endif