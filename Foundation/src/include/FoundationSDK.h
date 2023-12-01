#ifndef NX_FOUNDATION_FOUNDATIONSDK
#define NX_FOUNDATION_FOUNDATIONSDK
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
/**
A define for DLL export tagging.  This is to go only in DLL-side code.
*/
#include "Nxf.h"
#include "NxFoundationSDK.h"
#include "Observable.h"
#include "NxUserAllocatorDefault.h"
#include "CustomAssert.h"
#include <stdio.h>
#include <stdarg.h>
//#include <varargs.h>
//#include "Assert.h"

namespace NxFoundation
	{
class NXF_DLL_EXPORT  FoundationSDK : public Observable, public NxFoundationSDK
	{
	public:
	//NxFoundationSDK:
	virtual	void				release();
	virtual void				setErrorStream(NxUserOutputStream *);
	virtual NxUserOutputStream*	getErrorStream();
	virtual NxUserAllocator&	getAllocator();
	virtual NxProfilingZone*	createProfilingZone(const char * x);
//	virtual NxUserDynamicMesh*	createDefaultDynamicMesh();
	virtual NxDebugRenderable*	createDebugRenderable();
	virtual void				releaseDebugRenderable(NxDebugRenderable*& data);
	virtual void				renderDebugData(const NxUserDebugRenderer& renderer) const;
	virtual NxErrorCode			getLastError();
	virtual NxErrorCode			getFirstError();


	//Observable:
	virtual void event(NxU32 event, Observable &);

	//group access:
	NX_INLINE static FoundationSDK & getInstance();
	//!dbMessage is only executed when NX_USER_DEBUG_MODE is defined.
	NX_INLINE static void dbMessage(NxErrorCode, const char * file, int line, const char * messageFmt, ...);
	//!this is called by NX_ASSERT
	NX_INLINE static bool dbAssert(NxErrorCode, const char * file, int line, bool * ignoreAlways, const char * message);
	/**
	error reporting function. The optional ignore always should be settable by the user for assertions.
	Returns true if an assertion should break.
	*/
	NX_INLINE static bool error(NxErrorCode, const char * file, int line, bool * ignoreAlways, const char * messageFmt, ...);

	//group access variables:
	static NxUserAllocatorDefault defaultSDKAllocator;
	NxUserOutputStream * errorStream; //note: may be 0.

	NxU32	getNewID();
	void	freeID(NxU32 id);

	private:
	//methods
	FoundationSDK();
	~FoundationSDK();
	bool errorImpl(NxErrorCode, const char * file, int line, bool * ignoreAlways, const char * messageFmt, va_list );

	//variables
	NxU32					currentID;
	NxArraySDK<NxU32>		freeIDs;

	bool userHoldsReference;
	NxErrorCode firstError, lastError;

	static FoundationSDK * instance;

	typedef NxArraySDK<NxDebugRenderable*> DebugDataArray;
	DebugDataArray debugDataArray;

	//friends
	friend NxFoundationSDK *	::NxCreateFoundationSDK(NxU32 sdkVersion, NxUserOutputStream * errorStream, NxUserAllocator * allocator);
	};

NX_INLINE FoundationSDK & FoundationSDK::getInstance()
	{
	//NX_ASSERT(instance);	//should only be called by objects we instanced, so our object should also still exist.
	//we can't really fail an assert here because if there is no instance then we can't report an error.
	if (!instance)
		{
		#ifdef WIN32
		_asm { int 3 }
		#elif LINUX
		asm ( "int $3");
		#endif
		}
	return *instance;
	}

NX_INLINE bool FoundationSDK::error(NxErrorCode c, const char * file, int line, bool * ignoreAlways, const char * messageFmt, ...)
	{
	va_list va;
	va_start(va, messageFmt);
	getInstance().errorImpl(c, file, line, ignoreAlways, messageFmt, va);
	va_end(va);

	return false;	// Keep that false
	}

NX_INLINE void FoundationSDK::dbMessage(NxErrorCode c, const char * file, int line, const char * messageFmt, ...)
	{
#ifdef NX_USER_DEBUG_MODE	//this may be defined in the project settings, eventually separately for each SDK unit.
	
	va_list va;
	va_start(va, messageFmt);
	getInstance().errorImpl(c, file, line, 0, messageFmt, va);
	va_end(va); 
#endif
	}

NX_INLINE bool FoundationSDK::dbAssert(NxErrorCode c, const char * file, int line, bool * ignoreAlways, const char * messageFmt)
	{
	if(instance)			//don't assert here because we are called by NX_ASSERT on assertion failure!
		{
		return instance->errorImpl(c, file, line, ignoreAlways, messageFmt, 0);
		}
	else
		{
		return true;
		}
	}

	}

//shortcut macros:
//usage: FoundationSDK::dbMessage(NX_WARN, "static friction %f is is lower than dynamic friction %d", sfr, dfr);
#define NX_WARN NXE_DB_WARNING, __FILE__, __LINE__
#define NX_INFO	NXE_DB_INFO, __FILE__, __LINE__
#define NX_PRINT NXE_DB_PRINT, __FILE__, __LINE__



#endif

