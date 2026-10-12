# Fluid emitter global-pose rows 003559 and 003561

IDA confirms both rows call the `EmitterGetGlobalPose` backend callback with
the context at backend `+0x30`, the fluid handle at fluid `+0x80`, and the
emitter handle at internal emitter `+0x08`. They transpose the callback's
3x3 rotation words into the output matrix while copying the three translation
words unchanged. `phys_fn_003559` writes to the internal cached matrix at
`+0x48` and returns that address; `phys_fn_003561` writes to its caller-owned
matrix and returns that address.

`NxPhysicsFluidEmitterAbiTests` now calls each pinned-oracle function and the
candidate DLL helper directly. The differential verifies callback arguments,
all twelve output words, return pointers, the untouched tail of the caller
buffer, and x86 stack balance. The restored direct run reports:

```text
fluid emitter global-pose rows=2 callback_checks=2 stack_checks=2 mismatches=0
```

A mutation changed the rotation transpose (`source[3]` to `source[1]`). Both
cases rejected it (`mismatches=2`, harness exit 1); restoring the transpose
returned zero mismatches. The zero-mismatch line is registered as required
Phase 5 and Phase 7 coverage. Full Phase 5 and Phase 7 gate results are recorded
in the main completion roadmap after the gate runs.
