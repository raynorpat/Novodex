# Phase 7 row 000525: pair-flag shape index

Oracle row phys_fn_000525 at RVA 0x00010410 prepares a 0x40000-entry stack index of shapes from the static and selected dynamic pruner pools, keyed by each shape's ID at +0xd4. The following pair-entry conversion is continuation row 000527. The current CMake Release map resolves the candidate implementation NxSceneInternal::getPairFlagArray in Scene.obj at VA 0x100617c0.

The registered Phase 7 NxPhysicsPairFlagTests exercises getPairFlagArray with actor and shape pairs. A temporary mutation changed only the 000525 shape-index assignment to store null. Pair enumeration then dereferenced the missing shape and the candidate access-violated (candidate_exit=-1073741819, stdout_delta=24).

After restoring Physics/src/Scene.cpp byte-for-byte and rebuilding, actor- and shape-pair controls pass exactly (both exit 0, stdout_delta=0, exact stderr). The CMake candidate DLL SHA-256 is 7c579275fe577e56e88db111f02f342a46a4c8223359455db78bb3958a94545f. No public header changed.

This closes one Phase 7 row as differential_falsified. Phase 7 now has 342 closed and 218 deferred rows; continuation row 000527 remains open and full DLL reconstruction remains open.
