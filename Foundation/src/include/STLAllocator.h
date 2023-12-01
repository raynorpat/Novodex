#ifndef NX_FOUNDATION_STLAllocator
#define NX_FOUNDATION_STLAllocator
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include <xmemory>

#include "NxUserAllocator.h"
#include "NxFoundationSDK.h"

//extern NxUserAllocator * nxFoundationSDKAllocator;	//the SDK defines this.

/**
Allocator template for STL so it can use our user memory stuff.
*/
namespace NxFoundation
	{

template<class T>
class STLAllocator : public std::allocator<T>
	{
	public:
	NX_INLINE pointer allocate(size_type _N, const void *);//allocate for _N T-s.
	NX_INLINE char _FARQ *_Charalloc(size_type _N);	//allocate _N bytes.
	NX_INLINE void deallocate(void _FARQ *_P, size_type);
	};

template<class T>
NX_INLINE T* STLAllocator<T>::allocate(size_type _N, const void *)//allocate for _N T-s.
	{
	void * mem = nxFoundationSDKAllocator->malloc(_N * sizeof(T));	
	return (pointer)  mem;
	}

template<class T>
NX_INLINE char _FARQ *STLAllocator<T>::_Charalloc(size_type _N)	//allocate _N bytes.
	{
	void * mem = nxFoundationSDKAllocator->malloc(_N);	
	return (char _FARQ *)mem; 
	}

template<class T>
NX_INLINE void STLAllocator<T>::deallocate(void _FARQ *_P, size_type)
	{
	//operator delete(_P); 
	if (_P)		//stupid STL will call this even if we didn't allocate anything.
		nxFoundationSDKAllocator->free(_P);	
	}
	}
#endif