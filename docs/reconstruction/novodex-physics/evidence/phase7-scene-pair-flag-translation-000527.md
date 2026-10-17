# Phase 7 row 000527: pair-flag handle translation

Oracle row phys_fn_000527 at RVA 0x00010450 translates flagged hash entries into public shape or actor handles, sets the actor-pair bit, and advances the output array. The current CMake Release map resolves candidate implementation NxSceneInternal::getPairFlagArray in Scene.obj at VA 0x100617c0. The preceding shape-ID index row 000525 is independently closed.

The registered Phase 7 NxPhysicsPairFlagTests checks the returned actor handles and actor-pair bit. A temporary mutation nulled only the first actor-handle assignment in the 000527 continuation. The candidate transcript changed from objects=0.1 to objects=255.1; the staged differential rejected it (both sides exited 0, stdout_delta=2, exact stderr).

After restoring Physics/src/Scene.cpp byte-for-byte and rebuilding, actor- and shape-pair controls pass exactly (both exit 0, stdout_delta=0, exact stderr). The CMake candidate DLL SHA-256 is 2cc110579ebd3d6e2f4d60ed5f1971bc55c8bac7b3bd6f9097dedee99f7cef16. No public header changed.

This closes one Phase 7 row as differential_falsified. Phase 7 now has 343 closed and 217 deferred rows; full DLL reconstruction remains open.
