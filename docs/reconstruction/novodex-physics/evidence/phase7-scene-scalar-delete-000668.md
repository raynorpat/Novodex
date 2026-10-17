# Scene scalar-deleting thunk (`phys_fn_000668`)

IDA at oracle VA `0x100145c0` identifies a 34-byte `__thiscall` thunk. It calls Scene cleanup row `phys_fn_000663`, tests flag bit 0, conditionally frees `this` through `nxFoundationSDKAllocator` vtable slot `+0x14`, returns `this`, and uses `ret 4`. The thunk is referenced by a Scene vtable at `0x101066f4`.

The candidate implementation is `NxSceneInternal::scalarDeletingDestructor` in `Physics/src/Scene.cpp`, reached from the Scene scalar-deleting virtual slot. The fresh CMake linker map resolves `?scalarDeletingDestructor@NxSceneInternal@@QAEPAXH@Z` to preferred VA `0x1006dff0` (`Scene.obj`). Its declaration now returns `void*`; after shared cleanup it returns `this`.

`NxPhysicsSceneScalarDeleteTests` creates SDK-owned Scenes, reads the internal allocation at public wrapper offset `+0x24`, and calls the oracle RVA and candidate-map entry directly. Both sides report a 1,808-byte internal Scene, preserve it for flag 0, free it for flag 1, return the same pointer, and balance the x86 stack. The registered staged-pair control is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr).

The mutation disabled only the candidate's `flags & 1` free. Flag 0 still matched; flag 1 changed from `freed=1` to `freed=0`, and the candidate exited 1. The differential rejected it (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`). The restored candidate SHA-256 is `abbe7f8ae92d213803256026345714be68c890b0c137cd1e95eb82341666b116`; the mutant SHA-256 is `69b73ca46e12502d246040959d36413d1a4ab3bd2ac89c8b6f57ee6c29e1f3b5`. The pinned oracle image SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Raw restored and mutation transcripts are in `evidence/raw/phase7-scene-scalar-delete-000668-20261017/`. Public Physics headers were not changed.
