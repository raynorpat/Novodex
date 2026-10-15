# Lock-API call and test rows 002371 / 002375

Rows `phys_fn_002371` (RVA `0x0005b7e0`) and `phys_fn_002375` (RVA `0x0005b800`) are now connected to the candidate source harness in `NxPhysicsObjectLayoutTests`. The first loads the 32-bit word at `[self]` and passes it to the supplied `__stdcall` callback. The second passes `[self]` and its argument to its callback, then returns 1 when that callback returns zero and 0 otherwise. No public header was changed.

The registered object-layout differential reports a separate marker for each row and contributes mismatches to the test's failure count. The runner verifies the staged oracle identity and treats this as a candidate-source harness, so it does not attempt to load the candidate DLL as a second copy of the oracle.

Mutation checks establish that both rows are independently observable. Mutating the first callback value with `v ^ 1u` produces one mismatch for row 002371 while row 002375 remains exact; the staged run rejects the candidate with one failure. Inverting the second row's zero/nonzero return mapping produces two mismatches for row 002375 while row 002371 remains exact; the runner rejects the candidate with two failures. Rebuilt restored controls report zero mismatches for both rows.

The fresh Win32 Release phase gates and the full reconstruction tooling suite are recorded under `evidence/raw/phase7-lock-api-call-and-test-002371-002375-20261015/`. The Phase 5, 6, and 7 registered coverage floors are 2,773, 1,673, and 2,889 respectively. These are partial phase gates; the full-DLL reconstruction and Phase 8 audit remain open.
