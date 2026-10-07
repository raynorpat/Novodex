# Controller release wrapper (`phys_fn_002330`)

Ghidra's pinned listing for RVA `0x0005a4b0` identifies the release wrapper:
it rejects API reentry with the Controller.cpp line-97 error, sets the reentry
guard, obtains the embedded controller node from `controller + 4`, delegates
list unlinking to `phys_fn_002318` at `0x0005a170`, dispatches the node's scalar
deleting destructor when the node exists, and clears the guard.

`NxSceneInternal::releaseController` in `Physics/src/Scene.cpp` implements that
normal release path in one method: it guards reentry, removes the node from the
Scene list, clears its next link, deletes through the embedded proxy vtable,
and clears the guard. The implementation also preserves the oracle's
non-member error path. The public test path reaches it through `NpScene`.

`NxPhysicsSimulationTests` releases two controllers in order that exercises a
non-head unlink followed by head removal. It checks for stale links or error
callbacks; the staged oracle differential is exact. The focused mutation
registered for `phys_fn_002318` also detects a broken middle-node unlink, but
does not falsify this wrapper as a whole. Reentry and non-member error paths
remain supported by static evidence only.

See `scene-controller-factory.md` for the broader factory, allocation, and
list-lifecycle evidence. Public Physics headers are unchanged.
