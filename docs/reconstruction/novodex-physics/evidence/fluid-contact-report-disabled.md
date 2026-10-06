# Disabled fluid contact reporting

The pinned NxPhysics build documents the fluid contact-report setter and
getter as unavailable. Their exported implementations both report
`NXE_DB_WARNING` (206) from `NpScene.cpp`; the setter ignores its argument and
the getter returns null.

`NxPhysicsSimulationTests` passes a non-null pointer to a local marker byte to
the setter, then queries the callback. This confirms the unavailable setter
does not retain the pointer, without invoking any method on that marker. The
oracle reports:

- setter: line 377, `NxFluid::setUserFluidContactReport(): Feature not available!`
- getter: line 383, `NxFluid::getUserFluidContactReport(): Feature not available!`,
  returning null

Both warnings carry `\Epic\Novodex\SDKs\Physics\src\NpScene.cpp` as
the source path. The candidate initially emitted neither warning. After the
two wrappers were reconstructed, the staged differential matches exactly:
both processes exit 0, `stdout_delta=0`, and `stderr_exact=True` in
`build/FluidGate/fluid-report-green.log` (2026-10-05).

The coverage registry pins both warning lines in Phase 7. This establishes the
oracle-native unavailable behavior for these two APIs; it does not establish
the rest of the fluid manager or fluid contact pipeline.
