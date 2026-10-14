# Anisotropic friction helper row 000859 (`phys_fn_000859`)

The direct simulation fixture calls `cpmAnisotropicFriction0859` by its decorated symbol in the candidate map and pinned oracle RVA `0x0001cb30`. It uses a 90-degree rotated shape matrix and a local-X anisotropy direction, unequal U/V static and dynamic coefficients, then compares a single anisotropic material against two anisotropic materials. Both sides report the rotated world direction `(0,1,0)`, positive ordered U/V coefficients, and the expected changed U/V response after averaging the other anisotropic material. The exact candidate and oracle transcript matches (`stdout_delta=0`, `stderr_exact=True`).

A row-only immediate-return mutation is rejected by the registered `NxPhysicsSimulationTests` differential: oracle exit 0, candidate exit 1, stdout delta 4,002 bytes. Restoring `Physics/src/ContactPairManager.cpp` byte-for-byte returns exact output. The restored implementation passes Phases 5, 6, and 7 at 2,731/2,731, 1,656/1,656, and 1,978/1,978 coverage assertions.

The fixture adds no public-header changes. Mutation patch, source and DLL hashes, focused differential transcripts, and full gate transcripts are in `evidence/raw/phase7-anisotropic-friction-000859-20261014/`.
