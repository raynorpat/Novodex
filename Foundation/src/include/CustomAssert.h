#ifndef NX_FOUNDATION_ASSERT
#define NX_FOUNDATION_ASSERT

/*----------------------------------------------------------------------------*\
|
|						Public Interface to NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

/**
SDK-internal Assert macro.  This is a dependency of the FoundationSDK object because
the assert dialog needs to be routed to the user, so we don't have to include that sort
of GUI code in the sdks.  

For this reason it can only be used SDK side, in classes that can't be instanced by the user. 

User side code can use the ANSI assert().

This file is not called Assert.h to avoid conflict with standard assert.h header.
*/

	#ifndef NX_CUSTOM_ASSERT
		#define NX_CUSTOM_ASSERT
		#undef NX_ASSERT		//get rid of lame default assert in case it was already included
		#ifdef _DEBUG													\
			//! Custom ASSERT function. Various usages:
			//! ASSERT(condition)
			//! ASSERT(!"Not implemented")
			//! ASSERT(condition && "error text")
			
			
			#ifdef WIN32
			#define NX_ASSERT(exp)																	\
			{																						\
				static bool NX_ASSERT_ignore = false;													\
				if(!(exp) && !NX_ASSERT_ignore)															\
				{																					\
				if(NxFoundation::FoundationSDK::dbAssert(NXE_ASSERTION, __FILE__, __LINE__, &NX_ASSERT_ignore, #exp)) \
					{																				\
						_asm { int 3 }																\
					}																				\
				}																					\
			}
			#elif LINUX
			#define NX_ASSERT(exp)												\
				{											\
				static bool NX_ASSERT_ignore = false; \
				if(!(exp) && !NX_ASSERT_ignore)										\
					{													\
					if(NxFoundation::FoundationSDK::dbAssert(NXE_ASSERTION, __FILE__, __LINE__, &NX_ASSERT_ignore, #exp)) 	\
						{  asm ( "int $3");				\
						}					\
					}						\
				}
			#endif						
		#else
			//! Leave the {} so that you can write this kind of things safely in release mode:
			//!	if(condition)	NX_ASSERT()
			#define NX_ASSERT(exp)	{}
		#endif


	#endif
#endif
