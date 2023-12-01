#ifndef NX_FOUNDATION_EXCEPTION
#define NX_FOUNDATION_EXCEPTION
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include "NxException.h"
#include "NxAllocateable.h"
namespace NxFoundation
	{

class NXF_DLL_EXPORT Exception : public NxAllocateable, public NxException
	{
	NxErrorCode errorCode;
	const char * file;
	int line;
	public:
	Exception(NxErrorCode errorCode, const char * file, int line);
	NxErrorCode getErrorCode();
	const char * getFile();
	int getLine();
	};
	}
#endif