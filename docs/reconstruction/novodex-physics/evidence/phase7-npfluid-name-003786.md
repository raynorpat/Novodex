# Public `NxFluid::setName` and `getName`

`phys_fn_003786` (`0x8c240`) now occupies the public `NxFluid::setName` slot. The wrapper takes its Scene write lock, stores the caller-owned name pointer in the shared SDK pointer-binding registry using the internal Fluid pointer at wrapper+`0x14`, and releases the lock. Its paired public `getName` slot reads that registry under the read lock.

The registered simulation fixture stores a distinct string, reads it back, clears the name with null, and verifies the lookup is empty. The first differential was red because the setter vtable slot was null (oracle exit 0; candidate access violation). The restored candidate matches exactly. Replacing the registry write with a no-op is rejected (`candidate_exit=1`, `stdout_delta=60`); the restored control has `stdout_delta=0` and exact stderr.

Phase 5 and Phase 7 coverage floors include the public round-trip. Raw differential and phase-gate logs are under `evidence/raw/phase7-npfluid-name-003786/`.
