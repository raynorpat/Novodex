# Fluid emitter name rows 003858 and 003860

The name setter and getter preserve the caller's raw `const char*` in the internal emitter-name registry. The setter takes the wrapper write link, rejects write-lock and reentry cases, and updates the registry; the getter takes the read link and returns the registered pointer. The public `NxFluidEmitter` headers were not changed.

The direct oracle fixture pins `NxPhysics.dll` SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` from `D:\FlamingEnt__\Unreal_3\Binaries`. Its scratch image seeds the registry global at RVA `0x123c0c` and the oracle lock's held-owner words so the two methods can be tested without initializing the full SDK. The test compares the oracle, linked candidate, and candidate-DLL wrapper across first assignment, replacement, clearing, pointer identity, and preservation of an unrelated registry entry.

Each row was independently falsified. Replacing the setter's name update with the current registry value caused `mismatches=2`; replacing the getter's returned pointer with null caused `mismatches=2`. Both mutant processes exited 1. After restoring the implementation, the full `NxPhysicsFluidEmitterAbiTests` process exited 0 with `fluid emitter name rows=2 replacements=1 clears=1 candidate_dll=1 mismatches=0` and `fluid emitter aggregate cases=6 mismatches=0`.

The candidate map resolves the methods to `?setName@NpFluidEmitter@@UAEXPBD@Z` at RVA `0x0003df60` and `?getName@NpFluidEmitter@@UBEPBDXZ` at RVA `0x0003cda0`. Build, mutation, and restored-control logs are in `evidence/raw/phase7-fluid-emitter-name-003858-003860/`.

This closes these two wrapper rows only. The helper and broader fluid-emitter construction path remain separate reconstruction work.
