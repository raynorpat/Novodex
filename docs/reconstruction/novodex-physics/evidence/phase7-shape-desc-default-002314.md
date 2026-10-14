# Phase 7 row 002314: box shape descriptor default

phys_fn_002314 at oracle RVA 0x0005a0a0 is NxBoxShapeDesc::setToDefault. IDA's table at 0x10108818 places it in the box descriptor vtable's initializer slot. Its oracle body clears the common shape fields and the three dimensions at +0x4c, +0x50, and +0x54, matching the inline implementation in the unchanged public header Physics/include/NxBoxShapeDesc.h.

The Win32 Release map resolves the emitted candidate method ?setToDefault@NxBoxShapeDesc@@UAEXXZ at VA 0x1006d3e0. NxPhysicsObjectLayoutTests calls the actual loaded candidate DLL method on poisoned descriptor storage and compares 80 bytes (+0x08..+0x57) with the oracle call. Restored control: mismatches=0, full layout test exit 0.

The row-local mutation patches the first byte of the loaded candidate method to ret. It is rejected with mismatches=76 and full layout test exit 1. Four bytes remain equal because both implementations leave the descriptor name field at +0x48 untouched. The CMake candidate DLL SHA-256 is b9c0ae2b31ed23416c3ba9550f6bd07b31182f11ba3f46e55ab04b0a84433629. No public header or production source changed.

This closes one Phase 7 row as oracle_differential_falsified. Phase 7 now has 339 closed and 221 deferred rows; full DLL reconstruction remains open.
