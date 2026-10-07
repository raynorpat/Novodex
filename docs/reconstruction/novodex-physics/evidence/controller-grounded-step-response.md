# Grounded controller step response

Historical fixture: the isolated descriptor used a positive `stepOffset` at
the time this evidence was recorded. The test now uses zero offset to pin the
independent probe-enable byte; see
`controller-step-offset-gating.md` for the current regression and result.

An additional paired simulation fixture runs the low-obstacle grounded move in
a fresh scene, without the earlier controller actor. The controller starts at
`(0, 0.8, 0)`, has half-extents `(0.5, 0.5, 0.5)`, enables the `0.5` step
offset, and requests displacement `(2, -0.5, 0)` toward a static obstacle of
height `0.2` resting on the floor.

Both oracle and candidate finish at `(0.5, 0.5, 0)` with collision flags
`0x5`; the paired line is registered in Phase 7. This confirms the blocked
grounded step-probe response is stable in an isolated scene. It does not clear
the obstacle: the controller remains against its near face. Successful
step-over motion, other up axes, slopes, transformed obstacles, multiple
contacts, and hit callbacks remain open. Public Physics headers are unchanged.

The focused paired differential was run directly against the pinned oracle and
candidate DLLs; the output matched exactly:

```text
simulation controller-obstacle step-response-isolated position=3f000000.3f000000.00000000 flags=00000005
```

The fresh Phase 7 gate passes all 11 registered differentials with
1,357/1,357 coverage assertions. The immutable public-header check also passes
for all 80 files.
