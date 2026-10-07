# Controller resolver static trace — 2026-10-07

IDA Pro decompilation of `NxPhysics.dll+0x59320` (`phys_fn_002306`) confirms
that the current direct `move` path is a three-sweep resolver with a conditional
fourth probe. The virtual entry at `+0x59710` forwards the requested
displacement, active-group mask, minimum distance, and collision-flags output
to this resolver. The resolver builds an initial up-axis probe, a displacement
with the up-axis component removed, and the remaining up-axis displacement;
their query masks are `10`, `10`, and `1`. It combines the corresponding
collision bits into the output word.

The extra path is conditional on the controller's step-probe byte, a hit from
the third query, and a negative requested displacement along the selected
up-axis. It transforms the hit normal, compares its up component with zero and
the stored step threshold, then performs an additional query with component 1
set to the negative probe distance under a temporary global probe-mode byte. A
hit from that query clears the third collision bit. The listing does not show
an upward retry in this entry, so the
common description “successful step-up” is not yet evidence-backed for this
call path; establish it with a fixture against the pinned binary before
changing the candidate to synthesize such motion.

The registered simulation differential now includes a separate grounded
low-obstacle case with the step threshold set to `2.0`. Oracle and candidate
both stop at `(0.5, 0.5, 0)` with flags `0x5`. The first attempt reused a scene
with another controller; that made the candidate collide with the first
controller's generated actor and produced a false mismatch. Moving the fixture
to its own scene removed that interference. The exact high-threshold output
pins this input but does not demonstrate that the conditional correction
changes an observable result.

The correction branch is now observable in a separate high-threshold sloped
mesh fixture. The pinned oracle reports position bits
`c0004807.bf66f671.3f000000` with flags `0`. The candidate's initial
correction implementation reported `3f000000.3f0ccccc.3f000000`, flags `4`.
It now clears the same flag, but reports `c00bbae2.bf4aa900.3f000000`, flags
`0`. A separate, isolated scene with the correction threshold set to zero
reports `3f000000.3f0ccccc.3f000000`, flags `4` from both oracle and candidate.
This confirms the original three probes match for the same downward fixture;
the remaining difference is in the conditional fourth query. A trial that
applied the helper's 1.1 broad-phase inflation to the candidate's narrow-phase
SAT extents moved this baseline to Y=`3f147ae0` and introduced 38 transcript
differences; the trial was reverted. Further IDA
tracing shows that query computes its travel as
`max(0, stepOffset - initialUpProbe) + abs(requestedUpMotion)` and sweeps along
fixed negative Y. Its `minDistance` argument is
`min(moveMinDistance, 0.1 * travel)`; this is the resolver's short-segment
threshold, not a cap on travel. The triangle helper `sub_10057D00` writes the
hit point, collision normal, and hit fraction to its seven-float result record.
`sub_10058870` computes `R = D - 2*N*dot(N,D)`, then splits it into
`Npart = N*dot(N,R)` and `T = R - Npart`. While `byte_1012466C` is set, it
normalizes `Npart` and `T` separately; subsequent movement scales them by two
call-site parameters whose mapping is still open. The candidate still uses its
SAT entry axis and applies a direct tangent offset; the IDA trace does not prove
that its axis or response distance matches the helper result. For
the neighboring non-correction slope case, retaining
a triangle hit as a stop (rather than clipping only its dominant world axis)
makes the candidate position and flags exact. The descriptor-axis fixture also
remains exact. The focused simulation differential has one remaining transcript
line difference (`stdout_delta=2`, both processes exit 0, stderr exact;
`build/controller-slope-baseline-isolated-phase5.log`). The fresh Phase 5 staged gate stops
only on this same `NxPhysicsSimulationTests` delta; its other 16 targets match
exactly (`build/controller-fourth-probe-phase5.log`).

A trial switched the stored SAT axis to the unit triangle face normal and
subtracted a fixed `0.08` from the correction travel, based on the first
fixture's apparent travel reduction. The candidate then reported
`c007268e.bf418036.3ecccccd`, while the oracle remained
`c0004807.bf66f671.3ecccccd`. Moving the controller from the diagonal seam to
an interior point of the same triangle left the oracle pose unchanged. This
falsifies a face-normal plus fixed-skin correction as a sufficient explanation;
the trial and fixture adjustment were reverted. The helper's transformed hit
normal or its response coefficients remain the likely unresolved terms.

The next controller task is to reproduce the native hit normal, residual-motion
response, and thresholded fourth sweep, then falsify them with this fixture. The
all-scene viewer smoke suite is registered from every checked-in `.pds.ods`
file and has been re-run: 34 passed, five known baseline skips, none failed.
The current candidate still uses its own swept-bounds loop and has only
case-specific evidence for box and mesh contacts, overlap direction, wall
slide, slope classification, and the grounded +Y probe. `phys_fn_002306`
remains partial; this static trace does not close its behavior or the
surrounding controller cluster. Public Physics headers were not changed.

### Continuation — approved all-scene Viewer run and fourth-probe precision (2026-10-07)

The approved Viewer design is registered as 48 `^Viewer` CTest entries spanning all 39 available scenes. A fresh Release build and run after the controller resolver changes passed all 48 entries; the five established pinned-oracle asset cases skipped under their existing signatures (`build/viewer-approved-scenes-after-controller-retrace.log`).

The focused Phase 5 simulation differential remains red only on the sloped correction pose: oracle `c0004807.bf66f671.3f000000`, candidate `c0004807.bf66f672.3f000000`, flags `0` on both (`stdout_delta=2`, both processes exit 0, stderr exact; `build/simulation-after-retrace-final.log`). Reconstructing the response with double intermediates improved the previous three-ULP Y delta to one ULP. A separate Möller ray trial based on the callback decomp worsened the pose and was reverted. The native callback argument mapping and residual hit data remain open; this does not close the controller resolver or Phase 5 gate. Public Physics headers are unchanged.
The complete Phase 5 gate was rerun on this build: 16 of 17 targets are exact, `NxPhysicsSimulationTests` is the only red target (`stdout_delta=2`), and the 2,261-assertion coverage floor is met (`build/phase5-after-controller-retrace.log`).

The call-site mapping is now concrete. `sub_10058EA0` passes the query wrapper's current-position storage (`query + 0x24`) and the query displacement into `sub_10058870`; the fourth resolver call uses the same response coefficients as the other probes, with the normal component disabled and the tangent component enabled. The returned callback record carries the hit point, normal, and fraction. This narrows the remaining mismatch to how the fourth query's shape-specific origin and triangle callback produce that record. A trial that started the correction sweep at the stored controller position without the half-height offset produced `bff51d32.bf751d30.3f000000`, far from the oracle, so it was removed; the current candidate retains the offset and is still one ULP high in Y. The trial confirms that the query's effective origin depends on controller geometry, even though the resolver passes its stored position pointer unchanged. The staged simulation gate was rerun after restoring the existing path and still differs only on this correction pose.

Continuation — callback geometry falsification (2026-10-07): the IDA pseudocode for `sub_10057D00` shows the mesh callback checks the moving controller's vertex trajectories against triangle faces, then checks controller faces and mesh edges. The candidate's temporary fourth-query path reduced this to a point ray from the controller bottom center against the single triangle retained by the third probe. A test-first trial changed it to an AABB sweep from the controller center using the descriptor extents. That trial lost the correction entirely: candidate returned the uncorrected baseline pose with flags `4` while the oracle retained `c0004807.bf66f671.3f000000`, flags `0` (`build/controller-bounds-correction-diff.log`). The trial was reverted; the restored source rebuild reproduces the prior one-ULP mismatch (`build/controller-bounds-correction-reverted-diff.log`). This rejects a direct SAT AABB substitution as sufficient. The next implementation must reconstruct the callback's vertex/face/edge query over the mesh and its hit record before changing the response arithmetic again. No public Physics headers changed.

Correction to the 2026-10-07 precision finding: IDA pseudocode for `sub_10058870` exposed the exact response order. The reflected vector is normalized with an x87/double reciprocal, projected onto the callback normal, and split into separately normalized normal and tangent components while the temporary correction flag is set. The resolver consumes the callback normal; the candidate had recomputed it from the triangle, introducing a two-ULP normal error, and its float division introduced a further tangent rounding error. The candidate now uses the stored callback normal and matches the oracle slope correction exactly: `c0004807.bf66f671.3f000000`, flags `0` (`build/controller-native-response-diff.log`). The helper hit distance still differs by one ULP, but the complete observable simulation transcript matches.

Fresh full Phase 5 verification passes all 17 paired targets and all 2,261 coverage assertions (`build/phase5-controller-response-exact.log`). The approved Viewer suite runs all 48 registered entries covering 39 scenes: 43 pass and the five established pinned-oracle asset cases skip; no failures (`build/viewer-all-scenes-controller-exact.log`). Public header verification passes 80/80 and `git diff --check` is clean. This closes the controller correction delta and current Phase 5 red gate; it does not close full NxPhysics reconstruction or the still-open Phase 4 third-party cooking differential.
