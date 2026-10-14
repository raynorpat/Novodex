# Phase 7 row 000423: NpScene scalar deleting destructor

`phys_fn_000423` is the 34-byte scalar deleting destructor at oracle RVA `0x0000db00`. The candidate emits the equivalent MSVC thunk as `??_GNpScene@@UAEPAXI@Z` from `Physics/src/NpScene.cpp`. It must run the wrapper destructor and release the 40-byte `NpScene` object when the delete flag is set.

`NxPhysicsPopulatedSceneTeardownTests` now records the wrapper allocation size before releasing a populated scene and checks that the allocation is gone immediately afterward. The pinned oracle and restored candidate both report `bytes=40 freed=1`; their focused staged-pair transcript is exact (`stdout_delta=0`, exact stderr).

For the row-specific negative control, the candidate DLL's generated deleting-destructor entry at RVA `0x000419b0` was replaced with `mov eax, ecx; ret 4`. The ABI-preserving no-op skipped cleanup and deallocation. The registered teardown differential rejected the candidate (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=28`), and the test reported `FAIL NpScene scalar deleting destructor did not free its wrapper` with `freed=0`. The mutant candidate SHA-256 was `2a012e54120d92a06c2d4cf152e8dde1ab9a02a5bd2ff0cc0118f30df375e7f5`; the restored DLL SHA-256 was `4bb78a70b2cc85d5dd2faba749a909a99ac9692341448e2558f99b9f72e9ce0f`.

The fresh full Phase 7 gate passes at `1,980/1,980` coverage assertions. The inventory validates with Phase 7 at 333 closed / 227 deferred, 6,338 function rows, 5,138 data objects, and zero unexplained bytes. The full gate log and focused baseline, mutation, and restored transcripts are in `evidence/raw/phase7-npscene-scalar-delete-000423-20261014/`.

No public Physics header changed. The full-DLL reconstruction remains open.
