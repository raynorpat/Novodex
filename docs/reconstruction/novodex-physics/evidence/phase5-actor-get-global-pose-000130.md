# Actor full-pose getter `phys_fn_000130`

Oracle row `phys_fn_000130` at RVA `0x00004580` reads the actor pose under one
read guard. It builds the orientation from the dynamic quaternion or cached
static matrix, copies the position from the dynamic record or static body, and
returns all twelve pose words. The candidate implements the same single-lock
snapshot in `NpActorVtable::getGlobalPoseVal`.

The baseline `NxPhysicsActorLifecycleTests` staged-pair differential passed:
both children exited 0, `stdout_delta=0`, and stderr matched exactly. To target
this row, a temporary mutation added `1.0f` to the returned pose's Z
translation. The public static, dynamic, rotated, and quarter-turn pose cases
caught it with equal zero exits, exact stderr, and `stdout_delta=8`. Restoring
the source returned the transcript to `stdout_delta=0` with exact stderr.

The restored Release candidate DLL SHA-256 was
`cc92e36dd71c370a43d56d4421974dbddae7c31946caf8f8c663929ab88f7649`. Public
Physics headers were not changed.
