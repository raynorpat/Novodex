# Phase 7 stale contact-pair release: `phys_fn_001953`

`phys_fn_001953` (RVA `0x0004bd80`) is the Scene teardown stale-pair sweep. The destructor advances the scene frame, then removes each pair whose node stamp no longer matches that frame, deleting the node and erasing its pair-hash key. `cpmRetireStaleScenePairs` implements that ordering; the populated-scene fixture creates one contact pair with a static plane and dynamic sphere, then releases the scene.

The registered `NxPhysicsPopulatedSceneTeardownTests` differential is exact with the restored candidate: oracle and candidate both report `pairs_before=1 outstanding_after=15`, `stdout_delta=0`, and `stderr_exact=True`. The earlier Scene cleanup frees the pair storage separately, so the observable release state is the stable post-release allocation count.

For the mutation check, I temporarily disabled the `cpmRetireStaleScenePairs(scene)` call in `Scene::nxSceneDelete`, rebuilt `NxPhysics.dll`, and ran the same staged-pair differential against the pinned oracle. The oracle reported 15 outstanding blocks, while the mutant candidate reported 17 and exited 1; the gate rejected it with `stdout_delta=2` and `stderr_exact=False`. I restored the call, rebuilt, and reran the control; it passed with `stdout_delta=0` and `stderr_exact=True`. The mutation was applied in the active isolated worktree rather than a disposable archive.
