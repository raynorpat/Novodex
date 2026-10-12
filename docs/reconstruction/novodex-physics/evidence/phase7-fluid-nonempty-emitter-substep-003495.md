# Nonempty bodyless-emitter substep (`phys_fn_003495`)

The registered `NxPhysicsFluidEmitterAbiTests` fixture now invokes the pinned oracle at RVA `0x85a40` and the map-resolved candidate substep with a one-element emitter range. The emitter has a valid scratch fluid/backend/state record and an identity local pose, but no frame actor. This exercises the nonempty pre-pass (`003624` publishes the global pose), `FluidRun`, and the post-pass dispatch into the bodyless early return of `003601` without inventing an actor layout.

The differential compares the pose callback tuple and all twelve pose words, the `FluidRun` system/fluid/state handles, vector words, timestep bits and option, both stored timestep words, both cleared state words, the result, skipped repulsion callbacks, and x86 stack balance. The restored transcript is:

```text
fluid substep nonempty-emitter cases=1 pose_callback=1 fluid_run=1 payloads_match=1 timestep=1 state-clear=1 repulsion-skipped=1 stack=1/1 mismatches=0
```

The row-specific mutation replaced the per-emitter `nxNpFluidEmitterUpdateBodyState(*current)` call with an empty loop. Oracle and candidate still ran `FluidRun`, but the candidate omitted the pose callback and the registered line reported `pose_callback=0`, `payloads_match=0`, `mismatches=1`; the process exited 1. Restoring the call returned the focused probe to zero mismatches. Raw transcripts: `build/FluidGate/phase7-fluid-substep-nonempty-emitter-mutant.log` and `build/FluidGate/phase7-fluid-substep-nonempty-emitter-restored.log`.

Fresh Win32 Release CMake configure/build and gates passed: Phase 5 `2,638/2,638`, Phase 7 `1,665/1,665`; all 810 reconstruction tooling tests passed. Inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows. Both public Physics header manifests passed at 80 files each. Oracle `NxPhysics.dll` SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Candidate SHA-256: `8180eeee170d23d07139445d6e91edb888a454524cdf61d3d5cc6a9d643445b9`.

The fixture is deterministic and uses no random seed. Public headers are unchanged. This directly covers the nonempty bodyless-emitter caller path. The attached-body combination now also runs through the full `003495` caller path; see [phase7-fluid-attached-body-substep-003495.md](phase7-fluid-attached-body-substep-003495.md). Dirty static-collision reconstruction through `003505` and `003630` remains open.
