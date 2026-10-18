# Phase 7 masked getter: `phys_fn_003595`

`phys_fn_003595` at RVA `0x00088130` returns the dword at `this+0x10`, ANDed with its stack argument, and uses `ret 4`. The candidate implementation is `nxLockedAndRead` in `Physics/src/ObjectModel.cpp`.

The `NxPhysicsObjectLayoutTests` oracle/source fixture compares the oracle row and candidate helper on three value/mask pairs, including sparse and high-bit masks. The restored result reports `andget row=phys_fn_003595 cases=3 mismatches=0`.

Mutation testing changed the candidate AND to OR. The direct row fixture reported `mismatches=3`, and the registered oracle/source target rejected the mutant through its dependent mask-accessor checks. Restoring AND returned the direct differential to zero mismatches.

Fresh Phase 5, 6, and 7 gates pass at 2,928/2,924, 1,779/1,777, and 3,064/3,060 coverage assertions. The Phase 5 oracle proof pins the three-file fixture bundle and retains digest `3acf0dc7`. Raw differential and gate logs, plus the pinned oracle and fixture-source hashes, are in `evidence/raw/phase7-and-getter-003595-20261018/`.
