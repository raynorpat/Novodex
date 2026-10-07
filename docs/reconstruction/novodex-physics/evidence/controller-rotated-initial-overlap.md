# Rotated controller overlap exit

Date: 2026-10-06

The controller SAT path already matched the pinned resolver for rotated-box
impacts, but it treated a controller that began inside the expanded OBB as an
immediate hit. A paired public-scene fixture uses a 45-degree box with half
extents `(1, 0.5, 0.1)`, a box controller with half extents `(0.1, 0.25, 0.1)`
at the obstacle center, and two separate moves of one unit along `+X` and `-X`.

The test first went RED: the oracle completed both moves to `x=+1` and `x=-1`
with no collision flags, while the candidate stayed at `x=0` and reported side
flag 4 for both. The rotated-box sweep now checks whether the starting center
lies inside the expanded intervals on all nondegenerate SAT axes. When it
does, it skips that OBB for this move, matching the pinned behavior in both
directions. The axis-aligned box overlap path is unchanged.

Verification on the current candidate:

- Focused `NxPhysicsSimulationTests` staged-pair differential: oracle and
  candidate exit 0, `stdout_delta=0`, exact stderr (`build/controller-rotated-overlap-green.log`).
- Phase 7: pass, 1,361/1,361 registered assertions
  (`build/controller-rotated-overlap-phase7.log`).
- Phase 5: pass, 2,042/2,042 registered assertions
  (`build/controller-rotated-overlap-phase5.log`).
- Release Viewer selection: 48/48 completed; 43 passed and the five existing
  signature-verified oracle asset cases were skipped, across all 39 scene
  selections and both Viewer physics checks
  (`build/controller-rotated-overlap-viewer.log`).

This closes only the initial-overlap behavior for the tested rotated-box SAT
path. General transformed-controller sweeps, successful step-up behavior,
callbacks, and the rest of the controller interface remain open. No public
Physics headers changed.
