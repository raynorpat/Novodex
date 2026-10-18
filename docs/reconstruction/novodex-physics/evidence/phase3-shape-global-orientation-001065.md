# `phys_fn_001065` — shape global orientation

IDA decompilation at RVA `0x00023430` shows a read-locked shape wrapper: it takes the internal shape pointer from handle `+0x18`, computes a 3×3 orientation through the internal pose helper, releases the read lock, and returns the caller's output pointer. The corresponding candidate handler is `nxShapeHandleGetGlobalOrientation` in `Physics/src/NpActor.cpp`.

`NxPhysicsActorDynamicSetterTests` exercises the public output with both a rotated actor pose and a rotated local shape pose. The oracle and candidate staged-pair transcripts match exactly; the recorded global matrix is `331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0`.

For mutation falsification, the candidate handler was changed to zero-fill the output matrix. The oracle exited 0, the candidate exited 0, and the normalized transcript differed (`stdout_delta=4`), so the differential rejected the mutation. Restoring the handler produced `stdout_delta=0`, exact stderr, and `differential=pass`. The compressed full transcripts and hashes are in `evidence/raw/phase3-shape-global-orientation-001065-20261018/`.

The public SDK headers are unchanged. This proof closes this getter path; it does not claim full reconstruction of the shape subsystem.
