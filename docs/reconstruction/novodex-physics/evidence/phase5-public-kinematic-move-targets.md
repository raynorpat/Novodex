# Phase 5 public kinematic move targets

The three public `NxActor::moveGlobal*` methods now store kinematic targets
in the dynamic record's 0x20-byte block at `+0x118`. `moveGlobalPosition`
updates the first three words and ORs the position bit into the target
flags at `+0x0c`. `moveGlobalPose` stores position and a quaternion
converted from the requested matrix, then sets both target bits.
`moveGlobalOrientation` uses the actor's current position with the requested
orientation. The actor's current pose and auxiliary dirty queue stay
unchanged until simulation consumes the target.

Fresh-process probes compare position, orientation, full pose, a subsequent
position overwrite, and orientation as the first move after a new
kinematic transition. A nonkinematic dynamic actor ignores all three
commands in the tested path. Eight new oracle/candidate lines match exactly,
raising the Phase 5 floor from 812 to 820. The position-after-pose probe
caught an initial candidate bug that reset the orientation target bit;
the corrected write preserves it. Consumption during simulation and
lock/error branches remain open.
