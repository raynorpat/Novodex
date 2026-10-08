# Phase 5 box slot-10 center-and-diagonal row

IDA disassembly for `phys_fn_000937` at `0x10020670` confirms the arithmetic. The row writes pose-one translation to `out[0..2]`, loads the three dimensions, multiplies each by itself in x87, adds `dx*dx + dy*dy`, adds `dz*dz`, then executes `fsqrt` and stores `out[3]`. This establishes `sqrt((dx*dx + dy*dy) + dz*dz)`.

The registered `NxPhysicsObjectLayoutTests` fixture now drives non-default dimensions `(2.5, 3.75, 4.125)` and translation `(1.25, -2.5, 0.75)`. Oracle and candidate outputs match bitwise at `3fa00000.c0200000.3f400000.40c38275` (digest `4e1a53cd`). A targeted mutation that omits `dy*dy` for this non-unit case reports `boxrow candidate ok=0` and `layout candidate mismatches=1` (exit 1). Restoring the implementation reports `ok=1`, the same digest, and zero mismatches (exit 0).

The Phase 5 assertion floor remains 2,308 because this strengthens the existing registered row observation. No public Physics header or production behavior changed.

The Phase 5 coverage registry now pins the non-unit oracle transcript (`3fa00000.c0200000.3f400000.40c38275`), aggregate oracle digest (`48e9445b`), and candidate row digest (`4e1a53cd`). The clean Win32 Release build completed; after refreshing those pins, all 18 staged-pair targets and both Phase 5 oracle-differential targets passed, with 2,174 paired and 134 oracle-side coverage assertions (2,308 total).
