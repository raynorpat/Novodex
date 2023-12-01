/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "Nxf.h"
#include "Exception.h"
namespace NxFoundation
	{

Exception::Exception(NxErrorCode _errorCode, const char * _file, int _line) : errorCode(_errorCode), file(_file), line(_line)
	{
	//nothing
	}

NxErrorCode Exception::getErrorCode()
	{
	return errorCode;
	}

const char * Exception::getFile()
	{
	return file;
	}

int Exception::getLine()
	{
	return line;
	}
	}