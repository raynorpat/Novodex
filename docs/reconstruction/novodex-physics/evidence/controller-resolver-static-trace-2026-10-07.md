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

This trace narrows the next controller task: map the descriptor/object offsets
and helper output contract into an oracle fixture, then implement and falsify
the exact conditional correction behavior. The current candidate still uses
its own swept-bounds loop and has only case-specific evidence for box and mesh
contacts, overlap direction, wall slide, slope classification, and the grounded
+Y probe. `phys_fn_002306` remains partial; this static trace does not close its
behavior or the surrounding controller cluster. Public Physics headers were
not changed.
