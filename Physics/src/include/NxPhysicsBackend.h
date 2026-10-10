#ifndef NX_PHYSICS_BACKEND_H
#define NX_PHYSICS_BACKEND_H

// The build supplies one consistent backend to Physics and its dependencies.
// Never infer it from the host or silently fall back to a different backend.
#ifndef NX_PHYSICS_USE_X87
#error NX_PHYSICS_USE_X87 must be defined by the build as 0 or 1
#elif NX_PHYSICS_USE_X87 != 0 && NX_PHYSICS_USE_X87 != 1
#error NX_PHYSICS_USE_X87 must be exactly 0 or 1
#endif

#if NX_PHYSICS_USE_X87 && (!defined(_WIN32) || !defined(_M_IX86))
#error The retained x87 backend requires a Windows x86 target
#endif

#endif
