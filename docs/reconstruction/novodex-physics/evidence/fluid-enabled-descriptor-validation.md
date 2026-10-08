# Enabled FluidManager descriptor validation

`NxScene::createFluid` previously reproduced only the shipped disabled-backend
warning. The pinned Release binary has a second observable path: when the
manager's `+0x2b` flag is enabled, it validates `NxFluidDesc` before entering
the extension-backed creation path. An invalid descriptor reports
`NXE_INVALID_PARAMETER` from `FluidManager.cpp:140` with the message
`Supplied NxFluidDesc is not valid. createFluid returns NULL.`

The simulation fixture now seeds that manager flag and calls the public API
with `restDensity=0`. The implementation uses the existing public descriptor
validation method and emits the oracle diagnostic. The public headers remain
unchanged. Valid fluid creation, emitter combinations, and extension-backed
behavior remain open; this observation does not close the encompassing
`phys_fn_003649` reconstruction row.

The test was first run against the unmodified implementation and diverged from
the oracle (`stdout_delta=2`); after the implementation, the staged-pair
differential matched exactly (`stdout_delta=0`, both processes exit 0, exact
stderr). The full Phase 7 gate passed with 1,387 assertions. Its coverage floor
is now 1,387. The same registration audit exposed stale floors in Phases 5 and
6: their registries contain 2,311 and 1,066 lines respectively, so their floors
were corrected to those values. Phase 5 was rerun and passed with 2,311/2,311
assertions.

The Viewer all-scenes suite remains part of the approved validation design:
48 registered CTest entries cover the 39 available scenes. It is a separate
integration check from this test-seeded descriptor branch. After this change,
all 48 entries completed: 43 passed and the five established pinned-oracle
asset cases skipped by their existing signatures; there were no failures
(`build/viewer-all-scenes-fluid-slice.log`).
