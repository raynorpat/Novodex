# Task M7.4 — implicit mesh data setter (`phys_fn_003663`)

## Goal

Reconstruct the still-discovered Phase 7 row at RVA `0x0008ac50`, the internal `NxImplicitMesh::setMeshData` implementation, without changing public Physics headers.

## Oracle contract

IDA identifies a `__thiscall` method taking `NxMeshData&`. It validates the 12-word (0x30-byte) public structure using the same pointer/stride/count/flag consistency rules as `NxMeshData::isValid()`. Valid data is copied byte-for-byte to object offset `+0x08`. Invalid data leaves the object unchanged and reports `NXE_INVALID_PARAMETER`, source `\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\ImplicitMesh.cpp`, line 106, message `Supplied NxMeshData is not valid. setMeshData has no effect.` through the Foundation SDK. The method's apparent EAX value is outside the public `void` contract.

## Falsifiable test

Extend `NxPhysicsFluidEmitterAbiTests` to invoke the oracle RVA and the private method resolved from the candidate DLL map. Compare the complete 0x30-byte copied state and surrounding sentinels for default and fully populated valid records. Exercise representative invalid pointer, stride, count, and flag combinations with a live Foundation error stream; require identical error records, unchanged object bytes, and balanced x86 stacks. Mutation of the copy destination or a validation condition must fail the registered oracle test.

## Build and gate

Implement a private `NpImplicitMesh` method in a root `Physics/src/*.cpp` translation unit so the existing CMake source glob includes it after reconfiguration. Add the target method to the candidate-map differential. Register stable output lines in the existing Phase 5 and Phase 7 oracle coverage lists, update their measured floors, validate inventory/work-unit closure, and run both phase gates plus the reconstruction tooling suite.

## Constraints / review focus

No public header changes. Confirm the private member at `+0x08`, exact `NxMeshData` size, invalid-data error string/file/line, stack cleanup, and that the test calls the pinned oracle and actual candidate DLL rather than a second copy of the implementation.
