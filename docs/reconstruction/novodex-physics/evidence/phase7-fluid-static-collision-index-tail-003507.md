# Static fluid-collision index tail (`phys_fn_003507`)

IDA identifies the 89-byte row at RVA `0x86390` as the final continuation of the static-collision builder beginning at `0x861b0`. It iterates the collected mesh records, builds triangle/index associations with the mesh ordinal, copies the completed hash descriptor into collision state at `+0x0c`, destroys the temporary hash and signed-16-bit cell array, and returns the latter destroy result. The candidate maps the containing builder to `nxFluidCollisionBuildStaticCollision` in `FluidCollision.obj`; the function body includes this tail.

The registered `NxPhysicsFluidEmitterAbiTests` static-rebuild fixture covers empty and one-mesh cases and compares the resulting grid, arrays, parameters, return value, and x86 stack. In the nonempty case, a candidate-only mutation that increments the mesh ordinal before `nxFluidCollisionBuildTriangleIndices` is rejected (`grid=0`, `mismatches=1`, target exit 1). The restored candidate returns to `mismatches=0` and exit 0.

The reconciled current-main Phase 7 gate passes at 3,017/3,017 registered assertions, including this restored static-rebuild marker; its compressed transcript is `raw/phase7-fluid-static-collision-index-tail-003507-20261017/phase7-gate.log.gz`. Inventory validation reports 446 closed / 114 deferred Phase 7 rows.

This closes the recovered final index/copy/destroy continuation for Phase 7. Phase 8 still needs a full-image instruction/relocation audit of the containing builder and all required data/callback ownership. Raw mutation patch and restored/mutant transcripts are in [the evidence archive](raw/phase7-fluid-static-collision-index-tail-003507-20261017/).
