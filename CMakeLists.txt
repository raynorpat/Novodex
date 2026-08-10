cmake_minimum_required(VERSION 3.2...3.16)

project(Novodex VERSION 2.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)

#### FOUNDATION ####

file(GLOB_RECURSE FOUNDATION_HEADERS Foundation/include/*.h Foundation/src/include/*.h)
source_group("Header Files" FILES ${FOUNDATION_HEADERS})

file(GLOB FOUNDATION_SOURCES Foundation/src/*.cpp)
source_group("Source Files" FILES ${FOUNDATION_SOURCES})

if(WIN32)
	file(GLOB FOUNDATION_RESOURCES Foundation/src/win32/*.rc Foundation/src/win32/*.h)
	source_group("Resource Files" FILES ${FOUNDATION_RESOURCES})
endif()

add_library(NxFoundation SHARED ${FOUNDATION_HEADERS} ${FOUNDATION_SOURCES} ${FOUNDATION_RESOURCES})
set_target_properties(NxFoundation PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(NxFoundation PRIVATE 
	Foundation/include
	Foundation/src/include
)

target_compile_definitions(NxFoundation PRIVATE NX_FOUNDATION_DLL=1 FOUNDATIONDLL_EXPORTS=1)

if(WIN32)
	add_executable(NxFoundationExportTests tests/FoundationExportTests.cpp)
	target_include_directories(NxFoundationExportTests PRIVATE Foundation/include)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		add_executable(NxFoundationSDKTests tests/FoundationSDKTests.cpp)
		target_include_directories(NxFoundationSDKTests PRIVATE Foundation/include)
		add_executable(NxFoundationClusterTests tests/FoundationClusterTests.cpp)
		target_include_directories(NxFoundationClusterTests PRIVATE Foundation/include)
		add_executable(NxFoundationCustomArrayTests tests/FoundationCustomArrayTests.cpp Foundation/src/CustomArray.cpp)
		target_include_directories(NxFoundationCustomArrayTests PRIVATE Foundation/include Foundation/src)
	endif()
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	if(WIN32)
		target_compile_definitions(NxFoundation PRIVATE WIN64=1)
	endif()
	target_compile_definitions(NxFoundation PRIVATE NX64=1)
endif()

#### PHYSICS ####

file(GLOB_RECURSE PHYSICS_HEADERS Physics/include/*.h Physics/src/include/*.h)
source_group("Header Files" FILES ${PHYSICS_HEADERS})

file(GLOB PHYSICS_SOURCES Physics/src/*.cpp)
source_group("Source Files" FILES ${PHYSICS_SOURCES})

if(WIN32)
	file(GLOB PHYSICS_RESOURCES Physics/src/win32/*.rc Physics/src/win32/*.h)
	source_group("Resource Files" FILES ${PHYSICS_RESOURCES})
endif()

add_library(NxPhysics SHARED ${PHYSICS_HEADERS} ${PHYSICS_SOURCES} ${PHYSICS_RESOURCES})
target_link_libraries(NxPhysics NxFoundation)
set_target_properties(NxPhysics PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(NxPhysics PRIVATE
	Foundation/include
	Foundation/src/include
	Physics/include
	Physics/src/include
)

target_compile_definitions(NxPhysics PRIVATE NX_PHYSICS_DLL=1)

if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	add_executable(NxPhysicsExportTests tests/PhysicsExportTests.cpp)
	target_link_libraries(NxPhysicsExportTests bcrypt)
	add_executable(NxPhysicsSDKTests tests/PhysicsSDKTests.cpp)
	target_include_directories(NxPhysicsSDKTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsSDKTests bcrypt)
	add_executable(NxPhysicsCoreClusterTests tests/PhysicsCoreClusterTests.cpp)
	target_include_directories(NxPhysicsCoreClusterTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsCoreClusterTests bcrypt)
endif()
