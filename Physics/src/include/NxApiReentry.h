#ifndef NX_PHYSICS_API_REENTRY
#define NX_PHYSICS_API_REENTRY

// Shared process-wide API callback guard reconstructed from Scene.cpp's
// .data flag at oracle RVA 0x123c10.
extern "C" bool nxApiReentryIsActive(void);
extern "C" void nxSetApiReentryActive(bool active);

#endif
