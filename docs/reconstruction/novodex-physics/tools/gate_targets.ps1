# The one registry of differential test targets per reconstruction phase.
# run_phase_gate.ps1 and run_differential.ps1 both dot-source this file and
# neither keeps a second list, so target registration is centralized here.
# The phase plans address it through `run_differential.ps1 -Phase N`.
#
# Phase 1 is evidence-only. A phase with
# no targets cannot be gated, so both runners report it skipped and exit 3
# rather than reporting a pass. Phase 4 registers an oracle differential and no
# staged-pair differential, which is why run_phase_gate.ps1 counts all three
# lists before it decides a phase is ungated.

$NxPhaseTestTargets = [ordered] @{
    '1' = @()
    '2' = @('NxPhysicsExportTests', 'NxPhysicsSDKTests', 'NxPhysicsCoreClusterTests')
    '3' = @('NxPhysicsGeometryTests', 'NxPhysicsKernelFuzzTests')
    '4' = @()
    '5' = @('NxPhysicsActorLifecycleTests', 'NxPhysicsDynamicFirstTests', 'NxPhysicsEmptySceneTests', 'NxPhysicsActorNameTests', 'NxPhysicsActorMetadataTests', 'NxPhysicsActorBodyFlagTests', 'NxPhysicsActorDynamicsTests', 'NxPhysicsActorDynamicSetterTests', 'NxPhysicsActorMomentumTests', 'NxPhysicsActorForceTests', 'NxPhysicsActorCMassTests', 'NxPhysicsActorShapeMutationTests')
    '6' = @('NxPhysicsJointStagedPairTests', 'NxFoundationTangentTests', 'NxPhysicsJointAllocatorTests', 'NxPhysicsJointSlotTests')
    '7' = @('NxPhysicsJointStagedPairTests', 'NxPhysicsJointAllocatorTests', 'NxPhysicsJointSlotTests')
    '8' = @()
}

# Static-proof targets are NOT differentials. They link the candidate's own
# private translation units and check them against expectations read out of the
# disassembly, because the rows they cover cannot be reached through the public
# API and so have no oracle-side transcript. They are kept in a separate list so
# run_differential.ps1 can never treat one as a differential and so the phase
# gate reports them under their own name.
$NxPhaseStaticProofTargets = [ordered] @{
    '1' = @()
    '2' = @('NxPhysicsInternalTests')
    '3' = @()
    '4' = @()
    '5' = @()
    '6' = @()
    '7' = @()
    '8' = @()
}

# Oracle differentials, and why they are a third kind rather than either of the
# two above.
#
# A staged-pair differential resolves what it drives with GetProcAddress, so it
# can only reach exports. From Phase 3 Task 3 onwards the rows being
# reconstructed are internal -- the shape-pair dispatch matrix and the overlap
# tests it selects are not exported, and the public path that would reach them
# (a scene, actors, shapes) belongs to phases 4, 5 and 7. A static proof would
# reach them but has no oracle side at all.
#
# These targets take the pinned oracle's directory and its SHA-256, load it,
# check the hash themselves, and then call the recorded internal addresses,
# comparing against the reconstruction linked into the same process. They run
# once, not once per pair, and the comparison is the harness's own -- which is
# the one thing a symmetric differential structurally cannot do, since anything
# it stops covering it stops covering on both sides.
#
# What they still cannot do is notice that they stopped checking anything, so
# their coverage registration below is not optional decoration. Each registered
# line carries an oracle-side digest: it folds the pinned DLL's own answers, so
# it moves if the generator is degraded, if the inputs change, or if the oracle
# is not called -- and no change to the reconstruction can make one of them
# come out right.
$NxPhaseOracleDifferentialTargets = [ordered] @{
    '1' = @()
    '2' = @()
    '3' = @('NxPhysicsCollisionTests')
    '4' = @('NxPhysicsAssetTests', 'NxPhysicsThirdPartyTests')
    '5' = @('NxPhysicsObjectLayoutTests', 'NxPhysicsShapeVtableTests')
    '6' = @('NxPhysicsJointDescTests', 'NxPhysicsJointTests')
    '7' = @()
    '8' = @()
}

# Coverage assertions, and the reason they cannot live inside a harness.
#
# run_differential.ps1 runs ONE binary against two pairs and compares the two
# transcripts, so anything the harness stops covering it stops covering on both
# sides: the delta stays 0 and the gate passes. A self-check inside the harness
# is therefore structurally unable to fail the gate -- it can only print a
# number for a human to notice. That is exactly the shape of gate this program
# has already been caught building nine times.
#
# These are absolute values recorded from a known-good run against the pinned
# oracle. run_phase_gate.ps1 requires each line to appear at least twice in the
# differential transcript, once per pair, and fails if one stops appearing.
# A deliberate check: de-aiming the ray/triangle generator drops its hit count
# from 89978 to 3521 and leaves stdout_delta at 0, and this is what turns that
# from a silent pass into a failure.
$NxRequiredCoverageLines = [ordered] @{
    'NxPhysicsActorShapeMutationTests' = @(
        # NpActor.cpp completion Task 4: createShape (000070 -> Actor.cpp 000036)
        # and releaseShape (000072 -> 000024) on a static and a dynamic actor, every
        # family the harness builds (plane, sphere, box, capsule): the factory's
        # per-family sizes, promotion, appends growing the group arrays 2 -> 6 -> 14,
        # the pruner growth at the fifth prunable, swap-removal at each position,
        # the group teardown, installs into an empty actor, the group's own poses
        # (001018, before and after setGlobalPose), a Scene whose first shape comes
        # from createShape, and the Actor.cpp E1 reports 0x18f/0x19c/0x19d/0x1a4.
        'shape_mutation t4_static=1',
        'shape_mutation t4_dynamic=1',
        'shape_mutation t4_st_initial=1.2.0.s.c2.2.0.0.624.1.p100.10001.0',
        'shape_mutation t4_dy_initial=1.2.0.s.c2.2.0.1.624.1.p100.10001.0',
        'shape_mutation t4_prunables=0.1.2.1',
        'shape_mutation report=1.1ac.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::createShape: desc.isValid() fails!',
        'shape_mutation t4_st_invalid_memory=0.0.f',
        'shape_mutation t4_st_invalid_result=0.1',
        'shape_mutation t4_st_invalid=1.2.0.s.c2.2.0.0.624.1.p100.10001.0',
        'shape_mutation t4_st_sphere_memory=5.0.228.28.272.8.8.f',
        'shape_mutation t4_st_sphere_result=1.0',
        'shape_mutation t4_st_sphere=2.5.0.1.g2.2.2.2.0.bf800000.2.3.624.c2.3.0.0.0.1.c1.3.0.2.0.1.p100.10001.0',
        'shape_mutation t4_st_sphere_world=3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3fc00000.3e277df0.4088d2dc',
        'shape_mutation t4_st_capsule_memory=4.2.236.28.24.24.f.8.8',
        'shape_mutation t4_st_capsule_result=1.0',
        'shape_mutation t4_st_capsule=3.5.0.1.2.g3.6.3.6.0.bf800000.2.3.624.c2.3.0.0.0.1.c1.3.0.2.0.1.c3.3.0.4.0.1.p100.10001.0',
        'shape_mutation t4_st_capsule_world=3f7490ef.0.3e974e6d.3e11148c.3f60a940.beea80a5.be84c8a8.3ef57744.3f56a082.bf800000.3fdeb757.407fd60a',
        'shape_mutation t4_st_plane_memory=4.2.268.28.192.32.f.96.16',
        'shape_mutation t4_st_plane_result=1.0',
        'shape_mutation t4_st_plane=4.5.0.1.2.3.g4.6.4.6.0.bf800000.2.3.624.c2.3.0.0.0.1.c1.3.0.2.0.1.c3.3.0.4.0.1.c0.3.0.5.0.1.p100.10001.0',
        'shape_mutation t4_st_plane_world=3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000',
        'shape_mutation t4_st_box_memory=2.0.552.28.f',
        'shape_mutation t4_st_box_result=1.0',
        'shape_mutation t4_st_box=5.5.0.1.2.3.4.g5.6.5.6.0.bf800000.2.3.624.c2.3.0.0.0.1.c1.3.0.2.0.1.c3.3.0.4.0.1.c0.3.0.5.0.1.c2.3.0.6.0.1.p100.10001.0',
        'shape_mutation t4_st_box_world=3f43ccb3.3f24eb73.0.bf10bb0d.3f2bd490.bef57744.be9e222a.3ebbbe1f.3f60a940.40800000.4056d938.40268498',
        'shape_mutation t4_st_sphere2_memory=2.0.228.28.f',
        'shape_mutation t4_st_sphere2_result=1.0',
        'shape_mutation t4_st_sphere2=6.5.0.1.2.3.4.5.g6.6.6.6.0.bf800000.2.3.624.c2.3.0.0.0.1.c1.3.0.2.0.1.c3.3.0.4.0.1.c0.3.0.5.0.1.c2.3.0.6.0.1.c1.3.0.7.0.1.p100.10001.0',
        'shape_mutation t4_st_sphere2_world=3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3fc00000.3e277df0.4088d2dc',
        'shape_mutation t4_st_capsule2_memory=4.2.236.28.56.56.f.24.24',
        'shape_mutation t4_st_capsule2_result=1.0',
        'shape_mutation t4_st_capsule2=7.5.0.1.2.3.4.5.6.g7.14.7.14.0.bf800000.2.3.624.c2.3.0.0.0.1.c1.3.0.2.0.1.c3.3.0.4.0.1.c0.3.0.5.0.1.c2.3.0.6.0.1.c1.3.0.7.0.1.c3.3.0.8.0.1.p100.10001.0',
        'shape_mutation t4_st_capsule2_world=3f7490ef.0.3e974e6d.3e11148c.3f60a940.beea80a5.be84c8a8.3ef57744.3f56a082.bf800000.3fdeb757.407fd60a',
        'shape_mutation t4_dy_invalid_memory=0.0.f',
        'shape_mutation t4_dy_invalid_result=0.1',
        'shape_mutation t4_dy_invalid=1.2.0.s.c2.2.0.1.624.1.p100.10001.0',
        'shape_mutation t4_dy_capsule_memory=5.0.236.28.272.8.8.f',
        'shape_mutation t4_dy_capsule_result=1.0',
        'shape_mutation t4_dy_capsule=2.5.0.1.g2.2.2.2.0.bf800000.2.a.624.c2.3.0.1.0.1.c3.3.0.9.0.1.p100.10001.0',
        'shape_mutation t4_dy_capsule_world=3f7490ef.0.3e974e6d.3e11148c.3f60a940.beea80a5.be84c8a8.3ef57744.3f56a082.bf800000.3fdeb757.407fd60a',
        'shape_mutation t4_dy_sphere_memory=4.2.228.28.24.24.f.8.8',
        'shape_mutation t4_dy_sphere_result=1.0',
        'shape_mutation t4_dy_sphere=3.5.0.1.2.g3.6.3.6.0.bf800000.2.a.624.c2.3.0.1.0.1.c3.3.0.9.0.1.c1.3.0.b.0.1.p100.10001.0',
        'shape_mutation t4_dy_sphere_world=3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3fc00000.3e277df0.4088d2dc',
        'shape_mutation t4_dy_box_memory=4.2.552.28.192.32.f.96.16',
        'shape_mutation t4_dy_box_result=1.0',
        'shape_mutation t4_dy_box=4.5.0.1.2.3.g4.6.4.6.0.bf800000.2.a.624.c2.3.0.1.0.1.c3.3.0.9.0.1.c1.3.0.b.0.1.c2.3.0.c.0.1.p100.10001.0',
        'shape_mutation t4_dy_box_world=3f43ccb3.3f24eb73.0.bf10bb0d.3f2bd490.bef57744.be9e222a.3ebbbe1f.3f60a940.40800000.4056d938.40268498',
        'shape_mutation t4_st_group_pose=3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.0.2',
        'shape_mutation t4_dy_group_pose=3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.0.2',
        'shape_mutation t4_st_group_moved=3f60a940.3ef57744.0.bef57744.3f60a940.0.0.0.3f800000.bf800000.3f000000.40800000.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.0.2',
        'shape_mutation t4_dy_group_moved=3f60a940.3ef57744.0.bef57744.3f60a940.0.0.0.3f800000.bf800000.3f000000.40800000.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.0.2',
        'shape_mutation t4_dy_child_moved=3eb986f5.3f6e9a1d.0.bf6e9a1d.3eb986f5.0.0.0.3f800000.40072dd8.bd789a60.40400000',
        'shape_mutation t4_st_release_middle_memory=0.0.f',
        'shape_mutation t4_st_release_middle_errors=0',
        'shape_mutation t4_st_release_middle=6.5.0.1.2.6.4.5.g6.14.6.14.0.bf800000.2.3.624.c2.3.0.0.0.1.c1.3.0.2.0.1.c3.3.0.4.0.1.c3.3.0.8.0.1.c2.3.0.6.0.1.c1.3.0.7.0.1.p100.10001.0',
        'shape_mutation t4_st_release_first_memory=0.0.f',
        'shape_mutation t4_st_release_first_errors=0',
        'shape_mutation t4_st_release_first=5.5.5.1.2.6.4.g5.14.5.14.0.bf800000.2.3.624.c1.3.0.7.0.1.c1.3.0.2.0.1.c3.3.0.4.0.1.c3.3.0.8.0.1.c2.3.0.6.0.1.p100.10001.0',
        'shape_mutation t4_st_release_last_memory=0.0.f',
        'shape_mutation t4_st_release_last_errors=0',
        'shape_mutation t4_st_release_last=4.5.5.1.2.6.g4.14.4.14.0.bf800000.2.3.624.c1.3.0.7.0.1.c1.3.0.2.0.1.c3.3.0.4.0.1.c3.3.0.8.0.1.p100.10001.0',
        'shape_mutation t4_st_release_foreign_memory=0.0.f',
        'shape_mutation t4_st_release_foreign_errors=0',
        'shape_mutation t4_st_release_foreign=4.5.5.1.2.6.g4.14.4.14.0.bf800000.2.3.624.c1.3.0.7.0.1.c1.3.0.2.0.1.c3.3.0.4.0.1.c3.3.0.8.0.1.p100.10001.0',
        'shape_mutation t4_st_release_down_memory=0.0.f',
        'shape_mutation t4_st_release_down_errors=0',
        'shape_mutation t4_st_release_down=3.5.6.1.2.g3.14.3.14.0.bf800000.2.3.624.c3.3.0.8.0.1.c1.3.0.2.0.1.c3.3.0.4.0.1.p100.10001.0',
        'shape_mutation t4_st_release_down=2.5.2.1.g2.14.2.14.0.bf800000.2.3.624.c3.3.0.4.0.1.c1.3.0.2.0.1.p100.10001.0',
        'shape_mutation t4_st_release_down=1.5.1.g1.14.1.14.0.bf800000.2.3.624.c1.3.0.2.0.1.p100.10001.0',
        'shape_mutation report=1.18f.\Epic\Novodex\SDKs\Physics\src\Actor.cpp.Actor::releaseShape: Can''t release shape: A static actor can''t be left with no shapes!',
        'shape_mutation t4_st_release_only_memory=0.0.f',
        'shape_mutation t4_st_release_only_errors=1',
        'shape_mutation t4_st_release_only=1.5.1.g1.14.1.14.0.bf800000.2.3.624.c1.3.0.2.0.1.p100.10001.0',
        'shape_mutation t4_dy_release_middle_memory=0.0.f',
        'shape_mutation t4_dy_release_middle_errors=0',
        'shape_mutation t4_dy_release_middle=3.5.0.1.3.g3.6.3.6.0.bf800000.2.a.624.c2.3.0.1.0.1.c3.3.0.9.0.1.c2.3.0.c.0.1.p100.10001.0',
        'shape_mutation t4_dy_release_down_memory=0.0.f',
        'shape_mutation t4_dy_release_down_errors=0',
        'shape_mutation t4_dy_release_down=2.5.0.1.g2.6.2.6.0.bf800000.2.a.624.c2.3.0.1.0.1.c3.3.0.9.0.1.p100.10001.0',
        'shape_mutation t4_dy_release_down=1.5.0.g1.6.1.6.0.bf800000.2.a.624.c2.3.0.1.0.1.p100.10001.0',
        'shape_mutation t4_dy_release_only_memory=1.3.8.f.24.24.272',
        'shape_mutation t4_dy_release_only_errors=0',
        'shape_mutation t4_dy_release_only=0.ffffffff.p100.10000.0',
        'shape_mutation report=1.1a4.\Epic\Novodex\SDKs\Physics\src\Actor.cpp.Actor::releaseShape: shape not found!',
        'shape_mutation t4_dy_release_empty_memory=0.0.f',
        'shape_mutation t4_dy_release_empty_errors=1',
        'shape_mutation t4_dy_release_empty=0.ffffffff.p100.10000.0',
        'shape_mutation t4_dy_install_memory=2.0.228.28.f',
        'shape_mutation t4_dy_install_result=1.0',
        'shape_mutation t4_dy_install=1.1.4.s.c1.2.0.a.624.1.p100.10001.0',
        'shape_mutation t4_dy_install_world=3f60a940.3ef57744.0.bef57744.3f60a940.0.0.0.3f800000.bf853381.bf1e0710.40c00000',
        'shape_mutation report=1.19d.\Epic\Novodex\SDKs\Physics\src\Actor.cpp.Actor::releaseShape: shape not found!',
        'shape_mutation t4_dy_release_mismatch_memory=0.0.f',
        'shape_mutation t4_dy_release_mismatch_errors=1',
        'shape_mutation t4_dy_release_mismatch=1.1.4.s.c1.2.0.a.624.1.p100.10001.0',
        'shape_mutation t4_dy_release_single_memory=0.2.f.28.228',
        'shape_mutation t4_dy_release_single_errors=0',
        'shape_mutation t4_dy_release_single=0.ffffffff.p100.10000.0',
        'shape_mutation t4_dy_install2_memory=2.0.236.28.f',
        'shape_mutation t4_dy_install2_result=1.0',
        'shape_mutation t4_dy_install2=1.3.5.s.c3.2.0.a.624.1.p100.10001.0',
        'shape_mutation t4_dy_install2_world=3f56a082.3ef57744.3e84c8a8.beea80a5.3f60a940.be11148c.be974e6d.0.3f7490ef.c028a8e6.3fd6d0ca.40a00000',
        'shape_mutation t4_dy_promote_memory=5.0.268.28.272.8.8.f',
        'shape_mutation t4_dy_promote_result=1.0',
        'shape_mutation t4_dy_promote=2.5.5.6.g2.2.2.2.0.bf800000.2.e.624.c3.3.0.a.0.1.c0.3.0.d.0.1.p100.10001.0',
        'shape_mutation t4_dy_promote_world=3f60a940.3ef57744.0.bef57744.3f60a940.0.0.0.3f800000.bf800000.3f000000.40800000',
        'shape_mutation t4_single=1',
        'shape_mutation report=1.19c.\Epic\Novodex\SDKs\Physics\src\Actor.cpp.Actor::releaseShape: Can''t release shape: A static actor can''t be left with no shapes!',
        'shape_mutation t4_ss_release_only_memory=0.0.f',
        'shape_mutation t4_ss_release_only_errors=1',
        'shape_mutation t4_ss_release_only=1.1.0.s.c1.2.0.f.624.1.p100.20001.0',
        'shape_mutation t4_ss_box_memory=5.0.552.28.272.8.8.f',
        'shape_mutation t4_ss_box_result=1.0',
        'shape_mutation t4_ss_box=2.5.0.1.g2.2.2.2.0.bf800000.2.11.624.c1.3.0.f.0.1.c2.3.0.10.0.1.p100.20001.0',
        'shape_mutation t4_ss_box_world=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'shape_mutation t4_dy_actor_release_memory=3.11.8.8.24.f.24.608.28.236.28.268.8.8.8.272.80',
        'shape_mutation t4_st_actor_release_memory=0.7.f.24.28.228.56.56.272.80',
        'shape_mutation t4_bare_create_memory=15.3.80.24.608.2048.1024.2048.1024.2048.1024.1024.1024.8.1024.1024.8.f.2048.2048.2048',
        'shape_mutation t4_bare=1.0',
        'shape_mutation t4_bare_box_memory=14.3.552.2048.1024.2048.1024.2048.1024.1024.1024.28.60.96.16.8.f.2048.2048.2048',
        'shape_mutation t4_bare_box_result=1.0',
        'shape_mutation t4_bare_box=1.2.0.s.c2.2.0.0.624.1.p100.1.0',
        'shape_mutation t4_bare_box_world=3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000',
        'shape_mutation t4_bare_capsule_memory=5.0.236.28.272.8.8.f',
        'shape_mutation t4_bare_capsule_result=1.0',
        'shape_mutation t4_bare_capsule=2.5.0.1.g2.2.2.2.0.bf800000.2.2.624.c2.3.0.0.0.1.c3.3.0.1.0.1.p100.1.0',
        'shape_mutation t4_bare_capsule_world=3f7490ef.0.3e974e6d.3e11148c.3f60a940.beea80a5.be84c8a8.3ef57744.3f56a082.bf800000.3fdeb757.407fd60a',
        'shape_mutation t4_bare_group_pose=3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.0.2',
        'shape_mutation t4_bare_release_box_memory=0.0.f',
        'shape_mutation t4_bare_release_box_errors=0',
        'shape_mutation t4_bare_release_box=1.5.1.g1.2.1.2.0.bf800000.2.2.624.c3.3.0.1.0.1.p100.1.0',
        'shape_mutation t4_bare_release_capsule_memory=1.3.8.f.8.8.272',
        'shape_mutation t4_bare_release_capsule_errors=0',
        'shape_mutation t4_bare_release_capsule=0.ffffffff.p100.0.0',
        'shape_mutation t4_bare_release_memory=2.3.8.8.f.24.608.80',
        # NpActor.cpp completion Task 5: updateMassFromShapes (000164 -> Actor.cpp 000008 ->
        # each family's slot 4: box 000947, sphere 001371, capsule 001008, the group's 001024,
        # the plane's base 001249) with a density and with a total mass, on rotated and
        # centred shapes, a group, a trigger (skipped) and a plane (false: E1 0xa8), every
        # argument E1 (0x9a with NaN and -0.0, 0x9d, 0x9e, 0x9f, 0xa0, 0xa9), extreme values
        # (a denormal mass, an infinite density, FLT_MAX) and G1 0x98.
        'shape_mutation t5_created=1.1.1.1.1.1.1.1.1.1.1',
        'shape_mutation t5_box_initial=0.m40000000.3f000000.i3f800000.40000000.40400000.3f800000.3f000000.3eaaaaab.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.3e7d5777.0.0.3f780aa5.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.3f800000.0.0.0.3eec62e0.3d8f9c6e.0.3d8f9c6e.3ebe47cb.n2',
        'shape_mutation t5_box_density=0.m40800000.3e800000.i40ad5556.40b55556.3ed55580.3e3d0bd0.3e34b4b4.4019997b.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3e3995b3.bb66cf3c.bafc2ed8.bb66cf3c.3f30ad89.bf6f20b0.bafc2ed8.bf6f20b0.3ff1e18c.n4',
        'shape_mutation t5_box_total=0.m41200000.3dcccccd.i4158aaac.4162aaab.3f855570.3d973ca6.3d909090.3f75c25e.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3d9477c2.bab8a5be.ba49bf07.bab8a5be.3e8d57a0.bebf4d59.ba49bf07.bebf4d59.3f41813c.n6',
        'shape_mutation t5_sphere_density=0.m4029a560.3ec127b9.i3f18ae70.3f18ae70.3f18ae70.3fd69dea.3fd69dea.3fd69dea.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.3f000000.bf800000.40000000.w.3e7d5777.0.0.3f780aa5.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3fc00000.3e277df0.4088d2dc.3fd69dea.0.0.0.3fd69dea.3325df44.0.3325df44.3fd69dea.n4',
        'shape_mutation t5_sphere_total=0.m40e00000.3e124925.i3fc99999.3fc99999.3fc99999.3f228a29.3f228a29.3f228a29.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.3f000000.bf800000.40000000.w.3e7d5777.0.0.3f780aa5.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3fc00000.3e277df0.4088d2dc.3f228a29.0.0.0.3f228a29.b0c69d80.0.b0c69d80.3f228a29.n6',
        'shape_mutation t5_capsule_density=0.m3f96cbe4.3f594caf.i3ed27c9b.3d16cc00.3ed27c98.401bad61.41d94c87.401bad63.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.c0000000.3e800000.3f800000.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.c0000000.3e800000.3f800000.401bad61.0.0.0.41d94c87.0.0.0.401bad63.n4',
        'shape_mutation t5_capsule_total=0.m3e99999a.40555555.i3dd66669.3c1999b6.3dd66666.4118d5f7.42d5552e.4118d5f9.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.c0000000.3e800000.3f800000.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.c0000000.3e800000.3f800000.4118d5f7.0.0.0.42d5552e.0.0.0.4118d5f9.n6',
        'shape_mutation t5_centred_density=0.m41c00000.3d2aaaab.i42d00000.42a00000.42200000.3c1d89d9.3c4ccccd.3ccccccd.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3c1d89d9.0.0.0.3c4ccccd.0.0.0.3ccccccd.n4',
        'shape_mutation t5_centred_total=0.m41400000.3daaaaab.i42500000.42200000.41a00000.3c9d89d9.3ccccccd.3d4ccccd.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3c9d89d9.0.0.0.3ccccccd.0.0.0.3d4ccccd.n6',
        'shape_mutation t5_group_density=0.m40a664d0.3e44ee30.i40c70631.42015bf3.41e7f464.3e24a4b0.3cfd4f6a.3d0d44dd.f.3f24fdc9.bcc07981.3f43a595.3ebc4028.3f62ca66.be90cf38.bf2b9f5c.3eed32f0.3f1460a3.3fbba4a5.3da2f962.3eed2d4d.w.3ed8a27d.3ebbbc88.3e55161e.3f4d50a3.3f24fdc9.bcc07981.3f43a595.3f24e208.3f0e2af9.bf06ad55.bed2f942.3f54cf60.3ebf006b.401dd252.3fec8304.405c75a1.3db2118f.3d56ecff.bd091860.3d56ecff.3dafad90.bd100971.bd091860.bd100971.3d5b0111.n4',
        'shape_mutation t5_group_total=0.m41100000.3de38e39.i412c3d28.425fe634.4248bccb.3dbe3f55.3c9259ff.3ca33cf3.f.3f24fdc9.bcc07954.3f43a595.3ebc4027.3f62ca65.be90cf3b.bf2b9f5c.3eed32f2.3f1460a3.3fbba4a5.3da2f962.3eed2d4d.w.3ed8a27e.3ebbbc88.3e55161c.3f4d50a2.3f24fdc9.bcc07954.3f43a595.3f24e208.3f0e2af8.bf06ad56.bed2f943.3f54cf60.3ebf006a.401dd252.3fec8304.405c75a1.3d4dc2ae.3cf85971.bc9e6a4c.3cf85971.3d4aff83.bca66fba.bc9e6a4c.bca66fba.3cfd0fe3.n6',
        'shape_mutation t5_mixed_density=0.m406231d6.3e90ddca.i3f4b9340.3f4b9340.3f4b9340.3fa0f66f.3fa0f66f.3fa0f66f.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.3f000000.bf800000.40000000.w.3e7d5777.0.0.3f780aa5.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3fc00000.3e277df0.4088d2dc.3fa0f66f.0.0.0.3fa0f66f.b2e7b8a0.0.b2e7b8a0.3fa0f66f.n4',
        'shape_mutation report=1.a8.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::updateMassFromShapes: Compute mesh inertia tensor failed for one of the actor''s mesh shapes! Please change mesh geometry or supply a tensor manually!',
        'shape_mutation t5_plane_density=1.m40000000.3f000000.i3f800000.40000000.40400000.3f800000.3f000000.3eaaaaab.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3f000000.0.0.0.3eaaaaab.n2',
        'shape_mutation t5_plane_group=1.m40000000.3f000000.i3f800000.40000000.40400000.3f800000.3f000000.3eaaaaab.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3f000000.0.0.0.3eaaaaab.n2',
        'shape_mutation report=1.a9.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::updateMassFromShapes: Can''t compute mass from shapes: must have at least one non-trigger shape!',
        'shape_mutation t5_trigger_density=1.m40000000.3f000000.i3f800000.40000000.40400000.3f800000.3f000000.3eaaaaab.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3f000000.0.0.0.3eaaaaab.n2',
        'shape_mutation t5_trigger_total=1.m40000000.3f000000.i3f800000.40000000.40400000.3f800000.3f000000.3eaaaaab.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3f000000.0.0.0.3eaaaaab.n2',
        'shape_mutation report=1.9a.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::updateMassFromShapes: density and total Mass of a shape have to be nonnegative!',
        'shape_mutation t5_negative_density=1.m41200000.3dcccccd.i4158aaac.4162aaab.3f855570.3d973ca6.3d909090.3f75c25e.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3d9477c2.bab8a5be.ba49bf07.bab8a5be.3e8d57a0.bebf4d59.ba49bf07.bebf4d59.3f41813c.n6',
        'shape_mutation t5_negative_total=1.m41200000.3dcccccd.i4158aaac.4162aaab.3f855570.3d973ca6.3d909090.3f75c25e.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3d9477c2.bab8a5be.ba49bf07.bab8a5be.3e8d57a0.bebf4d59.ba49bf07.bebf4d59.3f41813c.n6',
        'shape_mutation t5_nan_density=1.m41200000.3dcccccd.i4158aaac.4162aaab.3f855570.3d973ca6.3d909090.3f75c25e.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3d9477c2.bab8a5be.ba49bf07.bab8a5be.3e8d57a0.bebf4d59.ba49bf07.bebf4d59.3f41813c.n6',
        'shape_mutation t5_nan_total=1.m41200000.3dcccccd.i4158aaac.4162aaab.3f855570.3d973ca6.3d909090.3f75c25e.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3d9477c2.bab8a5be.ba49bf07.bab8a5be.3e8d57a0.bebf4d59.ba49bf07.bebf4d59.3f41813c.n6',
        'shape_mutation report=1.9f.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::updateMassFromShapes: density or total mass must be nonzero!',
        'shape_mutation t5_both_zero=1.m41200000.3dcccccd.i4158aaac.4162aaab.3f855570.3d973ca6.3d909090.3f75c25e.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3d9477c2.bab8a5be.ba49bf07.bab8a5be.3e8d57a0.bebf4d59.ba49bf07.bebf4d59.3f41813c.n6',
        'shape_mutation t5_negative_zero=1.m41200000.3dcccccd.i4158aaac.4162aaab.3f855570.3d973ca6.3d909090.3f75c25e.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3d9477c2.bab8a5be.ba49bf07.bab8a5be.3e8d57a0.bebf4d59.ba49bf07.bebf4d59.3f41813c.n6',
        'shape_mutation report=1.a0.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::updateMassFromShapes: density and total mass may not both be nonzero!',
        'shape_mutation t5_both_nonzero=1.m41200000.3dcccccd.i4158aaac.4162aaab.3f855570.3d973ca6.3d909090.3f75c25e.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3d9477c2.bab8a5be.ba49bf07.bab8a5be.3e8d57a0.bebf4d59.ba49bf07.bebf4d59.3f41813c.n6',
        'shape_mutation report=1.9d.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::updateMassFromShapes: Actor must be dynamic!',
        'shape_mutation t5_static=1.static',
        'shape_mutation t5_static_negative=1.static',
        'shape_mutation report=1.9e.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::updateMassFromShapes: Actor must have shapes!',
        'shape_mutation t5_bare=1.m40000000.3f000000.i3f800000.40000000.40400000.3f800000.3f000000.3eaaaaab.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.3e7d5777.0.0.3f780aa5.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.3f800000.0.0.0.3eec62e0.3d8f9c6e.0.3d8f9c6e.3ebe47cb.n2',
        'shape_mutation t5_bare_zero=1.m40000000.3f000000.i3f800000.40000000.40400000.3f800000.3f000000.3eaaaaab.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.3e7d5777.0.0.3f780aa5.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.3f800000.0.0.0.3eec62e0.3d8f9c6e.0.3d8f9c6e.3ebe47cb.n2',
        'shape_mutation t5_tiny_density=0.m300.7f800000.id00.a00.500.0.0.0.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.0.0.0.0.0.0.0.0.0.n8',
        'shape_mutation t5_inf_density=0.m7f800000.0.i3f800000.3f800000.3f800000.3f800000.3f800000.3f800000.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3f800000.na',
        'shape_mutation t5_huge_total=0.m7f7fffff.200000.i7f800000.7f800000.7f800000.0.0.0.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.0.0.0.0.0.0.0.0.0.nc',
        'shape_mutation t5_centred_again=0.m41c00000.3d2aaaab.i42d00000.42a00000.42200000.3c1d89d9.3c4ccccd.3ccccccd.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.3c1d89d9.0.0.0.3c4ccccd.0.0.0.3ccccccd.ne',
        'shape_mutation report=2.98.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'shape_mutation t5_locked=1.m41200000.3dcccccd.i4158aaac.4162aaab.3f855570.3d973ca6.3d909090.3f75c25e.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3d9477c2.bab8a5be.ba49bf07.bab8a5be.3e8d57a0.bebf4d59.ba49bf07.bebf4d59.3f41813c.n6',
        # NpActor.cpp completion Task 5: the creation path's mass pass (000034 -> Actor.cpp 000026
        # -> 000008) over each family with a density or a total mass, its E1 0xe5/0xe6 and Scene.cpp
        # 0x228 reports; setDynamic (000122) on static actors with one shape, several shapes (a group)
        # and a density, on a dynamic actor again and on a shapeless one (E1 0x66, then a tensor), a
        # negative mass and a NaN pose (0x63), a trigger (0x7d) and a plane (0x7c) whose shapes stay
        # out of the Scene, G1 0x5b, a jointed actor (004103 breaks the joint), each then used with a
        # force, a torque, a velocity and the readbacks, and released (000628 -> 000030).
        'shape_mutation t5_dynamic_created=1.1',
        'shape_mutation t5_create_box_density=0.m40800000.3e800000.i40ad5556.40b55556.3ed55580.3e3d0bd0.3e34b4b4.4019997b.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3e3995b3.bb66cf3c.bafc2ed8.bb66cf3c.3f30ad89.bf6f20b0.bafc2ed8.bf6f20b0.3ff1e18c.n2',
        'shape_mutation t5_create_sphere_total=0.m40a00000.3e4ccccd.i3f900000.3f900000.3f900000.3f638e39.3f638e39.3f638e39.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.3f000000.bf800000.40000000.w.3e7d5777.0.0.3f780aa5.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3fc00000.3e277df0.4088d2dc.3f638e39.0.0.0.3f638e39.325a3120.0.325a3120.3f638e39.n2',
        'shape_mutation t5_create_capsule_density=0.m3f16cbe4.3fd94caf.i3e527c9b.3c96cc00.3e527c98.409bad61.42594c87.409bad63.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.c0000000.3e800000.3f800000.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.c0000000.3e800000.3f800000.409bad61.0.0.0.42594c87.0.0.0.409bad63.n2',
        'shape_mutation t5_create_group_density=0.m4047ac2c.3ea41bd3.i406ed43e.419b3b24.418b2c3c.3e8933e6.3d531782.3d6b72c5.f.3f24fdc9.bcc079d4.3f43a595.3ebc4027.3f62ca67.be90cf31.bf2b9f5c.3eed32eb.3f1460a5.3fbba4a5.3da2f962.3eed2d4d.w.3ed8a27a.3ebbbc87.3e551620.3f4d50a3.3f24fdc9.bcc079d4.3f43a595.3f24e208.3f0e2afb.bf06ad53.bed2f943.3f54cf5e.3ebf0072.401dd252.3fec8304.405c75a1.3e1463f6.3db31ad1.bd647df2.3db31ad1.3e1265f6.bd700fba.bd647df2.bd700fba.3db680e3.n2',
        'shape_mutation t5_create_group_total=0.m40c00000.3e2aaaab.i40e5a6e0.42154423.4205d332.3e0eaf80.3cdb86ff.3cf4db6c.f.3f24fdc9.bcc079b3.3f43a596.3ebc4027.3f62ca66.be90cf34.bf2b9f5c.3eed32ed.3f1460a4.3fbba4a5.3da2f962.3eed2d4d.w.3e589dfb.3ed04268.3de3231d.3f61bafa.3f24fdc9.bcc079b3.3f43a596.3ebc4027.3f62ca66.be90cf34.bf2b9f5c.3eed32ed.3f1460a4.3fbba4a5.3da2f962.3eed2d4d.3d9a5203.3cd4ff88.bd4190cc.3cd4ff88.3d2d1842.bce767fb.bd4190cc.bce767fb.3da09976.n2',
        'shape_mutation report=1.e6.\Epic\Novodex\SDKs\Physics\src\Actor.cpp.Actor::loadFromDescInternal: Can''t compute mass from shapes: must have at least one non-trigger shape!',
        'shape_mutation report=1.228.\Epic\Novodex\SDKs\Physics\src\Scene.cpp.Actor Initialisation failed: returned NULL.',
        'shape_mutation report=1.e5.\Epic\Novodex\SDKs\Physics\src\Actor.cpp.Actor::loadFromDescInternal: Compute mesh inertia tensor failed for one of the actor''s mesh shapes! Please change mesh geometry or supply a tensor manually!',
        'shape_mutation t5_create_failures=0.0.4',
        'shape_mutation t5_statics=1.1.1.1.1',
        'shape_mutation t5_single_before=0.1.r2.0.1.624.2.s6.100.60005',
        'shape_mutation t5_single_dynamic_memory=2.1.608.56.f.24',
        'shape_mutation t5_single_dynamic=0.m40400000.3eaaaaab.i3f000000.3fc00000.40200000.40000000.3f2aaaab.3ecccccd.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.3e7d5777.0.0.3f780aa5.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.40000000.0.0.0.3f1af9c5.3de5c717.0.3de5c717.3eec2e9a.n2',
        'shape_mutation t5_single_dynamic_root=1.1.r2.2.1.624.2.s7.100.50006',
        'shape_mutation t5_single_use=3eaaaaab.3f2aaaab.3f800000.bf800000.3ec0606e.3f735cd3.3f000000.bf800000.3e800000.3f000000.3fc00000.40200000.3f800000.40000000.40400000.40400000',
        'shape_mutation t5_single_mass=0.m40000000.3f000000.i402d5556.40355556.3e555580.3ebd0bd0.3eb4b4b4.4099997b.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3eb995b3.bbe6cf3c.bb7c2ed8.bbe6cf3c.3fb0ad89.bfef20b0.bb7c2ed8.bfef20b0.4071e18c.n4',
        'shape_mutation t5_several_before=0.3.r5.0.2.624.2.s7.100.50006',
        'shape_mutation t5_several_dynamic_memory=1.0.608.f',
        'shape_mutation t5_several_dynamic=0.m40800000.3e800000.i411f3828.424ef985.42399050.3dcdcddb.3c9e51a2.3cb09614.f.3f24fdc9.bcc079a0.3f43a595.3ebc4027.3f62ca66.be90cf35.bf2b9f5c.3eed32ee.3f1460a4.3fbba4a5.3da2f962.3eed2d4d.w.3ed8a27b.3ebbbc88.3e55161e.3f4d50a3.3f24fdc9.bcc079a0.3f43a595.3f24e208.3f0e2afa.bf06ad54.bed2f943.3f54cf5f.3ebf006f.401dd252.3fec8304.405c75a1.3d5e95f2.3d06541e.bcab5e77.3d06541e.3d5b98f3.bcb40bcd.bcab5e77.bcb40bcd.3d08e0ab.n2',
        'shape_mutation t5_several_dynamic_root=1.3.r5.2.2.624.2.s8.100.40007',
        'shape_mutation t5_several_use=3e800000.3f000000.3f400000.bd791468.bd404f9f.3d930bbd.3f000000.bf800000.3e800000.411f3828.424ef985.42399050.401dd252.3fec8304.405c75a1.40800000',
        'shape_mutation t5_dense_dynamic_memory=3.2.608.768.128.f.384.64',
        'shape_mutation t5_dense_dynamic=0.m3e490fdb.40a2f983.i3d8c5312.3bc91000.3d8c5310.41698412.4322f965.41698415.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.c0000000.3e800000.3f800000.w.0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.c0000000.3e800000.3f800000.41698412.0.0.0.4322f965.0.0.0.41698415.n2',
        'shape_mutation t5_dense_dynamic_root=1.1.r3.2.1.624.2.s9.100.30008',
        'shape_mutation t5_dense_use=40a2f983.4122f983.41747644.c0e98412.4222f965.41e98415.3f000000.bf800000.3e800000.3d8c5312.3bc91000.3d8c5310.c0000000.3e800000.3f800000.3e490fdb',
        'shape_mutation t5_single_again_memory=2.1.608.8.f.608',
        'shape_mutation t5_single_again=0.m40800000.3e800000.i40ad5556.40b55556.3ed55580.3e3d0bd0.3e34b4b4.4019997b.f.3f43cc8c.3f24eba1.0.bf24eba1.3f43cc8c.0.0.0.3f800000.40400000.3f800000.bf800000.w.3e6dfb6a.3dadbdcf.beaa1b4e.3f6900db.3f43cc8c.3f24eba1.0.bf10bb36.3f2bd46d.bef57744.be9e2256.3ebbbdfa.3f60a940.40800000.4056d938.40268498.3e3995b3.bb66cf3c.bafc2ed8.bb66cf3c.3f30ad89.bf6f20b0.bafc2ed8.bf6f20b0.3ff1e18c.n2',
        'shape_mutation t5_single_again_root=1.1.r2.2.1.624.2.s9.100.30008',
        'shape_mutation t5_single_again_use=3e800000.3f000000.3f400000.bdc344c8.bfd8d14b.4062ff44.3f000000.bf800000.3e800000.40ad5556.40b55556.3ed55580.40800000.4056d938.40268498.40800000',
        'shape_mutation report=1.66.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setDynamic: we need a valid massSpaceInertia if the actor has no shapes!',
        'shape_mutation t5_bare_no_tensor_memory=0.0.f',
        'shape_mutation t5_bare_no_tensor=1.m40000000.3f000000.i3f800000.40000000.40400000.3f800000.3f000000.3eaaaaab.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.3e7d5777.0.0.3f780aa5.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.3f800000.0.0.0.3eec62e0.3d8f9c6e.0.3d8f9c6e.3ebe47cb.n2',
        'shape_mutation t5_bare_no_tensor_root=1.0.s9.100.30008',
        'shape_mutation t5_bare_tensor_memory=1.1.608.f.608',
        'shape_mutation t5_bare_tensor=0.m40400000.3eaaaaab.i3f000000.3fc00000.40200000.40000000.3f2aaaab.3ecccccd.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.3e7d5777.0.0.3f780aa5.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.40000000.0.0.0.3f1af9c5.3de5c717.0.3de5c717.3eec2e9a.n2',
        'shape_mutation t5_bare_tensor_root=1.0.s9.100.30008',
        'shape_mutation t5_bare_use=3eaaaaab.3f2aaaab.3f800000.bf800000.3ec0606e.3f735cd3.3f000000.bf800000.3e800000.3f000000.3fc00000.40200000.3f800000.40000000.40400000.40400000',
        'shape_mutation report=1.63.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setDynamic: desc.isValid() fails!',
        'shape_mutation t5_negative_mass_memory=0.0.f',
        'shape_mutation t5_negative_mass=1.static',
        'shape_mutation t5_negative_mass_root=0.1.r2.0.1.624.2.s9.100.30008',
        'shape_mutation t5_nan_pose_memory=0.0.f',
        'shape_mutation t5_nan_pose=1.static',
        'shape_mutation t5_nan_pose_root=0.1.r2.0.1.624.2.s9.100.30008',
        'shape_mutation t5_trigger_before=0.1.r2.0.1.624.2.s9.100.30008',
        'shape_mutation report=1.7d.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setDynamic: Can''t compute mass from shapes: must have at least one non-trigger shape!',
        'shape_mutation t5_trigger_dynamic_memory=0.0.f',
        'shape_mutation t5_trigger_dynamic=1.static',
        'shape_mutation t5_trigger_dynamic_root=0.1.r2.0.1.0.2.s9.100.20008',
        'shape_mutation t5_plane_before=0.1.r0.0.1.624.2.s9.100.20008',
        'shape_mutation report=1.7c.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setDynamic: Compute mesh inertia tensor failed for one of the actor''s mesh shapes! Please change mesh geometry or supply a tensor manually!',
        'shape_mutation t5_plane_dynamic_memory=0.0.f',
        'shape_mutation t5_plane_dynamic=1.static',
        'shape_mutation t5_plane_dynamic_root=0.1.r0.0.1.0.2.s9.100.10008',
        'shape_mutation report=2.5b.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'shape_mutation t5_locked_dynamic_memory=0.0.f',
        'shape_mutation t5_locked_dynamic=1.static',
        'shape_mutation t5_locked_dynamic_root=0.1.r2.0.1.624.2.s9.100.10008',
        'shape_mutation t5_joint=1.1.0',
        'shape_mutation t5_jointed_dynamic_memory=1.1.608.f.608',
        'shape_mutation t5_jointed_dynamic=0.m40400000.3eaaaaab.i3f000000.3fc00000.40200000.40000000.3f2aaaab.3ecccccd.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.3e7d5777.0.0.3f780aa5.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.3f800000.40000000.40400000.40000000.0.0.0.3f1af9c5.3de5c717.0.3de5c717.3eec2e9a.n2',
        'shape_mutation t5_jointed_dynamic_root=1.1.r2.2.1.624.2.s11.100.1000a',
        'shape_mutation t5_joint_after=2.0.0.1',
        'shape_mutation t5_jointed_use=3eaaaaab.3f2aaaab.3f800000.bf800000.3ec0606e.3f735cd3.3f000000.bf800000.3e800000.3f000000.3fc00000.40200000.3f800000.40000000.40400000.40400000',
        'shape_mutation t5_joint_release_memory=1.11.24.f.24.608.28.552.80.24.8.608.28.228.80',
        'shape_mutation t5_single_release_memory=2.7.24.24.f.24.608.8.28.8.552.80',
        'shape_mutation t5_several_release_memory=1.13.56.f.24.608.28.552.28.228.28.236.24.24.24.272.80',
        'shape_mutation t5_bare_release_memory=0.3.f.24.608.80',
        # NpActor.cpp completion Task 5 review: boxes and capsules at inexact translations (000849
        # and 000853 through 000833's displaced path, 000829's float spills) through
        # updateMassFromShapes and creation, and setDynamic with a quiet and a signalling NaN mass
        # (000793's fld/fst quiets it).
        'shape_mutation t5r_0_box_density=0.m4019c0ec.3ed51ec6.i3fae410c.3eedce3a.3f8540ce.3f3c0c18.4009cb12.3f75e86c.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.3dcccccd.3e4cccce.3e99999a.w.0.3e190650.0.3f7d201a.3f7490ee.0.3e974e6d.0.3f800000.0.be974e6d.0.3f7490ee.bf040c24.3fc00000.3f350128.3f4119af.0.3d82aead.0.4009cb12.0.3d82aead.0.3f70dad3.n4',
        'shape_mutation t5r_0_box_total=0.m40966666.3e59df52.i402a740e.3f689e61.400258c0.3ec03d84.3f8cdda1.3efb6422.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.3dcccccd.3e4cccce.3e99999a.w.0.3e190650.0.3f7d201a.3f7490ee.0.3e974e6d.0.3f800000.0.be974e6d.0.3f7490ee.bf040c24.3fc00000.3f350128.3ec567f3.0.3d0598aa.0.3f8cdda1.0.3d0598aa.0.3ef639b1.n6',
        'shape_mutation t5r_0_capsule_density=0.m3fe298c8.3f109bfa.i3f24f18e.3dde1098.3f24f18e.3fc6a971.41138f7e.3fc6a971.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.3e99999a.3dcccccd.3e4ccccd.w.0.3e190650.0.3f7d201a.3f7490ee.0.3e974e6d.0.3f800000.0.be974e6d.0.3f7490ee.beb56627.3fb33333.3f0d6ad2.3fc6a970.0.32038d16.0.41138f7e.0.32038d16.0.3fc6a970.n4',
        'shape_mutation t5r_0_pair_total=0.m40bccccd.3e2d8f30.i40412065.3f78ed0f.401fc6e2.3ea9abd0.3f83a334.3ecd1603.f.3f7fb3ce.bc45f332.bd3f2789.3c3f706c.3f7ff900.bc0fd41b.3d3f9186.3c06ba25.3f7fb611.3e229e41.3e2eb0e0.3e8a8ba3.w.3ba6dbd4.3e0151f3.3babbf15.3f7df185.3f77d1ef.bc154b54.3e804efe.3c3f706c.3f7ff900.bc0fd41b.be8040ff.3c3b353d.3f77d223.beefc78c.3fbc3c82.3f295d51.3eabecc4.bbd516c5.3c88de96.bbd516c5.3f839e80.3bf1b085.3c88de96.3bf1b085.3ecae7dd.n4',
        'shape_mutation t5r_0_create_box=0.m40490ffa.3ea2f96a.i3fe3dee9.3f1b7cff.3fae410e.3f0fcd03.3fd2be1a.3f3c0c16.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.3dcccccd.3e4cccce.3e99999a.w.0.3e190650.0.3f7d201a.3f7490ee.0.3e974e6d.0.3f800000.0.be974e6d.0.3f7490ee.bf040c24.3fc00000.3f350128.3f13aa3a.0.3d47ddf8.0.3fd2be1a.0.3d47ddf8.0.3f382edd.n2',
        'shape_mutation t5r_0_create_capsule=0.m40466666.3ea5294b.i3f906b2d.3e426e95.3f906b2d.3f62e559.40a8882f.3f62e559.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.3e99999a.3dcccccd.3e4ccccd.w.0.3e190650.0.3f7d201a.3f7490ee.0.3e974e6d.0.3f800000.0.be974e6d.0.3f7490ee.beb56627.3fb33333.3f0d6ad2.3f62e558.0.3001575d.0.40a8882f.0.3001575d.0.3f62e558.n2',
        'shape_mutation t5r_0_create_pair=0.m4016c76a.3ed95323.i3f9a3bf6.3ec6cbf0.3f7f3381.3f5474c2.4024d4ef.3f806691.f.3f7fb3ce.bc45f332.bd3f2786.3c3f706c.3f7ff900.bc0fd41b.3d3f9183.3c06ba25.3f7fb611.3e229e41.3e2eb0e0.3e8a8ba3.w.3ba6dbd4.3e0151f4.3babbf15.3f7df185.3f77d1ef.bc154b54.3e804efe.3c3f706c.3f7ff900.bc0fd41b.be804100.3c3b353d.3f77d223.beefc78c.3fbc3c82.3f295d51.3f574733.bc856942.3d2b620f.bc856942.4024cf0b.3c975146.3d2b620f.3c975146.3f7e123d.n2',
        'shape_mutation t5r_1_box_density=0.m4019c0ec.3ed51ec6.i3fae410b.3eedce36.3f8540cb.3f3c0c19.4009cb14.3f75e871.f.3f800000.0.0.0.3f60a93f.bef57748.0.3ef57748.3f60a93f.bf333333.3fa66666.3ee66666.w.3e7a7f3a.3e14447a.bd176f87.3f75419f.3f7490ee.3e11148e.3e84c8a7.0.3f60a93f.bef57748.be974e6d.3eea80a8.3f56a080.bf9e2d1a.40266666.3f8b1b22.3f473a5e.3e17d254.3e1093ce.3e17d254.3ff0810d.3ef56625.3e1093ce.3ef56625.3f98722f.n4',
        'shape_mutation t5r_1_box_total=0.m40966666.3e59df52.i402a740d.3f689e5e.400258be.3ec03d85.3f8cdda3.3efb6425.f.3f800000.0.0.0.3f60a93f.bef57748.0.3ef57748.3f60a93f.bf333333.3fa66666.3ee66666.w.3e7a7f3a.3e14447a.bd176f87.3f75419f.3f7490ee.3e11148e.3e84c8a7.0.3f60a93f.bef57748.be974e6d.3eea80a8.3f56a080.bf9e2d1a.40266666.3f8b1b22.3ecbab9d.3d9b34f9.3d93cd16.3d9b34f9.3f75dde9.3e7adef0.3d93cd16.3e7adef0.3f1bd862.n6',
        'shape_mutation t5r_1_capsule_density=0.m3fe298c8.3f109bfa.i3f24f18e.3dde10d0.3f24f18e.3fc6a971.41138f59.3fc6a971.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.3ee66666.bf333333.3fa66666.w.0.3e190650.0.3f7d201a.3f7490ee.0.3e974e6d.0.3f800000.0.be974e6d.0.3f7490ee.3de9a18e.3f199999.3fc78bc8.3fc6a970.0.32038d16.0.41138f59.0.32038d16.0.3fc6a970.n4',
        'shape_mutation t5r_1_pair_total=0.m40bccccd.3e2d8f30.i41234a98.3fe8b593.4111f5d2.3dc8ac05.3f0ccf96.3de07fdf.f.3f5ff159.becd70ea.3e8b0e4d.3e8c8cac.3f5f5082.3ecf2730.becc6c10.be8f0a50.3f5f8c53.beb945d0.3f364129.3f332e8b.w.be1acefb.3ea20979.3e4f4d64.3f6a11a4.3f37bc39.beee8961.3f047c4e.3e8c8cac.3f5f5082.3ecf2730.bf23d358.be17e114.3f41044c.bf56bf3e.4000c37d.3f9ce09e.3e4c00da.be39a279.3d128b4d.be39a279.3ee34457.bd6107f1.3d128b4d.bd6107f1.3dea9581.n4',
        'shape_mutation t5r_1_create_box=0.m40490ffa.3ea2f96a.i3fe3dee7.3f1b7cfd.3fae410a.3f0fcd04.3fd2be1d.3f3c0c1a.f.3f800000.0.0.0.3f60a93f.bef57749.0.3ef57749.3f60a93f.bf333333.3fa66666.3ee66666.w.3e7a7f3b.3e14447a.bd176f88.3f75419f.3f7490ee.3e11148f.3e84c8a7.0.3f60a93f.bef57749.be974e6d.3eea80a9.3f56a080.bf9e2d1a.40266666.3f8b1b22.3f1859cf.3de8329f.3ddd1e4a.3de8329f.3fb7ea37.3ebba875.3ddd1e4a.3ebba875.3f69271b.n2',
        'shape_mutation t5r_1_create_capsule=0.m40466666.3ea5294b.i3f906b2d.3e426ec6.3f906b2d.3f62e559.40a88804.3f62e559.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.3ee66666.bf333333.3fa66666.w.0.3e190650.0.3f7d201a.3f7490ee.0.3e974e6d.0.3f800000.0.be974e6d.0.3f7490ee.3de9a18e.3f199999.3fc78bc8.3f62e558.0.3001575d.0.40a88804.0.3001575d.0.3f62e558.n2',
        'shape_mutation t5r_1_create_pair=0.m4016c76a.3ed95323.i40826847.3f39d87d.406921fc.3e7b4633.3fb05180.3e8c8e1c.f.3f5ff159.becd70ea.3e8b0e49.3e8c8cae.3f5f5082.3ecf272f.becc6c0c.be8f0a51.3f5f8c53.beb945d0.3f364129.3f332e8b.w.be1acefb.3ea20976.3e4f4d65.3f6a11a5.3f37bc3a.beee8961.3f047c4c.3e8c8cae.3f5f5082.3ecf272f.bf23d356.be17e116.3f41044c.bf56bf3e.4000c37d.3f9ce09e.3eff7217.bee871f1.3db77f53.bee871f1.3f8e49ac.be0ce352.3db77f53.be0ce352.3e92de6b.n2',
        'shape_mutation t5r_2_box_density=0.m4019c0ec.3ed51ec6.i3fae410b.3eedce2f.3f8540d0.3f3c0c19.4009cb18.3f75e868.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.4039999a.beb33333.3d4cccce.w.0.3e190650.0.3f7d201a.3f7490ee.0.3e974e6d.0.3f800000.0.be974e6d.0.3f7490ee.400574c4.3f733332.beb7ee8c.3f4119b0.0.3d82aea3.0.4009cb18.0.3d82aea3.0.3f70dad0.n4',
        'shape_mutation t5r_2_box_total=0.m40966666.3e59df52.i402a740d.3f689e56.400258c2.3ec03d85.3f8cdda8.3efb641e.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.4039999a.beb33333.3d4cccce.w.0.3e190650.0.3f7d201a.3f7490ee.0.3e974e6d.0.3f800000.0.be974e6d.0.3f7490ee.400574c4.3f733332.beb7ee8c.3ec567f4.0.3d05989d.0.3f8cdda8.0.3d05989d.0.3ef639ad.n6',
        'shape_mutation t5r_2_capsule_density=0.m3fe298c8.3f109bfa.i3f24f1a2.3dde110d.3f24f18e.3fc6a959.41138f30.3fc6a971.f.3f43ccc7.3f24eb5b.0.bf24eb5b.3f43ccc7.0.0.0.3f800000.3d4ccccd.4039999a.beb33333.w.bd51e300.3e0fbf4b.bead9782.3f6dc778.3f3b0e07.3f1d8db0.3e974e6d.bf24eb5b.3f43ccc7.0.be67738a.be42f2b1.3f7490ee.bf417347.40866666.3dce8db4.408ea2c4.40671411.bf6612a2.40671411.40c14081.bf8ef63d.bf6612a2.bf8ef63d.3fea3f2e.n4',
        'shape_mutation t5r_2_pair_total=0.m40bccccd.3e2d8f30.i41c73ac0.40049fc8.41cc2ef1.3d247941.3ef712fc.3d207bb3.f.3f4314b8.bf20d8ca.be2062c1.3f227c50.3f45cd66.bc3c6711.3e0352ad.bdb9a688.3f7cd2c2.4003f7db.3f1b0950.bd8a7904.w.3cf11f1a.3d8581fc.3eabf5a7.3f706ef0.3f4411f0.bf208552.3e11a29c.3f227c50.3f45cd66.bc3c6711.bdd249a5.3dcae8c3.3f7d616f.3f9ffd8e.3ff3eb0e.be655510.3e5b3788.be5b7c5d.bce24713.be5b7c5d.3e9bca8f.3d0ac4db.bce24713.3d0ac4db.3d325a42.n4',
        'shape_mutation t5r_2_create_box=0.m40490ffa.3ea2f96a.i3fe3dee7.3f1b7cf8.3fae4110.3f0fcd04.3fd2be24.3f3c0c13.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.4039999a.beb33333.3d4cccce.w.0.3e190650.0.3f7d201a.3f7490ee.0.3e974e6d.0.3f800000.0.be974e6d.0.3f7490ee.400574c4.3f733332.beb7ee8c.3f13aa3b.0.3d47dde8.0.3fd2be24.0.3d47dde8.0.3f382edb.n2',
        'shape_mutation t5r_2_create_capsule=0.m40466666.3ea5294b.i3f906b3e.3e426efc.3f906b2d.3f62e53f.40a887d5.3f62e559.f.3f43ccc7.3f24eb5b.0.bf24eb5b.3f43ccc7.0.0.0.3f800000.3d4ccccd.4039999a.beb33333.w.bd51e300.3e0fbf4b.bead9782.3f6dc778.3f3b0e07.3f1d8db0.3e974e6d.bf24eb5b.3f43ccc7.0.be67738a.be42f2b1.3f7490ee.bf417347.40866666.3dce8db4.4022e848.4003f5b1.bf0362af.4003f5b1.405cb794.bf23479c.bf0362af.bf23479c.3f85c4e3.n2',
        'shape_mutation t5r_2_create_pair=0.m4016c76a.3ed95323.i411f1baf.3f53d4f7.4123107b.3dcdf2af.3f9ab04f.3dc8f389.f.3f4314b8.bf20d8ca.be2062cd.3f227c50.3f45cd66.bc3c67b9.3e0352bd.bdb9a688.3f7cd2c2.4003f7db.3f1b0950.bd8a7904.w.3cf11f30.3d8581ee.3eabf5a7.3f706ef0.3f4411f1.bf208552.3e11a291.3f227c50.3f45cd66.bc3c67b9.bdd24986.3dcae8c3.3f7d6170.3f9ffd8e.3ff3eb0e.be655510.3f093f6f.bf096a87.bd8dab1a.bf096a87.3f431382.3dadc2f3.bd8dab1a.3dadc2f3.3ddf53a3.n2',
        'shape_mutation t5r_3_box_density=0.m4019c0ec.3ed51ec6.i3fae410c.3eedce37.3f8540cd.3f3c0c18.4009cb13.3f75e86e.f.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.0.0.3e99999a.w.3e7a7f35.3e14447a.bd176f85.3f75419f.3f7490ee.3e11148c.3e84c8a8.0.3f60a940.bef57744.be974e6d.3eea80a4.3f56a081.bf1c8109.3fa66666.3f3c91e1.3f473a5d.3e17d253.3e1093c9.3e17d253.3ff0810c.3ef5661f.3e1093c9.3ef5661f.3f98722d.n4',
        'shape_mutation t5r_3_box_total=0.m40966666.3e59df52.i402a740e.3f689e5f.400258bf.3ec03d84.3f8cdda2.3efb6423.f.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.0.0.3e99999a.w.3e7a7f35.3e14447a.bd176f85.3f75419f.3f7490ee.3e11148c.3e84c8a8.0.3f60a940.bef57744.be974e6d.3eea80a4.3f56a081.bf1c8109.3fa66666.3f3c91e1.3ecbab9b.3d9b34f5.3d93cd12.3d9b34f5.3f75dde9.3e7adeed.3d93cd12.3e7adeed.3f1bd860.n6',
        'shape_mutation t5r_3_capsule_density=0.m3fe298c8.3f109bfa.i3f24f18e.3dde1098.3f24f18e.3fc6a971.41138f7e.3fc6a971.f.3f43ccb3.3f24eb73.0.bf24eb73.3f43ccb3.0.0.0.3f800000.3e99999a.0.0.w.bd51e324.3e0fbf48.bead979f.3f6dc772.3f3b0df4.3f1d8dc7.3e974e6d.bf24eb73.3f43ccb3.0.be677373.be42f2ce.3f7490ee.bed3a90a.3fa66666.3eb90212.408ea31e.406714a9.bf661381.406714a9.40c140c1.bf8ef69b.bf661381.bf8ef69b.3fea3f51.n4',
        'shape_mutation t5r_3_pair_total=0.m40bccccd.3e2d8f30.i403fa7e1.3faeabb1.401af493.3eaaf923.3f3b9948.3ed377a4.f.3f6a3020.3e024dff.bec445e6.be84c7a5.3f69a3fb.bea1bf0f.3e9e8c44.3ec6dd9a.3f5e2e8b.3db4a791.0.3e58df6b.w.3e2a1907.bd0db22f.be01d781.3f7a30e9.3f7727c7.3e7205be.bde0c0e1.be84c7a5.3f69a3fb.bea1bf0f.3cd0d51e.3eaabad1.3f71427c.bf0d9aac.3fa66666.3f2052be.3eb6de7e.3db5d36d.3cbe8487.3db5d36d.3f2c8fa4.3dc86253.3cbe8487.3dc86253.3ee5a58f.n4',
        'shape_mutation t5r_3_create_box=0.m40490ffa.3ea2f96a.i3fe3dee9.3f1b7cfe.3fae410d.3f0fcd03.3fd2be1c.3f3c0c17.f.3f800000.0.0.0.3f60a940.bef57744.0.3ef57744.3f60a940.0.0.3e99999a.w.3e7a7f35.3e14447a.bd176f85.3f75419f.3f7490ee.3e11148c.3e84c8a8.0.3f60a940.bef57744.be974e6d.3eea80a4.3f56a081.bf1c8109.3fa66666.3f3c91e1.3f1859ce.3de8329c.3ddd1e40.3de8329c.3fb7ea36.3ebba872.3ddd1e40.3ebba872.3f692716.n2',
        'shape_mutation t5r_3_create_capsule=0.m40466666.3ea5294b.i3f906b2d.3e426e97.3f906b2d.3f62e559.40a8882d.3f62e559.f.3f43ccb2.3f24eb73.0.bf24eb73.3f43ccb2.0.0.0.3f800000.3e99999a.0.0.w.bd51e325.3e0fbf47.bead97a0.3f6dc772.3f3b0df3.3f1d8dc7.3e974e6d.bf24eb73.3f43ccb2.0.be677371.be42f2ce.3f7490ee.bed3a90a.3fa66666.3eb90212.4022e8ad.4003f606.bf03632c.4003f606.405cb7db.bf234806.bf03632c.bf234806.3f85c4f6.n2',
        'shape_mutation t5r_3_create_pair=0.m4016c76a.3ed95323.i3f990f44.3f0b7ebb.3f778006.3f561625.3feae787.3f84655a.f.3f6a3021.3e024dff.bec445e2.be84c7a4.3f69a3fb.bea1bf11.3e9e8c41.3ec6dd9a.3f5e2e8b.3db4a791.0.3e58df6b.w.3e2a1908.bd0db220.be01d780.3f7a30e9.3f7727c7.3e7205be.bde0c0d2.be84c7a4.3f69a3fb.bea1bf11.3cd0d4e6.3eaabad1.3f71427c.bf0d9aac.3fa66666.3f2052be.3f64fb6d.3e63ad00.3d6e8f38.3e63ad00.3fd81326.3e7ae9e8.3d6e8f38.3e7ae9e8.3f8fc716.n2',
        'shape_mutation t5r_nan_0=0.m7fc00000.7fc00000.i3e800000.3f000000.3f400000.40800000.40000000.3faaaaab.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.0.3e190650.0.3f7d201a.3f7490ee.0.3e974e6d.0.3f800000.0.be974e6d.0.3f7490ee.bf333333.3fa66666.3ee66666.40711865.0.bf40bb36.0.40000000.0.bf40bb36.0.3fc879de.n2',
        'shape_mutation t5r_nan_0_root=1.1.r2.2.1.624.2.s25.100.19',
        'shape_mutation t5r_nan_1=0.m7fe00000.7fe00000.i3e800000.3f000000.3f400000.40800000.40000000.3faaaaab.f.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.w.0.3e190650.0.3f7d201a.3f7490ee.0.3e974e6d.0.3f800000.0.be974e6d.0.3f7490ee.bf333333.3fa66666.3ee66666.40711865.0.bf40bb36.0.40000000.0.bf40bb36.0.3fc879de.n2',
        'shape_mutation t5r_nan_1_root=1.1.r2.2.1.624.2.s26.100.1a',
        'shape_mutation actor=1',
        'shape_mutation initial=1.2.1',
        'shape_mutation add_memory=5.0.552.28.272.8.8.f',
        'shape_mutation added=1.2.5.1.1',
        'shape_mutation group=2.2.2.2.1.1',
        'shape_mutation release_memory=0.0.f',
        'shape_mutation released=1.5.1',
        'shape_mutation released_group=1.1.1.1',
        'shape_mutation actor_release_memory=2.7.8.8.f.24.28.552.8.8.272.80'
    )
    'NxPhysicsShapeVtableTests' = @(
        'shape vtable oracle_digest=ed1294b6 cases=626 failures=0'
    )
    # Joint-open-items Task 4 review: seven rotated bodies (a general rotation,
    # 180 degrees about x, y and z, and three general rotations whose largest
    # diagonal is x, y and z) through every setter that ends in the
    # mass-frame refresh 000768, printing +0x5c, +0x124, +0x134, +0x158 and
    # +0x164 after each. Forty-two oracle-side lines were added to this list:
    # per body, the created +0x5c/+0x124 (000801, 000768), the +0x5c after
    # setGlobalPose and setGlobalOrientation (000196/000200's own
    # conversion), +0x134 after setCMassOffsetGlobalOrientation (000222) and
    # +0x158 after setCMassOffsetGlobalPose (000218).
    'NxPhysicsActorCMassTests' = @(
        # Task 3 review (000128): the reference getter quiets SNaNs in x and y (fld/fstp)
        # and moves z as a dword.
        'cmass reference_snan t=7fc00001.ffc00002.7fa00003',
        # NpActor.cpp completion Task 3 review (001315): every shape's global pose after the
        # CMass-global setters, as exact words.
        'cmass gm_single_general_created shape0 global_pose=3f650d79.3ee25b9e.3d8158ee.bea1af28.3f39efd6.bf1c4b72.bea1af28.3f06bca0.3f4a1af3.c0400000.40000000.3f800000',
        'cmass gm_single_general_pose shape0 global_pose=3f739e49.bbab561e.3e9d47f3.3e93e93f.3eb60b63.bf638e37.bdd62b82.3f6f4499.3eae035c.3f204af5.bfc9f4a0.40867520',
        'cmass gm_single_general_position shape0 global_pose=3f739e49.bbab561e.3e9d47f3.3e93e93f.3eb60b63.bf638e37.bdd62b82.3f6f4499.3eae035c.bf9fda85.40a582d8.bf2c5702',
        'cmass gm_single_general_orientation shape0 global_pose=bd919b0d.bf213da6.3f46015e.3f7dc595.be05e3aa.bc7b8e93.3de2ebd0.3f4400a4.3f22379c.bfbf739a.406672f1.bf98b08d',
        'cmass gm_single_flip_x_created shape0 global_pose=3f650d79.3ee25b9e.3d8158ee.bea1af28.3f39efd6.bf1c4b72.bea1af28.3f06bca0.3f4a1af3.c0400000.40000000.3f800000',
        'cmass gm_single_flip_x_pose shape0 global_pose=3f800000.0.0.0.bf800000.0.0.0.bf800000.3f400000.c0600000.40680000',
        'cmass gm_single_flip_x_position shape0 global_pose=3f800000.0.0.0.bf800000.0.0.0.bf800000.bf900000.40500000.bfa00000',
        'cmass gm_single_flip_x_orientation shape0 global_pose=bf53d13d.3eda5092.3ebb207e.3df980aa.3f4585d9.bf1fd66b.bf0c585e.bef1b4a2.bf30bb26.3f18872c.40b5edce.bfcab886',
        'cmass gm_group_general_created shape0 global_pose=3f650d79.3ee25b9e.3d8158ee.bea1af28.3f39efd6.bf1c4b72.bea1af28.3f06bca0.3f4a1af3.c0400000.40000000.3f800000',
        'cmass gm_group_general_created shape1 global_pose=3f1e0fef.be956609.bf3b024a.beca4004.3f3024a1.bf1bd4c5.3f2e24cd.3f2a164e.3e9e7f36.bffcf7ea.3cac7700.3ff5e50e',
        'cmass gm_group_general_pose shape0 global_pose=3f739e49.bbab561e.3e9d47f3.3e93e93f.3eb60b63.bf638e37.bdd62b82.3f6f4499.3eae035c.3f204af5.bfc9f4a0.40867520',
        'cmass gm_group_general_pose shape1 global_pose=3f30720d.bf21d9d4.beb53024.bef0e407.bc9c98d7.bf61d7c0.3f0d0d84.3f464893.be9f0b4f.401bb106.c04b05b0.40891268',
        'cmass gm_group_general_position shape0 global_pose=3f739e49.bbab561e.3e9d47f3.3e93e93f.3eb60b63.bf638e37.bdd62b82.3f6f4499.3eae035c.bf9fda85.40a582d8.bf2c5702',
        'cmass gm_group_general_position shape1 global_pose=3f30720d.bf21d9d4.beb53024.bef0e407.bc9c98d7.bf61d7c0.3f0d0d84.3f464893.be9f0b4f.3f0ec41a.4064fa50.bf176cbf',
        'cmass gm_group_general_orientation shape0 global_pose=bd919b0d.bf213da6.3f46015e.3f7dc595.be05e3aa.bc7b8e93.3de2ebd0.3f4400a4.3f22379c.bfbf739a.406672f1.bf98b08d',
        'cmass gm_group_general_orientation shape1 global_pose=3ec3d456.bead35a8.3f5c1c18.3ecd3638.bf47175d.bef7f503.3f551f14.3f07a3e2.be25bf34.3e8df442.409bfc58.be2df570',
        'cmass gm_group_near_x_created shape0 global_pose=3f650d79.3ee25b9e.3d8158ee.bea1af28.3f39efd6.bf1c4b72.bea1af28.3f06bca0.3f4a1af3.c0400000.40000000.3f800000',
        'cmass gm_group_near_x_created shape1 global_pose=3f1e0fef.be956609.bf3b024a.beca4004.3f3024a1.bf1bd4c5.3f2e24cd.3f2a164e.3e9e7f36.bffcf7ea.3cac7700.3ff5e50e',
        'cmass gm_group_near_x_pose shape0 global_pose=3f6634e0.3eaf653c.3e8b48dd.3ed8aa3b.bf56bafb.beaf653c.3de2fb7b.3ed8aa3b.bf6634dc.3f8f12b3.c05c7410.4080a7a8',
        'cmass gm_group_near_x_pose shape1 global_pose=3f4186f3.beb28077.bf0dd614.bebeed4d.bf6ccb24.3d95fd72.bf09bb3f.3e1adf24.bf544a2e.402755c3.c04b9b66.40098811',
        'cmass gm_group_near_x_position shape0 global_pose=3f6634e0.3eaf653c.3e8b48dd.3ed8aa3b.bf56bafb.beaf653c.3de2fb7b.3ed8aa3b.bf6634dc.bf41da9a.40538bf0.bf5ac2c0',
        'cmass gm_group_near_x_position shape1 global_pose=3f4186f3.beb28077.bf0dd614.bebeed4d.bf6ccb24.3d95fd72.bf09bb3f.3e1adf24.bf544a2e.3f3d570c.4064649a.c02e77ef',
        'cmass gm_group_near_x_orientation shape0 global_pose=bf800000.0.0.0.bf800000.0.0.0.3f800000.3ec00000.40500000.c0100000',
        'cmass gm_group_near_x_orientation shape1 global_pose=beecbb2e.3f302c0c.3f0f23c9.beb02c0c.bf386e1b.3f1a268a.3f51344d.3db02c08.3f11e479.bf600000.40700000.be800000',
        'cmass gm_group_near_y_created shape0 global_pose=3f650d79.3ee25b9e.3d8158ee.bea1af28.3f39efd6.bf1c4b72.bea1af28.3f06bca0.3f4a1af3.c0400000.40000000.3f800000',
        'cmass gm_group_near_y_created shape1 global_pose=3f1e0fef.be956609.bf3b024a.beca4004.3f3024a1.bf1bd4c5.3f2e24cd.3f2a164e.3e9e7f36.bffcf7ea.3cac7700.3ff5e50e',
        'cmass gm_group_near_y_pose shape0 global_pose=bf53d13d.3eda5092.3ebb207e.3df980aa.3f4585d9.bf1fd66b.bf0c585e.bef1b4a2.bf30bb26.401e21cb.bf8848c6.4052a3bd',
        'cmass gm_group_near_y_pose shape1 global_pose=3d80a9df.3f68749b.3ed40e0d.be40f08e.3ed62dbe.bf637792.bf7ae661.bcb5fc0b.3e4a1b19.3ffa26fd.c023014d.3fbb0bae',
        'cmass gm_group_near_y_position shape0 global_pose=bf53d13d.3eda5092.3ebb207e.3df980aa.3f4585d9.bf1fd66b.bf0c585e.bef1b4a2.bf30bb26.3f18872c.40b5edce.bfcab886',
        'cmass gm_group_near_y_position shape1 global_pose=3d80a9df.3f68749b.3ed40e0d.be40f08e.3ed62dbe.bf637792.bf7ae661.bcb5fc0b.3e4a1b19.3da26fca.40867f59.c05a7a29',
        'cmass gm_group_near_y_orientation shape0 global_pose=3f800000.0.0.0.bf800000.0.0.0.bf800000.bf900000.40500000.bfa00000',
        'cmass gm_group_near_y_orientation shape1 global_pose=3eecbb2e.bf302c0c.bf0f23c9.beb02c0c.bf386e1b.3f1a268a.bf51344d.bdb02c08.bf11e479.3e000000.40700000.c0500000',
        'cmass gm_group_near_z_created shape0 global_pose=3f650d79.3ee25b9e.3d8158ee.bea1af28.3f39efd6.bf1c4b72.bea1af28.3f06bca0.3f4a1af3.c0400000.40000000.3f800000',
        'cmass gm_group_near_z_created shape1 global_pose=3f1e0fef.be956609.bf3b024a.beca4004.3f3024a1.bf1bd4c5.3f2e24cd.3f2a164e.3e9e7f36.bffcf7ea.3cac7700.3ff5e50e',
        'cmass gm_group_near_z_pose shape0 global_pose=bf3d9b21.bf236e72.be5680f4.3ee0b7dd.bf321d5d.3f118e5e.bf023c03.3ea889e6.3f4ba6a0.3fae5d34.c06eec35.40614bf8',
        'cmass gm_group_near_z_pose shape1 global_pose=bf3bb99f.3d020ad3.3f2ddd95.3edb3a01.bf411ee2.3efec1ec.3f073451.3f27da2a.3f0a22b2.3eac5e7c.bfd98940.4089f1f4',
        'cmass gm_group_near_z_position shape0 global_pose=bf3d9b21.bf236e72.be5680f4.3ee0b7dd.bf321d5d.3f118e5e.bf023c03.3ea889e6.3f4ba6a0.bf034598.404113cb.bfad680f',
        'cmass gm_group_near_z_position shape1 global_pose=bf3bb99f.3d020ad3.3f2ddd95.3edb3a01.bf411ee2.3efec1ec.3f073451.3f27da2a.3f0a22b2.bfc4e861.40a19db0.bf10705b',
        'cmass gm_group_near_z_orientation shape0 global_pose=bf800000.0.0.0.3f800000.0.0.0.bf800000.3ec00000.40b80000.bfa00000',
        'cmass gm_group_near_z_orientation shape1 global_pose=beecbb2e.3f302c0c.3f0f23c9.3eb02c0c.3f386e1b.bf1a268a.bf51344d.bdb02c08.bf11e479.bf600000.40a80000.c0500000',
        'cmass gm_group_rotated_created shape0 global_pose=3f650d79.3ee25b9e.3d8158ee.bea1af28.3f39efd6.bf1c4b72.bea1af28.3f06bca0.3f4a1af3.c0400000.40000000.3f800000',
        'cmass gm_group_rotated_created shape1 global_pose=3f1e0fef.be956609.bf3b024a.beca4004.3f3024a1.bf1bd4c5.3f2e24cd.3f2a164e.3e9e7f36.bffcf7ea.3cac7700.3ff5e50e',
        'cmass gm_group_rotated_pose shape0 global_pose=beff41c6.3e50e9f5.bf57aeda.bf553f68.bec37446.3ecd0b78.be75adf2.3f66c6ae.3eb874b8.4023364e.c0135efd.40880d78',
        'cmass gm_group_rotated_pose shape1 global_pose=bf594c08.3ed5ced1.bea601bb.be41c5ce.3eaa5453.3f6c8229.3efcc147.3f587585.be503d12.3e0f54a6.c01688ab.408716d9',
        'cmass gm_group_rotated_position shape0 global_pose=beff41c6.3e50e9f5.bf57aeda.bf553f68.bec37446.3ecd0b78.be75adf2.3f66c6ae.3eb874b8.3f2cd937.408e5082.bf1f943c',
        'cmass gm_group_rotated_position shape1 global_pose=bf594c08.3ed5ced1.bea601bb.be41c5ce.3eaa5453.3f6c8229.3efcc147.3f587585.be503d12.bfde156c.408cbbab.bf274937',
        'cmass gm_group_rotated_orientation shape0 global_pose=be019a56.bf74bce1.be877e66.bf14d5d0.3e934690.bf42d65a.3f4dc0c7.3d6ba432.bf179b86.bfabdfc5.40b5a2f5.bffe0d00',
        'cmass gm_group_rotated_orientation shape1 global_pose=bf1a8e0c.bf1fd8ce.3efdc114.bf4ab34e.3f0ab706.be9050bc.bdbd8c90.bf10065a.bf52501b.bfc6afdd.40522383.c00c5f4d',
        # NpActor.cpp completion Task 3 (000204-000208 with 000756, 000789, 000746 and the
        # 000004 shape update / group slot 6 001018): single-shape and two-box actors with
        # rotated mass frames through setCMassGlobalPose/Position/Orientation, each step
        # printing the record words 000789 writes and every shape's global pose as exact
        # words (001315's composition, rewritten from the listing in the Task 3 review).
        'cmass gm_single_general created=1 shapes=1',
        'cmass gm_single_general input_mass_rotation=3edcdcdc.bf1b9b9c.3f2aaaaa.3ca0a090.3f3ebebe.3f2aaaaa.bf66e6e6.be8c8c8d.3eaaaaaa',
        'cmass gm_single_general input_first=3e088888.bf2aaaab.3f3bbbbb.3f6eeeef.3eaaaaaa.3e08888a.beaaaaaa.3f2aaaaa.3f2aaaab',
        'cmass gm_single_general input_second=bf3d9b20.bf236e72.be5680f5.3ee0b7dc.bf321d5c.3f118e5e.bf023c03.3ea889e5.3f4ba6a0',
        'cmass gm_single_general_created frame off=5c words=3e9d971b.3dd21ed0.be521ed0.3f6c62aa',
        'cmass gm_single_general_created frame off=124 words=3dbb3eba.3f09d54f.3e500cd6.3f500cd7',
        'cmass gm_single_general_created frame off=134 words=3eace2c6.be6d5927.3f698b39.3edb7d28.3f6695be.3d8fb82f.bf5685ae.3ebc1a67.3ece98b1',
        'cmass gm_single_general_created frame off=158 words=c0366666.3f0ccccc.3f000002',
        'cmass gm_single_general_created frame off=164 words=3eabab27.3d002452.bdf0b137.3d002452.3ee59a68.bdcad0ee.bdf0b137.bdcad0ee.3f1192d9',
        'cmass gm_single_general_created transform=c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.3f400000.bfa00000.3f000000.c0366666.3f0ccccc.3f000002',
        'cmass gm_single_general_pose frame off=5c words=3f0f769d.3e01946e.3db91d30.3f5040d4',
        'cmass gm_single_general_pose frame off=124 words=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'cmass gm_single_general_pose frame off=134 words=3e088888.bf2aaaab.3f3bbbbb.3f6eeeef.3eaaaaaa.3e08888a.beaaaaaa.3f2aaaaa.3f2aaaab',
        'cmass gm_single_general_pose frame off=158 words=3fc00000.c0100000.40480000',
        'cmass gm_single_general_pose frame off=164 words=3eafc274.3cb44f8b.bd8ab355.3cb44f8b.3f215905.bdbeb696.bd8ab355.bdbeb696.3ec1f6ca',
        'cmass gm_single_general_pose transform=3f204af5.bfc9f4a0.40867520.3f0f769d.3e01946e.3db91d30.3f5040d4.3f204af5.bfc9f4a0.40867520.3f0f769d.3e01946e.3db91d30.3f5040d4.3f400000.bfa00000.3f000000.3fc00000.c0100000.40480000',
        'cmass gm_single_general_position frame off=5c words=3f0f769d.3e01946e.3db91d30.3f5040d4',
        'cmass gm_single_general_position frame off=124 words=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'cmass gm_single_general_position frame off=134 words=3e088888.bf2aaaab.3f3bbbbb.3f6eeeef.3eaaaaaa.3e08888a.beaaaaaa.3f2aaaaa.3f2aaaab',
        'cmass gm_single_general_position frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_single_general_position frame off=164 words=3eafc274.3cb44f8b.bd8ab355.3cb44f8b.3f215905.bdbeb696.bd8ab355.bdbeb696.3ec1f6ca',
        'cmass gm_single_general_position transform=bf9fda85.40a582d8.bf2c5702.3f0f769d.3e01946e.3db91d30.3f5040d4.bf9fda85.40a582d8.bf2c5702.3f0f769d.3e01946e.3db91d30.3f5040d4.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_single_general_orientation frame off=5c words=3ea7162a.3e8dc53b.3f2d6a66.3f1929a8',
        'cmass gm_single_general_orientation frame off=124 words=be4c8b64.3e7fae3d.3f661ccf.3e99688b',
        'cmass gm_single_general_orientation frame off=134 words=bf3d9b20.bf236e72.be5680f5.3ee0b7dc.bf321d5c.3f118e5e.bf023c03.3ea889e5.3f4ba6a0',
        'cmass gm_single_general_orientation frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_single_general_orientation frame off=164 words=3f0a9085.bd95a7e8.3df4c7c0.bd95a7e8.3ed42f40.bde3ceee.3df4c7c0.bde3ceee.3ecb1afa',
        'cmass gm_single_general_orientation transform=bfbf739a.406672f1.bf98b08d.3ea7162a.3e8dc53b.3f2d6a66.3f1929a8.bfbf739a.406672f1.bf98b08d.3ea7162a.3e8dc53b.3f2d6a66.3f1929a8.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_single_general_final local_pose=3edcdcdc.bf1b9b9c.3f2aaaaa.3ca0a090.3f3ebebe.3f2aaaaa.bf66e6e6.be8c8c8d.3eaaaaaa.3f400000.bfa00000.3f000000',
        'cmass gm_single_general_final local_position=3f400000.bfa00000.3f000000',
        'cmass gm_single_general_final local_orientation=3edcdcdc.bf1b9b9c.3f2aaaaa.3ca0a090.3f3ebebe.3f2aaaaa.bf66e6e6.be8c8c8d.3eaaaaaa',
        'cmass gm_single_general_final global_pose=bf3d9b1d.bf236e73.be5680eb.3ee0b7db.bf321d59.3f118e5e.bf023c04.3ea889e1.3f4ba69d.bebffffd.40900000.bfdfffff',
        'cmass gm_single_general_final global_position=bebffffd.40900000.bfdfffff',
        'cmass gm_single_general_final global_orientation=bf3d9b1d.bf236e73.be5680eb.3ee0b7db.bf321d59.3f118e5e.bf023c04.3ea889e1.3f4ba69d',
        'cmass gm_single_flip_x created=1 shapes=1',
        'cmass gm_single_flip_x input_mass_rotation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass gm_single_flip_x input_first=3f800000.0.0.0.bf800000.0.0.0.bf800000',
        'cmass gm_single_flip_x input_second=bf53d13e.3eda5094.3ebb207e.3df980a8.3f4585d8.bf1fd66c.bf0c585f.bef1b4a4.bf30bb26',
        'cmass gm_single_flip_x_created frame off=5c words=3e9d971b.3dd21ed0.be521ed0.3f6c62aa',
        'cmass gm_single_flip_x_created frame off=124 words=3e9d971a.3dd21ed0.be521ecf.3f6c62ab',
        'cmass gm_single_flip_x_created frame off=134 words=3f650d79.3ee25b9e.3d8158ee.bea1af28.3f39efd6.bf1c4b72.bea1af28.3f06bca0.3f4a1af3',
        'cmass gm_single_flip_x_created frame off=158 words=c0366666.3f0ccccc.3f000002',
        'cmass gm_single_flip_x_created frame off=164 words=3f1cef38.bd9148b3.bda5fac8.bd9148b3.3ec49ad2.3da746bd.bda5fac8.3da746bd.3eb5f204',
        'cmass gm_single_flip_x_created transform=c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.3f400000.bfa00000.3f000000.c0366666.3f0ccccc.3f000002',
        'cmass gm_single_flip_x_pose frame off=5c words=3f800000.0.0.0',
        'cmass gm_single_flip_x_pose frame off=124 words=3f800000.0.0.0',
        'cmass gm_single_flip_x_pose frame off=134 words=3f800000.0.0.0.bf800000.0.0.0.bf800000',
        'cmass gm_single_flip_x_pose frame off=158 words=3fc00000.c0100000.40480000',
        'cmass gm_single_flip_x_pose frame off=164 words=3f2aaaab.0.0.0.3ecccccd.0.0.0.3e924925',
        'cmass gm_single_flip_x_pose transform=3f400000.c0600000.40680000.3f800000.0.0.0.3f400000.c0600000.40680000.3f800000.0.0.0.3f400000.bfa00000.3f000000.3fc00000.c0100000.40480000',
        'cmass gm_single_flip_x_position frame off=5c words=3f800000.0.0.0',
        'cmass gm_single_flip_x_position frame off=124 words=3f800000.0.0.0',
        'cmass gm_single_flip_x_position frame off=134 words=3f800000.0.0.0.bf800000.0.0.0.bf800000',
        'cmass gm_single_flip_x_position frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_single_flip_x_position frame off=164 words=3f2aaaab.0.0.0.3ecccccd.0.0.0.3e924925',
        'cmass gm_single_flip_x_position transform=bf900000.40500000.bfa00000.3f800000.0.0.0.bf900000.40500000.bfa00000.3f800000.0.0.0.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_single_flip_x_orientation frame off=5c words=3e1ac3df.3f6825d0.be9ac3df.3e80f88f',
        'cmass gm_single_flip_x_orientation frame off=124 words=3e1ac3e0.3f6825d0.be9ac3e0.3e80f890',
        'cmass gm_single_flip_x_orientation frame off=134 words=bf53d13e.3eda5094.3ebb207e.3df980a8.3f4585d8.bf1fd66c.bf0c585f.bef1b4a4.bf30bb26',
        'cmass gm_single_flip_x_orientation frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_single_flip_x_orientation frame off=164 words=3f113a5f.ba51fc6e.3e196482.ba51fc6e.3eb80403.bd895cab.3e196482.bd895cab.3ed9f29d',
        'cmass gm_single_flip_x_orientation transform=3f18872c.40b5edce.bfcab886.3e1ac3df.3f6825d0.be9ac3df.3e80f88f.3f18872c.40b5edce.bfcab886.3e1ac3df.3f6825d0.be9ac3df.3e80f88f.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_single_flip_x_final local_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.3f400000.bfa00000.3f000000',
        'cmass gm_single_flip_x_final local_position=3f400000.bfa00000.3f000000',
        'cmass gm_single_flip_x_final local_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass gm_single_flip_x_final global_pose=bf53d13d.3eda5092.3ebb207e.3df980aa.3f4585d9.bf1fd66b.bf0c585e.bef1b4a2.bf30bb26.bebffffb.40900000.bfe00000',
        'cmass gm_single_flip_x_final global_position=bebffffb.40900000.bfe00000',
        'cmass gm_single_flip_x_final global_orientation=bf53d13d.3eda5092.3ebb207e.3df980aa.3f4585d9.bf1fd66b.bf0c585e.bef1b4a2.bf30bb26',
        'cmass gm_group_general created=1 shapes=2',
        'cmass gm_group_general input_mass_rotation=3edcdcdc.bf1b9b9c.3f2aaaaa.3ca0a090.3f3ebebe.3f2aaaaa.bf66e6e6.be8c8c8d.3eaaaaaa',
        'cmass gm_group_general input_first=3e088888.bf2aaaab.3f3bbbbb.3f6eeeef.3eaaaaaa.3e08888a.beaaaaaa.3f2aaaaa.3f2aaaab',
        'cmass gm_group_general input_second=bf3d9b20.bf236e72.be5680f5.3ee0b7dc.bf321d5c.3f118e5e.bf023c03.3ea889e5.3f4ba6a0',
        'cmass gm_group_general_created frame off=5c words=3e9d971b.3dd21ed0.be521ed0.3f6c62aa',
        'cmass gm_group_general_created frame off=124 words=3dbb3eba.3f09d54f.3e500cd6.3f500cd7',
        'cmass gm_group_general_created frame off=134 words=3eace2c6.be6d5927.3f698b39.3edb7d28.3f6695be.3d8fb82f.bf5685ae.3ebc1a67.3ece98b1',
        'cmass gm_group_general_created frame off=158 words=c0366666.3f0ccccc.3f000002',
        'cmass gm_group_general_created frame off=164 words=3eabab27.3d002452.bdf0b137.3d002452.3ee59a68.bdcad0ee.bdf0b137.bdcad0ee.3f1192d9',
        'cmass gm_group_general_created transform=c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.3f400000.bfa00000.3f000000.c0366666.3f0ccccc.3f000002',
        'cmass gm_group_general_pose frame off=5c words=3f0f769d.3e01946e.3db91d30.3f5040d4',
        'cmass gm_group_general_pose frame off=124 words=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'cmass gm_group_general_pose frame off=134 words=3e088888.bf2aaaab.3f3bbbbb.3f6eeeef.3eaaaaaa.3e08888a.beaaaaaa.3f2aaaaa.3f2aaaab',
        'cmass gm_group_general_pose frame off=158 words=3fc00000.c0100000.40480000',
        'cmass gm_group_general_pose frame off=164 words=3eafc274.3cb44f8b.bd8ab355.3cb44f8b.3f215905.bdbeb696.bd8ab355.bdbeb696.3ec1f6ca',
        'cmass gm_group_general_pose transform=3f204af5.bfc9f4a0.40867520.3f0f769d.3e01946e.3db91d30.3f5040d4.3f204af5.bfc9f4a0.40867520.3f0f769d.3e01946e.3db91d30.3f5040d4.3f400000.bfa00000.3f000000.3fc00000.c0100000.40480000',
        'cmass gm_group_general_position frame off=5c words=3f0f769d.3e01946e.3db91d30.3f5040d4',
        'cmass gm_group_general_position frame off=124 words=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'cmass gm_group_general_position frame off=134 words=3e088888.bf2aaaab.3f3bbbbb.3f6eeeef.3eaaaaaa.3e08888a.beaaaaaa.3f2aaaaa.3f2aaaab',
        'cmass gm_group_general_position frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_group_general_position frame off=164 words=3eafc274.3cb44f8b.bd8ab355.3cb44f8b.3f215905.bdbeb696.bd8ab355.bdbeb696.3ec1f6ca',
        'cmass gm_group_general_position transform=bf9fda85.40a582d8.bf2c5702.3f0f769d.3e01946e.3db91d30.3f5040d4.bf9fda85.40a582d8.bf2c5702.3f0f769d.3e01946e.3db91d30.3f5040d4.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_group_general_orientation frame off=5c words=3ea7162a.3e8dc53b.3f2d6a66.3f1929a8',
        'cmass gm_group_general_orientation frame off=124 words=be4c8b64.3e7fae3d.3f661ccf.3e99688b',
        'cmass gm_group_general_orientation frame off=134 words=bf3d9b20.bf236e72.be5680f5.3ee0b7dc.bf321d5c.3f118e5e.bf023c03.3ea889e5.3f4ba6a0',
        'cmass gm_group_general_orientation frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_group_general_orientation frame off=164 words=3f0a9085.bd95a7e8.3df4c7c0.bd95a7e8.3ed42f40.bde3ceee.3df4c7c0.bde3ceee.3ecb1afa',
        'cmass gm_group_general_orientation transform=bfbf739a.406672f1.bf98b08d.3ea7162a.3e8dc53b.3f2d6a66.3f1929a8.bfbf739a.406672f1.bf98b08d.3ea7162a.3e8dc53b.3f2d6a66.3f1929a8.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_group_general_final local_pose=3edcdcdc.bf1b9b9c.3f2aaaaa.3ca0a090.3f3ebebe.3f2aaaaa.bf66e6e6.be8c8c8d.3eaaaaaa.3f400000.bfa00000.3f000000',
        'cmass gm_group_general_final local_position=3f400000.bfa00000.3f000000',
        'cmass gm_group_general_final local_orientation=3edcdcdc.bf1b9b9c.3f2aaaaa.3ca0a090.3f3ebebe.3f2aaaaa.bf66e6e6.be8c8c8d.3eaaaaaa',
        'cmass gm_group_general_final global_pose=bf3d9b1d.bf236e73.be5680eb.3ee0b7db.bf321d59.3f118e5e.bf023c04.3ea889e1.3f4ba69d.bebffffd.40900000.bfdfffff',
        'cmass gm_group_general_final global_position=bebffffd.40900000.bfdfffff',
        'cmass gm_group_general_final global_orientation=bf3d9b1d.bf236e73.be5680eb.3ee0b7db.bf321d59.3f118e5e.bf023c04.3ea889e1.3f4ba69d',
        'cmass gm_group_near_x created=1 shapes=2',
        'cmass gm_group_near_x input_mass_rotation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass gm_group_near_x input_first=3f6634e0.3eaf653c.3e8b48dd.3ed8aa3c.bf56bafc.beaf653c.3de2fb7b.3ed8aa3c.bf6634dc',
        'cmass gm_group_near_x input_second=bf800000.0.0.0.bf800000.0.0.0.3f800000',
        'cmass gm_group_near_x_created frame off=5c words=3e9d971b.3dd21ed0.be521ed0.3f6c62aa',
        'cmass gm_group_near_x_created frame off=124 words=3e9d971a.3dd21ed0.be521ecf.3f6c62ab',
        'cmass gm_group_near_x_created frame off=134 words=3f650d79.3ee25b9e.3d8158ee.bea1af28.3f39efd6.bf1c4b72.bea1af28.3f06bca0.3f4a1af3',
        'cmass gm_group_near_x_created frame off=158 words=c0366666.3f0ccccc.3f000002',
        'cmass gm_group_near_x_created frame off=164 words=3f1cef38.bd9148b3.bda5fac8.bd9148b3.3ec49ad2.3da746bd.bda5fac8.3da746bd.3eb5f204',
        'cmass gm_group_near_x_created transform=c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.3f400000.bfa00000.3f000000.c0366666.3f0ccccc.3f000002',
        'cmass gm_group_near_x_pose frame off=5c words=3f741dfd.3e4d9285.3dcd9285.3e4d9285',
        'cmass gm_group_near_x_pose frame off=124 words=3f741dfd.3e4d9285.3dcd9285.3e4d9285',
        'cmass gm_group_near_x_pose frame off=134 words=3f6634e0.3eaf653c.3e8b48dd.3ed8aa3c.bf56bafc.beaf653c.3de2fb7b.3ed8aa3c.bf6634dc',
        'cmass gm_group_near_x_pose frame off=158 words=3fc00000.c0100000.40480000',
        'cmass gm_group_near_x_pose frame off=164 words=3f1b701f.3de5a2e0.3d5f600e.3de5a2e0.3ede6205.bcb9f3aa.3d5f600e.bcb9f3aa.3e9f28f8',
        'cmass gm_group_near_x_pose transform=3f8f12b3.c05c7410.4080a7a8.3f741dfd.3e4d9285.3dcd9285.3e4d9285.3f8f12b3.c05c7410.4080a7a8.3f741dfd.3e4d9285.3dcd9285.3e4d9285.3f400000.bfa00000.3f000000.3fc00000.c0100000.40480000',
        'cmass gm_group_near_x_position frame off=5c words=3f741dfd.3e4d9285.3dcd9285.3e4d9285',
        'cmass gm_group_near_x_position frame off=124 words=3f741dfd.3e4d9285.3dcd9285.3e4d9285',
        'cmass gm_group_near_x_position frame off=134 words=3f6634e0.3eaf653c.3e8b48dd.3ed8aa3c.bf56bafc.beaf653c.3de2fb7b.3ed8aa3c.bf6634dc',
        'cmass gm_group_near_x_position frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_group_near_x_position frame off=164 words=3f1b701f.3de5a2e0.3d5f600e.3de5a2e0.3ede6205.bcb9f3aa.3d5f600e.bcb9f3aa.3e9f28f8',
        'cmass gm_group_near_x_position transform=bf41da9a.40538bf0.bf5ac2c0.3f741dfd.3e4d9285.3dcd9285.3e4d9285.bf41da9a.40538bf0.bf5ac2c0.3f741dfd.3e4d9285.3dcd9285.3e4d9285.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_group_near_x_orientation frame off=5c words=0.0.3f800000.0',
        'cmass gm_group_near_x_orientation frame off=124 words=0.0.3f800000.0',
        'cmass gm_group_near_x_orientation frame off=134 words=bf800000.0.0.0.bf800000.0.0.0.3f800000',
        'cmass gm_group_near_x_orientation frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_group_near_x_orientation frame off=164 words=3f2aaaab.0.0.0.3ecccccd.0.0.0.3e924925',
        'cmass gm_group_near_x_orientation transform=3ec00000.40500000.c0100000.0.0.3f800000.0.3ec00000.40500000.c0100000.0.0.3f800000.0.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_group_near_x_final local_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.3f400000.bfa00000.3f000000',
        'cmass gm_group_near_x_final local_position=3f400000.bfa00000.3f000000',
        'cmass gm_group_near_x_final local_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass gm_group_near_x_final global_pose=bf800000.0.0.0.bf800000.0.0.0.3f800000.bec00000.40900000.bfe00000',
        'cmass gm_group_near_x_final global_position=bec00000.40900000.bfe00000',
        'cmass gm_group_near_x_final global_orientation=bf800000.0.0.0.bf800000.0.0.0.3f800000',
        'cmass gm_group_near_y created=1 shapes=2',
        'cmass gm_group_near_y input_mass_rotation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass gm_group_near_y input_first=bf53d13e.3eda5094.3ebb207e.3df980a8.3f4585d8.bf1fd66c.bf0c585f.bef1b4a4.bf30bb26',
        'cmass gm_group_near_y input_second=3f800000.0.0.0.bf800000.0.0.0.bf800000',
        'cmass gm_group_near_y_created frame off=5c words=3e9d971b.3dd21ed0.be521ed0.3f6c62aa',
        'cmass gm_group_near_y_created frame off=124 words=3e9d971a.3dd21ed0.be521ecf.3f6c62ab',
        'cmass gm_group_near_y_created frame off=134 words=3f650d79.3ee25b9e.3d8158ee.bea1af28.3f39efd6.bf1c4b72.bea1af28.3f06bca0.3f4a1af3',
        'cmass gm_group_near_y_created frame off=158 words=c0366666.3f0ccccc.3f000002',
        'cmass gm_group_near_y_created frame off=164 words=3f1cef38.bd9148b3.bda5fac8.bd9148b3.3ec49ad2.3da746bd.bda5fac8.3da746bd.3eb5f204',
        'cmass gm_group_near_y_created transform=c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.3f400000.bfa00000.3f000000.c0366666.3f0ccccc.3f000002',
        'cmass gm_group_near_y_pose frame off=5c words=3e1ac3df.3f6825d0.be9ac3df.3e80f88f',
        'cmass gm_group_near_y_pose frame off=124 words=3e1ac3e0.3f6825d0.be9ac3e0.3e80f890',
        'cmass gm_group_near_y_pose frame off=134 words=bf53d13e.3eda5094.3ebb207e.3df980a8.3f4585d8.bf1fd66c.bf0c585f.bef1b4a4.bf30bb26',
        'cmass gm_group_near_y_pose frame off=158 words=3fc00000.c0100000.40480000',
        'cmass gm_group_near_y_pose frame off=164 words=3f113a5f.ba51fc6e.3e196482.ba51fc6e.3eb80403.bd895cab.3e196482.bd895cab.3ed9f29d',
        'cmass gm_group_near_y_pose transform=401e21cb.bf8848c6.4052a3bd.3e1ac3df.3f6825d0.be9ac3df.3e80f88f.401e21cb.bf8848c6.4052a3bd.3e1ac3df.3f6825d0.be9ac3df.3e80f88f.3f400000.bfa00000.3f000000.3fc00000.c0100000.40480000',
        'cmass gm_group_near_y_position frame off=5c words=3e1ac3df.3f6825d0.be9ac3df.3e80f88f',
        'cmass gm_group_near_y_position frame off=124 words=3e1ac3e0.3f6825d0.be9ac3e0.3e80f890',
        'cmass gm_group_near_y_position frame off=134 words=bf53d13e.3eda5094.3ebb207e.3df980a8.3f4585d8.bf1fd66c.bf0c585f.bef1b4a4.bf30bb26',
        'cmass gm_group_near_y_position frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_group_near_y_position frame off=164 words=3f113a5f.ba51fc6e.3e196482.ba51fc6e.3eb80403.bd895cab.3e196482.bd895cab.3ed9f29d',
        'cmass gm_group_near_y_position transform=3f18872c.40b5edce.bfcab886.3e1ac3df.3f6825d0.be9ac3df.3e80f88f.3f18872c.40b5edce.bfcab886.3e1ac3df.3f6825d0.be9ac3df.3e80f88f.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_group_near_y_orientation frame off=5c words=3f800000.0.0.0',
        'cmass gm_group_near_y_orientation frame off=124 words=3f800000.0.0.0',
        'cmass gm_group_near_y_orientation frame off=134 words=3f800000.0.0.0.bf800000.0.0.0.bf800000',
        'cmass gm_group_near_y_orientation frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_group_near_y_orientation frame off=164 words=3f2aaaab.0.0.0.3ecccccd.0.0.0.3e924925',
        'cmass gm_group_near_y_orientation transform=bf900000.40500000.bfa00000.3f800000.0.0.0.bf900000.40500000.bfa00000.3f800000.0.0.0.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_group_near_y_final local_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.3f400000.bfa00000.3f000000',
        'cmass gm_group_near_y_final local_position=3f400000.bfa00000.3f000000',
        'cmass gm_group_near_y_final local_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass gm_group_near_y_final global_pose=3f800000.0.0.0.bf800000.0.0.0.bf800000.bec00000.40900000.bfe00000',
        'cmass gm_group_near_y_final global_position=bec00000.40900000.bfe00000',
        'cmass gm_group_near_y_final global_orientation=3f800000.0.0.0.bf800000.0.0.0.bf800000',
        'cmass gm_group_near_z created=1 shapes=2',
        'cmass gm_group_near_z input_mass_rotation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass gm_group_near_z input_first=bf3d9b20.bf236e72.be5680f5.3ee0b7dc.bf321d5c.3f118e5e.bf023c03.3ea889e5.3f4ba6a0',
        'cmass gm_group_near_z input_second=bf800000.0.0.0.3f800000.0.0.0.bf800000',
        'cmass gm_group_near_z_created frame off=5c words=3e9d971b.3dd21ed0.be521ed0.3f6c62aa',
        'cmass gm_group_near_z_created frame off=124 words=3e9d971a.3dd21ed0.be521ecf.3f6c62ab',
        'cmass gm_group_near_z_created frame off=134 words=3f650d79.3ee25b9e.3d8158ee.bea1af28.3f39efd6.bf1c4b72.bea1af28.3f06bca0.3f4a1af3',
        'cmass gm_group_near_z_created frame off=158 words=c0366666.3f0ccccc.3f000002',
        'cmass gm_group_near_z_created frame off=164 words=3f1cef38.bd9148b3.bda5fac8.bd9148b3.3ec49ad2.3da746bd.bda5fac8.3da746bd.3eb5f204',
        'cmass gm_group_near_z_created transform=c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.3f400000.bfa00000.3f000000.c0366666.3f0ccccc.3f000002',
        'cmass gm_group_near_z_pose frame off=5c words=be4c8b64.3e7fae3d.3f661ccf.3e99688b',
        'cmass gm_group_near_z_pose frame off=124 words=be4c8b64.3e7fae3d.3f661ccf.3e99688b',
        'cmass gm_group_near_z_pose frame off=134 words=bf3d9b20.bf236e72.be5680f5.3ee0b7dc.bf321d5c.3f118e5e.bf023c03.3ea889e5.3f4ba6a0',
        'cmass gm_group_near_z_pose frame off=158 words=3fc00000.c0100000.40480000',
        'cmass gm_group_near_z_pose frame off=164 words=3f0a9085.bd95a7e8.3df4c7c0.bd95a7e8.3ed42f40.bde3ceee.3df4c7c0.bde3ceee.3ecb1afa',
        'cmass gm_group_near_z_pose transform=3fae5d34.c06eec35.40614bf8.be4c8b64.3e7fae3d.3f661ccf.3e99688b.3fae5d34.c06eec35.40614bf8.be4c8b64.3e7fae3d.3f661ccf.3e99688b.3f400000.bfa00000.3f000000.3fc00000.c0100000.40480000',
        'cmass gm_group_near_z_position frame off=5c words=be4c8b64.3e7fae3d.3f661ccf.3e99688b',
        'cmass gm_group_near_z_position frame off=124 words=be4c8b64.3e7fae3d.3f661ccf.3e99688b',
        'cmass gm_group_near_z_position frame off=134 words=bf3d9b20.bf236e72.be5680f5.3ee0b7dc.bf321d5c.3f118e5e.bf023c03.3ea889e5.3f4ba6a0',
        'cmass gm_group_near_z_position frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_group_near_z_position frame off=164 words=3f0a9085.bd95a7e8.3df4c7c0.bd95a7e8.3ed42f40.bde3ceee.3df4c7c0.bde3ceee.3ecb1afa',
        'cmass gm_group_near_z_position transform=bf034598.404113cb.bfad680f.be4c8b64.3e7fae3d.3f661ccf.3e99688b.bf034598.404113cb.bfad680f.be4c8b64.3e7fae3d.3f661ccf.3e99688b.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_group_near_z_orientation frame off=5c words=0.3f800000.0.0',
        'cmass gm_group_near_z_orientation frame off=124 words=0.3f800000.0.0',
        'cmass gm_group_near_z_orientation frame off=134 words=bf800000.0.0.0.3f800000.0.0.0.bf800000',
        'cmass gm_group_near_z_orientation frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_group_near_z_orientation frame off=164 words=3f2aaaab.0.0.0.3ecccccd.0.0.0.3e924925',
        'cmass gm_group_near_z_orientation transform=3ec00000.40b80000.bfa00000.0.3f800000.0.0.3ec00000.40b80000.bfa00000.0.3f800000.0.0.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_group_near_z_final local_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.3f400000.bfa00000.3f000000',
        'cmass gm_group_near_z_final local_position=3f400000.bfa00000.3f000000',
        'cmass gm_group_near_z_final local_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass gm_group_near_z_final global_pose=bf800000.0.0.0.3f800000.0.0.0.bf800000.bec00000.40900000.bfe00000',
        'cmass gm_group_near_z_final global_position=bec00000.40900000.bfe00000',
        'cmass gm_group_near_z_final global_orientation=bf800000.0.0.0.3f800000.0.0.0.bf800000',
        'cmass gm_group_rotated created=1 shapes=2',
        'cmass gm_group_rotated input_mass_rotation=bf53d13e.3eda5094.3ebb207e.3df980a8.3f4585d8.bf1fd66c.bf0c585f.bef1b4a4.bf30bb26',
        'cmass gm_group_rotated input_first=3f6634e0.3eaf653c.3e8b48dd.3ed8aa3c.bf56bafc.beaf653c.3de2fb7b.3ed8aa3c.bf6634dc',
        'cmass gm_group_rotated input_second=3e088888.bf2aaaab.3f3bbbbb.3f6eeeef.3eaaaaaa.3e08888a.beaaaaaa.3f2aaaaa.3f2aaaab',
        'cmass gm_group_rotated_created frame off=5c words=3e9d971b.3dd21ed0.be521ed0.3f6c62aa',
        'cmass gm_group_rotated_created frame off=124 words=3ebe8af5.3f6cdaf5.bd899d45.3cfe0e95',
        'cmass gm_group_rotated_created frame off=134 words=bf3898ad.3f315c36.3bf19f79.3f2f39f0.3f36c6eb.be16f5c0.bddbf4c6.bdcf5f3f.bf7d3227',
        'cmass gm_group_rotated_created frame off=158 words=c0366666.3f0ccccc.3f000002',
        'cmass gm_group_rotated_created frame off=164 words=3f09e4e5.be06a604.3cb001d4.be06a604.3f05bf6e.bd148cf5.3cb001d4.bd148cf5.3e9522b6',
        'cmass gm_group_rotated_created transform=c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.c0400000.40000000.3f800000.3e9d971b.3dd21ed0.be521ed0.3f6c62aa.3f400000.bfa00000.3f000000.c0366666.3f0ccccc.3f000002',
        'cmass gm_group_rotated_pose frame off=5c words=beb91fbd.3edeaaa3.3f3f98ca.beb15b45',
        'cmass gm_group_rotated_pose frame off=124 words=3f741dfd.3e4d9285.3dcd9285.3e4d9285',
        'cmass gm_group_rotated_pose frame off=134 words=3f6634e0.3eaf653c.3e8b48dd.3ed8aa3c.bf56bafc.beaf653c.3de2fb7b.3ed8aa3c.bf6634dc',
        'cmass gm_group_rotated_pose frame off=158 words=3fc00000.c0100000.40480000',
        'cmass gm_group_rotated_pose frame off=164 words=3f1b701f.3de5a2e0.3d5f600e.3de5a2e0.3ede6205.bcb9f3aa.3d5f600e.bcb9f3aa.3e9f28f8',
        'cmass gm_group_rotated_pose transform=4023364e.c0135efd.40880d78.beb91fbd.3edeaaa3.3f3f98ca.beb15b45.4023364e.c0135efd.40880d78.beb91fbd.3edeaaa3.3f3f98ca.beb15b45.3f400000.bfa00000.3f000000.3fc00000.c0100000.40480000',
        'cmass gm_group_rotated_position frame off=5c words=beb91fbd.3edeaaa3.3f3f98ca.beb15b45',
        'cmass gm_group_rotated_position frame off=124 words=3f741dfd.3e4d9285.3dcd9285.3e4d9285',
        'cmass gm_group_rotated_position frame off=134 words=3f6634e0.3eaf653c.3e8b48dd.3ed8aa3c.bf56bafc.beaf653c.3de2fb7b.3ed8aa3c.bf6634dc',
        'cmass gm_group_rotated_position frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_group_rotated_position frame off=164 words=3f1b701f.3de5a2e0.3d5f600e.3de5a2e0.3ede6205.bcb9f3aa.3d5f600e.bcb9f3aa.3e9f28f8',
        'cmass gm_group_rotated_position transform=3f2cd937.408e5082.bf1f943c.beb91fbd.3edeaaa3.3f3f98ca.beb15b45.3f2cd937.408e5082.bf1f943c.beb91fbd.3edeaaa3.3f3f98ca.beb15b45.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_group_rotated_orientation frame off=5c words=bf0aed0a.3f354f62.be7e4e1e.bec11561',
        'cmass gm_group_rotated_orientation frame off=124 words=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'cmass gm_group_rotated_orientation frame off=134 words=3e088888.bf2aaaab.3f3bbbbb.3f6eeeef.3eaaaaaa.3e08888a.beaaaaaa.3f2aaaaa.3f2aaaab',
        'cmass gm_group_rotated_orientation frame off=158 words=bec00000.40900000.bfe00000',
        'cmass gm_group_rotated_orientation frame off=164 words=3eafc274.3cb44f8b.bd8ab355.3cb44f8b.3f215905.bdbeb696.bd8ab355.bdbeb696.3ec1f6ca',
        'cmass gm_group_rotated_orientation transform=bfabdfc5.40b5a2f5.bffe0d00.bf0aed0a.3f354f62.be7e4e1e.bec11561.bfabdfc5.40b5a2f5.bffe0d00.bf0aed0a.3f354f62.be7e4e1e.bec11561.3f400000.bfa00000.3f000000.bec00000.40900000.bfe00000',
        'cmass gm_group_rotated_final local_pose=bf53d13e.3eda5094.3ebb207e.3df980a8.3f4585d8.bf1fd66c.bf0c585f.bef1b4a4.bf30bb26.3f400000.bfa00000.3f000000',
        'cmass gm_group_rotated_final local_position=3f400000.bfa00000.3f000000',
        'cmass gm_group_rotated_final local_orientation=bf53d13e.3eda5094.3ebb207e.3df980a8.3f4585d8.bf1fd66c.bf0c585f.bef1b4a4.bf30bb26',
        'cmass gm_group_rotated_final global_pose=3e088893.bf2aaaae.3f3bbbc1.3f6eeef7.3eaaaaa7.3e0888b3.beaaaab6.3f2aaab1.3f2aaab3.bebffff5.40900000.bfe00001',
        'cmass gm_group_rotated_final global_position=bebffff5.40900000.bfe00001',
        'cmass gm_group_rotated_final global_orientation=3e088893.bf2aaaae.3f3bbbc1.3f6eeef7.3eaaaaa7.3e0888b3.beaaaab6.3f2aaab1.3f2aaab3',
        'cmass variant=0 created=1',
        'cmass identity local_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'cmass identity local_position=0.0.0',
        'cmass identity local_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass identity global_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'cmass identity global_position=0.0.0',
        'cmass identity global_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass low_wake_before record=0.0.0.3f800000.2.3dcccccd.ffffffff',
        'cmass low_wake_after local_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.3f800000.40000000.40400000',
        'cmass low_wake_after local_position=3f800000.40000000.40400000',
        'cmass low_wake_after local_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass low_wake_after global_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.3f800000.40000000.40400000',
        'cmass low_wake_after global_position=3f800000.40000000.40400000',
        'cmass low_wake_after global_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass low_wake_after record=0.0.0.3f800000.3.3ecccccc.ffffffff',
        'cmass variant=1 created=1',
        'cmass offset local_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.3f800000.40000000.40400000',
        'cmass offset local_position=3f800000.40000000.40400000',
        'cmass offset local_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass offset global_pose=331302ae.bf7fffff.0.3f7fffff.331302ae.0.0.0.3f800000.40000001.40c00000.41100000',
        'cmass offset global_position=40000001.40c00000.41100000',
        'cmass offset global_orientation=331302ae.bf7fffff.0.3f7fffff.331302ae.0.0.0.3f800000',
        'cmass offset_initial record=0.0.3f3504f2.3f3504f3.2.3ecccccc.ffffffff',
        'cmass set_local_position local_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.40000000.40400000.40800000',
        'cmass set_local_position local_position=40000000.40400000.40800000',
        'cmass set_local_position local_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass set_local_position global_pose=331302ae.bf7fffff.0.3f7fffff.331302ae.0.0.0.3f800000.3f800002.40e00000.41200000',
        'cmass set_local_position global_position=3f800002.40e00000.41200000',
        'cmass set_local_position global_orientation=331302ae.bf7fffff.0.3f7fffff.331302ae.0.0.0.3f800000',
        'cmass set_local_position record=0.0.3f3504f2.3f3504f3.3.3ecccccc.ffffffff',
        'cmass set_local_orientation local_pose=3f800000.0.0.0.0.bf800000.0.3f800000.0.40000000.40400000.40800000',
        'cmass set_local_orientation local_position=40000000.40400000.40800000',
        'cmass set_local_orientation local_orientation=3f800000.0.0.0.0.bf800000.0.3f800000.0',
        'cmass set_local_orientation global_pose=331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0.3f800002.40e00000.41200000',
        'cmass set_local_orientation global_position=3f800002.40e00000.41200000',
        'cmass set_local_orientation global_orientation=331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0',
        'cmass set_local_orientation record=3f000000.3effffff.3effffff.3f000000.4.3ecccccc.ffffffff',
        'cmass set_local_pose local_pose=0.bf800000.0.3f800000.0.0.0.0.3f800000.40400000.40800000.40a00000',
        'cmass set_local_pose local_position=40400000.40800000.40a00000',
        'cmass set_local_pose local_orientation=0.bf800000.0.3f800000.0.0.0.0.3f800000',
        'cmass set_local_pose global_pose=bf7fffff.b31302ae.0.331302ae.bf7fffff.0.0.0.3f800000.34b72101.41000000.41300000',
        'cmass set_local_pose global_position=34b72101.41000000.41300000',
        'cmass set_local_pose global_orientation=bf7fffff.b31302ae.0.331302ae.bf7fffff.0.0.0.3f800000',
        'cmass set_local_pose record=0.0.3f800000.329302ae.6.3ecccccc.ffffffff',
        'cmass set_global_offset_position local_pose=0.bf800000.0.3f800000.0.0.0.0.3f800000.40400000.c03fffff.40400000',
        'cmass set_global_offset_position local_position=40400000.c03fffff.40400000',
        'cmass set_global_offset_position local_orientation=0.bf800000.0.3f800000.0.0.0.0.3f800000',
        'cmass set_global_offset_position global_pose=bf7fffff.b31302ae.0.331302ae.bf7fffff.0.0.0.3f800000.40dfffff.41000000.41100000',
        'cmass set_global_offset_position global_position=40dfffff.41000000.41100000',
        'cmass set_global_offset_position global_orientation=bf7fffff.b31302ae.0.331302ae.bf7fffff.0.0.0.3f800000',
        'cmass set_global_offset_position actor_pose=331302ae.bf7fffff.0.3f7fffff.331302ae.0.0.0.3f800000.40800000.40a00000.40c00000',
        'cmass set_global_offset_position record=0.0.3f800000.329302ae.7.3ecccccc.ffffffff',
        'cmass set_global_offset_position transform=40800000.40a00000.40c00000.0.0.3f3504f3.3f3504f3.40800000.40a00000.40c00000.0.0.3f3504f3.3f3504f3.40400000.c03fffff.40400000.40dfffff.41000000.41100000',
        'cmass set_global_offset_orientation local_pose=331302ae.0.bf7fffff.bf7fffff.0.b31302ae.0.3f800000.0.40400000.c03fffff.40400000',
        'cmass set_global_offset_orientation local_position=40400000.c03fffff.40400000',
        'cmass set_global_offset_orientation local_orientation=331302ae.0.bf7fffff.bf7fffff.0.b31302ae.0.3f800000.0',
        'cmass set_global_offset_orientation global_pose=3f7ffffe.0.0.0.0.bf7ffffe.0.3f800000.0.40dfffff.41000000.41100000',
        'cmass set_global_offset_orientation global_position=40dfffff.41000000.41100000',
        'cmass set_global_offset_orientation global_orientation=3f7ffffe.0.0.0.0.bf7ffffe.0.3f800000.0',
        'cmass set_global_offset_orientation actor_pose=331302ae.bf7fffff.0.3f7fffff.331302ae.0.0.0.3f800000.40800000.40a00000.40c00000',
        'cmass set_global_offset_orientation record=3f3504f3.0.0.3f3504f3.8.3ecccccc.ffffffff',
        'cmass set_global_offset_pose local_pose=0.3f7fffff.331302ae.0.331302ae.bf7fffff.bf800000.0.0.40800000.c07ffffe.40800000',
        'cmass set_global_offset_pose local_position=40800000.c07ffffe.40800000',
        'cmass set_global_offset_pose local_orientation=0.3f7fffff.331302ae.0.331302ae.bf7fffff.bf800000.0.0',
        'cmass set_global_offset_pose global_pose=0.0.3f7ffffe.0.3f7ffffe.0.bf800000.0.0.40ffffff.41100000.41200000',
        'cmass set_global_offset_pose global_position=40ffffff.41100000.41200000',
        'cmass set_global_offset_pose global_orientation=0.0.3f7ffffe.0.3f7ffffe.0.bf800000.0.0',
        'cmass set_global_offset_pose actor_pose=331302ae.bf7fffff.0.3f7fffff.331302ae.0.0.0.3f800000.40800000.40a00000.40c00000',
        'cmass set_global_offset_pose record=0.3f3504f3.0.3f3504f3.a.3ecccccc.ffffffff',
        'cmass set_global_mass_position local_pose=0.3f7fffff.331302ae.0.331302ae.bf7fffff.bf800000.0.0.40800000.c07ffffe.40800000',
        'cmass set_global_mass_position local_position=40800000.c07ffffe.40800000',
        'cmass set_global_mass_position local_orientation=0.3f7fffff.331302ae.0.331302ae.bf7fffff.bf800000.0.0',
        'cmass set_global_mass_position global_pose=0.34b504f2.3f7ffffc.0.3f7ffffc.b4b504f2.bf800000.0.0.41100001.411fffff.41300000',
        'cmass set_global_mass_position global_position=41100001.411fffff.41300000',
        'cmass set_global_mass_position global_orientation=0.34b504f2.3f7ffffc.0.3f7ffffc.b4b504f2.bf800000.0.0',
        'cmass set_global_mass_position actor_pose=34c76548.bf7ffffd.0.3f7ffffd.34c76548.0.0.0.3f800000.40a00002.40c00002.40e00000',
        'cmass set_global_mass_position record=0.3f3504f3.0.3f3504f3.a.3ecccccc.ffffffff',
        'cmass set_global_mass_position transform=40a00002.40c00002.40e00000.0.0.3f3504f1.3f3504f3.40a00002.40c00002.40e00000.0.0.3f3504f1.3f3504f3.40800000.c07ffffe.40800000.41100000.41200000.41300000',
        'cmass set_global_mass_orientation local_pose=0.3f7fffff.331302ae.0.331302ae.bf7fffff.bf800000.0.0.40800000.c07ffffe.40800000',
        'cmass set_global_mass_orientation local_position=40800000.c07ffffe.40800000',
        'cmass set_global_mass_orientation local_orientation=0.3f7fffff.331302ae.0.331302ae.bf7fffff.bf800000.0.0',
        'cmass set_global_mass_orientation global_pose=3f7fffff.337fffff.271302ae.25e28338.271302ae.bf7fffff.b3800000.3f7ffffe.0.41100000.41200000.41300000',
        'cmass set_global_mass_orientation global_position=41100000.41200000.41300000',
        'cmass set_global_mass_orientation global_orientation=3f7fffff.337fffff.271302ae.25e28338.271302ae.bf7fffff.b3800000.3f7ffffe.0',
        'cmass set_global_mass_orientation actor_pose=33800000.0.bf7fffff.b31302ae.3f800000.a5e28338.3f7fffff.331302ae.33800000.41500000.41600000.40e00001',
        'cmass set_global_mass_orientation record=3f3504f3.0.0.3f3504f3.a.3ecccccc.ffffffff',
        'cmass set_global_mass_orientation transform=41500000.41600000.40e00001.324fe77a.bf3504f3.b24fe77a.3f3504f3.41500000.41600000.40e00001.324fe77a.bf3504f3.b24fe77a.3f3504f3.40800000.c07ffffe.40800000.41100000.41200000.41300000',
        'cmass set_global_mass_pose local_pose=0.3f7fffff.331302ae.0.331302ae.bf7fffff.bf800000.0.0.40800000.c07ffffe.40800000',
        'cmass set_global_mass_pose local_position=40800000.c07ffffe.40800000',
        'cmass set_global_mass_pose local_orientation=0.3f7fffff.331302ae.0.331302ae.bf7fffff.bf800000.0.0',
        'cmass set_global_mass_pose global_pose=a678c84a.bf7ffffd.271302ae.3f7fffff.26a8d828.b31302ad.b3800000.0.3f7ffffe.41200001.41300000.41400000',
        'cmass set_global_mass_pose global_position=41200001.41300000.41400000',
        'cmass set_global_mass_pose global_orientation=a678c84a.bf7ffffd.271302ae.3f7fffff.26a8d828.b31302ad.b3800000.0.3f7ffffe',
        'cmass set_global_mass_pose actor_pose=bf7ffffe.b31302ae.2678c84a.0.331302ae.bf7fffff.331302ae.bf7fffff.33800000.41600000.41700000.41000000',
        'cmass set_global_mass_pose record=0.0.3f3504f3.3f3504f3.a.3ecccccc.ffffffff',
        'cmass set_global_mass_pose transform=41600000.41700000.41000000.b24fe779.3f3504f3.bf3504f3.b24fe779.41600000.41700000.41000000.b24fe779.3f3504f3.bf3504f3.b24fe779.40800000.c07ffffe.40800000.41200000.41300000.41400000',
        'cmass set_global_mass_pose body_rotation=0.bf800000.0.3f800000.0.0.0.0.3f800000',
        'cmass set_global_mass_pose pose_reference=bf7ffffe.b31302ae.2678c84a.0.331302ae.bf7fffff.331302ae.bf7fffff.33800000.41600000.41700000.41000000',
        'cmass set_global_mass_pose reference_identity=1',
        'cmass variant=2 created=1',
        'cmass rotated local_pose=3f800000.0.0.0.0.bf800000.0.3f800000.0.3f800000.40000000.40400000',
        'cmass rotated local_position=3f800000.40000000.40400000',
        'cmass rotated local_orientation=3f800000.0.0.0.0.bf800000.0.3f800000.0',
        'cmass rotated global_pose=331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0.40000001.40c00000.41100000',
        'cmass rotated global_position=40000001.40c00000.41100000',
        'cmass rotated global_orientation=331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0',
        'cmass static_created=1',
        'cmass static local_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'cmass static local_position=0.0.0',
        'cmass static local_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass static global_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'cmass static global_position=0.0.0',
        'cmass static global_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'cmass rot_general_created frame off=5c words=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'cmass rot_general_created frame off=124 words=be8ad5e3.3ee76479.3f46ff5f.3eafdbb9',
        'cmass rot_general_set_global_pose frame off=5c words=3f741dfd.3e4d9285.3dcd9285.3e4d9285',
        'cmass rot_general_set_global_orientation frame off=5c words=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'cmass rot_general_set_global_offset_orientation frame off=134 words=3edcdcdc.bf1b9b9c.3f2aaaaa.3ca0a097.3f3ebebd.3f2aaaa9.bf66e6e5.be8c8c8c.3eaaaaa8',
        'cmass rot_general_set_global_offset_pose frame off=158 words=3fffffff.bf000000.3f7ffff8',
        'cmass rot_flip_x_created frame off=5c words=3f800000.0.0.0',
        'cmass rot_flip_x_created frame off=124 words=3e80f890.3e9ac3e0.3f6825d0.be1ac3e0',
        'cmass rot_flip_x_set_global_pose frame off=5c words=be4c8b64.3e7fae3d.3f661ccf.3e99688b',
        'cmass rot_flip_x_set_global_orientation frame off=5c words=3f800000.0.0.0',
        'cmass rot_flip_x_set_global_offset_orientation frame off=134 words=bf53d13e.3eda5093.3ebb207e.3df980a9.3f4585d8.bf1fd66b.bf0c585e.bef1b4a2.bf30bb24',
        'cmass rot_flip_x_set_global_offset_pose frame off=158 words=3fffffff.bf000000.3f7ffff8',
        'cmass rot_flip_y_created frame off=5c words=0.3f800000.0.0',
        'cmass rot_flip_y_created frame off=124 words=3f661ccf.3e99688b.3e4c8b64.be7fae3d',
        'cmass rot_flip_y_set_global_pose frame off=5c words=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'cmass rot_flip_y_set_global_orientation frame off=5c words=0.3f800000.0.0',
        'cmass rot_flip_y_set_global_offset_orientation frame off=134 words=bf3d9b1f.bf236e72.be5680f5.3ee0b7dc.bf321d5c.3f118e5d.bf023c02.3ea889e4.3f4ba69e',
        'cmass rot_flip_y_set_global_offset_pose frame off=158 words=3fffffff.bf000000.3f7ffff8',
        'cmass rot_flip_z_created frame off=5c words=0.0.3f800000.0',
        'cmass rot_flip_z_created frame off=124 words=0.3f800000.0.0',
        'cmass rot_flip_z_set_global_pose frame off=5c words=3e1ac3df.3f6825d0.be9ac3df.3e80f88f',
        'cmass rot_flip_z_set_global_orientation frame off=5c words=0.0.3f800000.0',
        'cmass rot_flip_z_set_global_offset_orientation frame off=134 words=3f7fffff.0.32925461.0.bf7fffff.3296bb99.b2925461.3296bb99.bf7ffffe',
        'cmass rot_flip_z_set_global_offset_pose frame off=158 words=3fffffff.bf000000.3f7ffff8',
        'cmass rot_near_x_created frame off=5c words=3f741dfd.3e4d9285.3dcd9285.3e4d9285',
        'cmass rot_near_x_created frame off=124 words=bdcd9285.3e4d9285.3f741dfd.be4d9285',
        'cmass rot_near_x_set_global_pose frame off=5c words=0.0.3f800000.0',
        'cmass rot_near_x_set_global_orientation frame off=5c words=3f741dfd.3e4d9285.3dcd9285.3e4d9285',
        'cmass rot_near_x_set_global_offset_orientation frame off=134 words=bf7fffff.0.32925461.0.3f7fffff.3296bb99.32925461.b296bb99.bf7ffffe',
        'cmass rot_near_x_set_global_offset_pose frame off=158 words=3fffffff.bf000000.3f7ffff8',
        'cmass rot_near_y_created frame off=5c words=3e1ac3e0.3f6825d0.be9ac3e0.3e80f890',
        'cmass rot_near_y_created frame off=124 words=3f43702b.3f1dc369.be45caf8.bc16b31f',
        'cmass rot_near_y_set_global_pose frame off=5c words=3f800000.0.0.0',
        'cmass rot_near_y_set_global_orientation frame off=5c words=3e1ac3df.3f6825d0.be9ac3df.3e80f88f',
        'cmass rot_near_y_set_global_offset_orientation frame off=134 words=3e088887.bf2aaaab.3f3bbbba.3f6eeeef.3eaaaaa9.3e088889.beaaaaa9.3f2aaaa9.3f2aaaaa',
        'cmass rot_near_y_set_global_offset_pose frame off=158 words=3fffffff.bf000000.3f7ffff8',
        'cmass rot_near_z_created frame off=5c words=be4c8b64.3e7fae3d.3f661ccf.3e99688b',
        'cmass rot_near_z_created frame off=124 words=3db8c8c5.3f7ccb8e.bd8a9691.3de1d8f2',
        'cmass rot_near_z_set_global_pose frame off=5c words=0.3f800000.0.0',
        'cmass rot_near_z_set_global_orientation frame off=5c words=be4c8b64.3e7fae3d.3f661ccf.3e99688b',
        'cmass rot_near_z_set_global_offset_orientation frame off=134 words=3f6634e0.3eaf653b.3e8b48dd.3ed8aa3c.bf56bafb.beaf653b.3de2fb76.3ed8aa3b.bf6634da',
        'cmass rot_near_z_set_global_offset_pose frame off=158 words=3fffffff.bf000000.3f7ffff8',
        'cmass static pose_reference=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0'
    )
    'NxPhysicsActorForceTests' = @(
        # Task 3 review (000782 modes 0/1): inputs where adding the unrounded product and
        # adding it rounded give different words.
        'force t3_order inverse=3e8a60dd.3f44ec4f.3eb08d3d.3e79c190',
        'force t3_order acc=bd4fd26d.3fdd6094.bff094d3 ang=404019ad',
        # NpActor.cpp completion Task 3 (000782, 000791): a rotated body with a rotated,
        # offset mass frame, three addForce/addTorque/addForceAtPos accumulations per mode
        # 0-4 and 7 (wake only), with every accumulator, the velocity copies, the wake words
        # and the dirty word.
        'force t3_0_0 acc=401e8c09.3ee07320.bf360968.bed1eb85.3f07ae14.bf2b851f.3f4a3d71.bf547ae1.3f7851ec.bf88f5c3.3f90a3d7.bfa51eb8 vel=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f copy=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=30',
        'force t3_0_1 acc=3de147ae.be6b851f.3ebd70a4.40443078.3f9bb175.bfbd14b5.3f4a3d71.bf547ae1.3f7851ec.bf88f5c3.3f90a3d7.bfa51eb8 vel=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f copy=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=50',
        'force t3_0_2 acc=401e8c09.3ee07320.bf360968.c198745b.c0929bd1.c1aab6d8.3f4a3d71.bf547ae1.3f7851ec.bf88f5c3.3f90a3d7.bfa51eb8 vel=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f copy=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=70',
        'force t3_1_0 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.3f4a3d71.bf547ae1.3f7851ec.bf88f5c3.3f90a3d7.bfa51eb8 vel=402f2fe0.bf0588ff.3f72ec59.bf35c28f.3f07ae14.3f95c28f copy=402f2fe0.bf0588ff.3f72ec59.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=14',
        'force t3_1_1 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.3f4a3d71.bf547ae1.3f7851ec.bf88f5c3.3f90a3d7.bfa51eb8 vel=3ebd70a4.bf9851ec.4001eb85.4030fd45.3f9bb175.3eb9c1a5 copy=3ebd70a4.bf9851ec.4001eb85.4030fd45.3f9bb175.3eb9c1a5 wake=3ecccccc.3ecccccc dirty=18',
        'force t3_1_2 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.3f4a3d71.bf547ae1.3f7851ec.bf88f5c3.3f90a3d7.bfa51eb8 vel=402f2fe0.bf0588ff.3f72ec59.c19adac2.c0929bd1.c19bfe86 copy=402f2fe0.bf0588ff.3f72ec59.c19adac2.c0929bd1.c19bfe86 wake=3ecccccc.3ecccccc dirty=1c',
        'force t3_2_0 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.3f4a3d71.bf547ae1.3f7851ec.bf88f5c3.3f90a3d7.bfa51eb8 vel=41121062.3fa43958.bffc28f5.bf35c28f.3f07ae14.3f95c28f copy=41121062.3fa43958.bffc28f5.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=14',
        'force t3_2_1 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.3f4a3d71.bf547ae1.3f7851ec.bf88f5c3.3f90a3d7.bfa51eb8 vel=3ebd70a4.bf9851ec.4001eb85.4100c8b4.40403127.c0351eb8 copy=3ebd70a4.bf9851ec.4001eb85.4100c8b4.40403127.c0351eb8 wake=3ecccccc.3ecccccc dirty=18',
        'force t3_2_2 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.3f4a3d71.bf547ae1.3f7851ec.bf88f5c3.3f90a3d7.bfa51eb8 vel=41121062.3fa43958.bffc28f5.c1e328c0.c2196e2e.c20e1611 copy=41121062.3fa43958.bffc28f5.c1e328c0.c2196e2e.c20e1611 wake=3ecccccc.3ecccccc dirty=1c',
        'force t3_3_0 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.404a1129.be258024.bde37e70.bf88f5c3.3f90a3d7.bfa51eb8 vel=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f copy=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=90',
        'force t3_3_1 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.3f4a3d71.bf547ae1.3f7851ec.4019f307.3fe87e43.c006386e vel=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f copy=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=110',
        'force t3_3_2 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.404a1129.be258024.bde37e70.c19dbc09.c07ed13c.c1afac9a vel=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f copy=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=190',
        'force t3_4_0 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.4118c8b4.3fd24dd4.c041eb84.bf88f5c3.3f90a3d7.bfa51eb8 vel=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f copy=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=90',
        'force t3_4_1 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.3f4a3d71.bf547ae1.3f7851ec.40f60c49.4066978e.c0a947ae vel=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f copy=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=110',
        'force t3_4_2 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.4118c8b4.3fd24dd4.c041eb84.c1e60a08.c21707c8.c217ed1c vel=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f copy=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=190',
        'force t3_7_0 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.3f4a3d71.bf547ae1.3f7851ec.bf88f5c3.3f90a3d7.bfa51eb8 vel=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f copy=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=10',
        'force t3_7_1 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.3f4a3d71.bf547ae1.3f7851ec.bf88f5c3.3f90a3d7.bfa51eb8 vel=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f copy=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=10',
        'force t3_7_2 acc=3de147ae.be6b851f.3ebd70a4.bed1eb85.3f07ae14.bf2b851f.3f4a3d71.bf547ae1.3f7851ec.bf88f5c3.3f90a3d7.bfa51eb8 vel=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f copy=3ebd70a4.bf9851ec.4001eb85.bf35c28f.3f07ae14.3f95c28f wake=3ecccccc.3ecccccc dirty=10',
        # NpActor.cpp completion Task 2 (000150): local velocity changes on an irregularly
        # oriented body, the x87 row sums of the rotation helper. Oracle side.
        'force x87_created=1',
        'force x87_quat=3ea09a3e.bf045e28.3f344996.3ebe6611',
        'force x87_rotate_0=3fcaa981.c039e253.40429eff.3fcaa981.c039e253.40429eff',
        'force x87_rotate_1=c0d1c04d.bff99962.c06e3c51.c0d1c04d.bff99962.c06e3c51',
        'force x87_rotate_2=c421441c.43207f3f.44828478.c421441c.43207f3f.44828478',
        'force x87_rotate_3=c098c06f.404f6a49.3e56a3a8.c098c06f.404f6a49.3e56a3a8',
        'force x87_rotate_4=4110638b.c05d0e94.c164a329.4110638b.c05d0e94.c164a329',
        'force x87_rotate_5=be520b2a.be9cd029.3d8377b3.be520b2a.be9cd029.3d8377b3',
        # The lines registered before Task 2.
        'force mode=0 created=1',
        'force mode=0 stage=after_force dirty=20.2 linear=0.0.0 angular=0.0.0 force=40000000.40666667.40b33333 torque=0.0.0 smooth_force=0.0.0 smooth_torque=0.0.0',
        'force mode=0 stage=after_torque dirty=40.2 linear=0.0.0 angular=0.0.0 force=40000000.40666667.40b33333 torque=40800000.40a00000.40c00000 smooth_force=0.0.0 smooth_torque=0.0.0',
        'force mode=0 stage=repeated dirty=60.2 linear=0.0.0 angular=0.0.0 force=40800000.40e66667.41333333 torque=41000000.41200000.41400000 smooth_force=0.0.0 smooth_torque=0.0.0',
        'force mode=1 created=1',
        'force mode=1 stage=after_force dirty=4.2 linear=40000000.40666667.40b33333 angular=0.0.0 force=0.0.0 torque=0.0.0 smooth_force=0.0.0 smooth_torque=0.0.0',
        'force mode=1 stage=after_torque dirty=8.2 linear=40000000.40666667.40b33333 angular=40800000.40a00000.40c00000 force=0.0.0 torque=0.0.0 smooth_force=0.0.0 smooth_torque=0.0.0',
        'force mode=2 created=1',
        'force mode=2 stage=after_force dirty=4.2 linear=41200000.41900000.41e00000 angular=0.0.0 force=0.0.0 torque=0.0.0 smooth_force=0.0.0 smooth_torque=0.0.0',
        'force mode=2 stage=after_torque dirty=8.2 linear=41200000.41900000.41e00000 angular=41000000.41700000.41c00000 force=0.0.0 torque=0.0.0 smooth_force=0.0.0 smooth_torque=0.0.0',
        'force mode=3 created=1',
        'force mode=3 stage=after_force dirty=80.2 linear=0.0.0 angular=0.0.0 force=0.0.0 torque=0.0.0 smooth_force=40000000.40666667.40b33333 smooth_torque=0.0.0',
        'force mode=3 stage=after_torque dirty=100.2 linear=0.0.0 angular=0.0.0 force=0.0.0 torque=0.0.0 smooth_force=40000000.40666667.40b33333 smooth_torque=40800000.40a00000.40c00000',
        'force mode=4 created=1',
        'force mode=4 stage=after_force dirty=80.2 linear=0.0.0 angular=0.0.0 force=0.0.0 torque=0.0.0 smooth_force=41200000.41900000.41e00000 smooth_torque=0.0.0',
        'force mode=4 stage=after_torque dirty=100.2 linear=0.0.0 angular=0.0.0 force=0.0.0 torque=0.0.0 smooth_force=41200000.41900000.41e00000 smooth_torque=41000000.41700000.41c00000',
        'force sleepy_created=1',
        'force low_wake_initial=3dcccccd.3dcccccd.0',
        'force low_wake_after=3ecccccc.3ecccccc.0.30.2',
        'force forced_sleep_after=0.0.100.40.2',
        'force rotated_created=1',
        'force rotated_inverse=3eaaaaa9.31c40392.0.31c40392.3efffffe.0.0.0.3e800000',
        'force rotated_accel=402aaaa9.40effffe.40c00000',
        'force rotated_velocity=402aaaa9.40effffe.40c00000',
        'force local_created=1',
        'force local_accel=c0666665.40000000.40b33333.c09ffffe.407ffffe.40c00000',
        'force local_velocity=c0666665.40000000.40b33333.c09ffffe.407ffffe.40c00000',
        'force atpos_created=1',
        'force atpos_cmass=40800000.40a00000.40c00000',
        'force atpos global_force linear=0.0.0 angular=0.0.0 force=40000000.40666667.40b33333 torque=3f800000.3f2aaaab.bf000000 smooth_force=0.0.0 smooth_torque=0.0.0',
        'force atpos local_position linear=40000000.40666667.40b33333 angular=3f800000.3f2aaaab.bf000000 force=40000000.40666667.40b33333 torque=3f800000.3f2aaaab.bf000000 smooth_force=0.0.0 smooth_torque=0.0.0',
        'force atpos local_force linear=40000000.40666667.40b33333 angular=3f800000.3f2aaaab.bf000000 force=40000000.40666667.40b33333 torque=3f800000.3f2aaaab.bf000000 smooth_force=40000000.40666667.40b33333 smooth_torque=3f800000.3f2aaaab.bf000000',
        'force atpos both_local linear=41400000.41accccd.42066666 angular=40400000.402aaaab.c0200000 force=40000000.40666667.40b33333 torque=3f800000.3f2aaaab.bf000000 smooth_force=40000000.40666667.40b33333 smooth_torque=3f800000.3f2aaaab.bf000000',
        'force offset_created=1',
        'force offset_cmass=40000001.40c00000.41100000',
        'force atpos offset_local_position linear=0.0.0 angular=0.0.0 force=40000000.40666667.40b33333 torque=0.0.0 smooth_force=0.0.0 smooth_torque=0.0.0',
        'force atpos offset_both_local linear=c0666665.40000000.40b33333 angular=40bffffe.40a00001.3ffffffc force=40000000.40666667.40b33333 torque=0.0.0 smooth_force=0.0.0 smooth_torque=0.0.0',
        'force kinematic_created=1',
        'force kinematic_unchanged=0.0.0.0.0.0.0.0'
    )
    'NxPhysicsActorMomentumTests' = @(
        # NpActor.cpp completion Task 3 (000168, 000166, 000060/000742, 000134-000144): the
        # _fpclass gate on 1/m (negative, zero, -0, NaN, Inf, denormal, FLT_MIN inertias),
        # positive masses the report does not reach, the kinetic energy (one case where the
        # listing's order decides a tie), and the getters' W = R F orders and 000746 tensors
        # over mass frames near R^T and two frames where 000134's order differs from 000138's.
        'momentum t3_get_general created=1',
        'momentum t3_get_general cmass_orientation=bf1df347.bf47721d.bde38e3b.3e93e93e.beb60b63.3f638e38.bf3b6611.3f042ed8.3ee38e38',
        'momentum t3_get_general cmass_position=3eaaaaa9.3f8aaaab.402aaaab',
        'momentum t3_get_general cmass_orientation_only=bf1df347.bf47721d.bde38e3b.3e93e93e.beb60b63.3f638e38.bf3b6611.3f042ed8.3ee38e38',
        'momentum t3_get_general inertia=400352c4.3a185632.bf2cf961.3a185632.40858cf4.3f939d16.bf2cf961.3f939d16.40119350',
        'momentum t3_get_general inverse_inertia=3f0cc86b.bd5761d1.3e42944c.bd5761d1.3e913ddd.be234528.3e42944c.be234528.3f13b0e6',
        'momentum t3_get_general angular_momentum=3c888485.c0403612.40337d35',
        'momentum t3_get_inverse0 created=1',
        'momentum t3_get_inverse0 cmass_orientation=3f7fffff.32cccccc.b0888888.b0888888.3f800000.b1088884.334ccccc.3288888a.3f800000',
        'momentum t3_get_inverse0 cmass_position=3eaaaaa9.3f8aaaab.402aaaab',
        'momentum t3_get_inverse0 cmass_orientation_only=3f7fffff.32cccccc.b0888888.b0888888.3f800000.b1088884.334ccccc.3288888a.3f800000',
        'momentum t3_get_inverse0 inertia=3f9fffff.337aaaaa.336bbbba.337aaaaa.40200000.33022225.336bbbba.33022225.40980000',
        'momentum t3_get_inverse0 inverse_inertia=3f4ccccb.32162fc9.3322f116.32162fc9.3ecccccd.31cc14da.3322f116.31cc14da.3e579436',
        'momentum t3_get_inverse0 angular_momentum=3f5fffff.c04fffff.411f9999',
        'momentum t3_get_inverse1 created=1',
        'momentum t3_get_inverse1 cmass_orientation=3f7fffff.33fe10c2.337d0482.33f05cda.3f7ffff6.32cfa327.33512b33.32ea1356.3f7ffff8',
        'momentum t3_get_inverse1 cmass_position=be696e47.3ed09ac1.3f19cb26',
        'momentum t3_get_inverse1 cmass_orientation_only=3f7fffff.33fe10c2.337d0482.33f05cda.3f7ffff6.32cfa327.33512b33.32ea1356.3f7ffff8',
        'momentum t3_get_inverse1 inertia=3f9fffff.34e9e777.34b6e968.34e9e777.401ffff4.34446ee4.34b6e968.34446ee4.4097fff7',
        'momentum t3_get_inverse1 inverse_inertia=3f4ccccb.3412f549.335c9a12.3412f549.3eccccbd.328957e2.335c9a12.328957e2.3e579429',
        'momentum t3_get_inverse1 angular_momentum=3f600001.c04fffed.411f9990',
        'momentum t3_get_inverse2 created=1',
        'momentum t3_get_inverse2 cmass_orientation=3f7ffffe.b2f00a38.b0236e80.31c4a0e4.3f7fffff.3350705c.b384b54f.33866251.3f800001',
        'momentum t3_get_inverse2 cmass_position=bfc33128.3fdf856e.4036d2ac',
        'momentum t3_get_inverse2 cmass_orientation_only=3f7ffffe.b2f00a38.b0236e80.31c4a0e4.3f7fffff.3350705c.b384b54f.33866251.3f800001',
        'momentum t3_get_inverse2 inertia=3f9ffffe.b386a9d1.b3abf33c.b386a9d1.401ffffe.34cfc02a.b3abf33c.34cfc02a.40980002',
        'momentum t3_get_inverse2 inverse_inertia=3f4cccca.b1e2c2d6.b354deea.b1e2c2d6.3ecccccb.3317639d.b354deea.3317639d.3e579439',
        'momentum t3_get_inverse2 angular_momentum=3f5ffffb.c04ffffa.411f999b',
        'momentum t3_get_inverse3 created=1',
        'momentum t3_get_inverse3 cmass_orientation=3f7ffffd.b2b26c9a.331f07c1.b316c9b2.3f7fffff.b1014af6.33d890cd.b351745c.3f7ffffd',
        'momentum t3_get_inverse3 cmass_position=bfb79890.bf819dbc.4012bf5b',
        'momentum t3_get_inverse3 cmass_orientation_only=3f7ffffd.b2b26c9a.331f07c1.b316c9b2.3f7fffff.b1014af6.33d890cd.b351745c.3f7ffffd',
        'momentum t3_get_inverse3 inertia=3f9ffffc.b3cdc1ee.34a219d9.b3cdc1ee.401ffffe.b40c8149.34a219d9.b40c8149.4097fffc',
        'momentum t3_get_inverse3 inverse_inertia=3f4cccc8.b31c50ac.33bdfe15.b31c50ac.3ecccccb.b2aaf751.33bdfe15.b2aaf751.3e579431',
        'momentum t3_get_inverse3 angular_momentum=3f600007.c04ffffe.411f9995',
        'momentum t3_get_near3 created=1',
        'momentum t3_get_near3 cmass_orientation=3f7ff62f.bc25827c.bc663e52.3c241e56.3f7ffb7d.bbc819ac.3c673cd1.3bc37519.3f7ff84f',
        'momentum t3_get_near3 cmass_position=bfb79890.bf819dbc.4012bf5b',
        'momentum t3_get_near3 cmass_orientation_only=3f7ff62f.bc25827c.bc663e52.3c241e56.3f7ffb7d.bbc819ac.3c673cd1.3bc37519.3f7ff84f',
        'momentum t3_get_near3 inertia=3fa01ad4.bc49f32c.bd49bf85.bc49f32c.401fff51.bc63fb68.bd49bf85.bc63fb68.4097f9a3',
        'momentum t3_get_near3 inverse_inertia=3f4cc27f.3b82be5e.3c0819d0.3b82be5e.3eccd122.3a9f0c89.3c0819d0.3a9f0c89.3e57b4c3',
        'momentum t3_get_near3 angular_momentum=3f49c4f6.c0526b3d.411f4fca',
        'momentum t3_rf0 pose_orientation=3f7ffffc.acbc4cc0.b4db35b3.3455b488.3f800005.34ef07c0.b4a8c1be.3330a7e6.3f7ffffb',
        'momentum t3_rf0 orientation=3f7ffffc.acbc4d00.b4db35b3.3455b488.3f800005.34ef07c0.b4a8c1be.3330a7e6.3f7ffffb',
        'momentum t3_rf0 inverse_inertia=3f4cccc7.342af637.b4b527a3.342af637.3eccccdd.33ec9ea3.b4b527a3.33ec9ea3.3e57942e',
        'momentum t3_rf1 pose_orientation=3f800005.3482424e.b285c4c1.b3b6192b.3f800007.30b8f4d4.32ab3868.3504862a.3f7ffff6',
        'momentum t3_rf1 orientation=3f800005.3482424e.b285c4c1.b3b6192b.3f800007.30b8f4d5.32ab3868.3504862a.3f7ffff6',
        'momentum t3_rf1 inverse_inertia=3f4cccdd.32faf182.3259a145.32faf182.3ecccce3.345457c9.3259a145.345457c9.3e579425',
        'momentum t3_inertia 0 in=40000000.40400000.40800000 stored=40000000.40400000.40800000 inverse=3f000000.3eaaaaab.3e800000',
        'momentum t3_inertia 1 in=c0000000.40400000.40800000 stored=c0000000.40400000.40800000 inverse=bf000000.3eaaaaab.3e800000',
        'momentum t3_inertia 2 in=0.3f800000.3f800000 stored=0.3f800000.3f800000 inverse=0.0.0',
        'momentum t3_inertia 3 in=80000000.3f800000.3f800000 stored=80000000.3f800000.3f800000 inverse=0.0.0',
        'momentum t3_inertia 4 in=3f800000.7fc00000.3f800000 stored=3f800000.7fc00000.3f800000 inverse=0.0.0',
        'momentum t3_inertia 5 in=3f800000.3f800000.7f800000 stored=3f800000.3f800000.7f800000 inverse=3f800000.3f800000.0',
        'momentum t3_inertia 6 in=100.3f800000.3f800000 stored=100.3f800000.3f800000 inverse=0.0.0',
        'momentum t3_inertia 7 in=800000.3f800000.3f800000 stored=800000.3f800000.3f800000 inverse=7e800000.3f800000.3f800000',
        'momentum t3_inertia 8 in=3e800000.3e800000.ff800000 stored=3e800000.3e800000.ff800000 inverse=40800000.40800000.80000000',
        'momentum t3_mass 0 in=7f800000 stored=7f800000 inverse=0',
        'momentum t3_mass 1 in=100 stored=100 inverse=7f800000',
        'momentum t3_mass 2 in=800000 stored=800000 inverse=7e800000',
        'momentum t3_mass 3 in=40a00000 stored=40a00000 inverse=3e4ccccd',
        'momentum t3_energy 0 energy=4286f0a9',
        'momentum t3_energy 1 energy=3f000000',
        'momentum t3_energy 2 energy=3f000000',
        # NpActor.cpp completion Task 3 (000174, 000176, 000180, 000182): the velocity and
        # momentum setters' wake at, just below and just above the sleep thresholds, NaN and
        # asleep, with the dirty word; 000182's row sums over a general +0x164, including
        # three near-cancelling momenta where the listing's order decides the last bit.
        'momentum t3 created=1',
        'momentum t3 thresholds=3e800000.3f100000 inverse=3f06df56.bcb4a48b.3ddf8877.bcb4a48b.3ea521de.bdf014f6.3ddf8877.bdf014f6.3f0745c5',
        'momentum t3_wake lv_equal in=3f000000.0.0 lin=3f000000.0.0 ang=0.0.0 wake=3ecccccc.3ecccccc.0 dirty=14',
        'momentum t3_wake lv_below in=0.3effffff.0 lin=0.3effffff.0 ang=0.0.0 wake=3dcccccd.3dcccccd.0 dirty=4',
        'momentum t3_wake lv_above in=0.0.3f000001 lin=0.0.3f000001 ang=0.0.0 wake=3ecccccc.3ecccccc.0 dirty=14',
        'momentum t3_wake lv_mixed in=3e99999a.3ecccccd.0 lin=3e99999a.3ecccccd.0 ang=0.0.0 wake=3ecccccc.3ecccccc.0 dirty=14',
        'momentum t3_wake lv_nan in=ffc00000.0.0 lin=ffc00000.0.0 ang=0.0.0 wake=3dcccccd.3dcccccd.0 dirty=4',
        'momentum t3_wake lv_asleep in=40400000.40800000.40a00000 lin=40400000.40800000.40a00000 ang=0.0.0 wake=0.0.100 dirty=4',
        'momentum t3_wake av_equal in=0.3f400000.0 lin=40400000.40800000.40a00000 ang=0.3f400000.0 wake=3ecccccc.3ecccccc.0 dirty=18',
        'momentum t3_wake av_below in=3f3fffff.0.0 lin=40400000.40800000.40a00000 ang=3f3fffff.0.0 wake=3dcccccd.3dcccccd.0 dirty=8',
        'momentum t3_wake av_above in=0.0.3f400001 lin=40400000.40800000.40a00000 ang=0.0.3f400001 wake=3ecccccc.3ecccccc.0 dirty=18',
        'momentum t3_wake av_mixed in=3ee66666.3f19999a.0 lin=40400000.40800000.40a00000 ang=3ee66666.3f19999a.0 wake=3ecccccc.3ecccccc.0 dirty=18',
        'momentum t3_wake av_nan in=0.ffc00000.0 lin=40400000.40800000.40a00000 ang=0.ffc00000.0 wake=3dcccccd.3dcccccd.0 dirty=8',
        'momentum t3_wake av_asleep in=3f800000.40000000.40400000 lin=40400000.40800000.40a00000 ang=3f800000.40000000.40400000 wake=0.0.100 dirty=8',
        'momentum t3_wake lm_equal in=3f800000.0.0 lin=3f000000.0.0 ang=3f800000.40000000.40400000 wake=3ecccccc.3ecccccc.0 dirty=14',
        'momentum t3_wake lm_below in=0.3f7fffff.0 lin=0.3effffff.0 ang=3f800000.40000000.40400000 wake=3dcccccd.3dcccccd.0 dirty=4',
        'momentum t3_wake lm_above in=0.0.3f800001 lin=0.0.3f000001 ang=3f800000.40000000.40400000 wake=3ecccccc.3ecccccc.0 dirty=14',
        'momentum t3_wake lm_mixed in=3f19999a.3f4ccccd.0 lin=3e99999a.3ecccccd.0 ang=3f800000.40000000.40400000 wake=3ecccccc.3ecccccc.0 dirty=14',
        'momentum t3_wake lm_nan in=0.0.ffc00000 lin=0.0.ffc00000 ang=3f800000.40000000.40400000 wake=3dcccccd.3dcccccd.0 dirty=4',
        'momentum t3_wake lm_asleep in=40400000.40800000.40a00000 lin=3fc00000.40000000.40200000 ang=3f800000.40000000.40400000 wake=0.0.100 dirty=4',
        'momentum t3_wake am_general in=3fa66666.c02ccccd.3f666666 lin=3fc00000.40000000.40200000 ang=3f57b92c.bf80a362.3f6f18d7 wake=3ecccccc.3ecccccc.0 dirty=18',
        'momentum t3_wake am_general2 in=bebd70a4.3de147ae.40a9999a lin=3fc00000.40000000.40200000 ang=3ec32242.bf13e21f.402fd384 wake=3ecccccc.3ecccccc.0 dirty=18',
        'momentum t3_wake am_small in=3c23d70a.bca3d70a.3cf5c28f lin=3fc00000.40000000.40200000 ang=3c133126.bc26eacc.3c9e026f wake=3dcccccd.3dcccccd.0 dirty=8',
        'momentum t3_wake am_large in=3f333333.3f666666.bf8ccccd lin=3fc00000.40000000.40200000 ang=3e6a6092.3ecebd0c.bf1c3ff7 wake=3ecccccc.3ecccccc.0 dirty=18',
        'momentum t3_wake am_cancel_x in=c2838868.c1dbd53d.439bf301 lin=3fc00000.40000000.40200000 ang=b603e8f8.c22fe6f0.4320da55 wake=3ecccccc.3ecccccc.0 dirty=18',
        'momentum t3_wake am_cancel_y in=444e6cfc.42614caa.beb57702 lin=3fc00000.40000000.40200000 ang=43d8de56.b3b71261.42a6aa71 wake=3ecccccc.3ecccccc.0 dirty=18',
        'momentum t3_wake am_cancel_z in=c54e02be.c1e93add.44289883 lin=3fc00000.40000000.40200000 ang=c4cfca54.c17c6558.b71e1e16 wake=3ecccccc.3ecccccc.0 dirty=18',
        'momentum t3_wake am_nan in=ffc00000.3f800000.3f800000 lin=3fc00000.40000000.40200000 ang=ffc00000.ffc00000.ffc00000 wake=3dcccccd.3dcccccd.0 dirty=8',
        'momentum t3_wake am_asleep in=40400000.40800000.40a00000 lin=3fc00000.40000000.40200000 ang=40026faf.3f234735.40200963 wake=0.0.100 dirty=8',
        'momentum created=1',
        'momentum inverse_tensor=3f000000.0.0.0.3eaaaaab.0.0.0.3e800000',
        'momentum rotation_matrix=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'momentum inertia_frame=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'momentum initial_limit=42440000',
        'momentum linear_initial=40a00000.41200000.41700000',
        'momentum angular_initial=41000000.41700000.41c00000',
        'momentum global_inertia_initial=40000000.0.0.0.40400000.0.0.0.40800000',
        'momentum global_inverse_initial=3f000000.0.0.0.3eaaaaab.0.0.0.3e800000',
        'momentum energy_initial=43208000',
        'momentum max_angular=42a20000',
        'momentum linear_set=41200000.41700000.41a00000',
        'momentum linear_velocity=40000000.40400000.40800000',
        'momentum linear_record=40000000.40400000.40800000.40000000.40400000.40800000',
        'momentum angular_set=41200000.41900000.41e00000',
        'momentum angular_velocity=40a00000.40c00000.40e00000',
        'momentum angular_record=40a00000.40c00000.40e00000.40a00000.40c00000.40e00000',
        'momentum energy_set=43798000',
        'momentum mutation_allocs=0.0',
        'momentum dirty_limit=8000.2.1',
        'momentum dirty_linear=4.2.1',
        'momentum dirty_angular=8.2.1',
        'momentum kinematic_angular=40c00000.40e00000.41000000.40c00000.40e00000.41000000',
        'momentum kinematic_linear=0.0.0',
        'momentum rotated_created=1',
        'momentum rotated_quaternion=0.0.3f3504f3.3f3504f3',
        'momentum rotated_inverse=3eaaaaa9.31c40392.0.31c40392.3efffffe.0.0.0.3e800000',
        'momentum rotated_rotation=331302ae.bf7fffff.0.3f7fffff.331302ae.0.0.0.3f800000',
        'momentum rotated_frame=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'momentum rotated_angular_initial=413ffffe.411fffff.41c00000',
        'momentum rotated_global_inertia=403ffffe.b31302ad.0.b31302ad.3ffffffe.0.0.0.40800000',
        'momentum rotated_global_inverse=3eaaaaa9.31c40392.0.31c40392.3efffffe.0.0.0.3e800000',
        'momentum rotated_angular_set=418ffffd.419ffffe.41e00000',
        'momentum rotated_angular_velocity=40bffffe.411fffff.40e00000',
        'momentum rotated_energy=439f7fff',
        'momentum rotated_changed_inverse=3eaaaaa9.31c40392.0.31c40392.3efffffe.0.0.0.3e800000',
        'momentum rotated_changed_angular=41effffa.41effffc.42440000',
        'momentum rotated_changed_inertia=409ffffe.b39302ad.0.b39302ad.403ffffe.0.0.0.40e00000',
        'momentum rotated_changed_global_inverse=3e4ccccb.319ccfa9.0.319ccfa9.3eaaaaa9.0.0.0.3e124925',
        'momentum rotated_changed_velocity=40d55554.4127ffff.40e00000',
        'momentum offset_created=1',
        'momentum offset_quaternion=0.0.0.3f800000',
        'momentum offset_frame=3f800000.0.0.0.0.bf800000.0.3f800000.0',
        'momentum offset_rotation=3f800000.0.0.0.0.bf800000.0.3f800000.0',
        'momentum offset_inverse=3f000000.0.0.0.3e800000.0.0.0.3eaaaaab',
        'momentum offset_global_inertia=40000000.0.0.0.40800000.0.0.0.40400000',
        'momentum offset_global_inverse=3f000000.0.0.0.3e800000.0.0.0.3eaaaaab',
        'momentum offset_angular_initial=41000000.41a00000.41900000',
        'momentum offset_angular_velocity=40800000.40700000.41000000',
        'momentum offset_energy=43481800',
        'momentum static_created=1',
        'momentum static_global_inertia=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'momentum static_global_inverse=3f800000.0.0.0.3f800000.0.0.0.3f800000',
        'momentum static_energy=0'
    )
    'NxPhysicsActorDynamicSetterTests' = @(
        # NpActor.cpp completion Task 3 (contract NG and 000094): the read lock of the
        # unguarded readers, seen through the read-link block's flag/owner words (000142
        # takes none), and 000094's static-arm conversion over six orientations.
        'setter ng_nb_shapes_0 state=0.1',
        'setter ng_shapes_0 state=0.1',
        'setter ng_name_0 state=0.1',
        'setter ng_position_0 state=0.1',
        'setter ng_orientation_0 state=0.1',
        'setter ng_orientation_quat_0 state=0.1',
        'setter ng_pose_0 state=0.1',
        'setter ng_is_dynamic_0 state=0.1',
        'setter ng_inverse_inertia_0 state=1.0',
        'setter ng_nb_shapes_1 state=0.1',
        'setter ng_shapes_1 state=0.1',
        'setter ng_name_1 state=0.1',
        'setter ng_position_1 state=0.1',
        'setter ng_orientation_1 state=0.1',
        'setter ng_orientation_quat_1 state=0.1',
        'setter ng_pose_1 state=0.1',
        'setter ng_is_dynamic_1 state=0.1',
        'setter static_quat_0=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'setter static_quat_1=3f800000.0.0.0',
        'setter static_quat_2=0.3f800000.0.0',
        'setter static_quat_3=3f741dfd.3e4d9285.3dcd9285.3e4d9285',
        'setter static_quat_4=3e1ac3df.3f6825d0.be9ac3df.3e80f88f',
        'setter static_quat_5=be4c8b64.3e7fae3d.3f661ccf.3e99688b',
        # NpActor.cpp completion Task 2 (contract G1/E1): the Foundation error stream the
        # target now passes, enabled only for these cases, prints every report the pair
        # delivers. Static, kinematic and invalid-argument calls report E1 with the row's
        # line and message; with the write lock held by "another thread" every guarded
        # row reports G1 with its own line. Copied from the oracle side, duplicates once.
        'setter error_actors=1.1.1.0',
        'setter report=1.d2.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setLinearDamping: Actor must be dynamic!',
        'setter error_static_linear_damping=1',
        'setter report=1.d1.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setLinearDamping: The linear damping must be nonnegative!',
        'setter error_static_linear_damping_negative=1',
        'setter report=1.e1.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setAngularDamping: Actor must be dynamic!',
        'setter error_static_angular_damping=1',
        'setter report=1.e0.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setAngularDamping: The angular damping must be nonnegative!',
        'setter error_static_angular_damping_negative=1',
        'setter report=1.ba.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setMass: Actor must be dynamic!',
        'setter error_static_mass=1',
        'setter report=1.c6.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setMassSpaceInertiaTensor: Actor must be dynamic!',
        'setter error_static_inertia=1',
        'setter report=1.f4.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setLinearVelocity: Actor must be (non-kinematic) dynamic!',
        'setter error_static_linear_velocity=1',
        'setter report=1.fd.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setAngularVelocity: Actor must be (non-kinematic) dynamic!',
        'setter error_static_angular_velocity=1',
        'setter report=1.109.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setMaxAngularVelocity: Actor must be dynamic!',
        'setter error_static_max_angular=1',
        'setter report=1.114.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setLinearMomentum: Actor must be dynamic!',
        'setter error_static_linear_momentum=1',
        'setter report=1.11d.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setAngularMomentum: Actor must be (non-kinematic) dynamic!',
        'setter error_static_angular_momentum=1',
        'setter report=1.1d0.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::raiseBodyFlag: Actor must be dynamic!',
        'setter error_static_raise_body_flag=1',
        'setter report=1.1da.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::clearBodyFlag: Actor must be dynamic!',
        'setter error_static_clear_body_flag=1',
        'setter report=1.12b.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::addForceAtPos: Actor must be (non-kinematic) dynamic!',
        'setter error_static_force_at_pos=1',
        'setter report=1.132.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::addForceAtLocalPos: Actor must be (non-kinematic) dynamic!',
        'setter error_static_force_at_local_pos=1',
        'setter report=1.13c.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::addLocalForceAtPos: Actor must be (non-kinematic) dynamic!',
        'setter error_static_local_force_at_pos=1',
        'setter report=1.145.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::addLocalForceAtLocalPos: Actor must be (non-kinematic) dynamic!',
        'setter error_static_local_force_at_local_pos=1',
        'setter report=1.14e.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::addForce: Actor must be (non-kinematic) dynamic!',
        'setter error_static_force=1',
        'setter report=1.157.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::addLocalForce: Actor must be (non-kinematic) dynamic!',
        'setter error_static_local_force=1',
        'setter report=1.161.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::addTorque: Actor must be (non-kinematic) dynamic!',
        'setter error_static_torque=1',
        'setter report=1.16a.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::addLocalTorque: Actor must be (non-kinematic) dynamic!',
        'setter error_static_local_torque=1',
        'setter report=1.2a1.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::moveGlobalPosition: Actor must be kinematic!',
        'setter error_static_move_position=1',
        'setter report=1.291.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::moveGlobalPose: Actor must be kinematic!',
        'setter error_static_move_pose=1',
        'setter report=1.2ae.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::moveGlobalOrientation: Actor must be kinematic!',
        'setter error_static_move_orientation=1',
        'setter report=1.388.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setCMassOffsetLocalPose: Actor must be (non-kinematic) dynamic!',
        'setter error_static_cmass_local_pose=1',
        'setter report=1.394.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setCMassOffsetLocalPosition: Actor must be (non-kinematic) dynamic!',
        'setter error_static_cmass_local_position=1',
        'setter report=1.39f.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setCMassOffsetLocalOrientation: Actor must be (non-kinematic) dynamic!',
        'setter error_static_cmass_local_orientation=1',
        'setter report=1.3ac.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setCMassOffsetGlobalPose: Actor must be (non-kinematic) dynamic!',
        'setter error_static_cmass_offset_global_pose=1',
        'setter report=1.3b8.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setCMassOffsetGlobalPosition: Actor must be (non-kinematic) dynamic!',
        'setter error_static_cmass_offset_global_position=1',
        'setter report=1.3c1.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setCMassOffsetGlobalOrientation: Actor must be (non-kinematic) dynamic!',
        'setter error_static_cmass_offset_global_orientation=1',
        'setter report=1.268.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setCMassGlobalPose: Actor must be (non-kinematic) dynamic!',
        'setter error_static_cmass_global_pose=1',
        'setter report=1.276.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setCMassGlobalPosition: Actor must be (non-kinematic) dynamic!',
        'setter error_static_cmass_global_position=1',
        'setter report=1.281.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setCMassGlobalOrientation: Actor must be (non-kinematic) dynamic!',
        'setter error_static_cmass_global_orientation=1',
        'setter report=1.d9.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::setLinearDamping: Actor must be dynamic!',
        'setter error_static_get_linear_damping=0.1',
        'setter report=1.e8.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getAngularDamping: Actor must be dynamic!',
        'setter error_static_get_angular_damping=0.1',
        'setter report=1.1e3.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::readBodyFlag: Actor must be dynamic!',
        'setter error_static_read_body_flag=0.1',
        'setter report=1.343.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getLinearVelocity: Actor must be dynamic!',
        'setter error_static_get_linear_velocity=0.0.0.1',
        'setter report=1.34a.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getAngularVelocity: Actor must be dynamic!',
        'setter error_static_get_angular_velocity=0.0.0.1',
        'setter report=1.353.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getLinearMomentumVal: Cannot be called on a static actor!',
        'setter error_static_get_linear_momentum=0.0.0.1',
        'setter report=1.35a.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getAngularMomentumVal: Cannot be called on a static actor!',
        'setter error_static_get_angular_momentum=0.0.0.1',
        'setter report=1.328.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getMassSpaceInertiaTensorVal: Cannot be called on a static actor!',
        'setter error_static_get_inertia=0.0.0.1',
        'setter report=1.32f.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getGlobalInertiaTensorVal: Cannot be called on a static actor!',
        'setter error_static_get_global_inertia=3f800000.0.0.0.3f800000.0.0.0.3f800000.1',
        'setter report=1.338.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getGlobalInertiaTensorInverseVal: Cannot be called on a static actor!',
        'setter error_static_get_global_inverse=3f800000.0.0.0.3f800000.0.0.0.3f800000.1',
        'setter report=1.2f2.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getCMassLocalPose: Cannot be called on a static actor!',
        'setter error_static_get_cmass_local_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.1',
        'setter report=1.2fa.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getCMassLocalPosition: Cannot be called on a static actor!',
        'setter error_static_get_cmass_local_position=0.0.0.1',
        'setter report=1.301.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getCMassLocalOrientation: Cannot be called on a static actor!',
        'setter error_static_get_cmass_local_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000.1',
        'setter report=1.30a.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getCMassGlobalPose: Cannot be called on a static actor!',
        'setter error_static_get_cmass_global_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0.1',
        'setter report=1.314.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getCMassGlobalPosition: Cannot be called on a static actor!',
        'setter error_static_get_cmass_global_position=0.0.0.1',
        'setter report=1.31d.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::getCMassGlobalOrientation: Cannot be called on a static actor!',
        'setter error_static_get_cmass_global_orientation=3f800000.0.0.0.3f800000.0.0.0.3f800000.1',
        'setter report=1.1ac.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Actor::createShape: desc.isValid() fails!',
        'setter error_static_create_invalid_shape=0.1',
        'setter report=1.bb.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Body::setMass: mass is -3.000000, should be positive!',
        'setter error_dynamic_mass_negative=1',
        'setter report=1.bb.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.Body::setMass: mass is 0.000000, should be positive!',
        'setter error_dynamic_mass_zero=1',
        'setter error_dynamic_linear_damping_negative=1',
        'setter error_dynamic_angular_damping_negative=1',
        'setter error_dynamic_unchanged=40000000.3f000000.0.3d4ccccd',
        'setter error_dynamic_move_position=1',
        'setter error_dynamic_move_pose=1',
        'setter error_dynamic_move_orientation=1',
        'setter error_dynamic_create_invalid_shape=0.1',
        'setter error_dynamic_shapes_after_invalid=1.0',
        'setter error_kinematic_linear_velocity=1',
        'setter error_kinematic_angular_velocity=1',
        'setter error_kinematic_angular_momentum=1',
        'setter error_kinematic_linear_momentum=0',
        'setter error_kinematic_force=1',
        'setter error_kinematic_torque=1',
        'setter error_kinematic_local_force=1',
        'setter error_kinematic_local_torque=1',
        'setter error_kinematic_force_at_pos=1',
        'setter error_kinematic_force_at_local_pos=1',
        'setter error_kinematic_local_force_at_pos=1',
        'setter error_kinematic_local_force_at_local_pos=1',
        'setter error_kinematic_cmass_local_pose=1',
        'setter error_kinematic_cmass_local_position=1',
        'setter error_kinematic_cmass_local_orientation=1',
        'setter error_kinematic_cmass_offset_global_pose=1',
        'setter error_kinematic_cmass_offset_global_position=1',
        'setter error_kinematic_cmass_offset_global_orientation=1',
        'setter error_kinematic_cmass_global_pose=1',
        'setter error_kinematic_cmass_global_position=1',
        'setter error_kinematic_cmass_global_orientation=1',
        'setter error_kinematic_move_position=0',
        'setter report=2.21d.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_global_pose=1',
        'setter report=2.232.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_global_position=1',
        'setter report=2.242.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_global_orientation=1',
        'setter report=2.254.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_global_orientation_quat=1',
        'setter report=2.28f.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_move_pose=1',
        'setter report=2.29e.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_move_position=1',
        'setter report=2.2ac.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_move_orientation=1',
        'setter report=2.1ab.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_create_shape=1',
        'setter report=2.1b3.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_release_shape=1',
        'setter report=2.387.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_cmass_local_pose=1',
        'setter report=2.393.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_cmass_local_position=1',
        'setter report=2.39e.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_cmass_local_orientation=1',
        'setter report=2.3ab.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_cmass_offset_global_pose=1',
        'setter report=2.3b7.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_cmass_offset_global_position=1',
        'setter report=2.3c0.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_cmass_offset_global_orientation=1',
        'setter report=2.269.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_cmass_global_pose=1',
        'setter report=2.277.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_cmass_global_position=1',
        'setter report=2.282.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_cmass_global_orientation=1',
        'setter report=2.b9.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_mass=1',
        'setter report=2.c5.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_inertia=1',
        'setter report=2.d0.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_linear_damping=1',
        'setter report=2.df.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_angular_damping=1',
        'setter report=2.f3.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_linear_velocity=1',
        'setter report=2.fc.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_angular_velocity=1',
        'setter report=2.108.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_max_angular=1',
        'setter report=2.113.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_linear_momentum=1',
        'setter report=2.11c.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_angular_momentum=1',
        'setter report=2.12a.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_force_at_pos=1',
        'setter report=2.131.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_force_at_local_pos=1',
        'setter report=2.13b.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_local_force_at_pos=1',
        'setter report=2.144.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_local_force_at_local_pos=1',
        'setter report=2.14d.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_force=1',
        'setter report=2.156.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_local_force=1',
        'setter report=2.160.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_torque=1',
        'setter report=2.169.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_local_torque=1',
        'setter report=2.195.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_sleep_linear=1',
        'setter report=2.1a2.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_sleep_angular=1',
        'setter report=2.207.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_wake=1',
        'setter report=2.211.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_sleep=1',
        'setter report=2.1bb.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_raise_actor_flag=1',
        'setter report=2.1c1.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_clear_actor_flag=1',
        'setter report=2.1cf.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_raise_body_flag=1',
        'setter report=2.1d9.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_clear_body_flag=1',
        'setter report=2.22.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_save_to_desc=1',
        'setter report=2.1ff.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_name=1',
        'setter report=2.3cd.\Epic\Novodex\SDKs\Physics\src\NpActor.cpp.PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'setter error_locked_group=1',
        'setter error_locked_get_linear_damping=0.0',
        'setter error_locked_static_cmass_global_pose=1',
        'setter error_locked_static_state=1.0',
        'setter error_locked_unchanged=40000000.3f000000.0.3d4ccccd.40400000.40800000.40a00000.100.3ecccccc.1.0.0',
        # The lines registered before Task 2.
        'setter created=1',
        'setter initial_shape_vtable=1.1.1.1',
        'setter initial_shape_center=0.0.0.406f7751',
        'setter initial_shape_local=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'setter initial_shape_world=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'setter initial_pruner=1.1.7f7fffff.7f7fffff.7f7fffff.ff7fffff.ff7fffff.ff7fffff',
        'setter initial_wake=3ecccccc.3ecccccc.3cb851ec.3ca0902e',
        'setter group_initial=0.1.1',
        'setter mass=41000000.3e000000.41000000.40a00000.40c00000.40e00000.3e4ccccd.3e2aaaab.3e124925',
        'setter damping=3ecccccd.3f19999a.3ecccccd.3f19999a',
        'setter velocity=40e00000.41000000.41100000.41200000.41300000.41400000',
        'setter record_velocity=40e00000.41000000.41100000.41200000.41300000.41400000',
        'setter shadow_velocity=40e00000.41000000.41100000.41200000.41300000.41400000',
        'setter wake=3ecccccc.3ecccccc.3cb851ec.3ca0902e',
        'setter mutation_allocs=0.0',
        'setter dirty_mass=10000.2.1',
        'setter dirty_inertia=20000.2.1',
        'setter dirty_linear_damping=800.2.1',
        'setter dirty_angular_damping=1000.2.1',
        'setter dirty_linear_velocity=4.2.1',
        'setter dirty_angular_velocity=8.2.1',
        'setter kinematic_velocity=41500000.41600000.41700000.41800000.41880000.41900000',
        'setter kinematic_allocs=0.0',
        'setter dirty_sleep_linear=2000.2.1',
        'setter dirty_sleep_angular=4000.2.1',
        'setter sleep_thresholds=3d800000.3e800000.3e800000.3f000000',
        'setter dirty_wake=10.2.1',
        'setter wake_state=3f400000.3f400000.0.0',
        'setter dirty_sleep=10.2.1',
        'setter asleep_state=0.0.100.1',
        'setter group_asleep=1',
        'setter dirty_rewake=10.2.1',
        'setter rewake_state=3f000000.3f000000.0.0',
        'setter group_rewake=0',
        'setter dirty_negative_sleep_linear=2000.2.1',
        'setter negative_sleep_linear=3d800000.3e800000',
        'setter negative_wake=bf000000.0.1',
        'setter group_two_created=1',
        'setter group_two_awake=0.0',
        'setter group_one_asleep=0.0',
        'setter group_both_asleep=1.1',
        'setter group_one_rewoke=0.0',
        'setter dirty_position=1.2.1',
        'setter position=40400000.c0000000.40a00000.40400000.c0000000.40a00000.40400000.c0000000.40a00000.40400000.c0000000.40a00000',
        'setter shape_position=40400000.c0000000.40a00000.0.0.0.0.0.0.80002',
        'setter rotated_center=3fc00000.c0300000.40300000',
        'setter shape_rotated_pose=331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0.3fa00000.c0200000.40300000.80002',
        'setter shape_pruner_state=0.1.1020000.2.0',
        'setter pruner_update=1.1.0.2.7f7fffff.7f7fffff.7f7fffff.ff7fffff.ff7fffff.ff7fffff',
        'setter dirty_orientation_quat=2.2.1',
        'setter orientation_quat=0.0.3f3504f3.3f3504f3.0.0.3f3504f3.3f3504f3.40400000.c0000000.40a00000',
        'setter shape_orientation_quat=331302ae.bf7fffff.0.3f7fffff.331302ae.0.0.0.3f800000.40400000.c0000000.40a00000.80002',
        'setter matrix_case0=2.2.1.0.0.3f3504f3.3f3504f3.0.0.3f3504f3.3f3504f3.331302ae.bf7fffff.0',
        'setter matrix_case1=2.2.1.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0',
        'setter matrix_case2=2.2.1.0.3f800000.0.0.0.3f800000.0.0.bf800000.0.0',
        'setter matrix_case3=2.2.1.0.0.3f800000.0.0.0.3f800000.0.bf800000.0.0',
        'setter dirty_pose=3.2.1',
        'setter pose_record=40e00000.c0a00000.40000000.40e00000.c0a00000.40000000.0.0.3f3504f3.3f3504f3.40e00000.c0a00000.40000000.0',
        'setter pose_shape=331302ae.bf7fffff.0.3f7fffff.331302ae.0.0.0.3f800000.40e00000.c0a00000.40000000',
        'setter static_created=1',
        'setter static_sleep=1.1.0.0',
        'setter static_after=1.1.0.0',
        'setter static_position=c0800000.40c00000.c1000000.c0800000.40c00000.c1000000',
        'setter static_orientation_quat=331302ae.bf7fffff.0.3f7fffff.331302ae.0.0.0.3f800000',
        'setter static_orientation_matrix=3f800000.0.0.0.bf800000.0.0.0.bf800000',
        'setter static_shape_orientation_matrix=3f800000.0.0.0.bf800000.0.0.0.bf800000',
        'setter static_pose_body=0.bf800000.0.3f800000.0.0.0.0.3f800000.bf800000.40000000.40800000',
        'setter static_pose_shape=0.bf800000.0.3f800000.0.0.0.0.3f800000.bf800000.40000000.40800000',
        'setter static_shape_global_pose=0.bf800000.0.bf800000.0.0.0.0.bf800000.c0a00000.c0400000.3f800000.3f800000.0.0.0.bf800000.0.0.0.bf800000.40000000.c0400000.40a00000.80002.1',
        'setter multi_created=1',
        'setter multi_position=2.40000000.40400000.c0800000.40000000.40400000.c0800000',
        'setter multi_pose=2.331302ae.bf7fffff.0.40e00000.c0a00000.331302ae.bf7fffff.0.40e00000.c0a00000',
        'setter posed_created=1',
        'setter posed_local=3f800000.0.0.0.0.bf800000.0.3f800000.0.3f000000.be800000.3f400000',
        'setter posed_world=331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0.40100000.40600000.c0500000',
        'setter posed_mirror=331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0.40100000.40600000.c0500000',
        'setter posed_public_local_pose=3f800000.0.0.0.0.bf800000.0.3f800000.0.3f000000.be800000.3f400000',
        'setter posed_public_local_position=3f000000.be800000.3f400000',
        'setter posed_public_local_orientation=3f800000.0.0.0.0.bf800000.0.3f800000.0',
        'setter posed_public_local_val_pose=3f800000.0.0.0.0.bf800000.0.3f800000.0.3f000000.be800000.3f400000',
        'setter posed_public_local_val_position=3f000000.be800000.3f400000',
        'setter posed_public_local_val_orientation=3f800000.0.0.0.0.bf800000.0.3f800000.0',
        'setter posed_public_global_pose=331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0.40100000.40600000.c0500000',
        'setter posed_public_global_position=40100000.40600000.c0500000',
        'setter posed_public_global_orientation=331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0',
        'setter posed_public_global_val_pose=331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0.40100000.40600000.c0500000',
        'setter posed_public_global_val_position=40100000.40600000.c0500000',
        'setter posed_public_global_val_orientation=331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0',
        'setter posed_box_bounds=bf3ffffc.40200000.c0a80000.40a80000.40900000.bfa00000',
        'setter box_changed=40000000.40400000.40800000.40000000.40400000.40800000.80002.1',
        'setter box_changed_world_bounds=bfdffffe.3fc00000.c0c80000.40c80000.40b00000.be800000',
        'setter box_world_obb=40100000.40600000.c0500000.40000000.40400000.40800000.331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0',
        'setter box_saved=1.2.72.5.2.1.40000000.40400000.40800000.3f800000.0.0.0.0.bf800000.0.3f800000.0.3f000000.be800000.3f400000.1.0',
        'setter box_changed_aabb=c0000000.c0400000.c0800000.40000000.40400000.40800000',
        'setter box_local_position=c0000000.3f800000.40400000.c0000000.3f800000.40400000.3f800000.3f800001.bf800000.480002.1',
        'setter box_local_orientation=0.bf800000.0.3f800000.0.0.0.0.3f800000.bf7fffff.b31302ae.0.331302ae.bf7fffff.0.0.0.3f800000.480002.1',
        'setter box_local_pose=3f800000.0.0.0.0.bf800000.0.3f800000.0.3e800000.bfc00000.40200000.331302ae.0.3f7fffff.3f7fffff.0.b31302ae.0.3f800000.0.40600000.40500000.bfc00000.480002.1',
        'setter box_global_position=40000000.bffffffe.c0000000.407fffff.40a00000.c0c00000.480002.1',
        'setter box_global_orientation=3f7fffff.b31302ae.0.331302ae.3f7fffff.0.0.0.3f800000.0.bf7ffffe.0.3f7ffffe.0.0.0.0.3f800000.480002.1',
        'setter box_global_pose=331302ae.0.bf7fffff.bf7fffff.0.b31302ae.0.3f800000.0.c0bfffff.3f7ffffc.40c00000.3f7ffffe.0.0.0.0.bf7ffffe.0.3f800000.0.3f800001.c03ffffc.40000000.480002.1',
        'setter sphere_vtable=1.1.1.1',
        'setter sphere_public=1.1.1.0.0',
        'setter sphere_public_radius=3f800000',
        'setter sphere_world_bounds=bf800000.bf800000.bf800000.3f800000.3f800000.3f800000',
        'setter sphere_changed=3fc00000.3fc00000.80002.1',
        'setter sphere_center=0.0.0.3f800000',
        'setter sphere_radius=3f800000',
        'setter sphere_local_position=3f000000.bf800000.40000000.3f000000.bf800000.40000000.3f000000.bf800000.40000000.80002.1',
        'setter sphere_global_pose=3f800000.0.0.0.bf800000.0.0.0.bf800000.3f800000.40000000.40400000.3f800000.0.0.0.bf800000.0.0.0.bf800000.3f800000.40000000.40400000.80002.1',
        'setter sphere_saved=1.1.8.0.0.1.1.0.3f800000.0.0.0.bf800000.0.0.0.bf800000.3f800000.40000000.40400000.3fc00000',
        'setter capsule_vtable=1.1.1.1',
        'setter capsule_public=1.3.1.0.0',
        'setter capsule_public_dimensions=3f000000.3f800000',
        'setter capsule_world_bounds=bf000000.bf800000.bf000000.3f000000.3f800000.3f000000',
        'setter capsule_changed=3f400000.40000000.3f400000.3f800000.80002.2',
        'setter capsule_set_dimensions=3f800000.40400000.3f800000.3fc00000.80002.1',
        'setter capsule_changed_aabb=bf800000.c0200000.bf800000.3f800000.40200000.3f800000',
        'setter capsule_changed_world_bounds=bf800000.c0200000.bf800000.3f800000.40200000.3f800000',
        'setter capsule_center=0.0.0.3f800000',
        'setter capsule_dimensions=3f000000.3f000000',
        'setter capsule_local_position=3f000000.bf800000.40000000.3f000000.bf800000.40000000.3f000000.bf800000.40000000.80002.1',
        'setter capsule_global_pose=3f800000.0.0.0.bf800000.0.0.0.bf800000.3f800000.40000000.40400000.3f800000.0.0.0.bf800000.0.0.0.bf800000.3f800000.40000000.40400000.80002.1',
        'setter capsule_saved=1.3.8.0.0.1.1.0.3f800000.0.0.0.bf800000.0.0.0.bf800000.3f800000.40000000.40400000.3f800000.40400000.0',
        'setter plane_vtable=1.1.1.1',
        'setter plane_public=1.0.1.0.0',
        'setter plane_world_bounds=feffffff.feffffff.feffffff.7effffff.0.7effffff',
        'setter plane_center=0.0.0.7f7fffff',
        'setter plane_equation=0.3f800000.0.80000000',
        'setter plane_basis=bf800000.0.0.80000000.80000000.3f800000.1',
        'setter plane_changed=0.0.3f800000.c0200000.0.bf800000.0.3f800000.80000000.80000000.2.80002.1',
        'setter plane_local_position=3f000000.bf800000.40000000.3f000000.bf800000.40000000.3f000000.bf800000.40000000.80002.1',
        'setter plane_global_pose=3f800000.0.0.0.bf800000.0.0.0.bf800000.3f800000.40000000.40400000.3f800000.0.0.0.bf800000.0.0.0.bf800000.3f800000.40000000.40400000.80002.1',
        'setter plane_saved=1.0.8.0.0.1.1.0.3f800000.0.0.0.bf800000.0.0.0.bf800000.3f800000.40000000.40400000.0.0.3f800000.40200000'
    )
    'NxPhysicsActorDynamicsTests' = @(
        # NpActor.cpp completion Task 3 (000124, 000126, 000090, 000784): kinematic actors
        # with rotated, offset mass frames; each move's full target block, wake words and
        # dirty word, with the wake counter below, at and above 0.39999998f or asleep.
        'dynamics kin_general created=1',
        'dynamics kin_general_pose_low target=3d81b4e5.402c5f93.3f6aaaac.3.be8ad5e3.3ee76479.3f46ff5f.3eafdbb8 wake=3ecccccc.3ecccccc.0 dirty=10',
        'dynamics kin_general_orientation_at target=401511ed.bf29a80e.bf408c4c.3.3f2f8f2a.3d22d663.3f26a76f.3ea561c2 wake=3ecccccc.3ecccccc.0 dirty=0',
        'dynamics kin_general_position_below target=40733333.c09e6666.400e6666.1.0.0.0.0 wake=3ecccccc.3ecccccc.0 dirty=10',
        'dynamics kin_general_pose_asleep target=3d81b4e5.402c5f93.3f6aaaac.3.be8ad5e3.3ee76479.3f46ff5f.3eafdbb8 wake=0.0.100 dirty=0',
        'dynamics kin_general_orientation_high target=40540da7.bf4e81b4.3f2aaaac.3.be8ad5e3.3ee76479.3f46ff5f.3eafdbb8 wake=3f000000.3f000000.0 dirty=0',
        'dynamics kin_general_position_or target=be4ccccc.bee66666.40e33333.3.be8ad5e3.3ee76479.3f46ff5f.3eafdbb8 wake=3f000000.3f000000.0 dirty=0',
        'dynamics kin_identity_frame created=1',
        'dynamics kin_identity_frame_pose_low target=bfb2848d.3fa7a26f.3e205b64.3.3e1ac3e0.3f6825d0.be9ac3e0.3e80f890 wake=3ecccccc.3ecccccc.0 dirty=10',
        'dynamics kin_identity_frame_orientation_at target=3fff440e.3e79ffc0.3f7df506.3.be4c8b64.3e7fae3d.3f661ccf.3e99688b wake=3ecccccc.3ecccccc.0 dirty=0',
        'dynamics kin_identity_frame_position_below target=40733333.c09e6666.400e6666.1.0.0.0.0 wake=3ecccccc.3ecccccc.0 dirty=10',
        'dynamics kin_identity_frame_pose_asleep target=bfb2848d.3fa7a26f.3e205b64.3.3e1ac3e0.3f6825d0.be9ac3e0.3e80f890 wake=0.0.100 dirty=0',
        'dynamics kin_identity_frame_orientation_high target=3fed7b73.c00c2ec8.bdbf4938.3.3e1ac3e0.3f6825d0.be9ac3e0.3e80f890 wake=3f000000.3f000000.0 dirty=0',
        'dynamics kin_identity_frame_position_or target=be4ccccc.bee66666.40e33333.3.3e1ac3e0.3f6825d0.be9ac3e0.3e80f890 wake=3f000000.3f000000.0 dirty=0',
        'dynamics kin_rotated_frame created=1',
        'dynamics kin_rotated_frame_pose_low target=bfc66666.404ccccd.3feccccd.3.be4d9285.3f741dfd.3e4d9285.bdcd9285 wake=3ecccccc.3ecccccc.0 dirty=10',
        'dynamics kin_rotated_frame_orientation_at target=40005816.c0040f6a.3fa7e9fb.3.3f3a85c2.3eb7dba0.3f1537d0.3caa88ff wake=3ecccccc.3ecccccc.0 dirty=0',
        'dynamics kin_rotated_frame_position_below target=40733333.c09e6666.400e6666.1.0.0.0.0 wake=3ecccccc.3ecccccc.0 dirty=10',
        'dynamics kin_rotated_frame_pose_asleep target=bfc66666.404ccccd.3feccccd.3.be4d9285.3f741dfd.3e4d9285.bdcd9285 wake=0.0.100 dirty=0',
        'dynamics kin_rotated_frame_orientation_high target=3fd9999a.be99999a.3fcccccd.3.be4d9285.3f741dfd.3e4d9285.bdcd9285 wake=3f000000.3f000000.0 dirty=0',
        'dynamics kin_rotated_frame_position_or target=be4ccccc.bee66666.40e33333.3.be4d9285.3f741dfd.3e4d9285.bdcd9285 wake=3f000000.3f000000.0 dirty=0',
        'dynamics created=1',
        'dynamics mass=40a00000.3e4ccccd.40a00000.100',
        'dynamics inertia=40000000.40400000.40800000.3f000000.3eaaaaab.3e800000',
        'dynamics damping=3e4ccccd.3e99999a.3e4ccccd.3e99999a',
        'dynamics velocity=3f800000.40000000.40400000.40800000.40a00000.40c00000',
        'dynamics record_velocity=3f800000.40000000.40400000.40800000.40a00000.40c00000',
        'dynamics kinematic=180.0.0.0.0.1',
        'dynamics restored=100.3e4ccccd.3f000000.3eaaaaab.3e800000.0',
        'dynamics transition_allocs=1.1',
        'dynamics kinematic_dirty=b0000.2.1',
        'dynamics move_position=f.0:40e00000.1:c1000000.2:41100000.3:1.180.0.0.0',
        'dynamics move_position_dirty=0.0.0',
        'dynamics move_orientation=cf.0:0.1:0.2:0.3:3.6:3f3504f3.7:3f3504f3.180.0.0.0',
        'dynamics move_pose=d7.0:c0000000.1:40400000.2:40800000.4:3f800000.6:0.7:0.180.0.0.0',
        'dynamics move_position_after_pose=7.0:c0a00000.1:40c00000.2:40e00000.180.0.0.0',
        'dynamics move_pose_dirty=0.0.0',
        'dynamics move_orientation_fresh=0.0.0.3.0.0.3f3504f3.3f3504f3',
        'dynamics move_nonkinematic=100.0.0.0.0',
        'dynamics density_created=1',
        'dynamics density_mass=42c00000.3c2aaaab.42c00000',
        'dynamics density_inertia=43d00000.43a00000.43200000.3b1d89d9.3b4ccccd.3bcccccd'
    )
    'NxPhysicsActorBodyFlagTests' = @(
        # NpActor.cpp completion Task 3 (000785/000787's 000712 island-root refresh): a
        # three-record chain compressed on each kinematic transition, the root's +0x1e4
        # ORed with 2 only when it has an island object.
        'body_flag island_initial self=1.1.1 island=0.0.0 bits=0.0.0',
        'body_flag raise_island parents=2.2 bits=20.12 flags=180',
        'body_flag clear_island parents=2.2 bits=20.12 flags=100',
        'body_flag raise_no_island parents=2.2 bits=20.10 flags=180',
        'body_flag clear_no_island parents=2.2 bits=20.10 flags=100',
        # NpActor.cpp completion Task 2 (contract H1): the dirty list grown from a null
        # list through 2, 6 and 14 entries by eight clean ids, then the kinematic
        # transition (000785) growing it before its 0x20-byte block. Oracle side.
        'actor grow_start=9.256.7',
        'actor grow_0=1.2.800.0.1.0.8.0.1',
        'actor grow_1=2.2.800.1.0.0.0.0.1',
        'actor grow_2=3.6.800.2.1.1.18.8.1',
        'actor grow_3=4.6.800.3.0.0.0.0.1',
        'actor grow_4=5.6.800.4.0.0.0.0.1',
        'actor grow_5=6.6.800.5.0.0.0.0.1',
        'actor grow_6=7.14.800.6.1.1.38.18.1',
        'actor grow_7=8.14.800.7.0.0.0.0.1',
        'actor grow_list=0.1.2.3.4.5.6.7',
        'actor grow_kinematic=2.8.20.1.2.b0000.180.0.0.0.0',
        'actor grow_dynamic=0.1.20.1.b0000.100.3caaaaab.3b9d89d9.3c4ccccd.0',
        # The lines registered before Task 2.
        'actor bodyflag_created=1',
        'actor bodyflag_manager=1.0.ffffffff',
        'actor bodyflag_locklinks=1.1.1',
        'actor bodyflag_descriptor=1.0.100',
        'actor bodyflag_manager_raised=ffffffff',
        'actor bodyflag_raised=1.1.101',
        'actor bodyflag_manager_cleared=ffffffff',
        'actor bodyflag_cleared=1.0.1',
        'actor bodyflag_mutation_allocs=0.0',
        'actor bodyflag_requeued=80000.2.0.1'
    )
    'NxPhysicsActorMetadataTests' = @(
        'actor metadata_created=1',
        'actor metadata_descriptor=7.1.2.7',
        'actor metadata_collision_default=0',
        'actor metadata_raised=9.1.1.3',
        'actor metadata_cleared=1.0.1',
        'actor metadata_mutation_allocs=0.0'
    )
    'NxPhysicsActorNameTests' = @(
        'actor name_created=1',
        'actor name_descriptor=1.actor-descriptor',
        'actor name_shape_initial=1',
        'actor name_first=1.actor-first',
        'actor name_second=1.actor-second',
        'actor name_shape_separate=1.1',
        'actor name_cleared=1.1',
        'actor name_mutation_allocs=0.0'
    )
    'NxPhysicsEmptySceneTests' = @(
        'actor empty_scene_allocs=10',
        'actor empty_scene_alloc_sizes=710.28.4.20.4.20.18.14.a8.8',
        'actor empty_scene_frees=9',
        'actor empty_scene_sizes=14.18.20.4.20.4.28.a8.710',
        'actor empty_sdk_frees=7',
        'actor empty_sdk_sizes=20.c.38.124.90.8.38'
    )
    'NxPhysicsDynamicFirstTests' = @(
        'actor dynamic_first created=1 allocs=34 frees=6',
        'actor dynamic_first sizes=50.18.228.800.400.800.400.800.400.400.400.1c.260.800.400.800.400.800.400.400.400.8.400.400.3c.1c.8.4.4.4.60.10.8.8',
        'actor dynamic_first free_sizes=800.800.800.800.800.800',
        'actor dynamic_first scene=256.1.1.0.1',
        'actor dynamic_first prune=1/4.1.1.256'
    )
    'NxPhysicsActorLifecycleTests' = @(
        # Static-shape auxiliary registration and slot reuse through group release.
        'actor static init_aux_prefix=50.18.228.800.400.800.400.800.400.400.400',
        'actor static init_allocs=24',
        'actor static init_sizes=50.18.228.800.400.800.400.800.400.400.400.1c.90.1c.8.4.4.4.60.10.8.400.400.8',
        'actor static init_frees=3',
        'actor static init_free_sizes=800.800.800',
        'actor static prune_initial=256.1.1.256.1.1/4.1.1.256.1',
        'actor static prune_released=1.0.1.0.2.256',
        'actor static aux_counts_static=256/256.1/256.256/256.0/256.256/256',
        'actor static aux_sample_static_0=ffffffff.0.0.0.0.0',
        'actor static aux_sample_static_10=0.0.0.0.0.0',
        'actor static aux_sample_static_20=0.d00beed0.d00beed0.d00beed0.d00beed0.d00beed0',
        'actor static aux_sample_static_30=0.0.0.0.0.0',
        'actor static aux_sample_static_90=1.0.0.0.0.0',
        'actor static aux_counts_dynamic=256/256.2/256.256/256.0/256.256/256',
        'actor static aux_sample_dynamic_0=ffffffff.ffffffff.0.0.0.0',
        'actor static aux_sample_dynamic_10=0.1.0.0.0.0',
        'actor static aux_sample_dynamic_20=0.1.d00beed0.d00beed0.d00beed0.d00beed0',
        'actor static aux_sample_dynamic_30=0.0.0.0.0.0',
        'actor static aux_sample_dynamic_90=1.1.0.0.0.0',
        'actor static aux_counts_multi=256/256.5/256.256/256.0/256.256/256',
        'actor static aux_sample_multi_0=ffffffff.ffffffff.ffffffff.ffffffff.ffffffff.0',
        'actor static aux_sample_multi_10=2.1.0.3.4.0',
        'actor static aux_sample_multi_20=2.1.0.3.4.d00beed0',
        'actor static aux_sample_multi_30=0.0.0.0.0.0',
        'actor static aux_sample_multi_90=1.1.1.1.1.0',
        'actor static aux_counts_multi_released=256/256.2/256.256/256.0/256.256/256',
        'actor static aux_sample_multi_released_0=0.ffffffff.ffffffff.0.0.0',
        'actor static aux_sample_multi_released_10=2.1.0.4.4.0',
        'actor static aux_sample_multi_released_20=d00beed0.1.0.d00beed0.d00beed0.d00beed0',
        'actor static aux_sample_multi_released_30=0.0.0.0.0.0',
        'actor static aux_sample_multi_released_90=0.1.1.0.0.0',
        'actor static aux_counts_nonlast=256/256.1/256.256/256.0/256.256/256',
        'actor static aux_sample_nonlast_0=0.0.ffffffff.0.0.0',
        'actor static aux_sample_nonlast_10=2.1.0.4.4.0',
        'actor static aux_sample_nonlast_20=d00beed0.d00beed0.0.d00beed0.d00beed0.d00beed0',
        'actor static aux_sample_nonlast_30=0.0.0.0.0.0',
        'actor static aux_sample_nonlast_90=0.0.1.0.0.0',
        'actor static aux_counts_reuse=256/256.2/256.256/256.0/256.256/256',
        'actor static aux_sample_reuse_0=0.ffffffff.ffffffff.0.0.0',
        'actor static aux_sample_reuse_10=2.1.0.4.4.0',
        'actor static aux_sample_reuse_20=d00beed0.1.0.d00beed0.d00beed0.d00beed0',
        'actor static aux_sample_reuse_30=0.0.0.0.0.0',
        'actor static aux_sample_reuse_90=0.1.1.0.0.0',
        'actor shape_kind_static=2',
        'actor shape_link_static=1.0',
        'actor shape_public_static=1.1.1',
        'actor shape_public_item_static_0=1.0.0',
        'actor shape_public_handle_static_0=1c.1.0.1',
        'actor shape_handle_links_static_0=1.0.0.0.1',
        'actor shape_methods_static_0=2.1.1',
        'actor shape_is_sphere_static_0=0',
        'actor shape_dimensions_alias_static_0=1',
        'actor shape_dimensions_static_0=3f800000.40000000.40400000',
        'actor shape_kind_dynamic=2',
        'actor shape_link_dynamic=1.0',
        'actor shape_public_dynamic=1.1.1',
        'actor shape_public_item_dynamic_0=1.0.0',
        'actor shape_public_handle_dynamic_0=1c.1.0.1',
        'actor shape_handle_links_dynamic_0=1.0.0.0.1',
        'actor shape_methods_dynamic_0=2.1.1',
        'actor shape_is_sphere_dynamic_0=0',
        'actor shape_dimensions_alias_dynamic_0=1',
        'actor shape_dimensions_dynamic_0=3f800000.40000000.40400000',
        'actor shape_kind_multi=5',
        'actor shape_link_multi=1.0',
        'actor shape_public_multi=2.1.1',
        'actor shape_public_item_multi_0=1.0.0',
        'actor shape_public_handle_multi_0=1c.1.0.1',
        'actor shape_handle_links_multi_0=1.0.0.0.1',
        'actor shape_methods_multi_0=2.1.1',
        'actor shape_is_sphere_multi_0=0',
        'actor shape_dimensions_alias_multi_0=1',
        'actor shape_dimensions_multi_0=3f800000.40000000.40400000',
        'actor shape_public_item_multi_1=1.0.0',
        'actor shape_public_handle_multi_1=1c.1.0.1',
        'actor shape_handle_links_multi_1=1.0.0.0.1',
        'actor shape_methods_multi_1=2.1.1',
        'actor shape_is_sphere_multi_1=0',
        'actor shape_dimensions_alias_multi_1=1',
        'actor shape_dimensions_multi_1=40000000.3f800000.3f800000',
        'actor multi aux_arrays_initial=0/0.0/0.0/0.0/0.0/0',
        'actor multi aux_arrays_static=0/0.0/0.0/0.0/0.0/0',
        'actor multi first_dynamic_allocs=17',
        'actor multi first_dynamic_sizes=50.18.228.1c.260.800.400.800.400.800.400.400.400.8.3c.60.10',
        'actor multi first_dynamic_frees=3',
        'actor multi first_dynamic_free_sizes=800.800.800',
        'actor multi aux_arrays_dynamic=256/256.1/256.256/256.0/256.256/256',
        'actor multi aux_indices_dynamic=ffffffff,0,0,0:0,0,0,0:0,d00beed0,d00beed0,d00beed0:0,0,0,0',
        'actor multi aux_arrays_rotated=256/256.2/256.256/256.0/256.256/256',
        'actor multi aux_arrays_quarter=256/256.3/256.256/256.0/256.256/256',
        'actor multi aux_indices_quarter=ffffffff,ffffffff,ffffffff,0:0,1,2,0:0,1,2,d00beed0:0,0,0,0',
        'actor multi aux_arrays_quarter_released=256/256.2/256.256/256.0/256.256/256',
        'actor multi aux_indices_quarter_released=ffffffff,ffffffff,0,0:0,1,2,0:0,1,d00beed0,d00beed0:0,0,0,0',
        'actor multi aux_arrays_multi=256/256.3/256.256/256.0/256.256/256',
        'actor static created=1',
        'actor static dynamic=0',
        'actor static body=1 nested=0',
        'actor static body_alloc=50',
        'actor static position=40000000.bf800000.40800000',
        'actor static orientation=3f800000.00000000.00000000.00000000.3f800000.00000000.00000000.00000000.3f800000',
        'actor static public_quaternion=00000000.00000000.00000000.3f800000',
        'actor static pose=3f800000.00000000.00000000.00000000.3f800000.00000000.00000000.00000000.3f800000.40000000.bf800000.40800000',
        'actor saved_static pose=3f800000.00000000.00000000.00000000.3f800000.00000000.00000000.00000000.3f800000.40000000.bf800000.40800000',
        'actor saved_static saved_metadata=00000000.00000000.0000.1.1.1',
        'actor saved_static body_saved=0.8a7f8e1d.a5a5a5a5.a5a5a5a5.a5a5a5a5',
        'actor static local_point_velocity=00000000.00000000.00000000',
        'actor static world_point_velocity=00000000.00000000.00000000',
        'actor dynamic created=1',
        'actor dynamic dynamic=1',
        'actor dynamic body=1 nested=1',
        'actor dynamic body_alloc=50',
        'actor dynamic nested_alloc=260',
        'actor dynamic pose_alloc=50 cached=1 cached_nested=1',
        'actor dynamic position=c0400000.40000000.3f800000',
        'actor dynamic pose=3f800000.00000000.00000000.00000000.3f800000.00000000.00000000.00000000.3f800000.c0400000.40000000.3f800000',
        'actor saved_dynamic pose=3f800000.00000000.00000000.00000000.3f800000.00000000.00000000.00000000.3f800000.c0400000.40000000.3f800000',
        'actor saved_dynamic saved_metadata=3f800000.00000000.0000.1.1.1',
        'actor saved_dynamic body_saved=1.7bb36a35.42400000.00000100.00000004',
        'actor dynamic local_point_velocity=be800000.be000000.00000000',
        'actor dynamic world_point_velocity=40a80000.40ac0000.c0b00000',
        'actor local_velocity_grid_0=3eca57a8.becc49ba.3ed32617',
        'actor local_velocity_grid_1=3fc45a1d.bf69ad43.bf39930d',
        'actor local_velocity_grid_2=3f25475b.bfdf9e06.bf481d7e',
        'actor local_velocity_grid_3=40295104.be26b50c.3fb1888f',
        'actor local_velocity_grid_4=4099cb92.c001844c.c08ed013',
        'actor local_velocity_grid_5=40f0346e.3fa5bc01.3f11d14c',
        'actor local_velocity_grid_6=4066dd84.c03fc116.c051fb69',
        'actor local_velocity_grid_7=3e80c350.3ff69057.41619003',
        'actor local_velocity_grid_8=409c3ee2.bfed7f4f.be366fe0',
        'actor local_velocity_grid_9=41b1dded.40b1a24a.c0c3d0e0',
        'actor local_velocity_grid_10=4141e876.c0890a82.c165d108',
        'actor local_velocity_grid_11=404199c0.4002f076.4179b4ca',
        'actor local_velocity_grid_12=41a4c58e.c0c0c0fe.c1ea277c',
        'actor local_velocity_grid_13=40103634.c109af95.c0df5d4d',
        'actor local_velocity_grid_14=c10c7a7f.c1a3b3be.c19318cc',
        'actor local_velocity_grid_15=40184122.414d4dda.42427592',
        'actor world_velocity_grid_0=bf9089a0.bfd27bb2.bf6617c0',
        'actor world_velocity_grid_1=bfc3a29c.c01cb296.bf8ee633',
        'actor world_velocity_grid_2=c000e560.c0520c4a.bfa1a370',
        'actor world_velocity_grid_3=c0258106.c084a57a.bfab4395',
        'actor world_velocity_grid_4=c0822f83.c0a6e219.bf39c77a',
        'actor world_velocity_grid_5=c0a36ae6.c0c6e631.bed4d6a7',
        'actor world_velocity_grid_6=c0c8d288.c0e848e9.3bdb868c',
        'actor world_velocity_grid_7=c0f26666.c105851f.3f0a9fc2',
        'actor world_velocity_grid_8=c0d036fe.c124dede.c08a9fb2',
        'actor world_velocity_grid_9=c0f39400.c13a5aae.c0971f7b',
        'actor world_velocity_grid_10=c10cf5d6.c150caae.c0a38179',
        'actor world_velocity_grid_11=c1219f00.c1682ede.c0afc5a8',
        'actor world_velocity_grid_12=c142ef66.c134a978.40dadafe',
        'actor world_velocity_grid_13=c15a87fc.c1421683.4105af21',
        'actor world_velocity_grid_14=c17381df.c14f99f8.412017ec',
        'actor world_velocity_grid_15=c186ee88.c15d33d8.413ca7de',
        'actor rotated created=1',
        'actor rotated quaternion=00000000.00000000.3f800000.00000000',
        'actor rotated orientation=bf800000.00000000.00000000.00000000.bf800000.00000000.00000000.00000000.3f800000',
        'actor rotated public_quaternion=00000000.00000000.3f800000.00000000',
        'actor rotated pose=bf800000.00000000.00000000.00000000.bf800000.00000000.00000000.00000000.3f800000.40a00000.c0000000.40400000',
        'actor saved_rotated pose=bf800000.00000000.00000000.00000000.bf800000.00000000.00000000.00000000.3f800000.40a00000.c0000000.40400000',
        'actor saved_rotated saved_metadata=3f800000.00000000.0000.1.1.1',
        'actor saved_rotated body_saved=1.7bb36a35.42400000.00000100.00000004',
        'actor rotated local_point_velocity=40b80000.c0480000.40c00000',
        'actor rotated world_point_velocity=c0980000.c0f40000.41080000',
        'actor quarter created=1',
        'actor quarter quaternion=00000000.00000000.3f3504f3.3f3504f3',
        'actor quarter orientation=331302ae.bf7fffff.00000000.3f7fffff.331302ae.00000000.00000000.00000000.3f800000',
        'actor quarter public_quaternion=00000000.00000000.3f3504f3.3f3504f3',
        'actor quarter pose=331302ae.bf7fffff.00000000.3f7fffff.331302ae.00000000.00000000.00000000.3f800000.40a00000.c0000000.40400000',
        'actor saved_quarter pose=331302ae.bf7fffff.00000000.3f7fffff.331302ae.00000000.00000000.00000000.3f800000.40a00000.c0000000.40400000',
        'actor saved_quarter saved_metadata=3f800000.00000000.0000.1.1.1',
        'actor saved_quarter body_saved=1.7bb36a35.42400000.00000100.00000004',
        'actor quarter local_point_velocity=3fa00000.c0940000.40cfffff',
        'actor quarter world_point_velocity=c0980000.c0f40000.41080000',
        'actor static actor_alloc=18',
        'actor static linked_alloc=4',
        'actor static linked_child_alloc=20',
        'actor static body_actor=1 body_shape=1',
        'actor static body_shape_alloc=228',
        'actor static shape_helper_offset=9c',
        'actor dynamic actor_alloc=18',
        'actor dynamic pose_is_body=1',
        'scene objects_before_quarter=2/2',
        'scene objects_after_quarter=3/6',
        'actor quarter creation_allocs=6',
        'actor quarter creation_sizes=50.18.228.1c.260.18',
        'actor quarter creation_roles=2.1.0.0.4.0',
        'actor quarter retained_refs=ffff.ffff.ffff.56c',
        'actor links_alias=1.1.1',
        'actor link_scene_mask=010',
        'actor slot_scene_mask=008 slot_small=0',
        'scene actors_before=4',
        'scene actors_before_list=1',
        'scene release_frees=5',
        'scene release_sizes=18.260.1c.228.50',
        'scene release_creation_mask=3e',
        'scene actors_after=3',
        'scene quarter_still_listed=0',
        'scene static_release_frees=4',
        'scene static_release_sizes=18.1c.228.50',
        'scene actors_after_static=2',
        'scene static_still_listed=0',
        'actor multi broadphase_before=3c.2/4.1.1',
        'actor multi creation_allocs=12',
        'actor multi broadphase_after=3c.5/8.1.1',
        'actor multi creation_frees=2',
        'actor multi creation_free_sizes=60.10',
        'actor multi creation_sizes=50.18.110.228.1c.8.8.228.1c.260.c0.20',
        'actor multi broadphase_ref_0=0.0.1.0.0',
        'actor multi broadphase_ref_1=0.0.0.1.0',
        'actor multi broadphase_ref_2=1.0.0.0.0',
        'actor multi broadphase_ref_3=0.1.0.0.0',
        'actor multi broadphase_ref_4=0.0.0.0.1',
        'actor multi group_size=110',
        'actor multi group_index=0.ffff',
        'actor multi child_index_0=3.0',
        'actor multi child_index_1=4.0',
        'actor multi group_owner=1 owner_is_body=1 owner_is_actor=0',
        'actor multi body_scene_public=0 body_scene_internal=1',
        'actor multi scene_manager=1',
        'actor multi manager_guarded_size=a8',
        'actor multi group_array_e0=2/2',
        'actor multi group_array_f0=2/2',
        'actor multi scene_array6e8_before_release=0/2',
        'actor multi scene_array6e8_values_before_release=',
        'actor multi scene_array6e8_after_release=3/6',
        'actor multi scene_array6e8_values_after_release=3.4.0',
        'actor multi broadphase_released=3c.2/8.1.1',
        'actor multi release_allocs=1',
        'actor multi release_frees=11',
        'actor multi release_sizes=18.260.1c.228.1c.228.8.8.8.110.50',
        'actor multi aux_arrays_before_nonlast=256/256.2/256.256/256.0/256.256/256',
        'actor multi aux_indices_before_nonlast=ffffffff,ffffffff,0,0:0,1,2,0:0,1,d00beed0,d00beed0:0,0,0,0',
        'actor multi broadphase_before_nonlast=3c.2/8.1.1',
        'actor multi scene_array6e8_before_nonlast=3/6',
        'actor multi scene_array6e8_values_before_nonlast=3.4.0',
        'actor nonlast actor_ids_before=2/2.3.0',
        'actor nonlast body_c=1',
        'actor multi aux_arrays_after_nonlast=256/256.1/256.256/256.0/256.256/256',
        'actor multi aux_indices_after_nonlast=0,ffffffff,0,0:1,1,2,0:d00beed0,0,d00beed0,d00beed0:0,0,0,0',
        'actor multi broadphase_after_nonlast=3c.1/8.1.1',
        'actor multi scene_array6e8_after_nonlast=4/6',
        'actor multi scene_array6e8_values_after_nonlast=3.4.0.1',
        'actor nonlast actor_ids_after=3/6.3.0.1',
        'actor nonlast release_allocs=1',
        'actor nonlast release_alloc_sizes=18',
        'actor nonlast release_frees=6',
        'actor nonlast release_sizes=18.260.8.1c.228.50',
        'actor nonlast actors=1',
        'actor reuse created=1',
        'actor reuse creation_allocs=5',
        'actor reuse creation_sizes=50.18.228.1c.260',
        'actor multi aux_arrays_reuse=256/256.2/256.256/256.0/256.256/256',
        'actor multi aux_indices_reuse=ffffffff,ffffffff,0,0:1,0,2,0:1,0,d00beed0,d00beed0:0,0,0,0',
        'actor nonlast actor_ids_reuse=2/6.3.0',
        'actor multi scene_array6e8_reuse=3/6',
        'actor multi scene_array6e8_values_reuse=3.4.0',
        'actor multi broadphase_reuse=3c.2/8.1.1',
        'actor reuse body_c=1',
        'actor multi shape_index_reuse=1.0',
        'actor box defaults=0.0.1.0',
        'actor box mutated=5.1.20.10005',
        'actor box flags_default=8.0.8',
        'actor box flags_changed=0.20.20',
        'actor box flags_cleared=0.0.0',
        'actor box name_default=1',
        'actor box name_first=1.box-one',
        'actor box name_second=1.box-two',
        'actor box name_cleared=1',
        'actor box name_allocs=2.2',
        'actor box name_alloc_sizes=10.10',
        'actor box name_free_sizes=10.10',
        'actor box descriptor_created=1',
        'actor box descriptor=7.2.80.20007',
        'actor box descriptor_flags=0.20.20',
        'actor box descriptor_name=1.box-descriptor',
        'actor box descriptor_allocs=7',
        'actor box descriptor_alloc_sizes=50.18.228.1c.10.10.260',
        'actor box descriptor_release_frees=5',
        'actor box descriptor_release_sizes=18.260.1c.228.50',
        'actor scene_live_actor_frees=2.2',
        'actor scene_aux_arrays_freed=12.12',
        'actor record_id_array_before_scene=1/2.1.0',
        'actor old_record_id_array_freed=1.1',
        'actor scene_pruner_blocks_freed=9.9',
        'actor tracked_shapes_static=1/2.1',
        'actor tracked_shapes_dynamic=2/2.2',
        'actor tracked_shapes_rotated=3/4.3',
        'actor tracked_shapes_quarter=4/4.4',
        'actor tracked_shapes_quarter_released=3/4.3',
        'actor tracked_shapes_static_released=2/4.2',
        'actor tracked_shapes_multi=3/4.3',
        'actor tracked_shapes_multi_released=2/4.2',
        'actor scene_teardown_allocs=2',
        'actor scene_teardown_alloc_sizes=30.18',
        'actor scene_teardown_frees=45',
        'actor scene_teardown_sizes=14.18.20.4.20.4.28.18.260.1c.10.228.50.18.8.260.1c.228.50.400.400.400.400.400.400.400.400.400.400.400.400.a8.18.18.18.10.60.10.90.c0.20.3c.18.18.710',
        'actor sdk_teardown_frees=14',
        'actor sdk_teardown_sizes=20.c.30.10.38.124.4.4.4.8.1c.90.8.38'
    )
    # The Phase 6 joint-descriptor differential. Two cases over the two exported
    # rows, printing the whole descriptor surface before and after each call. The
    # degenerate zero and NaN axes are quarantined in the harness with the reason
    # recorded there (evidence/phase6-joints.md 7p); these three lines pin the
    # finite path, which is exact.
    # The Phase 6 joint differential. Four revolute cases: build the descriptor, create
    # the joint, read every value back, release. The transcript is identical to the
    # pinned oracle's after the runner normalises the pair-identity lines (diff 0), so
    # these four lines pin the whole surface: the two actors surviving the round trip,
    # the anchor and axis the descriptor carried, the state, and the release.
    # The staged-pair joint differential: the same harness built as its own target so
    # the closure schema has a target that loads the rebuilt module. Four assertions,
    # quoted from the transcript.
    # Joint-families Task 3a added two prismatic cases (NxJointType 0) to the same
    # harness and four prismatic lines to each list, copied verbatim from the
    # oracle side of a staged-pair run (and the oracle differential): created, the
    # anchor/axis/state, the type and is-queries, and one saveToDesc line.
    # Joint-families Task 3b added two cylindrical cases (NxJointType 2) and four
    # cylindrical lines to each list, copied the same way: created, the
    # anchor/axis/state, the type and is-queries, and one saveToDesc line (the
    # saved local normals, which come from NxNormalToTangents).
    # Joint-families Task 3c added two spherical cases (NxJointType 3) and four
    # spherical lines to each list, copied the same way: created, the
    # anchor/axis/state, getFlags/getProjectionMode (internal slots 12 and 14
    # through the Np bodies the spherical table shares with revolute), and one
    # saveToDesc line (the twist and swing limits).
    # Joint-families Task 3d added two point-on-line cases (NxJointType 4) and
    # four point-on-line lines to each list, copied the same way: created, the
    # anchor/axis/state, the type and is-queries, and one saveToDesc line.
    # Joint-families Task 3e added two point-in-plane cases (NxJointType 5) and
    # four point-in-plane lines to each list, copied the same way.
    # Joint-families Task 3f added two distance cases (NxJointType 6) and four
    # distance lines to each list, copied the same way: created, the
    # anchor/axis/state, the type and is-queries, and the saveToDesc line with
    # the family fields (maxDistance, minDistance, spring, flags).
    # Joint-families Task 3g added two pulley cases (NxJointType 7) and four
    # pulley lines to each list, copied the same way: created, the
    # anchor/axis/state, the type and is-queries, and the saveToDesc line with
    # the family fields (distance, stiffness, ratio, flags).
    # Joint-families Task 3h added two fixed cases (NxJointType 8) and four fixed
    # lines to each list, copied the same way: created, the anchor/axis/state, the
    # type and is-queries, and the saveToDesc line with the saved local anchors
    # (NxFixedJointDesc has no field of its own).
    # Joint-families Task 3i added two D6 cases (NxJointType 9) and four D6 lines
    # to each list, copied the same way from the oracle side of the staged-pair
    # run: created, the anchor/axis/state, the type and is-queries, and the saved
    # motions (the oracle's D6 saveToDesc writes the base part only, so the saved
    # family fields are the case's sentinel values).
    # Joint-open-items Task 2 wired NxScene::releaseJoint to the Scene's joint
    # rows and added what the scene reports about its joints: every family case
    # prints getNbJoints and one resetJointIterator/getNextJoint pass (count,
    # joints yielded, their types in order, whether the case's joint is among
    # them, and the read after the end) before and after its release, and a
    # release-then-create cycle (revolute, spherical, fixed; release the
    # middle; create D6; release the head and the tail; create prismatic)
    # prints the same after each step and leaves two joints for the scene
    # release. Twenty-seven lines were added to each list, copied verbatim from
    # the oracle side of a staged-pair run: the index-0 before/after lines of the
    # ten families, the six cycle states and the cycle's closing line.
    # Joint-open-items Task 4 added near-z axes over the identity fixture
    # (indices 4 and 5: (0.1, 0.2, 0.97) normalised and (0, 0, 1)) and a
    # rotated-body fixture in a scene of its own (actor a turned 90 degrees
    # about y, actor b at the unit quaternion (1, 2, 3, 4) / sqrt(30);
    # indices 10-13), every family over each, with the revolute case now
    # printing its saved local frames too, and the Task 4 cases also printing
    # the rotated actors' body-record words and each internal joint's
    # orientation-dependent words (through the public object's +0x18).
    # Thirty-six lines were added to each list, copied verbatim from the
    # oracle side of a staged-pair run: the two actors' read-back poses (the
    # body record's +0x5c quaternion from phys_fn_000801's matrix
    # conversion), three near-z lines, every family's index-10 created line
    # (the candidate's NxJointDesc_SetGlobalAxis failed isValid there before
    # this task), saved local frames and read-back anchors/axes over the
    # rotated bodies, the record's +0x124/+0x134/+0x164 words (phys_fn_000768
    # and 000746), the prismatic and fixed relative rotations (004378,
    # 004244) and one revolute frame-quaternion block (004101).
    # The Task 4 review added two posed fixtures (180 degrees about x and y;
    # general rotations with largest diagonal x and y; indices 20-23) so the
    # creation conversions (000801, 000768) take their pivot arms, and
    # fifteen more oracle-side lines to each list.
    'NxPhysicsJointStagedPairTests' = @(
        'case=revolute index=0 created=yes',
        'case=revolute index=0 out_anchor=00000000.00000000.00000000 out_axis=3f800000.00000000.00000000 state=0',
        'case=revolute index=0 actors a=match b=match',
        'case=revolute index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=prismatic index=0 created=yes',
        'case=prismatic index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=prismatic index=3 type=0 is_prismatic=yes is_revolute=no',
        'case=prismatic index=3 saved anchor0=40000000.40800000.00000000 anchor1=c0000000.40800000.00000000',
        'case=cylindrical index=0 created=yes',
        'case=cylindrical index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=cylindrical index=3 type=2 is_cylindrical=yes is_prismatic=no',
        'case=cylindrical index=3 saved normal0=bed105ec.bed105ec.3f5105ec normal1=bed105ec.bed105ec.3f5105ec',
        'case=spherical index=0 created=yes',
        'case=spherical index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=spherical index=3 flags=00000009 projection_mode=1',
        'case=spherical index=3 saved twist_limit=bf000000.3e800000.3f800000.3f400000.00000000.3f000000 swing_limit=3f200000.3f000000.3f400000',
        'case=point_on_line index=0 created=yes',
        'case=point_on_line index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=point_on_line index=3 type=4 is_point_on_line=yes is_revolute=no',
        'case=point_on_line index=3 saved anchor0=40000000.40800000.00000000 anchor1=c0000000.40800000.00000000',
        'case=point_in_plane index=0 created=yes',
        'case=point_in_plane index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=point_in_plane index=3 type=5 is_point_in_plane=yes is_revolute=no',
        'case=point_in_plane index=3 saved anchor0=40000000.40800000.00000000 anchor1=c0000000.40800000.00000000',
        'case=distance index=0 created=yes',
        'case=distance index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=distance index=3 type=6 is_distance=yes is_revolute=no',
        'case=distance index=0 saved max_distance=40200000 min_distance=3f000000 spring=41200000.3f000000.3e800000 flags=00000007',
        'case=pulley index=0 created=yes',
        'case=pulley index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=pulley index=3 type=7 is_pulley=yes is_distance=no',
        'case=pulley index=0 saved distance=40c00000 stiffness=3f400000 ratio=3fc00000 flags=00000001',
        'case=fixed index=0 created=yes',
        'case=fixed index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=fixed index=3 type=8 is_fixed=yes is_pulley=no',
        'case=fixed index=3 saved anchor0=40000000.40800000.00000000 anchor1=c0000000.40800000.00000000',
        'case=d6 index=0 created=yes',
        'case=d6 index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=d6 index=3 type=9 is_d6=yes is_fixed=no',
        'case=d6 index=0 saved motions=2.0.2.0.2.0',
        'case=revolute index=0 scene_joints when=before_release count=1 enumerated=1 order=1 self=yes end=null',
        'case=revolute index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=prismatic index=0 scene_joints when=before_release count=1 enumerated=1 order=0 self=yes end=null',
        'case=prismatic index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=cylindrical index=0 scene_joints when=before_release count=1 enumerated=1 order=2 self=yes end=null',
        'case=cylindrical index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=spherical index=0 scene_joints when=before_release count=1 enumerated=1 order=3 self=yes end=null',
        'case=spherical index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=point_on_line index=0 scene_joints when=before_release count=1 enumerated=1 order=4 self=yes end=null',
        'case=point_on_line index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=point_in_plane index=0 scene_joints when=before_release count=1 enumerated=1 order=5 self=yes end=null',
        'case=point_in_plane index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=distance index=0 scene_joints when=before_release count=1 enumerated=1 order=6 self=yes end=null',
        'case=distance index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=pulley index=0 scene_joints when=before_release count=1 enumerated=1 order=7 self=yes end=null',
        'case=pulley index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=fixed index=0 scene_joints when=before_release count=1 enumerated=1 order=8 self=yes end=null',
        'case=fixed index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=d6 index=0 scene_joints when=before_release count=1 enumerated=1 order=9 self=yes end=null',
        'case=d6 index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=cycle index=1 scene_joints when=three_created count=3 enumerated=3 order=8.3.1 self=yes end=null',
        'case=cycle index=2 scene_joints when=middle_released count=2 enumerated=2 order=8.1 self=no end=null',
        'case=cycle index=3 scene_joints when=d6_created count=3 enumerated=3 order=9.8.1 self=yes end=null',
        'case=cycle index=4 scene_joints when=head_released count=2 enumerated=2 order=8.1 self=no end=null',
        'case=cycle index=5 scene_joints when=tail_released count=1 enumerated=1 order=8 self=no end=null',
        'case=cycle index=6 scene_joints when=prismatic_created count=2 enumerated=2 order=0.8 self=yes end=null',
        'rotated_fixture actor=a t=00000000.3f800000.00000000 quat=00000000.3f3504f3.00000000.3f3504f3',
        'rotated_fixture actor=a row0=33800000.00000000.3f7fffff row1=00000000.3f800000.00000000 row2=bf7fffff.00000000.33800000',
        'rotated_fixture actor=b t=40800000.bf800000.40000000 quat=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'rotated_fixture actor=b row0=3e088889.bf2aaaab.3f3bbbbb row1=3f6eeeef.3eaaaaaa.3e088889 row2=beaaaaaa.3f2aaaaa.3f2aaaac',
        'case=revolute index=4 out_anchor=3f800000.c0000000.3f000000 out_axis=3dcdbcfe.3e4dbcfe.3f797527 state=0',
        'case=cylindrical index=4 saved normal0=3f7eb479.bca62f73.bdc97fed normal1=3f7eb479.bca62f73.bdc97fed',
        'case=d6 index=5 saved normal0=3f800000.00000000.00000000 normal1=3f800000.00000000.00000000',
        'case=revolute index=10 created=yes',
        'case=prismatic index=10 created=yes',
        'case=cylindrical index=10 created=yes',
        'case=spherical index=10 created=yes',
        'case=point_on_line index=10 created=yes',
        'case=point_in_plane index=10 created=yes',
        'case=distance index=10 created=yes',
        'case=pulley index=10 created=yes',
        'case=fixed index=10 created=yes',
        'case=d6 index=10 created=yes',
        'case=revolute index=10 saved anchor0=c03fffff.3f800000.3f800001 anchor1=40044444.406aaaaa.bf91110f',
        'case=revolute index=11 saved axis0=bf13cd39.3f13cd3a.3f13cd3a axis1=3ed8c69a.3e4511a0.3f62a115',
        'case=revolute index=12 saved normal0=3dc97ff4.bca62f73.3f7eb478 normal1=3e1609a3.bf3c5379.3f294caf',
        'case=revolute index=13 saved anchor0=00000000.bf800000.00000000 anchor1=3f888888.3fd55556.c0844444',
        'case=revolute index=12 out_anchor=bfbffffc.3e7ffffc.40ffffff out_axis=3dcdbcfc.3e4dbcfe.3f797525 state=0',
        'case=d6 index=12 out_anchor=bfbffffc.3e7ffffc.40ffffff out_axis=3dcdbcfc.3e4dbcfe.3f797525 state=0',
        'case=fixed index=13 out_anchor=33c88888.b31dddde.b4000000 out_axis=00000000.00000000.3f7ffffe state=0',
        'case=d6 index=12 saved anchor0=c0ffffff.bf400000.bfbffffb anchor1=bfc88888.41015555.3e0888b9',
        'case=pulley index=11 saved normal0=bf5105ec.bed105ec.bed105ea normal1=bf352744.3f2e2f9a.3e43169d',
        'case=spherical index=13 saved normal0=33800000.00000000.3f7fffff normal1=3e088889.bf2aaaab.3f3bbbbb',
        'case=point_in_plane index=12 saved axis0=bf797526.3e4dbcfe.3dcdbd05 axis1=bdfdbe69.3f264e19.3f4005bb',
        'rotated_fixture actor=a record off=124 words=00000000.3f3504f2.00000000.3f3504f4',
        'rotated_fixture actor=a record off=164 words=3e3ffffe.00000000.a5800000.00000000.3e400000.00000000.a5800000.00000000.3e3fffff',
        'rotated_fixture actor=b record off=124 words=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'rotated_fixture actor=b record off=134 words=3e088889.bf2aaaab.3f3bbbbb.3f6eeeef.3eaaaaaa.3e088889.beaaaaaa.3f2aaaaa.3f2aaaac',
        'rotated_fixture actor=b record off=164 words=3e3fffff.31599999.32099999.31599999.3e400000.314cccce.32099999.314cccce.3e400001',
        'case=prismatic index=10 internal off=16c words=3e8432a4.3e8432a3.bf0432a5.3f464bf7',
        'case=fixed index=10 internal off=16c words=bffffffd.c0000000.40800000.3e8432a4.3e8432a3.bf0432a5.3f464bf7',
        'case=revolute index=12 internal off=0ac words=bddb3f43.bf32eec6.bf34c7db.bd14ac04.bf22b49e.bea0571e.bf2e5b9a.be3d1a85',
        'rotated_fixture actor=flip_xy_a record off=05c words=3f800000.00000000.00000000.00000000',
        'rotated_fixture actor=flip_xy_b record off=05c words=00000000.3f800000.00000000.00000000',
        'rotated_fixture actor=near_xy_a record off=124 words=3f741dfd.3e4d9285.3dcd9285.3e4d9285',
        'rotated_fixture actor=near_xy_a record off=134 words=3f6634e0.3eaf653c.3e8b48dd.3ed8aa3b.bf56bafb.beaf653c.3de2fb7b.3ed8aa3b.bf6634dc',
        'rotated_fixture actor=near_xy_b record off=124 words=3e1ac3e0.3f6825d0.be9ac3e0.3e80f890',
        'rotated_fixture actor=near_xy_b record off=134 words=bf53d13e.3eda5094.3ebb207f.3df980a9.3f4585d8.bf1fd66c.bf0c585f.bef1b4a4.bf30bb26',
        'case=revolute index=20 created=yes',
        'case=d6 index=20 created=yes',
        'case=revolute index=20 saved anchor0=3f800000.bf800000.c0400000 anchor1=40400000.40400000.bf800000',
        'case=spherical index=21 saved normal0=3f7eb479.3ca62f73.3dc97fed normal1=bf7eb479.bca62f73.3dc97fed',
        'case=revolute index=22 saved anchor0=3fd3d426.3f45f6fc.c0312b31 anchor1=40132ae7.3f103e58.c06a3bca',
        'case=revolute index=23 saved normal0=3f600d83.3ea1e72b.3ebb7000 normal1=bf4591f6.3ee8fa38.3ee37124',
        'case=fixed index=20 internal off=16c words=40800000.40000000.c0000000.80000000.80000000.3f800000.00000000',
        'case=fixed index=22 internal off=16c words=403e3a09.40793aaa.bcce5840.3d6e3390.3e300fdb.3f6b9cc1.3eb15b45',
        'case=prismatic index=23 internal off=16c words=3d6e3390.3e300fdb.3f6b9cc1.3eb15b45',
        'case=cycle left_for_scene_release=2'
    )
    # NxNormalToTangents, which NxJointDesc::setGlobalAxis calls. The digest folds
    # 240000 inputs' output words from the pinned NxFoundation.dll, so it moves if
    # the generator or its seed changes or the oracle is not called; the case lines
    # pin one word the reconstruction got wrong in each way it was wrong before:
    # z_nonunit (the z arm's t1, from normalisation order), xy_nonunit_neg (the
    # xy arm's t2.x), z_large (the z arm's float spill of y*y + z*z, which
    # overflows to give k == 0) and xy_nan_payloads (x87 NaN propagation).
    # Joint-open-items Task 5 follow-up: a fresh process creates the Foundation
    # with allocator A, then the Physics SDK with allocator B, and creates and
    # releases a revolute and a distance joint. The oracle's joint rows (the
    # family constructors, 000665's internal allocation, 000661's pointer-array
    # grow, the deleting destructors) use the Foundation's allocator, so every
    # joint block lands in A and none in B. Copied verbatim from the ORACLE side;
    # before the follow-up the candidate put all of them in B.
    'NxPhysicsJointAllocatorTests' = @(
        'case=allocator family=revolute created=yes',
        'case=allocator family=revolute window=create allocator=foundation mallocs=3 frees=0 reallocs=0 sizes=204,1c,8',
        'case=allocator family=revolute window=create allocator=physics mallocs=0 frees=0 reallocs=0 sizes=none',
        'case=allocator family=revolute released=yes',
        'case=allocator family=revolute window=release allocator=foundation mallocs=0 frees=2 reallocs=0 sizes=none',
        'case=allocator family=revolute window=release allocator=physics mallocs=0 frees=0 reallocs=0 sizes=none',
        'case=allocator family=distance created=yes',
        'case=allocator family=distance window=create allocator=foundation mallocs=2 frees=0 reallocs=0 sizes=184,1c',
        'case=allocator family=distance window=create allocator=physics mallocs=0 frees=0 reallocs=0 sizes=none',
        'case=allocator family=distance released=yes',
        'case=allocator family=distance window=release allocator=foundation mallocs=0 frees=2 reallocs=0 sizes=none',
        'case=allocator family=distance window=release allocator=physics mallocs=0 frees=0 reallocs=0 sizes=none'
    )
    # Task 6 review adds 12: the control word read back inside two step
    # windows per mode, and D6JointDump.txt read back after the pair is
    # unloaded and flushed (0x0f7f blocks included, now that NxPhysics links
    # legacy_stdio_float_rounding.obj; FLT_MAX printed as bits, see the test).
    # Joint-open-items Task 6: the internal-slot differential. Each family's
    # internal joint (public +0x18) has its visualization (4), emulated step
    # (1, 7, 6), impulse (0) and projection (8) slots called through its own
    # table by index, under 0x027f and again under 0x0f7f; revolute's break
    # test (2) last; then NxFindRotationMatrix over both of its arms. Per case:
    # creation, the renderer call count and first call, the step's record
    # count, record 0 under each control word, the joint words the 0x0f7f step
    # changed, the first 0x0f7f impulse and projection changes, release.
    # Copied verbatim from the ORACLE side. Before this task the candidate
    # differed in every projection that moved a body (000754 was a stub) and
    # in revolute's second projection and the rotation cases (the Foundation's
    # NxFindRotationMatrix rounded where the oracle's does not).
    'NxPhysicsJointSlotTests' = @(
        'slots family=revolute config=0 created=yes',
        'slots family=revolute config=0 call=vis cw=027f calls=24',
        'slots family=revolute config=0 call=vis cw=027f vis n=0 arrow p=3fd067e6.3f34c3b4.bee0427e d=3eb851ec.3ef5c28f.3f4ccccd length=3f800000 scale=3f900000 color=00ffffff',
        'slots family=revolute config=0 call=step cw=027f records count=4 capacity=4 window=00000000.00000004',
        'slots family=revolute config=0 call=step cw=027f record=0 words=3eb851ec.3ef5c28f.3f4ccccd.cdc80403.SUPPORT0.SUPPORT1.cdcdcdcd.cdcdcdcd.cdcdcdcd.cdcdcdcd.cdcdcdcd.cdcdcdcd.JOINT.c01695f9.00000000.3c2b8c05.3b1075ea.00000000.7f7fffff.00000000',
        'slots family=revolute config=0 call=step cw=0f7f record=0 words=3ebb91ad.3f0e0ae6.3f3f37d6.cdc80403.SUPPORT0.SUPPORT1.cdcdcdcd.cdcdcdcd.cdcdcdcd.cdcdcdcd.cdcdcdcd.cdcdcdcd.JOINT.c01e89d4.00000000.3c2b8bd9.3b1075c4.00000000.7f7fffff.00000000',
        'slots family=revolute config=0 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000004 1ac=c144deaf>c00423f6 1b0=4086d390>3f34fd97 1b4=c0be710b>bf7fa64f 1b8=402b353b>402ab44f 1bc=bece90ed>bed16932 1c0=bf401ac2>bf421e7a',
        'slots family=revolute config=0 call=impulse cw=0f7f support0 changed 000=40a7ecfc>3f897fb9 004=bf20c560>3e82d190 008=bf41b8d2>3d627b60 010=be2d04e1>3e8c9888 014=bffe92d5>be49d309 018=bfcd25ee>bf8981e7',
        'slots family=revolute config=0 call=project0 cw=0f7f body0 changed 018=3e60ced3>3e60cecb 01c=3f6f3a70>3f6f3ad5 020=3df12294>3df12098 024=3d6894d1>3d6892dc 028=3e5951be>3e59516c 02c=bdcd2ad8>bdcd2bb3 030=3f786cf1>3f786cf3 124=3d6894d1>3d6892de',
        'slots family=revolute config=0 call=break cw=027f state=2 record0_flags=cdc80423',
        'slots family=revolute config=0 released=yes',
        'slots family=revolute config=1 created=yes',
        'slots family=revolute config=1 call=vis cw=027f calls=9',
        'slots family=revolute config=1 call=vis cw=027f vis n=0 arrow p=3fd067e6.3f34c3b4.bee0427e d=3eb851ec.3ef5c28f.3f4ccccd length=3f800000 scale=3f900000 color=00ffffff',
        'slots family=revolute config=1 call=step cw=027f records count=4 capacity=4 window=00000000.00000004',
        'slots family=revolute config=1 call=step cw=027f record=0 words=3eb851ec.3ef5c28f.3f4ccccd.cdca6e02.SUPPORT0.SUPPORT1.cdcdcdcd.cdcdcdcd.cdcdcdcd.cdcdcdcd.cdcdcdcd.cdcdcdcd.JOINT.00000000.40200000.3fa61e05.3f84e4d1.00000000.42200000.00000000',
        'slots family=revolute config=1 call=step cw=0f7f record=0 words=3eb851eb.3ef5c28e.3f4ccccd.cdca6e02.SUPPORT0.SUPPORT1.cdcdcdcd.cdcdcdcd.cdcdcdcd.cdcdcdcd.cdcdcdcd.cdcdcdcd.JOINT.00000000.40200000.3fa61e06.3f84e4d1.00000000.42200000.00000000',
        'slots family=revolute config=1 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000004 1ac=c144deaf>c144debb 1b0=4086d390>4086d395 1b8=402b353b>402b353c 1bc=bece90ed>bece90ef 1c8=3f00a861>3f00a862 1cc=3e052da0>3e052da1',
        'slots family=revolute config=1 call=impulse cw=0f7f support0 changed 000=40a7ecfc>40a7ed09 004=bf20c560>bf20c572 008=bf41b8d2>bf41b8ec 010=be2d04e1>be2d04f1 014=bffe92d5>bffe92e3 018=bfcd25ee>bfcd25ed',
        'slots family=revolute config=1 released=yes',
        'slots family=prismatic config=0 created=yes',
        'slots family=prismatic config=0 call=vis cw=027f calls=3',
        'slots family=prismatic config=0 call=vis cw=027f vis n=0 arrow p=3fd067e6.3f34c3b4.bee0427e d=3eb851ec.3ef5c28f.3f4ccccd length=3f800000 scale=3f900000 color=00ffffff',
        'slots family=prismatic config=0 call=step cw=027f records count=7 capacity=8 window=00000000.00000007',
        'slots family=prismatic config=0 call=step cw=027f record=0 words=00000000.bf525e01.3f11e1dd.cdc80001.SUPPORT0.SUPPORT1.bf0cb7eb.bf5b0796.bf9dec91.3ee4643c.3f614771.3fa26e17.JOINT.c110f496.00000000.3ede372e.3e9b8d06.00000000.7f7fffff.00000000',
        'slots family=prismatic config=0 call=step cw=0f7f record=0 words=00000000.bf525e00.3f11e1dd.cdc80001.SUPPORT0.SUPPORT1.bf0cb7eb.bf5b0794.bf9dec8f.3ee46439.3f614773.3fa26e18.JOINT.c110f495.00000000.3ede3730.3e9b8d07.00000000.7f7fffff.00000000',
        'slots family=prismatic config=0 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000007',
        'slots family=prismatic config=0 released=yes',
        'slots family=cylindrical config=0 created=yes',
        'slots family=cylindrical config=0 call=vis cw=027f calls=3',
        'slots family=cylindrical config=0 call=vis cw=027f vis n=0 arrow p=3fd067e6.3f34c3b4.bee0427e d=3eb851ec.3ef5c28f.3f4ccccd length=3f800000 scale=3f900000 color=00ffffff',
        'slots family=cylindrical config=0 call=step cw=027f records count=4 capacity=8 window=00000000.00000004',
        'slots family=cylindrical config=0 call=step cw=027f record=0 words=00000000.bf525e01.3f11e1dd.cdc80001.SUPPORT0.SUPPORT1.bf0cb7eb.bf5b0796.bf9dec91.3ee4643c.3f614771.3fa26e17.JOINT.c110f496.00000000.3ede372e.3e9b8d06.00000000.7f7fffff.00000000',
        'slots family=cylindrical config=0 call=step cw=0f7f record=0 words=00000000.bf525e00.3f11e1dd.cdc80001.SUPPORT0.SUPPORT1.bf0cb7eb.bf5b0794.bf9dec8f.3ee46439.3f614773.3fa26e18.JOINT.c110f495.00000000.3ede3730.3e9b8d07.00000000.7f7fffff.00000000',
        'slots family=cylindrical config=0 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000004',
        'slots family=cylindrical config=0 released=yes',
        'slots family=spherical config=0 created=yes',
        'slots family=spherical config=0 call=vis cw=027f calls=72',
        'slots family=spherical config=0 call=vis cw=027f vis n=0 line p0=3f00cfcc.3f34c3b4.bee0427e p1=403033f3.3f34c3b4.bee0427e color=00ff0000',
        'slots family=spherical config=0 call=step cw=027f records count=5 capacity=8 window=00000000.00000005',
        'slots family=spherical config=0 call=step cw=027f record=0 words=3ebc5009.3f144939.3f3a3b08.cdc80403.SUPPORT0.SUPPORT1.bf0cb7eb.bf5b0794.bf9dec8f.3ee46439.3f614773.3fa26e18.JOINT.c0c7a287.00000000.3bbe4a63.3ad97996.00000000.7f7fffff.00000000',
        'slots family=spherical config=0 call=step cw=0f7f record=0 words=3ebc5009.3f144939.3f3a3b08.cdc80403.SUPPORT0.SUPPORT1.bf0cb7eb.bf5b0794.bf9dec8f.3ee46439.3f614773.3fa26e18.JOINT.c0c7a288.00000000.3bbe4a62.3ad97995.00000000.7f7fffff.00000000',
        'slots family=spherical config=0 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000005 1d8=3fc00001>3fc00000 1e8=3e930ed0>3e930ecf 1f0=bc78906d>bb26d657 1f4=3cd8b5c1>3b9174ce 1f8=bce697b7>bb9ac618 1fc=3c2bb604>3c2bb603',
        'slots family=spherical config=0 call=impulse cw=0f7f support0 changed 000=bde7972e>bdd85dc9 004=3f1d03dc>3f1abc94 008=3e805c90>3e80630b 010=3efa4395>3ef5f904 014=3e734d62>3e6cc2d6 018=bed1e8df>bee02886',
        'slots family=spherical config=0 released=yes',
        'slots family=spherical config=1 created=yes',
        'slots family=spherical config=1 call=vis cw=027f calls=10',
        'slots family=spherical config=1 call=vis cw=027f vis n=0 line p0=3f00cfcc.3f34c3b4.bee0427e p1=403033f3.3f34c3b4.bee0427e color=00ff0000',
        'slots family=spherical config=1 call=step cw=027f records count=1 capacity=8 window=00000000.00000001',
        'slots family=spherical config=1 call=step cw=027f record=0 words=00000000.00000000.00000000.cdc80026.SUPPORT0.SUPPORT1.00000000.00000000.00000000.00000000.00000000.00000000.JOINT.00000000.00000000.00000000.00000000.00000000.00000000.00000000',
        'slots family=spherical config=1 call=step cw=0f7f record=0 words=00000000.00000000.00000000.cdc80026.SUPPORT0.SUPPORT1.00000000.00000000.00000000.00000000.00000000.00000000.JOINT.00000000.00000000.00000000.00000000.00000000.00000000.00000000',
        'slots family=spherical config=1 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000001 1d8=3fc00001>3fc00000 1e8=3e930ed0>3e930ecf 1f0=c144deb0>c144debb 1f4=4086d390>4086d395 1f8=c0be710c>c0be710b 208=402b353b>402b353c',
        'slots family=spherical config=1 call=impulse cw=0f7f support0 changed 000=40a7ecfe>40a7ed09 004=bf20c560>bf20c571 008=bf41b8d2>bf41b8e9 010=be2d04d8>be2d04eb 014=bffe92d9>bffe92e3 018=bfcd25eb>bfcd25ee',
        'slots family=spherical config=1 released=yes',
        'slots family=point_on_line config=0 created=yes',
        'slots family=point_on_line config=0 call=vis cw=027f calls=11',
        'slots family=point_on_line config=0 call=vis cw=027f vis n=0 line p0=3f00cfcc.3f34c3b4.bee0427e p1=403033f3.3f34c3b4.bee0427e color=00ff0000',
        'slots family=point_on_line config=0 call=step cw=027f records count=2 capacity=8 window=00000000.00000002',
        'slots family=point_on_line config=0 call=step cw=027f record=0 words=3f6ed5f7.be3da9ea.be9e0d99.cdc80001.SUPPORT0.SUPPORT1.bc7ce295.3dff3c0f.bdf8acd4.3b762f75.3b9184df.3c0e59dc.JOINT.c15073dd.00000000.40399027.4001e4e8.00000000.7f7fffff.00000000',
        'slots family=point_on_line config=0 call=step cw=0f7f record=0 words=3f6ed5f7.be3da9e9.be9e0d98.cdc80001.SUPPORT0.SUPPORT1.bc7ce27b.3dff3c02.bdf8acc3.3b762fa1.3b91853e.3c0e59e1.JOINT.c15073eb.00000000.4039902a.4001e4ea.00000000.7f7fffff.00000000',
        'slots family=point_on_line config=0 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000002',
        'slots family=point_on_line config=0 released=yes',
        'slots family=point_in_plane config=0 created=yes',
        'slots family=point_in_plane config=0 call=vis cw=027f calls=11',
        'slots family=point_in_plane config=0 call=vis cw=027f vis n=0 line p0=3f00cfcc.3f34c3b4.bee0427e p1=403033f3.3f34c3b4.bee0427e color=00ff0000',
        'slots family=point_in_plane config=0 call=step cw=027f records count=1 capacity=8 window=00000000.00000001',
        'slots family=point_in_plane config=0 call=step cw=027f record=0 words=3eb851ec.3ef5c28f.3f4ccccd.cdc80001.SUPPORT0.SUPPORT1.bdb7c706.bfc52d15.3f76f2e4.bc1f7fe2.3faff1a4.bf5202de.JOINT.c10f5d42.00000000.3ec1fbe2.3e87c9eb.00000000.7f7fffff.00000000',
        'slots family=point_in_plane config=0 call=step cw=0f7f record=0 words=3eb851eb.3ef5c28e.3f4ccccd.cdc80001.SUPPORT0.SUPPORT1.bdb7c715.bfc52d13.3f76f2e2.bc1f7feb.3faff1a3.bf5202dd.JOINT.c10f5d48.00000000.3ec1fbe5.3e87c9ec.00000000.7f7fffff.00000000',
        'slots family=point_in_plane config=0 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000001',
        'slots family=point_in_plane config=0 released=yes',
        'slots family=distance config=0 created=yes',
        'slots family=distance config=0 call=vis cw=027f calls=1',
        'slots family=distance config=0 call=vis cw=027f vis n=0 line p0=3fc00001.3f400000.bf000000 p1=3fe0cfca.3f298768.bec084fc color=00f0f0f0',
        'slots family=distance config=0 call=step cw=027f records count=1 capacity=8 window=00000000.00000001',
        'slots family=distance config=0 call=step cw=027f record=0 words=bf5c3b75.3e96d37a.bed50a82.cdc80200.SUPPORT0.SUPPORT1.3e80ac5e.3f86f2ce.3e683efb.be88733e.bf867fbe.be458af9.JOINT.c2287c85.00000000.3c337b5f.3b337b5f.00000000.7f7fffff.00000000',
        'slots family=distance config=0 call=step cw=0f7f record=0 words=bf5c3b77.3e96d37e.bed50a7d.cdc80200.SUPPORT0.SUPPORT1.3e80ac5e.3f86f2cc.3e683f03.be88733e.bf867fbc.be458b04.JOINT.c2287c83.00000000.3c337b5e.3b337b5e.00000000.7f7fffff.00000000',
        'slots family=distance config=0 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000001',
        'slots family=distance config=0 released=yes',
        'slots family=distance config=1 created=yes',
        'slots family=distance config=1 call=vis cw=027f calls=1',
        'slots family=distance config=1 call=vis cw=027f vis n=0 line p0=3fc00001.3f400000.bf000000 p1=3fe0cfca.3f298768.bec084fc color=00f0f0f0',
        'slots family=distance config=1 call=step cw=027f records count=1 capacity=8 window=00000000.00000001',
        'slots family=distance config=1 call=step cw=027f record=0 words=bf5c3b75.3e96d37a.bed50a82.cdc80001.SUPPORT0.SUPPORT1.3e80ac5e.3f86f2ce.3e683efb.be88733e.bf867fbe.be458af9.JOINT.c141f218.00000000.3f4739f8.3f0b7561.00000000.7f7fffff.00000000',
        'slots family=distance config=1 call=step cw=0f7f record=0 words=bf5c3b77.3e96d37e.bed50a7d.cdc80001.SUPPORT0.SUPPORT1.3e80ac5e.3f86f2cc.3e683f03.be88733e.bf867fbc.be458b04.JOINT.c141f211.00000000.3f4739fc.3f0b7563.00000000.7f7fffff.00000000',
        'slots family=distance config=1 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000001',
        'slots family=distance config=1 released=yes',
        'slots family=pulley config=0 created=yes',
        'slots family=pulley config=0 call=vis cw=027f calls=2',
        'slots family=pulley config=0 call=vis cw=027f vis n=0 line p0=3fc00000.3f400000.bf000000 p1=00000000.40a00000.00000000 color=00f0f0f0',
        'slots family=pulley config=0 call=step cw=027f records count=1 capacity=8 window=00000000.00000001',
        'slots family=pulley config=0 call=step cw=027f record=0 words=00000000.00000000.00000000.cdc80026.00000000.SUPPORT1.00000000.00000000.00000000.00000000.00000000.00000000.JOINT.00000000.00000000.00000000.00000000.00000000.00000000.00000000',
        'slots family=pulley config=0 call=step cw=0f7f record=0 words=00000000.00000000.00000000.cdc80026.00000000.SUPPORT1.00000000.00000000.00000000.00000000.00000000.00000000.JOINT.00000000.00000000.00000000.00000000.00000000.00000000.00000000',
        'slots family=pulley config=0 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000001 1a0=3f6feefc>3f6feefb 1a4=3de1d1de>3de1d1dd 1b0=be0c4320>be0c431f 1b4=3f0d232b>3f0d232a 1b8=b2000000>31800000 1bc=3fd3b4c0>3fd3b4bf',
        'slots family=pulley config=0 call=impulse cw=0f7f joint changed 1d8=00000000>b71d5139 1dc=00000000>c316c103',
        'slots family=pulley config=0 released=yes',
        'slots family=fixed config=0 created=yes',
        'slots family=fixed config=0 call=vis cw=027f calls=0',
        'slots family=fixed config=0 call=step cw=027f records count=6 capacity=8 window=00000000.00000006',
        'slots family=fixed config=0 call=step cw=027f record=0 words=3f800000.00000000.00000000.cdc80001.SUPPORT0.SUPPORT1.00000000.bf800000.3f000000.00000000.00000000.00000000.JOINT.c16ffff0.00000000.3fa5d3ad.3f682858.00000000.7f7fffff.00000000',
        'slots family=fixed config=0 call=step cw=0f7f record=0 words=3f800000.00000000.00000000.cdc80001.SUPPORT0.SUPPORT1.00000000.bf800000.3f000000.00000000.00000000.00000000.JOINT.c16fffff.00000000.3fa5d3ae.3f682859.00000000.7f7fffff.00000000',
        'slots family=fixed config=0 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000006',
        'slots family=fixed config=0 released=yes',
        'slots family=d6 config=0 created=yes',
        'slots family=d6 config=0 call=vis cw=027f calls=10',
        'slots family=d6 config=0 call=vis cw=027f vis n=0 line p0=3f00cfcc.3f34c3b4.bee0427e p1=403033f3.3f34c3b4.bee0427e color=00ff0000',
        'slots family=d6 config=0 call=step cw=027f records count=2 capacity=8 window=00000000.00000002',
        'slots family=d6 config=0 call=step cw=027f record=0 words=3eb851ed.3ef5c290.3f4ccccd.cdc80001.SUPPORT0.SUPPORT1.3d23d70c.bfb0a3d7.3f4f5c2a.bc1f8001.3faff1a4.bf5202df.JOINT.c10f5d4e.00000000.3ed8d5bf.3e97c8d2.00000000.7f7fffff.00000000',
        'slots family=d6 config=0 call=step cw=0f7f record=0 words=3ebda8a6.3ef4add1.3f4be72a.cdc80001.SUPPORT0.SUPPORT1.3d1ab0f7.bfb09f61.3f4f725a.bc1eba35.3faff0a2.bf51f876.JOINT.c06ffff0.00000000.3ed8d893.3e97cacd.00000000.7f7fffff.00000000',
        'slots family=d6 config=0 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000002',
        'slots family=d6 config=0 call=project0 cw=0f7f body0 changed 018=3d1b7c80>3d1b7d20 01c=3f86e0ba>3f86e0b8 020=3dad696c>3dad6940 024=3dd0c160>3dd0c15a 028=3e54cbfd>3e54cbf5 02c=bdd8987a>bdd8986f 124=3dd0c161>3dd0c15e 128=3e54cbfd>3e54cbf9',
        'slots family=d6 config=0 released=yes',
        'slots family=d6 config=1 created=yes',
        'slots family=d6 config=1 call=vis cw=027f calls=10',
        'slots family=d6 config=1 call=vis cw=027f vis n=0 line p0=3f00cfcc.3f34c3b4.bee0427e p1=403033f3.3f34c3b4.bee0427e color=00ff0000',
        'slots family=d6 config=1 call=step cw=027f records count=2 capacity=8 window=00000000.00000002',
        'slots family=d6 config=1 call=step cw=027f record=0 words=3e3a5a12.3e87e084.3ec3eb64.cdc80403.SUPPORT0.SUPPORT1.3d1ab0f7.bfb09f61.3f4f725a.bc1eba35.3faff0a2.bf51f876.JOINT.3f217401.00000000.40a61b8a.40688cf4.00000000.7f7fffff.00000000',
        'slots family=d6 config=1 call=step cw=0f7f record=0 words=3e39ec22.3e948d13.3eba9e87.cdc80403.SUPPORT0.SUPPORT1.3d1ab0f7.bfb09f61.3f4f725a.bc1eba35.3faff0a2.bf51f876.JOINT.b5cc3ed7.00000000.40a60552.40686dd8.00000000.7f7fffff.00000000',
        'slots family=d6 config=1 call=step cw=0f7f joint changed 160=ffffffff>00000000 164=00000000>00000002',
        'slots family=d6 config=1 call=project0 cw=0f7f body0 changed 018=bb2bf400>bb2be400 01c=3f80dd59>3f80dd56 020=bc9a8d40>bc9a8d80 024=3d2311fb>3d2311f9 028=3e5188dc>3e5188d5 02c=bdd5bd77>bdd5bd6d 030=3f78f240>3f78f23f 124=3d2311fc>3d2311fd',
        'slots family=d6 config=1 released=yes',
        'rotation case=0 from=3f800000.00000000.00000000 to=00000000.3f800000.00000000 m=00000000.bf800000.00000000.3f800000.00000000.00000000.00000000.00000000.3f800000',
        'rotation case=1 from=3eb851ec.3ef5c28f.3f4ccccd to=be533c2f.3f6da3b4.3e9e6d23 m=3f563dbd.bea9dba7.bedeec97.3f08de2a.3f2a8262.3f05273a.3df085ff.bf2b0679.3f3c1a29',
        'rotation case=2 from=3e0b5948.bf73dc3e.3e8b5948 to=3f1f0fe8.3dd41535.bf46d3e2 m=3edb5939.beb4fa4f.3f54e225.3f5bc3d2.be030cb4.befe4a8f.3e905fae.3f6d38e5.3e7e945e',
        'rotation case=3 from=3eb851ec.3ef5c28f.3f4ccccd to=3eb82498.3ef68c03.3f4c9a6f m=3f7ffffb.ba3c8fae.b5167aee.3a3c8fae.3f7fffe6.3ad183d7.b51e29f4.bad183d7.3f7fffea',
        'rotation case=4 from=bf71865b.3ea1043e.3dd6b052 to=3f00a514.bf00a514.bf341a4f m=bf1bf947.3d0bf191.bf4acf3b.3f2a92fe.3f104fa0.bef9eb07.3edc1d66.bf534465.bebb8151',
        'rotation case=10 from=3dcdd4ed.3f341a4f.3f341a4f to=3dcdd4ed.3f341a4f.3f341a4f m=3f800000.26000000.26000000.25800000.3f800000.a6000000.25800000.a6000000.3f800000',
        'rotation case=11 from=3ea1043e.3f71865b.3dd6b052 to=3ea1043e.3f71865b.3dd6b052 m=3f800000.25000000.00000000.25000000.3f800000.00000000.00000000.00000000.3f800000',
        'rotation case=12 from=3f4cfeaf.3dccfeaf.3f172f07 to=3f4cfeaf.3dccfeaf.3f172f07 m=3f800000.00000000.00000000.00000000.3f800000.00000000.a5800000.00000000.3f800000',
        'rotation case=13 from=3f4cfeaf.3f172f07.3dccfeaf to=3f4cfeaf.3f172f07.3dccfeaf m=3f800000.00000000.00000000.a5800000.3f800000.00000000.00000000.00000000.3f800000',
        'rotation case=14 from=3dcdd4ed.3f341a4f.3f341a4f to=bdcdd4ed.bf341a4f.bf341a4f m=bf800000.b38912fb.b38912fb.338912fb.00000000.bf800000.338912fb.bf800000.00000000',
        'rotation case=15 from=bea1043e.3f71865b.bdd6b052 to=3ea1043e.bf71865b.3dd6b052 m=3f4ccccc.3f19999b.31e3f4e6.3f19999b.bf4ccccc.b2aaf7ab.b1e3f4e6.32aaf7ab.bf800000',
        'rotation case=16 from=3eb851ec.3ef5c28f.3f4ccccd to=3eb84fa8.3ef5cca5.3f4cca48 m=3f800000.3886318d.b87bdad6.b8862d85.3f800000.39031810.387be36d.b9031708.3f800000',
        'rotation case=17 from=bf4cfeaf.3dccfeaf.bf172f07 to=bf4cfe27.3dcd32a1.bf172ea2 m=3f800000.38a7df04.33155384.b8a7df03.3f800000.b8776f58.b3299baa.38776f57.3f800000',
        'export=NxFindRotationMatrix present=yes',
        'slots family=revolute config=0 call=step cw=027f control_inside=027f',
        'slots family=revolute config=0 call=step cw=0f7f control_inside=0f7f',
        'slots family=d6 config=1 call=step cw=027f control_inside=027f',
        'slots family=d6 config=1 call=step cw=0f7f control_inside=0f7f',
        'd6dump present=yes',
        'd6dump line=16 text=maxForce: f32:7f7fffff,  bias:  -8.960279',
        'd6dump line=21 text=A position    (XYZ) : 1.531677, 0.793200, -0.428584',
        'd6dump line=27 text=Angle: 0.124414',
        'd6dump line=36 text=maxForce: f32:7f7fffff,  bias:  -3.749996',
        'd6dump line=64 text=rel orientation (XYZW): 0.000000, -0.000000, 0.005017, 0.999987',
        'd6dump line=72 text=maxForce: f32:7f7fffff,  bias:  -0.000002',
        'd6dump lines=77'
    )
    'NxFoundationTangentTests' = @(
        'tangent sweep unit=120000 threshold=60000 scaled=60000 digest=5db0093f',
        'tangent coverage arm_z=93923 arm_xy=146112',
        'tangent case=z_nonunit arm=z n=40400000.40800000.41400000 t1=00000000.bf72dce9.3ea1e89b t2=3f791716.bd957440.be602e60',
        'tangent case=xy_nonunit_neg arm=xy n=40400000.c0800000.3f000000 t1=3f4ccccd.3f19999a.00000000 t2=bd748a54.3da306e3.3f7ebac2',
        'tangent case=z_large arm=z n=60ad78ec.612d78ec.61d8d727 t1=00000000.80000000.00000000 t2=ffc00000.ffc00000.ffc00000',
        'tangent case=xy_nan_payloads arm=xy n=7fc12345.7fc54321.00000000 t1=7fc54321.7fc54321.7fc54321 t2=7fc54321.7fc54321.7fc54321'
    )
    'NxPhysicsJointTests' = @(
        'case=revolute index=0 created=yes',
        'case=revolute index=0 out_anchor=00000000.00000000.00000000 out_axis=3f800000.00000000.00000000 state=0',
        'case=revolute index=0 actors a=match b=match',
        'case=revolute index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=prismatic index=0 created=yes',
        'case=prismatic index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=prismatic index=3 type=0 is_prismatic=yes is_revolute=no',
        'case=prismatic index=3 saved anchor0=40000000.40800000.00000000 anchor1=c0000000.40800000.00000000',
        'case=cylindrical index=0 created=yes',
        'case=cylindrical index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=cylindrical index=3 type=2 is_cylindrical=yes is_prismatic=no',
        'case=cylindrical index=3 saved normal0=bed105ec.bed105ec.3f5105ec normal1=bed105ec.bed105ec.3f5105ec',
        'case=spherical index=0 created=yes',
        'case=spherical index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=spherical index=3 flags=00000009 projection_mode=1',
        'case=spherical index=3 saved twist_limit=bf000000.3e800000.3f800000.3f400000.00000000.3f000000 swing_limit=3f200000.3f000000.3f400000',
        'case=point_on_line index=0 created=yes',
        'case=point_on_line index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=point_on_line index=3 type=4 is_point_on_line=yes is_revolute=no',
        'case=point_on_line index=3 saved anchor0=40000000.40800000.00000000 anchor1=c0000000.40800000.00000000',
        'case=point_in_plane index=0 created=yes',
        'case=point_in_plane index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=point_in_plane index=3 type=5 is_point_in_plane=yes is_revolute=no',
        'case=point_in_plane index=3 saved anchor0=40000000.40800000.00000000 anchor1=c0000000.40800000.00000000',
        'case=distance index=0 created=yes',
        'case=distance index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=distance index=3 type=6 is_distance=yes is_revolute=no',
        'case=distance index=0 saved max_distance=40200000 min_distance=3f000000 spring=41200000.3f000000.3e800000 flags=00000007',
        'case=pulley index=0 created=yes',
        'case=pulley index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=pulley index=3 type=7 is_pulley=yes is_distance=no',
        'case=pulley index=0 saved distance=40c00000 stiffness=3f400000 ratio=3fc00000 flags=00000001',
        'case=fixed index=0 created=yes',
        'case=fixed index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=fixed index=3 type=8 is_fixed=yes is_pulley=no',
        'case=fixed index=3 saved anchor0=40000000.40800000.00000000 anchor1=c0000000.40800000.00000000',
        'case=d6 index=0 created=yes',
        'case=d6 index=3 out_anchor=40000000.40800000.00000000 out_axis=3f13cd3a.3f13cd3a.3f13cd3a state=0',
        'case=d6 index=3 type=9 is_d6=yes is_fixed=no',
        'case=d6 index=0 saved motions=2.0.2.0.2.0',
        'case=revolute index=0 scene_joints when=before_release count=1 enumerated=1 order=1 self=yes end=null',
        'case=revolute index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=prismatic index=0 scene_joints when=before_release count=1 enumerated=1 order=0 self=yes end=null',
        'case=prismatic index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=cylindrical index=0 scene_joints when=before_release count=1 enumerated=1 order=2 self=yes end=null',
        'case=cylindrical index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=spherical index=0 scene_joints when=before_release count=1 enumerated=1 order=3 self=yes end=null',
        'case=spherical index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=point_on_line index=0 scene_joints when=before_release count=1 enumerated=1 order=4 self=yes end=null',
        'case=point_on_line index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=point_in_plane index=0 scene_joints when=before_release count=1 enumerated=1 order=5 self=yes end=null',
        'case=point_in_plane index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=distance index=0 scene_joints when=before_release count=1 enumerated=1 order=6 self=yes end=null',
        'case=distance index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=pulley index=0 scene_joints when=before_release count=1 enumerated=1 order=7 self=yes end=null',
        'case=pulley index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=fixed index=0 scene_joints when=before_release count=1 enumerated=1 order=8 self=yes end=null',
        'case=fixed index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=d6 index=0 scene_joints when=before_release count=1 enumerated=1 order=9 self=yes end=null',
        'case=d6 index=0 scene_joints when=after_release count=0 enumerated=0 order=none self=no end=null',
        'case=cycle index=1 scene_joints when=three_created count=3 enumerated=3 order=8.3.1 self=yes end=null',
        'case=cycle index=2 scene_joints when=middle_released count=2 enumerated=2 order=8.1 self=no end=null',
        'case=cycle index=3 scene_joints when=d6_created count=3 enumerated=3 order=9.8.1 self=yes end=null',
        'case=cycle index=4 scene_joints when=head_released count=2 enumerated=2 order=8.1 self=no end=null',
        'case=cycle index=5 scene_joints when=tail_released count=1 enumerated=1 order=8 self=no end=null',
        'case=cycle index=6 scene_joints when=prismatic_created count=2 enumerated=2 order=0.8 self=yes end=null',
        'rotated_fixture actor=a t=00000000.3f800000.00000000 quat=00000000.3f3504f3.00000000.3f3504f3',
        'rotated_fixture actor=a row0=33800000.00000000.3f7fffff row1=00000000.3f800000.00000000 row2=bf7fffff.00000000.33800000',
        'rotated_fixture actor=b t=40800000.bf800000.40000000 quat=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'rotated_fixture actor=b row0=3e088889.bf2aaaab.3f3bbbbb row1=3f6eeeef.3eaaaaaa.3e088889 row2=beaaaaaa.3f2aaaaa.3f2aaaac',
        'case=revolute index=4 out_anchor=3f800000.c0000000.3f000000 out_axis=3dcdbcfe.3e4dbcfe.3f797527 state=0',
        'case=cylindrical index=4 saved normal0=3f7eb479.bca62f73.bdc97fed normal1=3f7eb479.bca62f73.bdc97fed',
        'case=d6 index=5 saved normal0=3f800000.00000000.00000000 normal1=3f800000.00000000.00000000',
        'case=revolute index=10 created=yes',
        'case=prismatic index=10 created=yes',
        'case=cylindrical index=10 created=yes',
        'case=spherical index=10 created=yes',
        'case=point_on_line index=10 created=yes',
        'case=point_in_plane index=10 created=yes',
        'case=distance index=10 created=yes',
        'case=pulley index=10 created=yes',
        'case=fixed index=10 created=yes',
        'case=d6 index=10 created=yes',
        'case=revolute index=10 saved anchor0=c03fffff.3f800000.3f800001 anchor1=40044444.406aaaaa.bf91110f',
        'case=revolute index=11 saved axis0=bf13cd39.3f13cd3a.3f13cd3a axis1=3ed8c69a.3e4511a0.3f62a115',
        'case=revolute index=12 saved normal0=3dc97ff4.bca62f73.3f7eb478 normal1=3e1609a3.bf3c5379.3f294caf',
        'case=revolute index=13 saved anchor0=00000000.bf800000.00000000 anchor1=3f888888.3fd55556.c0844444',
        'case=revolute index=12 out_anchor=bfbffffc.3e7ffffc.40ffffff out_axis=3dcdbcfc.3e4dbcfe.3f797525 state=0',
        'case=d6 index=12 out_anchor=bfbffffc.3e7ffffc.40ffffff out_axis=3dcdbcfc.3e4dbcfe.3f797525 state=0',
        'case=fixed index=13 out_anchor=33c88888.b31dddde.b4000000 out_axis=00000000.00000000.3f7ffffe state=0',
        'case=d6 index=12 saved anchor0=c0ffffff.bf400000.bfbffffb anchor1=bfc88888.41015555.3e0888b9',
        'case=pulley index=11 saved normal0=bf5105ec.bed105ec.bed105ea normal1=bf352744.3f2e2f9a.3e43169d',
        'case=spherical index=13 saved normal0=33800000.00000000.3f7fffff normal1=3e088889.bf2aaaab.3f3bbbbb',
        'case=point_in_plane index=12 saved axis0=bf797526.3e4dbcfe.3dcdbd05 axis1=bdfdbe69.3f264e19.3f4005bb',
        'rotated_fixture actor=a record off=124 words=00000000.3f3504f2.00000000.3f3504f4',
        'rotated_fixture actor=a record off=164 words=3e3ffffe.00000000.a5800000.00000000.3e400000.00000000.a5800000.00000000.3e3fffff',
        'rotated_fixture actor=b record off=124 words=3e3af4b9.3ebaf4b9.3f0c378c.3f3af4ba',
        'rotated_fixture actor=b record off=134 words=3e088889.bf2aaaab.3f3bbbbb.3f6eeeef.3eaaaaaa.3e088889.beaaaaaa.3f2aaaaa.3f2aaaac',
        'rotated_fixture actor=b record off=164 words=3e3fffff.31599999.32099999.31599999.3e400000.314cccce.32099999.314cccce.3e400001',
        'case=prismatic index=10 internal off=16c words=3e8432a4.3e8432a3.bf0432a5.3f464bf7',
        'case=fixed index=10 internal off=16c words=bffffffd.c0000000.40800000.3e8432a4.3e8432a3.bf0432a5.3f464bf7',
        'case=revolute index=12 internal off=0ac words=bddb3f43.bf32eec6.bf34c7db.bd14ac04.bf22b49e.bea0571e.bf2e5b9a.be3d1a85',
        'rotated_fixture actor=flip_xy_a record off=05c words=3f800000.00000000.00000000.00000000',
        'rotated_fixture actor=flip_xy_b record off=05c words=00000000.3f800000.00000000.00000000',
        'rotated_fixture actor=near_xy_a record off=124 words=3f741dfd.3e4d9285.3dcd9285.3e4d9285',
        'rotated_fixture actor=near_xy_a record off=134 words=3f6634e0.3eaf653c.3e8b48dd.3ed8aa3b.bf56bafb.beaf653c.3de2fb7b.3ed8aa3b.bf6634dc',
        'rotated_fixture actor=near_xy_b record off=124 words=3e1ac3e0.3f6825d0.be9ac3e0.3e80f890',
        'rotated_fixture actor=near_xy_b record off=134 words=bf53d13e.3eda5094.3ebb207f.3df980a9.3f4585d8.bf1fd66c.bf0c585f.bef1b4a4.bf30bb26',
        'case=revolute index=20 created=yes',
        'case=d6 index=20 created=yes',
        'case=revolute index=20 saved anchor0=3f800000.bf800000.c0400000 anchor1=40400000.40400000.bf800000',
        'case=spherical index=21 saved normal0=3f7eb479.3ca62f73.3dc97fed normal1=bf7eb479.bca62f73.3dc97fed',
        'case=revolute index=22 saved anchor0=3fd3d426.3f45f6fc.c0312b31 anchor1=40132ae7.3f103e58.c06a3bca',
        'case=revolute index=23 saved normal0=3f600d83.3ea1e72b.3ebb7000 normal1=bf4591f6.3ee8fa38.3ee37124',
        'case=fixed index=20 internal off=16c words=40800000.40000000.c0000000.80000000.80000000.3f800000.00000000',
        'case=fixed index=22 internal off=16c words=403e3a09.40793aaa.bcce5840.3d6e3390.3e300fdb.3f6b9cc1.3eb15b45',
        'case=prismatic index=23 internal off=16c words=3d6e3390.3e300fdb.3f6b9cc1.3eb15b45',
        'case=cycle left_for_scene_release=2'
    )
    'NxPhysicsJointDescTests' = @(
        'case=0 actors a=null b=null in_anchor=3f800000.40000000.40400000 in_axis=3f000000.3f000000.3f000000',
        'after_axis localNormal0=bed105ec.bed105ec.3f5105ec localNormal1=bed105ec.bed105ec.3f5105ec localAxis0=3f13cd3a.3f13cd3a.3f13cd3a localAxis1=3f13cd3a.3f13cd3a.3f13cd3a localAnchor0=3f800000.40000000.40400000 localAnchor1=3f800000.40000000.40400000 flags=00000002',
        'after_axis localNormal0=80000000.80000000.3f800000 localNormal1=80000000.80000000.3f800000 localAxis0=00000000.3f800000.00000000 localAxis1=00000000.3f800000.00000000 localAnchor0=3f800000.00000000.00000000 localAnchor1=3f800000.00000000.00000000 flags=00000002'
    )
    'NxPhysicsKernelFuzzTests' = @(
        'fuzz generator=xorshift32 scalar_iterations=120000 vector_iterations=40000 aimed_iterations=60000',
        'fuzz seeds scalar=13579bdf vector=02468ace aimed=feedface',
        # One line, not two. The first version of this registration pinned
        # `checks=480000` and a bare `hits=89978` beside it -- and NINE export
        # lines in PhysicsKernelFuzzTests.cpp end with ` hits=%u`, so the bare
        # count was satisfied by any of them. That is the twelfth defect this
        # program recorded, still live in the file that describes it as fixed:
        # it was fixed for the twelve `fuzz coverage name=` lines and not for
        # this one, because the test that enforces uniqueness compared
        # registered lines only against each other and never against what the
        # harness can actually print. It does both now.
        'fuzz name=NxRayTriIntersect.aimed present=1 checks=480000 digest=85fb27ffd1c76541 hits=89978',

        # The Task 2 second dispatch adds five blocks covering the remaining
        # eleven exports. The iteration counts and seeds are pinned for the same
        # reason as the four above.
        'fuzz box_iterations=40000 aimed_box_iterations=40000 capsule_iterations=30000 sat_iterations=20000 normals_iterations=4000',
        'fuzz seeds box=1a2b3c4d aimedbox=5e6f7a8b capsule=9c0d1e2f sat=0f1e2d3c normals=7b8a9d6e',

        # Every generator that can lose coverage has one line, and each line
        # carries the export name.
        #
        # It carries the name because the first version of this registration
        # pinned bare counts, and `hits=34575` is emitted by BOTH
        # NxSegmentAABBIntersect.aimed and NxSegmentBoxIntersect.aimed. Since
        # run_phase_gate.ps1 matches by substring and requires two occurrences
        # across the two pair transcripts, either export alone satisfied it.
        # De-aiming the segment/AABB generator drove it from 34575 to 92 -- 99.7%
        # of the aimed path lost -- and the gate still passed. That was the
        # twelfth gate in this program found unable to fail, and it was inside
        # the fix for the eleventh.
        #
        # test_gate_targets.py now asserts that these strings are unique and
        # that every aimed generator has one, so the next person to add a block
        # cannot reintroduce the collision by hand.
        'fuzz coverage name=NxRayAABBIntersect.aimed reached=34577',
        'fuzz coverage name=NxRayAABBIntersect2.aimed reached=34485',
        'fuzz coverage name=NxSegmentAABBIntersect.aimed reached=34575',
        'fuzz coverage name=NxSegmentBoxIntersect.aimed reached=34575',
        'fuzz coverage name=NxRayOBBIntersect.aimed reached=27852',
        'fuzz coverage name=NxSegmentOBBIntersect.aimed reached=27837',
        'fuzz coverage name=NxRayCapsuleIntersect.aimed reached=23558',
        'fuzz coverage name=NxSweptSpheresIntersect.aimed reached=25544',
        'fuzz coverage name=NxBoxBoxIntersect.aimed reached=17606',
        'fuzz coverage name=NxSeparatingAxis reached=19860',
        'fuzz coverage name=NxSeparatingAxis.aimed reached=11543',
        'fuzz coverage name=NxBuildSmoothNormals reached=8000',
        'fuzz name=NxBuildSmoothNormals present=1 checks=236384'
    )

    # NxPhysicsCollisionTests runs once against the pinned oracle rather than
    # once per pair, so run_phase_gate.ps1 requires each of these exactly once
    # rather than twice. Two kinds of line are registered:
    #
    #   * the run's own identity -- generator, seeds, control words, and the
    #     72-slot matrix result -- so a silently reseeded or shortened run is a
    #     failure and not a quieter pass;
    #   * one line per driven row, truncated after `oracle=<digest>`. The digest
    #     is over the shipped DLL's own return values, so it pins that the
    #     oracle was called, with these inputs, this many times. Nothing on the
    #     reconstruction side can produce it.
    #
    # Verified by the strong form rather than by editing a pinned number:
    # de-aiming the aimed generator (dropping the separation aiming so every
    # aimed pair is placed at random) moves eight of the fourteen digests and
    # the gate fails naming them.
    'NxPhysicsCollisionTests' = @(
        'collision generator=xorshift32 pair_iterations=60000 aimed_iterations=60000',
        'collision seeds pair=c0ffee11 aimed=5eed10ad',
        'collision control_words default=027f simulate=0f7f',
        'matrix slots=72 null=33 wrong=0',
        # 2592 probes: for each of the 36 ordered type pairs and each of the 36
        # slots, the oracle's own dispatcher is handed a matrix with only that
        # slot filled and asked whether it calls it. That measures the index
        # rule and the pair swap instead of restating them, which the first
        # version of this check did not -- it compared the index function
        # against its own body.
        'matrix index_rule probes=2592 wrong=0',

        'collision name=plane_sphere.random index=1 rva=0x00048a20 owner=phys_fn_001899 checks=120000 oracle=4513405a6ba24fb9',
        'collision name=plane_sphere.aimed index=1 rva=0x00048a20 owner=phys_fn_001899 checks=120000 oracle=ed8746bf83b7801f',
        'collision name=plane_box.random index=2 rva=0x00047e90 owner=phys_fn_001881 checks=120000 oracle=51aa3757383b013a',
        'collision name=plane_box.aimed index=2 rva=0x00047e90 owner=phys_fn_001881 checks=120000 oracle=145322535617c6e3',
        'collision name=plane_capsule.random index=3 rva=0x00048270 owner=phys_fn_001889 checks=120000 oracle=ddaa2f876eab5b67',
        'collision name=plane_capsule.aimed index=3 rva=0x00048270 owner=phys_fn_001889 checks=120000 oracle=13bbd3e24d52fa75',
        'collision name=sphere_sphere.random index=7 rva=0x0004b800 owner=phys_fn_001931 checks=120000 oracle=c2a2c19b0ab2c847',
        'collision name=sphere_sphere.aimed index=7 rva=0x0004b800 owner=phys_fn_001931 checks=120000 oracle=360355218ada0851',
        'collision name=sphere_box.random index=8 rva=0x00049e70 owner=phys_fn_001915 checks=120000 oracle=d95a5fe09a4528bb',
        'collision name=sphere_box.aimed index=8 rva=0x00049e70 owner=phys_fn_001915 checks=120000 oracle=5aa56c46d5c81c7f',
        'collision name=sphere_capsule.random index=9 rva=0x0004a3e0 owner=phys_fn_001921 checks=120000 oracle=5911a75688da4c0c',
        'collision name=sphere_capsule.aimed index=9 rva=0x0004a3e0 owner=phys_fn_001921 checks=120000 oracle=ad24a9efaf9a8d8d',
        'collision name=box_box.random index=14 rva=0x000389d0 owner=phys_fn_001738 checks=120000 oracle=64b9dac9e8320ea1',
        'collision name=box_box.aimed index=14 rva=0x000389d0 owner=phys_fn_001738 checks=120000 oracle=705c16798f16a4c3',
        'collision name=box_corner index=- rva=0x00020750 owner=phys_fn_000943 checks=1440000 oracle=a112564364ebb703',
        'collision name=sphere_box_data index=- rva=0x00049ca0 owner=phys_fn_001913 checks=120000 oracle=66f01d58c6bf128b',

        # phys_fn_001739, the leaf of the box/box subtree and the only row under
        # matrix A [BOX][BOX] that calls nothing. The digest is over the whole
        # 80-bit register the row returns, because its one caller compares that
        # register against 0.0f at 0x00039938 before narrowing it.
        'collision name=box_quad_depth index=- rva=0x00038a90 owner=phys_fn_001739 checks=1200000 oracle=64e0e657f9d2f3bb',

        # phys_fn_001741 with its phys_fn_001743 continuation, the box/box
        # clipping and manifold row. Two families because they answer different
        # questions and neither substitutes for the other: `.random` draws every
        # pose, extent and centre through nxPick, which is the only thing that
        # reaches the integer extent tests' NaN behaviour and the negative-extent
        # case; `.aimed` builds real box pairs and is where the manifold modes
        # live. Each digest folds the oracle's own contact count and then its own
        # points and separations over that count, the way nxFoldStream does.
        'collision name=box_clip.random index=- rva=0x00038ba0 owner=phys_fn_001741 checks=2782400 oracle=13358be7c75d880c',
        'collision name=box_clip.aimed index=- rva=0x00038ba0 owner=phys_fn_001741 checks=16158496 oracle=3adbe72ad076985c',
        # The three manifold modes, separated by STAGE and read off the oracle's
        # own returned points. The aimed family gives the reference box an
        # identity pose with a zero centre, so stage 5 is the identity map and
        # each contact says where it came from: stage 4 puts both of y and z
        # exactly on the face boundary (`vertex_face`), stage 3's four clip cases
        # put exactly one there (`edge_clip`), its fifth case emits an integer
        # zero x (`plane_cross`), and stage 2's corners are strictly inside
        # (`face_face`). `swap_differs` is the same geometry driven with the
        # other box as the reference. `over_sixteen` and `max_contacts` are the
        # oracle overrunning phys_fn_001749's own sixteen-contact frame.
        'collision coverage name=box_clip.random emitted=158900 zero_count=219210 swap_differs=18616 nan_depth=143258 over_sixteen=77 max_contacts=20 default_mismatches=0 simulate_mismatches=0',
        'collision coverage name=box_clip.aimed emitted=994906 zero_count=37806 face_face=139832 edge_clip=215448 vertex_face=158054 plane_cross=14636 swap_differs=53290 nan_depth=0 over_sixteen=16 max_contacts=18 default_mismatches=0 simulate_mismatches=0',

        # phys_fn_001745, the fifteen-axis separating-axis search and the row
        # that chooses which face the clip row above is handed. Two families for
        # the same reason box_clip has two: `.random` draws every pose element,
        # extent and centre through nxPick, which is what reaches the edge
        # test's UNSIGNED INTEGER compare on a NaN projection and on a negative
        # radius sum, and a minimum search whose six candidates are all NaNs;
        # `.aimed` builds real box pairs and is what puts a manifold behind
        # each of the six dispatch arms. Each digest folds the oracle's own
        # count, the cache byte it wrote, the contact normal it wrote and then
        # its own contacts over its own count.
        'collision name=box_axis.random index=- rva=0x00039c10 owner=phys_fn_001745 checks=2933136 oracle=aa8dab9438d06f35',
        'collision name=box_axis.aimed index=- rva=0x00039c10 owner=phys_fn_001745 checks=10747984 oracle=2e654ff01e6b9df7',
        # `arm0..arm5` are the six slots of the 24-byte switch table at
        # 0x0003acc0, counted from the byte the oracle itself wrote at
        # 0x0003a458 rather than from anything the harness decided; `negated`
        # compares the returned normal against the raw words of the row it came
        # from, which is exact because `fchs` always flips the sign bit.
        #
        # THE THREE THAT ARE ABOUT sink+0xe8. `warm_entry` counts calls that
        # arrived with an index in 1..6, which SKIP the nine edge-axis tests
        # outright (0x0003a01e) and bias the remembered axis by 0.999f;
        # `carry_warmed` counts calls that arrived warm only because an EARLIER
        # PAIR in the same sink left the byte behind, which is exactly what a
        # reimplementation that clears it per call gets wrong; and `edge_only`
        # counts geometries the same row separates from a cold byte and does not
        # separate from a warm one, which is the measurement that says both the
        # edge tests and the shortcut past them are reached.
        #
        # `at_sixteen` and `over_sixteen` are the answer to the question
        # box_clip left open. Driven with a SYNTHETIC reference face that row
        # reached 18 and 20 contacts against phys_fn_001749's sixteen-entry
        # frame; here the face comes from the search the shipped chain uses, and
        # across 288,000 calls the count fills the frame exactly 14 times and
        # never exceeds it.
        'collision coverage name=box_axis.random arm0=88226 arm1=751 arm2=718 arm3=749 arm4=658 arm5=747 separated=52151 negated=45925 warm_entry=78236 carry_warmed=6328 edge_only=7786 emitted=57321 at_sixteen=0 over_sixteen=0 max_contacts=12 default_mismatches=0 simulate_mismatches=0',
        'collision coverage name=box_axis.aimed arm0=18766 arm1=18346 arm2=19468 arm3=18490 arm4=19090 arm5=18782 separated=31058 negated=56148 warm_entry=81090 carry_warmed=9338 edge_only=1276 emitted=545749 at_sixteen=14 over_sixteen=0 max_contacts=16 default_mismatches=0 simulate_mismatches=0',

        # phys_fn_001748, the transpose-and-copy shim. Driven as a row of its
        # own because the transposition is the whole of what it does and no
        # caller can tell a transpose from its inverse unless the pose is
        # asymmetric. `aimed` counts calls given a real rotation and physical
        # extents; the rest are raw bit patterns.
        #
        # AND IT IS WHERE THE OVERFLOW QUESTION IS ANSWERED. Its arguments are
        # exactly the entry's -- `Shape+0xe4` and `Shape+0x0c` for each shape --
        # but the arrays are the harness's own eighty slots rather than
        # phys_fn_001749's sixteen, so a manifold larger than the frame can be
        # measured instead of crashing the run. `aimed_max=18` is that
        # measurement: with a real rotation, physical extents and a physical
        # placement, the shipped chain returns eighteen contacts into a frame
        # that holds sixteen and whose seventeenth entry is its return address.
        'collision name=box_shim index=- rva=0x0003ace0 owner=phys_fn_001748 checks=2991776 oracle=3d36c90baebbedee',
        'collision coverage name=box_shim aimed=15954 separated=13747 emitted=130986 carry_warmed=5342 over_sixteen=2 aimed_max=18 max_contacts=18 default_mismatches=0 simulate_mismatches=0',

        # phys_fn_001749, matrix A [BOX][BOX] -- the last entry of matrix A this
        # phase closes, and the only one that emits a manifold. The widths are
        # 7 + 4n because both the header and the normal block are written with
        # NO PREDICATE here, so w11 is one contact, w15 two and w31 six.
        #
        # `warm_carried` and `axis_cleared` are the warm start end to end: up to
        # four box pairs go into ONE sink without a reset between them, which is
        # the state the shipped pipeline is in, and `axis_cleared` counts the
        # pairs that wrote 0 over the remembered axis because they produced
        # nothing (0x0003ae5d). `overflow_skipped` and `probe_max` come from a
        # pre-flight call to phys_fn_001748 with the identical arguments and a
        # copy of the live cache byte: a pair whose manifold would not fit the
        # entry's frame is counted and NOT driven, because driving it would
        # return the oracle into a contact coordinate.
        'collision name=contact_box_box index=14 rva=0x0003add0 owner=phys_fn_001749 checks=6007270 oracle=f9afbd75702ac4a2',
        'collision coverage name=contact_box_box emitted=53718 negated=25094 static0=4136 static1=3994 warm_carried=18611 axis_cleared=13132 w11=1454 w15=8850 w31=6712 at_sixteen=18 max_contacts=16 overflow_skipped=0 probe_max=16 default_mismatches=0 simulate_mismatches=0',

        # Two of Task 2's exports, driven here because the recovered matrix put
        # them inside the simulation step and their own differential runs under
        # the CRT default control word only. NxRayTriIntersect agrees under
        # both. NxBuildSmoothNormals agrees under the default and differs on 24
        # of 459,676 checks under 0x0f7f; that 24 is pinned in its coverage line
        # below rather than dropped, so it fails if it moves in either
        # direction, including toward zero.
        'collision name=step_ray_tri index=- rva=export owner=phys_fn_001712 checks=1560000 oracle=2bb3aaedbd4ac9ed',
        'collision name=step_smooth_normals index=- rva=export owner=phys_fn_002146 checks=919352 oracle=6d5d4be60a607d94',

        # The branch mix behind each digest. A digest moving tells you the run
        # changed; these tell you how, and they are what fails first when an
        # aimed generator stops aiming.
        'collision coverage name=plane_sphere.random true=66180 false=53820',
        'collision coverage name=plane_sphere.aimed true=92130 false=27870 swap_differs=30091',
        'collision coverage name=plane_box.random true=75073 false=44927',
        'collision coverage name=plane_box.aimed true=110278 false=9722 swap_differs=23726',
        'collision coverage name=plane_capsule.random true=70218 false=49782',
        'collision coverage name=plane_capsule.aimed true=86896 false=33104 swap_differs=26644',
        'collision coverage name=sphere_sphere.random true=15402 false=104598',
        'collision coverage name=sphere_sphere.aimed true=68652 false=51348 swap_differs=0',
        'collision coverage name=sphere_box.random true=35838 false=84162',
        'collision coverage name=sphere_box.aimed true=72338 false=47662 swap_differs=36169',
        'collision coverage name=sphere_capsule.random true=16473 false=103527',
        'collision coverage name=sphere_capsule.aimed true=47640 false=72360 swap_differs=4774',
        'collision coverage name=box_box.random true=37228 false=82772',
        'collision coverage name=box_box.aimed true=83894 false=36106 swap_differs=0',
        # The only kernel in this component that writes floats, so the only one
        # where a NaN payload is observable at all. 37792 of its 1,440,000
        # output words are non-finite; at the mixture Task 2's harness used it
        # was 182, which is why that generator could not have measured the
        # architecture flag here either.
        'collision coverage name=box_corner non_finite_words=37792',
        'collision coverage name=sphere_box_data true=81470 centre_inside=22495',
        # `reversed` and `aimed_inside` are the two halves of the containment
        # test: the oracle accepts one winding only, so a generator that stopped
        # building the accepted one would leave the whole interpolation dead and
        # `interpolated` would collapse to whatever the raw-bit draws reach.
        # `non_finite` is the oracle's own answer being an infinity or a NaN,
        # which can only happen once a NaN has walked the containment test --
        # the loop continues on unordered where it leaves on equal. Every one of
        # the 12,737 is a NaN and no draw produces an infinity, and unlike
        # segment_segment this block does NOT canonicalise them: removing the
        # canonicalisation leaves mismatches=0, so the payloads agree too.
        'collision coverage name=box_quad_depth aimed_inside=11250 reversed=22482 interpolated=36640 non_finite=12737 default_mismatches=0 simulate_mismatches=0',
        'collision coverage name=step_ray_tri hits=47907 non_finite_words=97007 default_mismatches=0 simulate_mismatches=0',
        'collision coverage name=step_smooth_normals non_finite_words=65973 default_mismatches=0 simulate_mismatches=0',

        # The first contact-generation entry. The digest is over the whole
        # stream -- pair headers, normal blocks and contact records -- not
        # over a count, and the coverage line pins the branch mix behind it:
        # `emitted - normal_blocks` is how often both the header and the
        # normal block were correctly skipped, which is the ordering rule,
        # and `negated_path` how often the pair was emitted against the body
        # the sink is not oriented to, which is the normal-orientation rule.
        'collision name=contact_plane_sphere index=1 rva=0x00048a70 owner=phys_fn_001901 checks=2596000 oracle=5d51a6ed6f586cda',
        # A width histogram, not buckets. w4 is a bare record, w7 a header
        # whose normal happened to equal the cleared cache, w8 a normal
        # block without a header and w11 both. The previous counters keyed
        # on total words and could not distinguish w7 from nothing at all.
        'collision coverage name=contact_plane_sphere emitted=59488 w4=5288 w7=12 w8=2768 w11=51420 negated_path=50004',

        # The emitter driven directly, with real feature ids. No matrix A
        # entry reachable today passes anything but 0xffff, so without this
        # block both halves of the feature rule -- the validity test at
        # 0x0001d694 and the swap at 0x0001d63c -- are dead code.
        'collision name=contact_emit index=- rva=0x0001d610 owner=phys_fn_000873 checks=4453104 oracle=deaa1fc557071411',
        'collision coverage name=contact_emit real_feature_pairs=25106 fifth_words=25276',

        # Matrix A [PLANE][CAPSULE] and the vtable slot it dispatches through.
        #
        # phys_fn_001261 has its own block because the entry cannot reach all of
        # it: phys_fn_001891 always passes hintFlags = 0 and a distance limit
        # equal to the segment length it just measured, so the NX_RAYCAST_NORMAL
        # branch and every other limit are dead from there. `wrote_normal` is
        # what fails if that stops being driven.
        'collision name=shape_raycast_plane index=- rva=0x00025350 owner=phys_fn_001261 checks=5880000 oracle=569f78e4d5df74ec',
        'collision coverage name=shape_raycast_plane hits=36662 wrote_normal=18252 aimed=22353',
        # `swept` and `swept_emitted` are the two halves of the capsule+0xe8
        # flag: the first is how often the generator set NX_SWEPT_SHAPE and the
        # second how often the vtable call then reported a hit, so a generator
        # that stopped setting the bit loses both. `two` is the oracle's own
        # contact count moving by two, which is the multiple-contact rule and
        # the one thing no other block in this target reaches.
        'collision name=contact_plane_capsule index=3 rva=0x00048370 owner=phys_fn_001891 checks=2161616 oracle=6ddd7e0506885f19',
        'collision coverage name=contact_plane_capsule emitted=38322 one=14258 two=24064 swept=49582 swept_emitted=9242 zero_axis=40868 w4=710 w8=1694 w11=13166 w15=22054',

        # Matrix A [SPHERE][CAPSULE] and the sphere's own slot 5.
        #
        # phys_fn_001377 gated on its default-word half only. Under 0x0f7f it
        # differed on 13 of 2,940,000, and every one was hit.worldImpact or the
        # distance derived from it -- fields NxRaySphereIntersect writes, not
        # fields this row computes. The recovered matrix had put that export
        # inside the simulation step for the first time, the third time this
        # program had a Task 2 row reopened by a reachability discovery, and
        # the count of 13 was registered so that any move would fail.
        # 0637850 (Geometry.cpp square roots through fsqrt at the live control
        # word instead of __CIsqrt) removed that in-step difference: the count
        # went from 13 to 0, and NxRaySphereIntersect now matches under 0x0f7f.
        # The line stays registered so that a regression back to nonzero fails.
        'collision name=shape_raycast_sphere index=- rva=0x00027c70 owner=phys_fn_001377 checks=5880000 oracle=6bee7065d00d060e',
        'collision coverage name=shape_raycast_sphere hits=54437 wrote_normal=27073 aimed=22568 behind=5666 default_mismatches=0 simulate_mismatches=0',
        # `coincident` is the sphere centre placed exactly on the capsule axis,
        # the only input that reaches the zero-length-normal return at
        # 0x0004a811, and `beyond_end` is the closest point falling past an
        # endpoint rather than on the interior. Neither is reached by a random
        # pair often enough to matter.
        'collision name=contact_sphere_capsule index=9 rva=0x0004a4b0 owner=phys_fn_001923 checks=1802896 oracle=6a6da9567aa6eb2d',
        'collision coverage name=contact_sphere_capsule emitted=38192 swept=50378 swept_emitted=17834 zero_axis=34458 coincident=4074 beyond_end=11046 w4=1 w8=3127 w11=35064',

        # phys_fn_001690, and the first row in this programme closed on one
        # phase's ledger by another phase's gate. It is a PHASE 2 row that Phase
        # 2 deferred `homeless_shared_code` naming phases 3 and 4 as its drivers;
        # Phase 3's capsule/capsule entries are the reachability it lacked, and
        # this target is the proof. gates/phase2-closure.json records it closed
        # with `discharged_by_phase: 3` and names this gate.
        #
        # `parallel` is the only input class that reaches the second region tree
        # at 0x000343cf at all -- two capsule axes from random rotations are
        # never within the 1e-5f determinant epsilon -- and dropping it takes the
        # digest from f97e71b6 to 7c10ebea while the harness's own exit code
        # stays 0. `null_params` drives the two output pointers the callee
        # null-checks and no censused caller ever leaves null. `canonical_nan`
        # counts the words compared as NaN rather than bit for bit; it is here so
        # that a generator which stops producing non-finite inputs fails the gate
        # instead of quietly making the comparison stricter.
        'collision name=segment_segment index=- rva=0x00033e80 owner=phys_fn_001690 checks=2160000 oracle=f97e71b61b01a17c',
        'collision coverage name=segment_segment parallel=8903 degenerate=12015 null_params=7323 interior=22793 clamped_s=58807 clamped_t=47928 non_finite=22038 canonical_nan=42009 default_mismatches=0 simulate_mismatches=662',

        # phys_fn_001010, slot 5 on a CAPSULE shape, driven at its own address.
        # `untouched_normal` equalling `hits` is the measurement that this row
        # writes no normal under any hint flags -- every combination is driven --
        # and it is what phys_fn_001775's swept path depends on. Making the row
        # write one the way the sphere's does moves 65,343 words here and 31,150
        # in the contact block.
        # simulate_mismatches counts the words that differ under the in-step
        # word 0x0f7f. 0637850 (Geometry.cpp square roots through fsqrt at the
        # live control word instead of __CIsqrt) took it from 242 to 0; the
        # line stays registered so that a regression back to nonzero fails.
        'collision name=shape_raycast_capsule index=- rva=0x00022480 owner=phys_fn_001010 checks=5880000 oracle=28ac6dc0d51aa6bd',
        'collision coverage name=shape_raycast_capsule hits=32821 untouched_normal=32821 aimed=22571 zero_axis=15030 default_mismatches=0 simulate_mismatches=0',

        # Matrix A [CAPSULE][CAPSULE]. Four things here cannot be reached by a
        # generator that does the obvious thing:
        #
        #   f00/f01/f10/f11 -- the two NX_SWEPT_SHAPE bits driven independently.
        #     The branch is an OR and the ray selector is shape1's bit alone, so
        #     setting both together drives three of the four combinations into
        #     one branch and cannot tell the OR from an AND.
        #   seeded_normal -- the swept contact normal is uninitialised stack, so
        #     the harness seeds the frame with a repeated dword and counts the
        #     emissions that carry it back out. Registering it is what stops the
        #     seed being quietly dropped, which would make the comparison agree
        #     for the wrong reason.
        #   parallel and c3/c4 -- the endpoint clip only runs above a 0.9998f
        #     axis dot product, and its three- and four-contact cases need the
        #     two half heights matched and the axes co-located to within a 0.1%
        #     parameter tolerance. Independent draws reach none of it.
        #   c1/c2 -- the single-contact path and the two-contact clip.
        #
        # simulate_mismatches counts the words that differ under the in-step
        # word 0x0f7f. 0637850 (Geometry.cpp square roots through fsqrt at the
        # live control word instead of __CIsqrt) took it from 43 to 0; the
        # line stays registered so that a regression back to nonzero fails.
        'collision name=contact_capsule_capsule index=21 rva=0x0003d9d0 owner=phys_fn_001775 checks=1073192 oracle=d94c81f08538ddac',
        'collision coverage name=contact_capsule_capsule emitted=20238 f00=25362 f01=25044 f10=24804 f11=25386 swept_emitted=10174 seeded_normal=10008 parallel=37380 zero_axis=42726 coincident=3830 beyond_end=10224 c1=18889 c2=1305 c3=20 c4=24 default_mismatches=0 simulate_mismatches=0',

        # Matrix A [PLANE][BOX], the one entry the oracle does not route through
        # phys_fn_000873 -- it inlines the stream logic. The reconstruction
        # shares the three append levels with the emitter and duplicates only the
        # predicates, which is where the two oracle copies actually differ, so
        # `header_rewrite` and `zero_normal` are the lines that keep the
        # duplication honest: they count the inputs that reach a missing
        # predicate, and giving the inlined copy the emitter's header test moves
        # 452,624 words.
        #
        # `c6=45492 c7=0` is the contact cap. The entry returns as soon as the
        # sixth contact is emitted, from a shape with eight corners, so a box
        # wholly below a plane loses two of them -- the only buffer limit this
        # component can reach, and the RED mode this file listed as absent.
        'collision name=contact_plane_box index=2 rva=0x00047f20 owner=phys_fn_001883 checks=7450176 oracle=192a285f9e8892eb',
        'collision coverage name=contact_plane_box emitted=66152 header_rewrite=30298 zero_normal=3318 below_plane=13072 negated=50410 c1=2470 c2=4330 c4=10012 c6=45492 c7=0 w11=2470 w15=4330 w31=45492 default_mismatches=0 simulate_mismatches=0',

        # phys_fn_001281 and phys_fn_002266, the two rows both sphere entries of
        # matrix A reach that leave no trace in the contact stream. Under the SDK
        # as it ships NX_CONTINUOUS_CD is 0.0f, so the guard returns without
        # touching the sink and the whole branch is invisible to a stream
        # differential -- which is why they are driven at their own addresses.
        #
        # `continuous_cd` is that parameter read back out of the oracle's own
        # array through the oracle's own getParameter. It is the reachability
        # claim for phys_fn_002264, the 1,943-byte continuous-collision sweep
        # this program has NOT reconstructed, and pinning it here means the claim
        # fails if it ever stops holding. `index_probes` sets each of the other
        # 58 parameters non-zero in turn and checks the guard's answer does not
        # move, which measures that it reads index 11 instead of restating the
        # `push 0xb` at 0x00056656.
        'collision name=shape_owner index=- rva=0x000257a0 owner=phys_fn_001281 checks=8000 oracle=87f31ac4e078fb65',
        'collision coverage name=shape_owner probes=8000 static_first=1980',
        'collision name=ccd_guard index=- rva=0x00056650 owner=phys_fn_002266 checks=16000 oracle=62da6260b65713a5',
        'collision coverage name=ccd_guard continuous_cd=00000000 probes=8000 returned_true=8000 sink_untouched=8000 index_probes=58 index_wrong=0',

        # Matrix A [SPHERE][SPHERE]. `coincident` and `coincident_emitted` are
        # the two sides of the 1e-5f test at 0x0004b8ff: the generator places a
        # third of the coincident draws below it and the rest above, so either
        # count going to zero is a generator that stopped straddling it.
        # `static0` and `static1` are the two null-`owner->[8]` branches, of
        # which the first had never been driven by anything in this target.
        'collision name=contact_sphere_sphere index=7 rva=0x0004b860 owner=phys_fn_001933 checks=2061456 oracle=40ec7aaacfefd69f',
        'collision coverage name=contact_sphere_sphere emitted=49285 coincident=13352 coincident_emitted=9126 repeated=14897 static0=4281 static1=4228 negated=24926 w4=8529 w8=2356 w11=38400 default_mismatches=0 simulate_mismatches=0',

        # phys_fn_001917 driven at its own address, because the entry above it
        # reaches the second of its two algorithms -- the sphere centre inside
        # the box -- only by accident. `axis_x`/`axis_y`/`axis_z` are which face
        # won, read off the oracle's own normal rather than recomputed, so a
        # generator that stopped placing centres inside loses all three.
        'collision name=sphere_box_contact index=- rva=0x00049f00 owner=phys_fn_001917 checks=3480000 oracle=97bce5b099aebebf',
        'collision coverage name=sphere_box_contact true=82140 centre_inside=22596 axis_x=6969 axis_y=7042 axis_z=7007 default_mismatches=0 simulate_mismatches=0',

        # Matrix A [SPHERE][BOX]. The entry hands the emitter the sphere as
        # `object1` and the box as `object0`, which is shape0 in the slot
        # plane/sphere gives shape1; the two materials and the two identities are
        # driven apart so that swapping them moves words.
        'collision name=contact_sphere_box index=8 rva=0x0004a2d0 owner=phys_fn_001919 checks=2453136 oracle=966d357901d1dba9',
        'collision coverage name=contact_sphere_box emitted=65826 centre_inside=8638 repeated=15287 static0=4173 static1=4171 negated=25490 w4=18442 w8=7236 w11=40148 default_mismatches=0 simulate_mismatches=0'
    )

    # The Phase 4 asset-format gate. It went GREEN in P4 Task 3 for the pmap
    # load path and the mesh-header rejects, and stayed green as Task 4 added
    # the mesh stream writer. The registrations are still about the ORACLE
    # side: every line below is a fact about the shipped DLL alone. The digest
    # folds the oracle's own answers -- acceptance, error count, the reported
    # source line, the cell count, the decoded grid, and for the writer cases
    # the acceptance plus a fold over everything its writer stored -- over all
    # 30 probes, and each case line is that case's answer. Nothing on the
    # reconstruction side appears in any of them.
    #
    # The nine `writer` cases are INPUTS both sides serialise, not recorded
    # expectations: what the oracle contributes is its complete store log --
    # field order, sizes, the four conditional arms, the blob length and the
    # collapsed buffer -- folded into one FNV-1a digest per case. Two of them
    # (array_a_only / array_b_only) differ in exactly one buffer's contents and
    # register both digests for that reason.
    #
    # `line=0x3d3` and `line=0x3da` are the two rejection sites in
    # PenetrationMap.cpp, reported by the oracle through the harness's own
    # NxUserOutputStream, so they pin which branch each malformed fixture took
    # rather than only that it was rejected.
    #
    # pmap.minimal_valid and pmap.boundary_res1_signclear carry the SAME grid
    # digest on purpose. The sign bit at the end of the stream only ORs
    # 0x80000000 into a cell, and a cell no value record touched is already
    # 0xffffffff, so the two fixtures differ in one bit of input and not at all
    # in output. Registering both is what would catch a reconstruction that made
    # them differ.
    'NxPhysicsAssetTests' = @(
        'asset fixtures pmap=14 mesh=6 writer=9 release=1',
        'asset rows pmap_create=phys_fn_002047 pmap_load=phys_fn_002035 mesh_header=phys_fn_002262 mesh_writer=phys_fn_002162 release_pmap=phys_fn_002051',
        'asset coverage driven=30 accepted=13 rejected=16 errors=10',
        'asset oracle digest=eaefc573',

        'pmap case=pmap.minimal_valid dimension=minimal_valid bytes=17 accepted=1 errors=0 line=0x000 resolution=1 cells=1 grid=e3160fb1',
        'pmap case=pmap.multi_value dimension=multi_element bytes=30 accepted=1 errors=0 line=0x000 resolution=2 cells=8 grid=5b517625',
        'pmap case=pmap.boundary_res4 dimension=boundary bytes=25 accepted=1 errors=0 line=0x000 resolution=4 cells=64 grid=06a34ac5',
        'pmap case=pmap.boundary_res1_signclear dimension=boundary bytes=17 accepted=1 errors=0 line=0x000 resolution=1 cells=1 grid=e3160fb1',
        'pmap case=pmap.bad_magic_0 dimension=malformed bytes=17 accepted=0 errors=1 line=0x3d3 resolution=0 cells=0 grid=00000000',
        'pmap case=pmap.bad_magic_1 dimension=malformed bytes=17 accepted=0 errors=1 line=0x3d3 resolution=0 cells=0 grid=00000000',
        'pmap case=pmap.bad_magic_2 dimension=malformed bytes=17 accepted=0 errors=1 line=0x3d3 resolution=0 cells=0 grid=00000000',
        'pmap case=pmap.bad_magic_3 dimension=malformed bytes=17 accepted=0 errors=1 line=0x3d3 resolution=0 cells=0 grid=00000000',
        'pmap case=pmap.bad_version_00000000 dimension=malformed bytes=17 accepted=0 errors=1 line=0x3da resolution=0 cells=0 grid=00000000',
        'pmap case=pmap.bad_version_00000003 dimension=malformed bytes=17 accepted=0 errors=1 line=0x3da resolution=0 cells=0 grid=00000000',
        'pmap case=pmap.bad_version_00000005 dimension=malformed bytes=17 accepted=0 errors=1 line=0x3da resolution=0 cells=0 grid=00000000',
        'pmap case=pmap.bad_version_ffffffff dimension=malformed bytes=17 accepted=0 errors=1 line=0x3da resolution=0 cells=0 grid=00000000',
        'pmap case=pmap.truncated_after_magic dimension=truncated bytes=84 accepted=0 errors=1 line=0x3da resolution=0 cells=0 grid=00000000',
        'pmap case=pmap.truncated_mid_tag dimension=truncated bytes=83 accepted=0 errors=1 line=0x3d3 resolution=0 cells=0 grid=00000000',

        # dwords_read is the measurement that says WHERE the reader stopped, not
        # only that it refused. A reader that validated both tags before
        # rejecting either would read two dwords for mesh.bad_tag0 and these
        # three lines would move.
        'mesh case=mesh.bad_tag0 dimension=malformed accepted=0 dwords_read=1',
        'mesh case=mesh.bad_tag0_zero dimension=malformed accepted=0 dwords_read=1',
        'mesh case=mesh.bad_tag0_byteswapped dimension=malformed accepted=0 dwords_read=1',
        'mesh case=mesh.bad_tag1 dimension=malformed accepted=0 dwords_read=2',
        'mesh case=mesh.bad_tag1_zero dimension=malformed accepted=0 dwords_read=2',
        'mesh case=mesh.truncated_after_tag0 dimension=truncated accepted=0 dwords_read=2',

        # The writer cases. `events` counts stored stream calls and `bytes`
        # counts the buffer bytes alone -- the dword and float stores carry
        # their values in the digest but not in this counter. The four
        # single_flag cases each move exactly one conditional arm; hull_present
        # and hull_mode_bit differ only in flags bit 2 versus bit 3, which is
        # the pair that catches a writer assembling the hull bits in the wrong
        # places.
        'writer case=writer.full dimension=everything accepted=1 events=19 bytes=334 digest=8ca61415',
        'writer case=writer.minimal dimension=nothing_optional accepted=1 events=15 bytes=64 digest=3abc793c',
        'writer case=writer.empty_mesh dimension=zero_counts accepted=1 events=15 bytes=16 digest=8ed70ae9',
        'writer case=writer.materials_only dimension=single_flag accepted=1 events=16 bytes=82 digest=66ef8a8f',
        'writer case=writer.remap_only dimension=single_flag accepted=1 events=16 bytes=88 digest=439a7c22',
        'writer case=writer.array_a_only dimension=single_flag accepted=1 events=16 bytes=88 digest=557261ac',
        'writer case=writer.array_b_only dimension=single_flag accepted=1 events=16 bytes=88 digest=e5919c82',
        'writer case=writer.hull_present dimension=hull accepted=1 events=15 bytes=148 digest=e80ef3eb',
        'writer case=writer.hull_mode_bit dimension=hull accepted=1 events=15 bytes=148 digest=68416241',

        'release case=null_data returned=1 data_size=5a5a5a5a data=00000000'
    )

    # The Phase 5 object-layout gate. RED on purpose until Tasks 2 and 3
    # transcribe the object-model classes; its own exit code fails the phase
    # until then, exactly the way the asset gate was born. Every line below is
    # a fact about the shipped DLL alone:
    #
    #   * eight `vt` digests over the loaded oracle's slot words -- the two
    #     actor tables in full (87 and 88 slots; the dynamic table's window
    #     includes the adjacent one-slot member table its ctor installs into
    #     the +8 subobject) and a twelve-slot window of every shape final plus
    #     the base-shape table;
    #   * `colobj`, phys_fn_001193 run on a poisoned 0x1c buffer with a marked
    #     argument: three vtables, a zeroed word, and the argument stored at
    #     both +8 and +0x18 -- the borrowed collision-object layout pinned
    #     byte for byte by the oracle's own constructor;
    #   * `owner`, the four-byte accessor phys_fn_001281 reading +0x04.
    'NxPhysicsObjectLayoutTests' = @(
        'vt name=actor_interface slots=87 digest=62936499',
        'vt name=actor_dynamic slots=88 digest=cdd44a90',
        'vt name=shape_base slots=12 digest=650a3f61',
        'vt name=box slots=12 digest=5daef065',
        'vt name=capsule slots=12 digest=ef7766c2',
        'vt name=plane slots=12 digest=2368c934',
        'vt name=sphere slots=12 digest=86cf5cb3',
        'vt name=mesh slots=12 digest=cef7ce84',
        'colobj ctor=phys_fn_001193 size=28 digest=e1df25e5 vptr_final=10107218 zero04=00000000 arg_at_8=a5a5a5a5 vptr_member=101072a0 arg_at_18=a5a5a5a5',
        'owner accessor=phys_fn_001281 mark=13579bdf returned=13579bdf',
        'hull row=phys_fn_000953..71 vertexCount=8 faceCount=6 zero=0 face2_offset=0xb8 vertices_offset=0x10',
        'hull static tables digest=f835c8c3',
        'hull support row=phys_fn_000975 min_bits=c19c0000 max_bits=4eada5a5',
        'hull sharedhook row=phys_fn_000985 stable=1 words=00000000.00000000.00000000',
        'shapebase ctor=phys_fn_001273 size=224 digest=de5f5000 zero08=00000000 pose_diag=3f800000 zero9c=00000000 zeroa0=00000000 prun24=ffffffff prun28=0000ffff sentinel_d0=7fffffff arg_d4=5a5a5a5a hw_dc=6 hw_de=8',
        'boxshape ctor=phys_fn_000977 size=552 digest=ac5ed12f sentinel_d0=2 arg_d4=5a5a5a5a dims=3f800000.3f800000.3f800000 face0_corners=00000000 face5_corners=00000000 verts_poison=cdcdcdcd floats_poison=cdcdcdcd colobj_ok=1',
        'sphere ctor=phys_fn_001349 size=228 digest=37ea7205 sentinel_d0=1 arg_d4=5a5a5a5a radius_e0=00000000 colobj_ok=1',
        'capsule ctor=phys_fn_000987 size=236 digest=ba7317e3 sentinel_d0=3 arg_d4=5a5a5a5a float_e0=00000000 float_e4=00000000 colobj_ok=1',
        'plane ctor=phys_fn_001247 size=268 digest=abed37e0 sentinel_d0=0 arg_d4=5a5a5a5a normal=00000000.3f800000.00000000 dist_ec=00000000 word108=1 tangent_f0=bf800000.00000000.00000000 binormal_fc=80000000.80000000.3f800000 colobj_ok=1',
        'mesh ctor=phys_fn_001379 size=232 digest=422a1f78 sentinel_d0=4 arg_d4=5a5a5a5a word_e0=00000000 word_e4=00000000 colobj_ok=1',
        'basevt slots=4:001249,5:004812,7:001035 ret4=0 ret5=00000000 ret7=0',
        'basesave row=phys_fn_001277 saved=1 digest=bc9dc964 pose_diag=3f800000 word38=00000008 word3c=00000000 word40=00000000 poison_head=cdcdcdcd',
        'boxrow slot10=phys_fn_000937 out=00000000.00000000.00000000.3fddb3d7',
        'boxrow2 slot11=phys_fn_000939 out=00000000.00000000.00000000.3fddb3d7 slot13=phys_fn_000927 saved=1 digest=853c971d dims_at_4c=3f800000',
        'boxrow3 slot8=phys_fn_000941 minmax=bf800000.bf800000.bf800000.3f800000.3f800000.3f800000 slot9=phys_fn_000935 minmax=bf800000.bf800000.bf800000.3f800000.3f800000.3f800000',
        'boxrow4 slots14-16=phys_fn_001391 stable=1',
        'planesave row=phys_fn_001251 saved=1 digest=7629841d normal_y=3f800000 neg_d=80000000',
        'meshword row=phys_fn_001381 mark_hit=1',
        'aabbrows sph9=phys_fn_001361 minmax=00000000.00000000.00000000.00000000.00000000.00000000 cap10=phys_fn_001001 cr=00000000.00000000.00000000.00000000 cap11=phys_fn_001003 cr=00000000.00000000.00000000.00000000',
        'meshrows slot13=phys_fn_001385 saved=1 digest=0ccd08b4 slot11=phys_fn_001387 words=0badf00d.00000000.00000000.13579bdf',
        'sphlocal row=phys_fn_001367 minmax=80000000.80000000.80000000.00000000.00000000.00000000',
        'setrad row=phys_fn_001357 stored=3fa00000 expected=3fa00000',
        'capsetrad row=phys_fn_000995 stored=3fc00000 expected=3fc00000',
        'sphdtor row=phys_fn_001375 digest=c4a35d15',
        'sphload row=phys_fn_001353 rad=40200000 group=0006',
        'setgroup row=phys_fn_001329 hw_d8=0007 expected=0007',
        'dtors2 plane=phys_fn_001263 digest=7ac6fe28 mesh=phys_fn_001399 digest=b3c6ab70',
        'meshwords44 row=phys_fn_001389 out=deadbeef.cafebabe.12345678.00000000.00000000.00000000',
        'ownerupd row=phys_fn_001315 noop=1',
        'capload row=phys_fn_000989 rad=3fc00000',
        'planeload row=phys_fn_001265 ny=3f800000',
        'meshload row=phys_fn_001383 bound=1',
        'boxload row=phys_fn_000981 stored=1',
        'massframe row=phys_fn_000851 mass_scaled=4302e653 mass_unit=4282e653 digest=0bed6c36',
        'boxmass row=phys_fn_000849 mass_scaled=42f00000 mass_unit=42700000 digest=d82de90e',
        'capmass row=phys_fn_000853 e=4287663e.4287663e.41f56fdb.421d1463 f=41756fdb.4207663e.4207663e.419d1463 digest=ab81bd0c',
        'paxis row=phys_fn_000831 s0=41660000 q0=c0ae0000 digest=1d701701',
        'errstream row=phys_fn_001357 invalid_fires=1 valid_fires=0 digest=c2ab04f6',
        'grouperr row=phys_fn_001329 invalid_fires=1 d8=00000005 c8=00000020 digest=b6f879ec',
        'loaderr row=sphere+capsule fires=1/1 rad=bf800000.bf800000 hh=00000000 digest=8f125103',
        'materialboot row=template flags=00000000 digest=dcc6a675',
        'mzero row=phys_fn_000847 zA=00000000 zB=42424242 digest=23206019',
        'owndtor row=oracle flag=00000002 clr=1/1/1 pop=0 mv=0 fl=1 pair=0 freed=0 digest=6d133744',
        # The remover chain decoded to its growth arms. vecgrow drives
        # phys_fn_000028 through both reallocs (capacity 2 -> 6 -> 14); the
        # allocation-stream deltas are over the emulator's need sizes.
        'vecgrow row=oracle proxy=c0c0c0c0 count=8 cap=14 elems=1 mops=3 mbytes=100 fops=2 fbytes=40 digest=6f1d9bd7',
        'vecgrow2 row=oracle noalloc=1 stored=1 digest=8eac5155',
        # phys_fn_002410 with the free vector AT capacity: grow mid-release,
        # duplicate push on a released slot, virgin sentinel (-1) skips the
        # push but STILL runs the unlink -- cntA[3] is rewritten and the
        # cursor pops twice. That quirk is this family's reason to exist.
        'relgrow row=oracle nopush=1 s37zero=1 poison=1 mv2=1 fl=10/18 dup=1 mops=1 mbytes=76 fops=1 fbytes=36 digest=dc548b5a',
        # phys_fn_002344 driven at the field-address contract the real dtor
        # uses (0x00026c13): middle removal, matched-last shrink, duplicates
        # falling in one pass, no-match, empty list.
        'pairrm row=oracle cases=5 digest=f0bdae43',
        # Task 4 actor scaffolding: eight guarded body reads on a fake actor
        # {+0x10 scene lock context, +0x14 body}. The guards run real
        # kernel32 primitives over a real CRITICAL_SECTION; the thread id is
        # folded only as a recorded/not-zeroed predicate because tids are
        # live state.
        'actorsm row=oracle digest=1bdc2fa8',
        'actorctor row=oracle ct=1/1/1/1/1 dd=1/1/1 adj=1/1/1 w=1/1/1 digest=19f4915a',
        # Slate 2: the record energy word -- whose cross terms carry their
        # scale factors SQUARED, the defect the first transcription missed --
        # the +0x84 zero test, and the write-guard flag writers including
        # their kind-2 deadlock-report arms.
        'actorsm2 row=oracle energy=42500000/42500000 digest=663451db',
        # Slate 3: the damping getters -- kind-1 warnings on a null record,
        # each folded immediately (a shared cap keeps only the last report).
        'actorsm3 row=oracle lin=3eb33333 ang=3e000000 digest=5bc8156a',
        # Slate 4: the guarded binding WRITE (slot 83) and the pose read
        # with body-default fallback (slot 6); value-equality predicates
        # fold because the binding key is each side's own body pointer.
        'actorsm4 row=oracle digest=0ecc5ea9',
        # Slate 5: the sleep chain -- recursive path compression over the
        # +0x1e8 caches, the +0x1fc group walk over the +0x84 words, and the
        # guarded wrapper; the compression store itself is folded.
        'actorsm5 row=oracle digest=75b57124',
        # Slate 6: the CMass local frame getters -- kind-1 static-actor
        # warnings with identity-plus-zero / shipped-identity defaults,
        # each warning folded per arm.
        'actorsm6 row=oracle digest=6f47ea3d',
        # Slate 7: five guarded three-word readers -- position, inertia
        # diagonal, linear velocity, angular velocity, linear momentum --
        # each with its own kind-1 static-actor warning folded per arm.
        'actorsm7 row=oracle digest=0b4b17e4',
        # Slate 8: the group WRITER (line 0x3cd contended report, readback
        # through slot 86), the sub-object forwarder driven via a planted
        # __thiscall sentinel, and the id allocator over both arms.
        'miscsm row=oracle digest=9460eb64',
        # Slate 9: readBodyFlag (dynamic-required warning on the static arm),
        # the member deleting dtor (linked-CRT release), and two bound-pool
        # deleting dtors with adapter releases and post-free vptr folds.
        'miscsm2 row=oracle digest=c4bc7155',
        # Slate 11: pool-class lifecycle rows -- chained dtor, its adjustor
        # thunk, and the CRT-free dtor; all on stack blocks with flags=0
        # (no allocator interaction).
        'slate11 row=oracle digest=2a145f64',
        # phys_fn_002379: virtual slot-1 wrapper -- four drives over two
        # receivers with distinct vtables: slot-1 dispatch (slot 0 silent),
        # receiver identity, exactly-once, discarded callback result.
        'slot1wrapper row=oracle cases=4 failures=0 digest=05167dcd',
        # phys_fn_002352: the +0x28 adjustor thunk into SdkContainer::empty --
        # owned buffer freed through the FOUNDATION global's adapter (not the
        # SDK holder), external buffer kept with entries surviving the shared
        # tail, null entries a no-op, all three clearing exactly {cap, count}.
        'addthunk row=oracle failures=0 digest=f9aac08b',
        # Slate 12: two leaf rows off a marked record -- the +0xde flag-bits
        # reader (phys_fn_001287) over five masks and the descriptor getter
        # (phys_fn_000931): translation, rotation, dims-at-+0xe4 into a
        # 15-word record. The getter's dims read is past the 0xe0 base
        # extent; the drive's buffer is 0xf0 bytes so the read is defined.
        'shapeleaf row=oracle digest=cb0e48f9',
        'ownctor row=oracle d4=00000003 sent_ok=1 mirror_ok=1 shp_ok=1 digest=641beb67',
        'material row=template flags=00000000 digest=527814f5',
        'capdtor row=phys_fn_001014 digest=7a376673',
        'capaabb row=phys_fn_001004 minmax=80000000.80000000.80000000.00000000.00000000.00000000',
        'layout coverage tables=8 colobj=1 owner=1 hull=1 shapebase=1 boxshape=1 sphere=1 capsule=1 plane=1 mesh=1 basevt=3 basesave=1 boxrow=6 planesave=1 sphererows=4 capsave=1 meshword=1 aabbrows=3 meshrows=2 sphlocal=1 setrad=1 capsetrad=1 planeext=1 sphdtor=1 capdtor=1 setgroup=1 dtors2=2 sphload=1 slot1wrapper=1 addthunk=1 shapeleaf=1',
        'layout oracle digest=16dceb3c',
        'hull support candidate ok=1 min_bits=c19c0000 max_bits=4eada5a5',
        'hull sharedhook candidate ok=1 stable=1',
        'basevt candidate ok=1 ret4=0 ret5=00000000 ret7=0',
        'basesave candidate ok=1 digest=bc9dc964',
        'boxrow candidate ok=1 digest=2f2dc4eb',
        'boxrow2 candidate ok=1 d11=2f2dc4eb d13=853c971d',
        'boxrow3 candidate ok=1 d8=8428d8b5 d9=8428d8b5',
        'boxrow4 candidate ok=1',
        'planesave candidate ok=1 digest=7629841d',
        'aabbrows candidate ok=1 d9=e2ba14a5 dc=0b2ae445',
        'meshrows candidate ok=1 d13=0ccd08b4 dw=966271e0',
        'sphlocal candidate ok=1 d8=33c61825',
        'dtors2 candidate ok=1 plane=7ac6fe28 mesh=b3c6ab70',
        'capaabb candidate ok=1 da=33c61825',
        'capdtor candidate ok=1 dc=7a376673',
        'setrad candidate ok=1 rad=3fa00000',
        'capsetrad candidate ok=1 rad=3fc00000',
        'setgroup candidate ok=1 hw_d8=0007',
        'sphload candidate ok=1 rad=40200000 group=0006',
        'ownerupd candidate ok=1',
        'massframe candidate ok=1 digest=0bed6c36',
        'boxmass candidate ok=1 digest=d82de90e',
        'capmass candidate ok=1 digest=ab81bd0c',
        'paxis candidate ok=1 digest=1d701701',
        'errstream candidate ok=1 digest=c2ab04f6',
        'grouperr candidate ok=1 digest=b6f879ec',
        'loaderr candidate ok=1 digest=8f125103',
        'material candidate ok=1 digest=527814f5',
        'ownctor candidate ok=1 digest=641beb67',
        'mzero candidate ok=1 digest=23206019'
        'materialboot candidate ok=1 digest=dcc6a675',
        'owndtor candidate ok=1 digest=6d133744',
        'vecgrow candidate ok=1 count=8 cap=14 elems=1 digest=6f1d9bd7/8eac5155',
        'relgrow candidate ok=1 nopush=1 s37zero=1 poison=1 mv2=1 fl=10/18 dup=1 digest=dc548b5a'
        'pairrm candidate ok=1 digest=f0bdae43',
        'actorsm candidate ok=1 digest=1bdc2fa8',
        'actorctor candidate ok=1 ct=1/1/1/1/1 dd=1/1/1 adj=1/1/1 w=1/1/1 digest=19f4915a',
        'actorsm2 candidate ok=1 energy=42500000/42500000 digest=663451db',
        'actorsm3 candidate ok=1 lin=3eb33333 ang=3e000000 digest=5bc8156a'
        'actorsm4 candidate ok=1 digest=0ecc5ea9'
        'actorsm5 candidate ok=1 digest=75b57124'
        'actorsm6 candidate ok=1 digest=6f47ea3d'
        'actorsm7 candidate ok=1 digest=0b4b17e4'
        'miscsm candidate ok=1 digest=9460eb64'
        'miscsm2 candidate ok=1 digest=c4bc7155'
        'slate11 candidate ok=1 digest=2a145f64'
        'slot1wrapper candidate cases=4 failures=0 mismatches=0 digest=05167dcd'
        'addthunk candidate failures=0 mismatches=0 digest=f9aac08b'
        'shapeleaf candidate ok=1 digest=cb0e48f9'
    )

    # The vendored third-party differential. Phase 4 vendors qhull 2003.1 and
    # OPCODE 1.3 instead of reconstructing them -- 722 census rows and 377,842
    # bytes -- and a vendored row that nothing runs is present, not proven. This
    # target is what makes the difference measurable.
    #
    # Two kinds of line, and both are facts about the shipped DLL alone:
    #
    #   * one per driven row family, truncated after `oracle=<digest>`. The
    #     digest folds the pinned DLL's own answers over that family's inputs, so
    #     it moves if the generator is degraded, if the inputs change, or if the
    #     oracle is not called. Nothing in the vendored tree can produce it.
    #   * the layout assertions, each carrying the address that measured it.
    #     These are where vendoring goes wrong SILENTLY: a stock OPC_BaseModel.h
    #     compiles, links, runs, and gives Model four vtable slots where the
    #     image has seven.
    #
    # `verdict=` is registered deliberately. Nine families are `exact`; two are
    # `divergent`, and those two are NOT claimed as proofs -- IceSegment.cpp is
    # stock, and what diverges is a 2003 x87 code generator against a 2026 one
    # rounding in different places. Their mismatch counts are pinned so that a
    # change in either direction, including toward zero, fails and has to be
    # explained. A `divergent` line that turned into `exact` would mean somebody
    # changed the comparison, not that the compilers agreed.
    'NxPhysicsThirdPartyTests' = @(
        'thirdparty libraries qhull=2003.1 opcode=1.3-standalone',

        # qhull. Pure library functions with no dependence on the `qh` global
        # state, so they can be driven without the arena at .data:0x00125080 --
        # which is a NovodeX row Task 2b owns.
        'thirdparty name=qh_crossproduct rva=0x0005eac0 owner=phys_fn_002465 source=geom2.c:50 words=24000 oracle=6a99847d mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qh_pointdist rva=0x0005fb70 owner=phys_fn_002505 source=geom2.c:1307 words=8000 oracle=74544940 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qh_maxabsval rva=0x0005f530 owner=phys_fn_002493 source=geom2.c:927 words=4000 oracle=2840cd56 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qh_rand rva=0x0005fe20 owner=phys_fn_002513 source=geom2.c:1566 words=3000 oracle=a150104e mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qh_set rva=0x0007f2a0 owner=phys_fn_003308 source=qset.c:561,765,861,1076 words=21000 oracle=fdf34663 mismatches=0 worst_ulp=0 verdict=exact',

        # OPCODE. `container` is the borrowed-buffer modification end to end:
        # the two guards are different tests and the family drives both, at
        # growth factors either side of zero and at exactly zero.
        'thirdparty name=container rva=0x000b4d70 owner=phys_fn_004836 source=Ice/IceContainer.cpp:40,81,97,153 words=3456 oracle=7abefb4c mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=container_copy rva=0x000b4f00 owner=phys_fn_004844 source=Ice/IceContainer.cpp:67 words=56 oracle=ea2f4a34 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=radixsort rva=0x000e32c0 owner=phys_fn_005157 source=Ice/IceRevisitedRadix.cpp:170,186,238,350 words=1254 oracle=469ea556 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=complete_pruning rva=0x000b4530 owner=phys_fn_004816 source=NovodexBoxPruning.cpp words=730 oracle=39d67cbd mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=segment_sqrdist.grid rva=0x000f0560 owner=phys_fn_005493 source=Ice/IceSegment.cpp:29 words=60000 oracle=e2c89342 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=segment_sqrdist.wide rva=0x000f0560 owner=phys_fn_005493 source=Ice/IceSegment.cpp:29 words=40000 oracle=4dbf889d mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=mesh_topology rva=0x000e8fd0 owner=phys_fn_005357 source=OPC_MeshInterface.cpp:178,228 words=1200 oracle=936f761e mismatches=0 worst_ulp=0 verdict=exact',

        # The layout modifications. Each is a number read off the disassembly at
        # the address in the line, and each is a way a stock header is wrong
        # without saying so.
        'layout sizeof_BuildSettings=20 expected=20 rva=0x000e92b0,0x000e91cc ok',
        'layout sizeof_OPCODECREATE=32 expected=32 rva=0x000e92b0 ok',
        'layout OPCODECREATE.mSettings=8 expected=8 rva=0x000e9129 ok',
        'layout OPCODECREATE.mDeserializeFrom=4 expected=4 rva=0x000e9122 ok',
        'layout BuildSettings.mLimit=0 expected=0 rva=0x000e9129 ok',
        'layout BuildSettings.mRules=4 expected=4 rva=0x000e92b2 ok',
        'layout sizeof_AABBTreeOfTrianglesBuilder=72 expected=72 rva=0x000e9100 ok',

        # The three added BuildSettings members and the 28 added
        # AABBTreeBuilder bytes, identified by the Task 2a fix pass, and the two
        # NovodeX statements in OPC_AABBTree.cpp driven rather than inspected.
        # The two `_is_float` lines are here because Task 2a declared both as
        # udword: same size, same offset, silently wrong value, and no offset
        # assertion in this file could see it.
        'layout AABBTreeBuilder.mSettings=4 expected=4 rva=0x000e9060 ok',
        'layout BuildSettings.mNovodeXExtendValue=12 expected=12 rva=0x000f0efe ok',
        'layout BuildSettings.mNovodeXExtendAxis=16 expected=16 rva=0x000f0ec0 ok',
        'layout BuildSettings.mNovodeXInflate=20 expected=20 rva=0x000f0f83 ok',
        'layout AABBTreeBuilder.mNovodeXRootBV=32 expected=32 rva=0x000f0ed7 ok',
        'layout AABBTreeBuilder.mNovodeXCaptureRootBV=56 expected=56 rva=0x000f1187 ok',
        'layout BuildSettings.mNovodeXExtendValue_is_float=1 expected=1 rva=0x000f0efe fld dword ptr [esi+0x0c] ok',
        'layout BuildSettings.mNovodeXInflate_is_float=1 expected=1 rva=0x000f0f8e fld dword ptr [esi+0x14] ok',
        'layout BuildSettings.defaults_are_off=1 expected=1 rva=0x000e92b0 ok',
        'layout AABBTree.defaults_root_min_y=0 expected=0 rva=0x000f0ec0 cmp edx,-1 skips the block ok',
        'layout AABBTree.defaults_root_max_y=1082130432 expected=1082130432 rva=0x000f0f83 test eax,eax skips the inflate ok',
        'layout AABBTree.extend_pulls_min=3225419776 expected=3225419776 rva=0x000f0efe fcomp [esi+edx*4+0x20]; 0x000f0f3b stores ok',
        'layout AABBTree.extend_leaves_max=1082130432 expected=1082130432 rva=0x000f0f47 fcomp [esi+edx*4+0x2c] not taken ok',
        'layout AABBTree.capture_latch_cleared=0 expected=0 rva=0x000f0efa mov byte ptr [esi+0x38],0 ok',
        'layout AABBTree.captured_root_max_y=1082130432 expected=1082130432 rva=0x000f0ed7 lea eax,[esi+0x20] + six moves ok',
        'layout AABBTree.inflate_min=3204448256 expected=3204448256 rva=0x000f0f8e fld [esi+0x14]; fsub ok',
        'layout AABBTree.inflate_max=1083179008 expected=1083179008 rva=0x000f0ff5 fadd ok',
        'layout RayCollider.mMaxDist=132 expected=132 rva=0x000b5770 ok',
        'layout RayCollider.mClosestHit=140 expected=140 rva=0x000b579d ok',
        'layout sizeof_Container=16 expected=16 rva=0x000b4d70 ok',
        'layout sizeof_RadixSort=24 expected=24 rva=0x000e32c0 ok',
        # 48 in the image AND 48 in a stock compile -- which is what says
        # AABBTree itself is not modified. It is registered for that reason.
        'layout sizeof_AABBTree=48 expected=48 rva=0x000e919a ok',
        'layout sizeof_Segment=24 expected=24 rva=0x000f0560 ok',
        'layout MeshInterface.mTris=8 expected=8 rva=0x000e9012 ok',

        # The vtable shapes, asserted by DISPATCHING through the slot rather than
        # by counting: nothing in C++ says how many virtuals a class has, and a
        # stock header compiles, links and runs with the wrong number. The tree
        # is given seven nodes first so the comparison is 196 rather than 0 -- an
        # earlier version compared 0 against 0 and would have passed against a
        # vtable with no slot 7 in it at all.
        'layout AABBOptimizedTree.GetUsedBytes_slot7=196 expected=196 rva=0x000e90c0 jmp [eax+0x1c] ok',
        'layout AABBOptimizedTree.added_slot4=0 expected=0 rva=0x000e9420 jmp [edx+0x10] ok',
        'layout BaseModel.GetUsedBytes_slot2=196 expected=196 rva=.rdata:0x0011bac8 ok',
        'layout BaseModel.added_slot4=0 expected=0 rva=0x000e9420 ok',

        # P4 Task 2b. The NovodeX rows inside the OPCODE span -- the only
        # families here whose candidate side is a RECONSTRUCTION rather than a
        # vendored source, which is why every one of them was broken on purpose
        # before it was registered. `prunable_pruner` is the one that needed the
        # harness fixed rather than the reconstruction: driving it with the
        # world-AABB callback always installed made moving `mFlags |= 2` inside
        # that callback's own guard come out green.
        'thirdparty name=prunable_ctor rva=0x000b54a0 owner=phys_fn_004874 source=IcePrunable.cpp words=42 oracle=676afd9a mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=prunable_flags rva=0x000b54f0 owner=phys_fn_004876 source=IcePrunable.cpp words=5760 oracle=849bed75 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=prunable_pruner rva=0x000b5590 owner=phys_fn_004884 source=IcePrunable.cpp words=20900 oracle=75538179 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=prunable_ranges rva=0x000b55e0 owner=phys_fn_004888 source=IcePrunable.cpp:152,174 words=304 oracle=f3da15ff mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=radix_setrankbuffers rva=0x000e3ea0 owner=phys_fn_005177 source=Ice/IceRevisitedRadix.h words=56 oracle=d8046a75 mismatches=0 worst_ulp=0 verdict=exact',

        # IcePrunable.cpp's layout, read off the instructions named in each line.
        # These are not modifications a stock header gets wrong -- there is no
        # stock header -- but the consequence is identical: one wrong offset and
        # the five families above compare the wrong bytes.
        'layout sizeof_AABB=24 expected=24 rva=0x000b55a6 lea eax,[eax+eax*2]; lea eax,[edx+eax*8] ok',
        'layout Prunable.mOwner=4 expected=4 rva=0x000b55c0 ok',
        'layout Prunable.mFlags=8 expected=8 rva=0x000b54f0 ok',
        'layout Prunable.mMember0C=12 expected=12 rva=0x000b54a9 ok',
        'layout Prunable.mMember0C.mPrunable=16 expected=16 rva=0x000b54e4 ok',
        'layout Prunable.mPruner=32 expected=32 rva=0x000b559d ok',
        'layout Prunable.mPrunable24=36 expected=36 rva=0x000b54c3 ok',
        'layout Prunable.mHandle=40 expected=40 rva=0x000b5590 ok',
        'layout Prunable.mPruningType=42 expected=42 rva=0x000b55ed ok',
        'layout Prunable.mPruningSection=43 expected=43 rva=0x000b561d ok',
        'layout sizeof_Prunable0C=20 expected=20 rva=0x000e7330,0x000b54a9 ok',
        'layout Pruner.mWorldBoxes=20 expected=20 rva=0x000b55a0 ok',

        'thirdparty coverage driven=17 divergent=0 words=193758 layout_checks=47',
        'thirdparty oracle digest=74ebc669',

        # Vendored correspondence, Task 4: execution evidence for the matched
        # vendored groups the families above never reached -- OPCODE's model
        # build, every collider over every tree kind, the vanilla AABBTree,
        # SweepAndPrune, the ICE maths, and qhull's hull. The colliders query the
        # ORACLE-built models on both sides, so they compare the colliders and
        # not the builds. The two summary lines above are printed where they
        # always were; the pair at the end of this block carries the totals.
        #
        # `verdict=exact` families are registered whole. A `divergent` family is
        # registered only up to its oracle digest: what it diverges by is the
        # candidate's own behaviour, and pinning it would register candidate-only
        # output. Each is attributed in evidence/vendored-correspondence.md
        # (Task 4): the summation-order and register-lifetime work unit (the
        # three-product sums among them are in sum_grouping.csv) for *_x87,
        # ice_plane_triangle, ice_matrix4x4 and ice_obb; inputs placed exactly on a boundary, where that same last bit
        # decides a hit, for *_boundary; qhull's own random rotation ("QR1"),
        # which rounds differently and so changes the merges after it, for
        # qhull_hull_rotated. `layout_checks=` in the second coverage line is the
        # same 47: the new families add no layout assertion.
        'thirdparty name=opcode_model_build rva=0x000e9100 owner=phys_fn_005368 source=OPC_Model.cpp,OPC_BaseModel.cpp,OPC_AABBTree.cpp,OPC_OptimizedTree.cpp,OPC_TreeBuilders.cpp words=22607 oracle=737affc0 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_model_build_x87 rva=0x000f09b0 owner=phys_fn_005513 source=OPC_AABBTree.cpp,OPC_TreeBuilders.cpp,OPC_OptimizedTree.cpp words=19406 oracle=902f1257',
        'thirdparty name=opcode_ray rva=0x000ba6f0 owner=phys_fn_004932 source=OPC_RayCollider.cpp words=2585 oracle=45be8f49 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_ray_x87 rva=0x000b84c0 owner=phys_fn_004921 source=OPC_RayCollider.cpp,OPC_RayTriOverlap.h words=525 oracle=24e93b36',
        'thirdparty name=opcode_ray_boundary rva=0x000b9070 owner=phys_fn_004925 source=OPC_RayCollider.cpp,OPC_RayAABBOverlap.h words=3284 oracle=37e0cc32',
        'thirdparty name=opcode_sphere rva=0x000e1360 owner=phys_fn_005105 source=OPC_SphereCollider.cpp,OPC_SphereTriOverlap.h words=9103 oracle=961ae915 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_obb rva=0x000de0d0 owner=phys_fn_005067 source=OPC_OBBCollider.cpp,OPC_BoxBoxOverlap.h,OPC_TriBoxOverlap.h words=13635 oracle=e1061b14 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_aabb rva=0x000ef0d0 owner=phys_fn_005434 source=OPC_AABBCollider.cpp,OPC_TriBoxOverlap.h words=8589 oracle=1b68b5c5 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_lss rva=0x000d4b90 owner=phys_fn_005027 source=OPC_LSSCollider.cpp,OPC_LSSAABBOverlap.h,OPC_LSSTriOverlap.h words=11986 oracle=7ad8f2f6 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_planes rva=0x000e2b60 owner=phys_fn_005138 source=OPC_PlanesCollider.cpp,OPC_PlanesAABBOverlap.h,OPC_PlanesTriOverlap.h words=12599 oracle=831f1266 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_treecollider rva=0x000d13c0 owner=phys_fn_004986 source=OPC_TreeCollider.cpp,OPC_TriTriOverlap.h,OPC_TriBoxOverlap.h,OPC_BoxBoxOverlap.h words=5432 oracle=e95063d8 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_treecollider_boundary rva=0x000bbd60 owner=phys_fn_004948 source=OPC_TreeCollider.cpp,OPC_TriTriOverlap.h words=872 oracle=c12dbe7c',
        'thirdparty name=opcode_aabbtree rva=0x000f10c0 owner=phys_fn_005523 source=OPC_AABBTree.cpp,OPC_TreeBuilders.cpp words=4715 oracle=9078ecba mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_ray_vanilla rva=0x000ba880 owner=phys_fn_004934 source=OPC_RayCollider.cpp words=710 oracle=915a09b5 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_sphere_vanilla rva=0x000e14d0 owner=phys_fn_005107 source=OPC_SphereCollider.cpp words=1534 oracle=c0e95680 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_aabb_vanilla rva=0x000ef230 owner=phys_fn_005436 source=OPC_AABBCollider.cpp words=1334 oracle=65c5c51a mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_aabbtree_refit rva=0x000f11b0 owner=phys_fn_005525 source=OPC_AABBTree.cpp words=2874 oracle=269b1591 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_refit rva=0x000e9410 owner=phys_fn_005378 source=OPC_BaseModel.cpp,OPC_OptimizedTree.cpp words=17709 oracle=aa7a9145 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_sap rva=0x000e6ca0 owner=phys_fn_005283 source=OPC_SweepAndPrune.cpp words=674 oracle=c5135e72 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=ice_aabb rva=0x000e2d20 owner=phys_fn_005141 source=Ice/IceAABB.cpp words=7800 oracle=5b4cf6ae mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=ice_plane_triangle rva=0x000e31c0 owner=phys_fn_005155 source=Ice/IcePlane.cpp,Ice/IceTriangle.cpp words=4000 oracle=20b460cd',
        'thirdparty name=ice_indexedtriangle rva=0x000e4160 owner=phys_fn_005187 source=Ice/IceIndexedTriangle.cpp words=1000 oracle=4660350a mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=ice_matrix4x4 rva=0x000e4400 owner=phys_fn_005197 source=Ice/IceMatrix4x4.cpp words=6800 oracle=d0f85eff',
        'thirdparty name=ice_obb rva=0x000e4580 owner=phys_fn_005199 source=Ice/IceOBB.cpp words=10800 oracle=4f2c5548',
        'thirdparty name=qhull_hull rva=0x0007d180 owner=phys_fn_003234 source=qhull.c,poly.c,poly2.c,merge.c,geom.c,geom2.c,qset.c,mem.c,global.c words=58488 oracle=c824ff7f mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_hull_x87 rva=0x0005c5c0 owner=phys_fn_002425 source=geom.c,geom2.c,merge.c words=32036 oracle=20ffcbef',
        'thirdparty name=qhull_hull_rotated rva=0x0005ff40 owner=phys_fn_002520 source=geom2.c,qhull.c,poly.c,poly2.c,merge.c words=9431 oracle=0a2f0b05',
        'thirdparty coverage driven=44 divergent=9 words=464286 layout_checks=47',
        'thirdparty oracle digest=c16f0c0c',

        # Vendored correspondence, Task 5a: candidate-built trees queried by the
        # candidate's colliders, against the oracle's trees and colliders over the
        # same inputs. The exact family (the volume colliders and tree pairs on the
        # models opcode_model_build builds exactly) is registered whole. The rays on
        # those models, and everything on the quantized and tied models, are
        # DIVERGENT, registered up to the oracle digest, and held by the harness's
        # recorded ceilings (kDivergentCeilings); the rays diverge by the collider's
        # own last bit, not the tree's (evidence/vendored-correspondence.md, Task 5a).
        # The pair above keeps printing where Task 4 put it; the pair below carries
        # the totals.
        'thirdparty name=opcode_candidate_trees rva=0x000e9100 owner=phys_fn_005368 source=OPC_Model.cpp,OPC_TreeBuilders.cpp,OPC_OptimizedTree.cpp,OPC_SphereCollider.cpp,OPC_OBBCollider.cpp,OPC_AABBCollider.cpp,OPC_LSSCollider.cpp,OPC_PlanesCollider.cpp,OPC_TreeCollider.cpp words=3674 oracle=1474f5d6 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=opcode_candidate_trees_ray rva=0x000ba6f0 owner=phys_fn_004932 source=OPC_RayCollider.cpp,OPC_RayAABBOverlap.h,OPC_RayTriOverlap.h words=292 oracle=f9c26128',
        'thirdparty name=opcode_candidate_trees_x87 rva=0x000f09b0 owner=phys_fn_005513 source=OPC_AABBTree.cpp,OPC_TreeBuilders.cpp,OPC_OptimizedTree.cpp words=18694 oracle=b7ff4dc5',
        'thirdparty coverage driven=47 divergent=11 words=486946 layout_checks=47',
        'thirdparty oracle digest=5f87aa37',
        # qhull-gap Task 1: qhull's output, trace, option and merge paths, run through
        # the NovodeX driver sequence with qhull's printing captured on both sides
        # (the host object's slots, and each side's own CRT through files its own
        # fopen opened; evidence/qhull-gap.md). Each family is a discrete tape and a
        # float tape. The discrete families that compare exactly are registered
        # whole; the float families, qhull_paths and qhull_rotation are DIVERGENT,
        # registered up to the oracle digest, and held by kDivergentCeilings. The
        # pairs above keep printing where Tasks 4 and 5a put them; the pair below
        # carries the totals.
        'thirdparty name=qhull_output rva=0x0006d800 owner=phys_fn_002866 source=io.c,geom2.c,poly2.c,stat.c words=113021 oracle=6f5465b7 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_output_x87 rva=0x0006d800 owner=phys_fn_002866 source=io.c,geom.c,geom2.c words=54558 oracle=9dc801f7',
        'thirdparty name=qhull_output_dims rva=0x0006d200 owner=phys_fn_002862 source=io.c,geom2.c,poly2.c,stat.c words=59282 oracle=cfd26e23 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_output_dims_x87 rva=0x0006d200 owner=phys_fn_002862 source=io.c,geom.c,geom2.c words=27902 oracle=f1b8f0e7',
        'thirdparty name=qhull_output_delaunay rva=0x00066de0 owner=phys_fn_002703 source=io.c,geom2.c,poly2.c words=37707 oracle=4fca2cfe mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_output_delaunay_x87 rva=0x00066de0 owner=phys_fn_002703 source=io.c,geom.c,geom2.c words=17498 oracle=4320756f',
        'thirdparty name=qhull_output_delaunay3 rva=0x00066de0 owner=phys_fn_002703 source=io.c,geom2.c,poly2.c words=51622 oracle=0aadd5bb mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_output_delaunay3_x87 rva=0x00066de0 owner=phys_fn_002703 source=io.c,geom.c,geom2.c words=23590 oracle=bc696174',
        'thirdparty name=qhull_trace rva=0x0007d180 owner=phys_fn_003234 source=qhull.c,poly.c,poly2.c,merge.c,geom.c,geom2.c,io.c,qset.c,mem.c,global.c words=37863 oracle=d0697d45 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_trace_x87 rva=0x0007d180 owner=phys_fn_003234 source=qhull.c,poly.c,poly2.c,merge.c,geom.c,geom2.c,io.c words=16754 oracle=07d0eefc',
        'thirdparty name=qhull_options rva=0x000626f0 owner=phys_fn_002587 source=global.c,qhull.c,poly.c,poly2.c,merge.c,geom2.c words=35342 oracle=6e73c257 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_options_x87 rva=0x000626f0 owner=phys_fn_002587 source=global.c,merge.c,geom.c,geom2.c words=29002 oracle=675ae947',
        'thirdparty name=qhull_merge rva=0x0007d180 owner=phys_fn_003234 source=qhull.c,poly.c,poly2.c,merge.c,qset.c words=84575 oracle=a0e11bc1 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_merge_x87 rva=0x0007d180 owner=phys_fn_003234 source=geom.c,geom2.c,merge.c words=77301 oracle=3639a851',
        'thirdparty name=qhull_random rva=0x00061490 owner=phys_fn_002550 source=geom2.c,global.c,qhull.c,merge.c words=10722 oracle=ba2b5e75 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_random_x87 rva=0x00061490 owner=phys_fn_002550 source=geom2.c,geom.c words=7184 oracle=1baa6368',
        'thirdparty name=qhull_direct rva=0x00068ce0 owner=phys_fn_002779 source=io.c,geom2.c,poly2.c,stat.c,qset.c words=127485 oracle=2cf4876b mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_direct_x87 rva=0x00068ce0 owner=phys_fn_002779 source=io.c,geom.c,geom2.c words=22629 oracle=3e023a50',
        'thirdparty name=qhull_paths rva=0x0005c5c0 owner=phys_fn_002425 source=geom.c,qhull.c,poly2.c,merge.c,io.c words=26445 oracle=c2d1ad84',
        'thirdparty name=qhull_paths_x87 rva=0x0005c5c0 owner=phys_fn_002425 source=geom.c,geom2.c,merge.c words=17625 oracle=f493b1a6',
        'thirdparty name=qhull_rotation rva=0x0005fec0 owner=phys_fn_002518 source=geom2.c,global.c,qhull.c,merge.c words=9980 oracle=e80e1851',
        'thirdparty name=qhull_rotation_x87 rva=0x0005fec0 owner=phys_fn_002518 source=geom2.c,geom.c words=8724 oracle=07282fca',
        'thirdparty coverage driven=69 divergent=24 words=1383757 layout_checks=47',
        'thirdparty oracle digest=ef0868a5'
    )
}

# How many coverage assertions each phase must actually evaluate.
#
# This exists because the target lists above can be emptied. Removing
# 'NxPhysicsCollisionTests' from $NxPhaseOracleDifferentialTargets['3'] made
# run_phase_gate.ps1 skip the whole oracle-differential block AND the loop that
# applies the 37 registrations, and Phase 3 still reported status=pass with the
# python suite unchanged. The "nothing registered cannot be gated" guard did not
# see it, because that guard reads only the staged-pair list. That was the
# fifteenth gate in this program found unable to fail.
#
# A count is the fix rather than another list, because it fails on *both* ways
# of losing the assertions: emptying a target list drops the evaluations, and
# deleting the registrations drops them too. Lowering a floor to match requires
# editing this file and tools/tests/test_gate_targets.py, which pins the minimum
# independently, so the two edits have to appear together in a diff.
$NxPhaseCoverageFloor = [ordered] @{
    '1' = 0
    '2' = 0
    '3' = 103  # 18 for NxPhysicsKernelFuzzTests, 85 for NxPhysicsCollisionTests
    '4' = 159  # 34 for NxPhysicsAssetTests, 125 for NxPhysicsThirdPartyTests (67 + 29 from
               # vendored-correspondence Task 4 + 5 from its Task 5a + 24 from qhull-gap Task 1)
    '5' = 1849 # 126 object-layout, 1 shape-vtable and 1722 public actor/pruner/box/scene lines
               # (744 + 251 from NpActor.cpp completion Task 2, 136 from its Task 4, 142 from its
               # Task 5 and its review; RED on purpose:
               # vtables family open)
    '6' = 403  # 3 oracle-descriptor + 118 oracle-joint + 118 staged-pair-joint + 6 tangent
               # + 12 joint-allocator + 146 joint-slot
    '7' = 276  # the 118 + 12 + 146 STAGED-PAIR assertions; the oracle-differential assertions
               # belong to NxPhysicsJointDescTests and NxPhysicsJointTests, which phase 7
               # does not run
    '8' = 0
}

# The registry proper, written out rather than derived.
#
# These three lists used to be built by flattening the phase maps above, which
# made every "selected target is registered" check in the two runners compare a
# value against a set built from that value. Adding 'NxNotInTheRegistry' to a
# phase list printed `pass: selected oracle-differential target is registered`
# and `pass: test target is registered in gate_targets.ps1` -- three assertions
# that could not fire, of exactly the class this program keeps finding, in the
# file that describes finding them. A registry derived from the thing it checks
# is not a registry.
#
# So registering a target is two edits in this file and not one, the same shape
# as $NxPhaseCoverageFloor below: put the name on a phase list, and put it in
# the list of names this programme knows. A typo in either one fails.
$NxRegisteredTestTargets = @(
    'NxPhysicsActorLifecycleTests',
    'NxPhysicsActorNameTests',
    'NxPhysicsActorMetadataTests',
    'NxPhysicsActorBodyFlagTests',
    'NxPhysicsActorDynamicsTests',
    'NxPhysicsActorDynamicSetterTests',
    'NxPhysicsActorShapeMutationTests',
    'NxPhysicsActorMomentumTests',
    'NxPhysicsActorForceTests',
    'NxPhysicsActorCMassTests',
    'NxPhysicsDynamicFirstTests',
    'NxPhysicsEmptySceneTests',
    'NxPhysicsCoreClusterTests',
    'NxFoundationTangentTests',
    'NxPhysicsExportTests',
    'NxPhysicsGeometryTests',
    'NxPhysicsJointAllocatorTests',
    'NxPhysicsJointSlotTests',
    'NxPhysicsJointStagedPairTests',
    'NxPhysicsKernelFuzzTests',
    'NxPhysicsSDKTests'
)
$NxRegisteredStaticProofTargets = @(
    'NxPhysicsInternalTests'
)
$NxRegisteredOracleDifferentialTargets = @(
    'NxPhysicsAssetTests',
    'NxPhysicsCollisionTests',
    'NxPhysicsJointDescTests',
    'NxPhysicsJointTests',
    'NxPhysicsObjectLayoutTests',
    'NxPhysicsShapeVtableTests',
    'NxPhysicsThirdPartyTests'
)
$NxSkippedExitCode = 3
