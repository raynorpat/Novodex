# Fluid manager actor-release traversal (`phys_fn_003639`)

The oracle function at RVA `0x89dc0` walks the manager’s primary fluid range and calls the per-fluid actor-release handler. The `NxPhysicsFluidEmitterAbiTests` differential calls that oracle address and resolves candidate `nxNpFluidManagerNotifyReleasedActor` from the built DLL map. Its fixture seeds one live fluid with three emitters, two attached to the released actor.

Restored control produces six callbacks on both sides and matching state (`state_mismatches=0`, `callback_mismatches=0`). Replacing the candidate traversal call with an array increment produces six oracle callbacks and zero candidate callbacks; the probe reports `state_mismatches=27`, `callback_mismatches=6`, and `mismatches=33`, then exits 1. The registered Phase 7 runner rejects the mutant and passes after restoration at 1,698 assertions against the 1,697 floor.
