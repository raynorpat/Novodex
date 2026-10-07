# Kind-5 joint support row differential

The kind-5 dispatch to `phys_fn_004399` (`NxPhysics.dll+0x000afc10`) now has a
direct internal oracle differential. The test builds the measured support-body,
support-row, and Scene arrays in memory, calls the pinned per-island wrapper at
`+0x0009b120`, then runs the linked `nxSolveJointSupportRecords` with the same
fixture. The test does not claim that a public joint factory currently emits
kind 5; this isolates the solver row whose public producer has not been found.

The two fixtures exercise an unconstrained impulse and the max-force clamp. Each
executes one iterative pass and the final-record pass. The comparison includes
record flags and solver values, body linear/angular velocities and post-pass
copies, and the joint slot-3 accumulated-force callback.

The pinned oracle (`sha256=4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`)
and candidate match exactly:

```text
joint_support kind5 cases=2 oracle=88b713b7bc0870c9 candidate=88b713b7bc0870c9 mismatches=0
joint_support inputs=2 digest=85a7065a061c36cd
```

Mutation check: disabling the `kind == 5` dispatch produced a candidate digest
of `054f1ec1bdf3ea09` and 11 mismatches. Restoring dispatch returned the exact
oracle match.

The registered Phase 7 gate passes at 1,372/1,372 coverage assertions, including
the new result and input digests. It also passes the 80-header immutable check,
inventory validation (6,338 functions, 5,138 data objects, zero unexplained),
the CMake Release build, and all existing staged-pair Phase 7 targets.
