# Enabled FluidManager descriptor validation

`NxScene::createFluid` previously reproduced only the shipped disabled-backend
warning. The pinned Release binary has a second observable path: when the
manager's `+0x2b` flag is enabled, it validates `NxFluidDesc` before entering
the extension-backed creation path. An invalid descriptor reports
`NXE_INVALID_PARAMETER` from `FluidManager.cpp:140` with the message
`Supplied NxFluidDesc is not valid. createFluid returns NULL.`

The simulation fixture seeds that manager flag and calls the public API with
`restDensity=0`. It now also checks twelve additional invalid descriptors:
kernel radius below one, zero particle spacing, zero stiffness, zero viscosity,
damping above one, negative dynamic restitution, an empty simulation-method
mask, `maxParticles` above `0x7d00`, invalid initial-particle pointer pairing,
missing initial-position stride, missing write-flags stride, and an invalid
surface-mesh descriptor. Each must return null and emit exactly one
`NXE_INVALID_PARAMETER` diagnostic at `FluidManager.cpp:140`. The implementation
uses the existing descriptor validation method; no public header changed. A
size-targeted allocator injection fails the lazy `0x34`-byte manager
allocation. Both binaries return null without a diagnostic and leave the scene
manager pointer clear. Failing the first `0x84`-byte internal-fluid allocation
also returns null without a diagnostic or callback and does not append a second
fluid. A separate injection fails the first `0x78`-byte internal-emitter
allocation. The oracle skips that emitter before calling
`FluidGetRestParticleDistance`, then processes the remaining three descriptors;
the candidate now follows the same order. Array-growth failures, later object
allocations, and the remaining extension-backed lifecycle stay open, so the
encompassing `phys_fn_003649` row remains open.

The original rest-density test was first run against the unmodified
implementation and diverged from the oracle (`stdout_delta=2`); after the
implementation, the staged-pair differential matched exactly. The expanded
twelve-case validation and all three targeted allocation-failure differentials
match exactly (`stdout_delta=0`, both processes exit 0, exact stderr). Fresh
Phase 5 and Phase 7 gates pass at 2,678/2,678 and 1,711/1,711 assertions, and
both require the descriptor and allocator coverage markers.

The Viewer all-scenes suite remains part of the approved validation design:
48 registered CTest entries cover the 39 available scenes. It is a separate
integration check from this test-seeded descriptor branch. After this change,
all 48 entries completed: 43 passed and the five established pinned-oracle
asset cases skipped by their existing signatures; there were no failures
(`build/viewer-all-scenes-fluid-slice.log`).
