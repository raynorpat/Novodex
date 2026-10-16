# Fluid descriptor `userData` initialization (`phys_fn_003503`)

IDA at RVA `0x00086080` shows the fluid constructor passing the descriptor's final pointer field to `sub_1000EDC0` with the new fluid as the key. That pointer is the `NxFluidDesc::userData` field. The public `NxFluid` base stores its `userData` member at wrapper offset `+4`.

`NxSceneInternal::createFluid` allocated and zeroed the lightweight `NxFluid` wrapper but left that member null. The registered `NxPhysicsSimulationTests` fixture now gives the descriptor a sentinel and checks the returned public member. Before the fix, oracle printed `copied=1`, candidate printed `copied=0`, and the differential failed with `stdout_delta=2`.

The candidate now copies `desc.userData` into `solver+4` when constructing the wrapper. The restored differential reports `oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True`. A no-copy mutation of that assignment again reports `stdout_delta=2` and is rejected by `run_differential.ps1`.

The fixture coverage line is registered in `gate_targets.ps1`; this raises the Phase 5 coverage floor to 2,790. This closes only the descriptor-to-wrapper `userData` behavior. Row `phys_fn_003503` remains open because the full constructor callback table and the remaining function body are not reconstructed.
