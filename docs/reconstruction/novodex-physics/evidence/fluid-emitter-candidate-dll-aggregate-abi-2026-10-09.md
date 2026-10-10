# Fluid emitter aggregate getters through the candidate DLL

**Result:** the six `NxFluidEmitter` aggregate getters match the pinned oracle when dispatched through a wrapper constructed by the candidate `NxPhysics.dll`. This closes the candidate-routing gap for those six getters; it does not establish fluid creation, factory reachability, or the rest of the emitter API.

## Reproduction

- Source commit: `fc651e7c` (`Probe fluid emitter aggregates through candidate DLL`).
- Configuration: Win32 Release, CMake build at `D:\Novodex-build-island-teardown`.
- Oracle: `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`, SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
- Restored candidate: `NxPhysicsCandidate.dll`, SHA-256 `63F29A35BE21FD9F23E2FA2B333C44F98C0FFBAFEB3A02DC81A001F7D4F73B57`.
- Probe command: `NxPhysicsFluidEmitterAbiTests.exe <oracle directory> <oracle NxPhysics.dll SHA-256>`, launched from the candidate output directory so it loads that directory's `NxPhysicsCandidate.dll` and `NxPhysics.map`.

The harness resolves `??0NpFluidEmitter@@QAE@PAX@Z` from the candidate linker map, invokes the constructor from the loaded candidate module on scratch storage, and confirms the primary vtable belongs to that module. It seeds the internal read-lock link, then calls the six aggregate getters through the candidate-owned `NxFluidEmitter` vtable and compares returned bytes, hidden structure-return pointers, and stack balance to the pinned oracle.

Restored probe output:

```text
fluid emitter candidate_dll ctor size=24 vptr_in_module=1 mismatches=0
fluid emitter ctor size=24 secondary_vptr_nonnull=1 internal=1 mismatches=0
fluid emitter raw_abi cases=6 retptr=6 stack_balanced=6 mismatches=0
fluid emitter candidate_dll raw_abi cases=6 retptr=6 stack_balanced=6 mismatches=0
fluid emitter flags cases=2 mismatches=0
fluid emitter backend flags cases=7 callback_mismatches=0 state_mismatches=0
fluid emitter aggregate cases=6 mismatches=0
```

## Mutation check

A DLL-only mutation changed `NpFluidEmitter::getLocalPositionVal`'s source offset from `0x3c` to `0x40`. Only `NxPhysics.dll` was rebuilt and copied over the staged `NxPhysicsCandidate.dll`; the test executable was unchanged. Mutated DLL SHA-256: `5A1DE20CD69A63BC6C8C0269CA0D5C4C33107F7E8D5AC01C7F1D0436311E5497`. The probe rejected the mutant with a `candidate DLL fluid emitter raw ABI mismatch: local_position` diagnostic and nonzero exit. The source was restored, the candidate rebuilt, and the clean probe above passed again.

## Gate coverage

After adding the candidate-DLL constructor and six aggregate-return assertions, Phase 5 passed at 2,619/2,619 and Phase 7 passed at 1,447/1,447. The full Python tools suite passed 816 tests and 732 subtests. The Phase 5 gate also reported unchanged public Physics headers (80/80) and inventory accounting of 6,338 functions, 5,138 data objects, and zero unexplained entries. The focused probe and current gates use the committed source; the exact restored candidate hash above was separately re-run directly after the DLL-only mutation was restored.

This extends the earlier test-linked aggregate probe in `fluid-emitter-aggregate-abi-2026-10-09.md`; it does not replace that record. Fluid emitter creation remains disabled in the pinned implementation, so the scratch wrapper proves candidate DLL code/vtable routing for these getters without claiming a production construction path. M1 and the full DLL reconstruction remain open.
