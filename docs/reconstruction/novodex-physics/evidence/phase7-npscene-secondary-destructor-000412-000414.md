# NpScene secondary-base destructor rows 000412 and 000414

The primary `NxScene` interface and the secondary `EmbeddedHookBase` subobject
now match the oracle's layout. `NpScene` inherits the 12-byte polymorphic base
at `this + 8`; its primary fields remain at `+0xc` and `+0x10`. The source
implementation is [NpScene.cpp](../../../../Physics/src/NpScene.cpp), and the
shared base definition is `Physics/src/include/EmbeddedHookBase.h`.

IDA identified `phys_fn_000412` at RVA `0xd960` as an 8-byte adjustor thunk:
it subtracts 8 from `this` and transfers to `phys_fn_000414` at RVA `0xd970`.
The latter is the full `NpScene` destructor. It switches both vptrs to the
destruction state, tears down the condition, write/read locks, and events, calls
the secondary-base destructor, and restores the `NxScene` vptr. The constructor
at RVA `0xc310` calls the base constructor at `this + 8`, confirming the
subobject placement.

The candidate MSVC map confirms the emitted deleting adjustor
`??_ENpScene@@W7AEPAXI@Z` in `NpScene.obj` and the complete destructor
`??1NpScene@@UAE@XZ`. The direct secondary-vtable probe compares both paths to
the pinned oracle: the nondeleting call returns the root pointer, transitions
both vptrs, and records six frees; the deleting call returns the root and
records seven frees. The member-only negative control is detected.

A no-op destructor mutation was rejected by `NxPhysicsSceneConstructorTests`
with `candidate_exit=1` and `stdout_delta=6`; the restored oracle/candidate
control is exact. The full Phase 7 Win32 Release gate then passed at
`coverage_assertions_evaluated=1721 floor=1721` (`phase_gate=7 status=pass`).
The candidate DLL SHA-256 was
`9a1285f2c39930b5bcf6720288b72b4b8b7022912d3ea593381b6fee963163fb`.
