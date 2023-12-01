/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "Nxf.h"
#include "FoundationSDK.h"
#include "NxUserOutputStream.h"
#include "NxUserAllocatorDefault.h"
#include "Exception.h"
//#include "DefaultTriangleMesh.h"
#include "NxProfiler.h"
#include "DebugRenderable.h"
#include "NxUserDebugRenderer.h"
#ifdef LINUX
#include <string.h>
#endif

NXF_DLL_EXPORT NxUserAllocator * nxFoundationSDKAllocator = 0;


namespace NxFoundation
	{
NxUserAllocatorDefault FoundationSDK::defaultSDKAllocator;
FoundationSDK * FoundationSDK::instance = 0;


FoundationSDK::FoundationSDK():
	currentID	(0)
	{
	errorStream = 0;
	userHoldsReference = true;
	firstError = lastError = NXE_NO_ERROR;
	}

FoundationSDK::~FoundationSDK()
	{
	// Delete debug data
	unsigned nbDebugData = debugDataArray.size();
	for(unsigned i=0;i<nbDebugData;i++)
		{
		delete debugDataArray[i];
		}
	}

void FoundationSDK::release()
	{
	userHoldsReference = false;
	if (getNumObservers() == 0)	//our observers are the derived SDKs, which need to be freed first.
		{
		delete instance;
		instance = 0;
		}
	}

void FoundationSDK::setErrorStream(NxUserOutputStream * stream)
	{
	errorStream = stream;
	}

NxUserOutputStream * FoundationSDK::getErrorStream()
	{
	return errorStream;
	}
NxUserAllocator & FoundationSDK::getAllocator()
	{
	return *nxFoundationSDKAllocator;
	}

NxProfilingZone * FoundationSDK::createProfilingZone(const char * x)
	{
	return NX_NEW NxProfiler::DefineZone(x);
	}
/* obsolete
NxUserDynamicMesh * FoundationSDK::createDefaultDynamicMesh()
	{
	return NX_NEW DefaultTriangleMesh();
	}
*/
NxDebugRenderable* FoundationSDK::createDebugRenderable()
	{
	NxDebugRenderable* data = NX_NEW DebugRenderable();
	if(!data)	return NULL;

	debugDataArray.pushBack(data);
	return data;
	}

void FoundationSDK::releaseDebugRenderable(NxDebugRenderable*& data)
	{
	if(data)
		{
		DebugRenderable * d = static_cast<DebugRenderable *>(data);

		// Find in the array
		unsigned nbDebugData = debugDataArray.size();
		for(unsigned i=0;i<nbDebugData;i++)
			{
			if(debugDataArray[i]==data)
				{
				// Found at location i
				debugDataArray.replaceWithLast(i);
				delete d;
				data = NULL;
				return;
				}
			}
		}
	}

void FoundationSDK::renderDebugData(const NxUserDebugRenderer& renderer) const
	{
	unsigned nbDebugData = debugDataArray.size();
	for(unsigned i=0;i<nbDebugData;i++)
		{
		renderer.renderData(*debugDataArray[i]);
		}
	}

void FoundationSDK::event(NxU32 e, Observable &)
	{
	if (e == OE_LAST_OBSERVER_REMOVED)
		if (!userHoldsReference)
			{
			//the user no longer holds a reference, and now the SDKs don't either.
			delete instance;
			instance = 0;
			}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NxU32 FoundationSDK::getNewID()
	{
	// If recycled IDs are available, use them
	NxU32 size = freeIDs.size();
	if(size)
		{
		NxU32 id = freeIDs[size-1];	// Recycle last ID
		freeIDs.popBack();
		return id;
		}
	// Else create a new ID
	return currentID++;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FoundationSDK::freeID(NxU32 id)
	{
	// Allocate on first call
	// Add released ID to the array of free IDs
	freeIDs.pushBack(id);
	}


NxErrorCode FoundationSDK::getLastError()
	{
	NxErrorCode e = lastError;
	lastError = NXE_NO_ERROR;
	return e;
	}

NxErrorCode FoundationSDK::getFirstError()
	{
	NxErrorCode e = firstError;
	firstError = NXE_NO_ERROR;
	return e;
	}


bool FoundationSDK::errorImpl(NxErrorCode e, const char *file, int line, bool * ignoreAlways, const char * messageFmt, va_list va)
	{
	//set the first and last error codes:
	lastError = e;
	if (firstError == NXE_NO_ERROR)
		firstError = e;


	if (errorStream)
		{
		if (e == NXE_ASSERTION)
			{
			switch (errorStream->reportAssertViolation(messageFmt, file, line))
				{
				case NX_AR_BREAKPOINT:
					return true;
				case NX_AR_IGNORE:
					*ignoreAlways = true;
					//no break on purpose!
				case NX_AR_CONTINUE:
				default:
					return false;
				}
			}
		else
			{

			//create a string from the format strings and the optional params
			// We draw the line at a 1MB string.
			const int maxSize = 1000000;

			// If the string is less than 161 characters,
			// allocate it on the stack because this saves
			// the malloc/free time.
			const int bufSize = 161;
			char stackBuffer[bufSize];
			char* stringBuffer = stackBuffer;
			int attemptedSize = bufSize - 1;
			#ifdef WIN32
			int numChars = _vsnprintf(stackBuffer, attemptedSize, messageFmt, va);	//TODO: this is non portable!?
			#else
				int numChars = vsnprintf(stackBuffer, attemptedSize, messageFmt, va);
			#endif
			if(numChars == -1)
				{
				// Now use the heap.
				
				while((numChars == -1) && (attemptedSize < maxSize))
					{
					// Try a bigger size
					attemptedSize *= 2;
					stringBuffer = (char*)nxFoundationSDKAllocator->realloc(stringBuffer, attemptedSize + 1);
			#ifdef WIN32
					numChars = _vsnprintf(stringBuffer, attemptedSize, messageFmt, va);
			#else
					numChars = vsnprintf(stringBuffer, attemptedSize, messageFmt, va);
			#endif
					}
				}
			//we should now have the string in stringBuffer; if not, then there isn't much we can do.

			if (e == NXE_DB_PRINT)
				errorStream->print(stringBuffer);
			else
				errorStream->reportError(e, stringBuffer, file, line);
			}
		}


	//do we have exceptions support?	TODO: MS specific define -- leave it or use our own??
#if 0 // temporarily commented out by JWR (TODO TODO TODO)
#ifdef _CPPUNWIND	
	if (e < 100)
		throw new Exception(e,file,line);	//no operators yet!
#endif
#endif


	return false;
	}
}

NxFoundationSDK *	NxCreateFoundationSDK(NxU32 sdkVersion,NxUserOutputStream * es, NxUserAllocator * allocator)
	{
	if (sdkVersion != NX_FOUNDATION_SDK_VERSION)
		{
		return 0;
		}

	if (!NxFoundation::FoundationSDK::instance)
		{
		//init static fields here:
		if (allocator)
			nxFoundationSDKAllocator = allocator;
		else  
			nxFoundationSDKAllocator = &NxFoundation::FoundationSDK::defaultSDKAllocator;	//gotta assign before new is called.

		NxFoundation::FoundationSDK::instance = NX_NEW NxFoundation::FoundationSDK();
		}
	NxFoundation::FoundationSDK::instance->setErrorStream(es);

	NxFoundation::FoundationSDK::instance->userHoldsReference = true;	//user has a reference again
	return NxFoundation::FoundationSDK::instance;
	}




#ifdef DEBUG_DETERMINISM

#include "CustomArray.h"
static CustomArray* inRecorder=NULL;
static CustomArray* outRecorder=NULL;

void initRecorder(const char* filename)
	{
	NX_DELETE_SINGLE(inRecorder);
	NX_DELETE_SINGLE(outRecorder);

	if(!inRecorder && filename)
		inRecorder = new CustomArray(filename);

	if(!outRecorder)
		outRecorder = new CustomArray;
	}

void closeRecorder(const char* filename)
	{
	if(!inRecorder && outRecorder)
		{
		outRecorder->ExportToDisk(filename);
		exit(0);
		}
	NX_DELETE_SINGLE(inRecorder);
	NX_DELETE_SINGLE(outRecorder);
	}

void recordString(const char* str)
	{
	if(!str)	return;
	if(inRecorder)
		{
		const char* expectedString = (const char*)inRecorder->GetString();
		NX_ASSERT(strcmp(expectedString, str)==0);
		}

	if(outRecorder)	outRecorder->Store(str).Store(NxU8(0));
	}

void recordFloat(float f)
	{
	if(inRecorder)
		{
		NxU32 expectedBinaryFloat = inRecorder->GetDword();
		NxF32 ff = (NxF32&)(expectedBinaryFloat);
		NX_ASSERT(NX_IR(f)==expectedBinaryFloat);
		}

	if(outRecorder)	outRecorder->Store(f);
	}

void recordDword(int d)
	{
	if(inRecorder)
		{
		int expectedBinary = inRecorder->GetDword();
		NX_ASSERT(d==expectedBinary);
		}

	if(outRecorder)	outRecorder->Store(d);
	}

void recordPos(const NxVec3& p)
	{
	if(inRecorder)
		{
		NxU32 expectedBinaryX = inRecorder->GetDword();
		NxU32 expectedBinaryY = inRecorder->GetDword();
		NxU32 expectedBinaryZ = inRecorder->GetDword();
		NxF32 fx = (NxF32&)(expectedBinaryX);
		NxF32 fy = (NxF32&)(expectedBinaryY);
		NxF32 fz = (NxF32&)(expectedBinaryZ);
		NX_ASSERT(NX_IR(p.x)==expectedBinaryX);
		NX_ASSERT(NX_IR(p.y)==expectedBinaryY);
		NX_ASSERT(NX_IR(p.z)==expectedBinaryZ);
		}

	if(outRecorder)	outRecorder->Store(p.x).Store(p.y).Store(p.z);
	}

void recordQuat(const NxQuat& q)
	{
	if(inRecorder)
		{
		NxU32 expectedBinaryX = inRecorder->GetDword();
		NxU32 expectedBinaryY = inRecorder->GetDword();
		NxU32 expectedBinaryZ = inRecorder->GetDword();
		NxU32 expectedBinaryW = inRecorder->GetDword();
		float fx = q.x();
		float fy = q.y();
		float fz = q.z();
		float fw = q.w();
		NX_ASSERT(NX_IR(fx)==expectedBinaryX);
		NX_ASSERT(NX_IR(fy)==expectedBinaryY);
		NX_ASSERT(NX_IR(fz)==expectedBinaryZ);
		NX_ASSERT(NX_IR(fw)==expectedBinaryW);
		}

	if(outRecorder)	outRecorder->Store(q.x()).Store(q.y()).Store(q.z()).Store(q.w());
	}

void recordMatrix(const NxMat33& o)
	{
	if(inRecorder)
		{
		NxU32 expectedBinary00 = inRecorder->GetDword();
		NxU32 expectedBinary01 = inRecorder->GetDword();
		NxU32 expectedBinary02 = inRecorder->GetDword();
		NxU32 expectedBinary10 = inRecorder->GetDword();
		NxU32 expectedBinary11 = inRecorder->GetDword();
		NxU32 expectedBinary12 = inRecorder->GetDword();
		NxU32 expectedBinary20 = inRecorder->GetDword();
		NxU32 expectedBinary21 = inRecorder->GetDword();
		NxU32 expectedBinary22 = inRecorder->GetDword();
		NxF32 f00 = (NxF32&)(expectedBinary00);
		NxF32 f01 = (NxF32&)(expectedBinary01);
		NxF32 f02 = (NxF32&)(expectedBinary02);
		NxF32 f10 = (NxF32&)(expectedBinary10);
		NxF32 f11 = (NxF32&)(expectedBinary11);
		NxF32 f12 = (NxF32&)(expectedBinary12);
		NxF32 f20 = (NxF32&)(expectedBinary20);
		NxF32 f21 = (NxF32&)(expectedBinary21);
		NxF32 f22 = (NxF32&)(expectedBinary22);
		NX_ASSERT(NX_IR(o(0,0))==expectedBinary00);
		NX_ASSERT(NX_IR(o(0,1))==expectedBinary01);
		NX_ASSERT(NX_IR(o(0,2))==expectedBinary02);
		NX_ASSERT(NX_IR(o(1,0))==expectedBinary10);
		NX_ASSERT(NX_IR(o(1,1))==expectedBinary11);
		NX_ASSERT(NX_IR(o(1,2))==expectedBinary12);
		NX_ASSERT(NX_IR(o(2,0))==expectedBinary20);
		NX_ASSERT(NX_IR(o(2,1))==expectedBinary21);
		NX_ASSERT(NX_IR(o(2,2))==expectedBinary22);
		}

	if(outRecorder)	outRecorder
		->	Store(o(0,0))
			.Store(o(0,1))
			.Store(o(0,2))
			.Store(o(1,0))
			.Store(o(1,1))
			.Store(o(1,2))
			.Store(o(2,0))
			.Store(o(2,1))
			.Store(o(2,2));
	}
#endif
