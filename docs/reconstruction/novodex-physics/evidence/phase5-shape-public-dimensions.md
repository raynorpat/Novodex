# Phase 5 public shape dimension mutation

Actor-created sphere and capsule public handles now accept positive
dimension changes. Sphere `setRadius` writes the internal radius at
`+0xe0`, updates its owned pose/pruner generation once, and marks the
shape's deferred geometry dirty bit. Capsule `setRadius` writes `+0xe0`;
`setHeight` stores half the requested full height at `+0xe4`. Each capsule
call updates the owned pose/pruner generation once.

The public actor drive changes a sphere radius from 1.0 to 1.5 and a
capsule from radius 0.5/height 1.0 to radius 0.75/height 2.0. It compares
public getters, internal geometry words, shape update flags and pruner
generation deltas. Both new lines match the pinned DLL and candidate,
raising Phase 5's floor from 768 to 770. Invalid dimensions, scene-lock
error paths, and the remaining shape operations still need reconstruction.
