# Phase 7 NpScene callback rows 000350–000360

These six rows write/read the scene callback slots at `+0x6ac`, `+0x6b0`, and `+0x6b4`. The notifier pair is now called directly by `NxPhysicsSimulationTests`; the trigger/contact callback pairs are exercised by its existing trigger/contact simulation fixture.

All runs use the registered `NxPhysicsSimulationTests` staged-pair differential with the pinned UE3 oracle. The isolated archive was based on mainline `469e0b39`; source was restored byte-for-byte after each mutation (source SHA-256 `46e36419b12318be79aeea3f0403720bb35df31be0e1ee89f5382e7425f40e89`). Updated fixture SHA-256: `750ef6d78f7c07ec32216c8ee8f11d0a4574bd1557c1cd52c007df176809d3a5`. Each restored control exits zero with `stdout_delta=0` and `stderr_exact=True`.
`phys_fn_000350` mutant `stdout_delta=799`; restored control `stdout_delta=0`.

`phys_fn_000352` mutant `stdout_delta=799`; restored control `stdout_delta=0`.

`phys_fn_000354` mutant `stdout_delta=3958`; restored control `stdout_delta=0`.

`phys_fn_000356` mutant `stdout_delta=2`; restored control `stdout_delta=0`.

`phys_fn_000358` mutant `stdout_delta=1053`; restored control `stdout_delta=0`.

`phys_fn_000360` mutant `stdout_delta=2`; restored control `stdout_delta=0`.

| Row | Mutation | Mutant result | Mutant DLL SHA-256 | Restored DLL SHA-256 |
|---|---|---|---|---|
| `phys_fn_000350` | `mScene->at<NxUserNotify*>(0x6ac) = callback;` → `mScene->at<NxUserNotify*>(0x6ac) = 0;` | `differential target=NxPhysicsSimulationTests oracle_exit=0 candidate_exit=1 both_exit_zero=False stdout_delta=799 stderr_exact=False` | `061F1AE8F21C8C6B1ECBEDD4265C7D4955CE736DC79715E1F582BE29B0AA87DB` | `EA0BCAD959DE5551EAE7FD2A552F7209C3BBBC90D65E13C0465F76E0187034D7` |
| `phys_fn_000352` | `return mScene ? mScene->at<NxUserNotify*>(0x6ac) : 0;` → `return 0;` | `differential target=NxPhysicsSimulationTests oracle_exit=0 candidate_exit=1 both_exit_zero=False stdout_delta=799 stderr_exact=False` | `563645DED5EE58E6739FCA20A88EDCB3D58313B9E1E1C6663B8E658F5A624A42` | `F25D631B99075511DE9AB53445411A588EF76D90C525ACB75A5E15AB5D63CC0E` |
| `phys_fn_000354` | `mScene->at<NxUserTriggerReport*>(0x6b0) = callback;` → `mScene->at<NxUserTriggerReport*>(0x6b0) = 0;` | `differential target=NxPhysicsSimulationTests oracle_exit=0 candidate_exit=1 both_exit_zero=False stdout_delta=3958 stderr_exact=False` | `57A87C1B8808298CB93B59559F34CBFB8A4D5861B26C73738413426358C5DF9F` | `587A335EACAA15B200865EB5729638628BD00A45FD9CBC9FD57FFB8A68722299` |
| `phys_fn_000356` | `return mScene ? mScene->at<NxUserTriggerReport*>(0x6b0) : 0;` → `return 0;` | `differential target=NxPhysicsSimulationTests oracle_exit=0 candidate_exit=0 both_exit_zero=True stdout_delta=2 stderr_exact=True` | `C05DDECAFBD5429AF0E1D79782FE62369521C1513294B27CF8D490CD0091C610` | `375658C34F250901144D38B4A32D9AA1BA74B7B9B707FCA8866766D5FCED6FF5` |
| `phys_fn_000358` | `if(mScene) mScene->at<void*>(0x6b4) = callback;` → `if(mScene) mScene->at<void*>(0x6b4) = 0;` | `differential target=NxPhysicsSimulationTests oracle_exit=0 candidate_exit=1 both_exit_zero=False stdout_delta=1053 stderr_exact=True` | `8452A3A559E516087AD6CD0061775F89DFE4674DC34D6E34BA269EAD11480195` | `7A0D2502B32A4B8BBB9BC3B6C49A2C2B72472625A5377D974B2B22C4D1130D4F` |
| `phys_fn_000360` | `NxUserContactReport* report = mScene ? mScene->at<NxUserContactReport*>(0x6b4) : 0;` → `NxUserContactReport* report = 0;` | `differential target=NxPhysicsSimulationTests oracle_exit=0 candidate_exit=0 both_exit_zero=True stdout_delta=2 stderr_exact=True` | `521D10DE5C46C97BC5F65555680C0AF44B52E8989F65123FBA10806BF150499F` | `B5945FBB4358188CCE3A63ABEAF383C1200E6EA349EAA8B95A456A3D237CCD46` |

Mutations and restored controls are archived as the candidate DLL SHA-256 identities above; full runner transcripts remain in the ignored build directory as `build/npscene-000<row>-mutant.log` and `...-restored.log`. The source patch files contain one changed line per row and apply independently to the recorded mainline source.
