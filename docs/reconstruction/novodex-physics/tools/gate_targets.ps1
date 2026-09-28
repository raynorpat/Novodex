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
    '5' = @('NxPhysicsActorLifecycleTests', 'NxPhysicsDynamicFirstTests', 'NxPhysicsEmptySceneTests', 'NxPhysicsActorNameTests', 'NxPhysicsActorMetadataTests', 'NxPhysicsActorBodyFlagTests', 'NxPhysicsActorDynamicsTests', 'NxPhysicsActorDynamicSetterTests', 'NxPhysicsActorMomentumTests', 'NxPhysicsActorForceTests', 'NxPhysicsActorCMassTests', 'NxPhysicsActorShapeMutationTests', 'NxPhysicsBodyCreationTests')
    '6' = @('NxPhysicsJointStagedPairTests', 'NxFoundationTangentTests', 'NxPhysicsJointAllocatorTests', 'NxPhysicsJointSlotTests', 'NxPhysicsEffectorTests', 'NxPhysicsCoreDumpTests')
    '7' = @('NxPhysicsJointStagedPairTests', 'NxPhysicsJointAllocatorTests', 'NxPhysicsJointSlotTests', 'NxPhysicsSceneRaycastTests', 'NxPhysicsSceneVisualizeTests', 'NxPhysicsEffectorTests', 'NxPhysicsCoreDumpTests')
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
        # Scene-raycast Task 4, shape: 000989's return, the mass-frame rows
        # 000829/000833 and BOX slot 7 (000951) against the oracle rows. The
        # oracle side's lines, appended as their own statement.
        'shape vtable capsule_load_return oracle=1 candidate=1',
        'shape vtable massframe oracle_digest=7c450cef cases=201 failures=0',
        'shape vtable boxsweep oracle_digest=2c5d5c09 cases=84 failures=0'
        # Scene-raycast Task 4, box hull: 000973 and BOX slot 12 (000981), facade
        # slots 9 and 10 (000957, 000959) against the oracle rows. The oracle
        # side's line, appended as its own statement.
        'box hull oracle_digest=e0477220 cases=314 failures=0'
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
        # The shape rows (scene-raycast Task 4, shape: 000933 through 001309 on
        # dynamic and static owners, 000935 on a rotated local pose, 000993 and
        # 000995 on a capsule). The oracle's lines, appended as their own
        # statement so the line above is not edited.
        'setter geometry_actor=0.1',
        'setter geometry_obb=0.0.4094cccd.bfd99999.40b80000.3f333333.3fa66666.40066666.3f2e3abc.beeb6ef9.3f11d0d9.3f24a368.3ba1120f.bf439d28.3eb22871.3f63410c.3e98f887',
        'setter geometry_bounds=0.0.40184c72.c070b7cc.406e6615.40dd7361.3eb8f194.40f8ccf5',
        'setter geometry_obb=0.1.40408db8.be99c434.40b6a61f.3f333333.3fa66666.40066666.3e4d9d93.bf634975.bed43861.3f79b0c5.3e095e90.3e3a9cb7.bdd7f68b.bee30a90.3f642177',
        'setter geometry_actor=1.1',
        'setter geometry_obb=1.0.40408db9.be99c438.40b6a61f.3f333333.3fa66666.40066666.3e4d9d94.bf634976.bed43860.3f79b0c5.3e095e92.3e3a9cb6.bdd7f687.bee30a90.3f642177',
        'setter geometry_capsule_actor=1',
        'setter geometry_capsule_dimensions=3e99999a.3fd9999a.3e99999a.3f59999a.80002',
        'setter geometry_capsule_radius=3ee66666.3ee66666.80002'
    )
    'NxPhysicsActorDynamicsTests' = @(
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
    # The body record's constructor and destructor rows (scene-raycast block
    # Task 4, body-creation: 000797, 000801, 000793/000795, 000722, 000748,
    # 000776/000799). Every line is the oracle's, from NxPhysicsBodyCreationTests
    # on the pinned pair: the record words the constructor chain writes for
    # default, live-SDK-parameter, explicit, zero-element-tensor and kinematic
    # descriptors (not the density bodies' tensor, which is 000008's), the record
    # ids and the release traffic (000799 frees the kinematic block).
    'NxPhysicsBodyCreationTests' = @(
        'bodycreate default_created=1',
        'bodycreate default_mass=41700000',
        'bodycreate default_inverse_mass=3d888889',
        'bodycreate default_sleep=3cb851ec.3ca0902e.42440000',
        'bodycreate default_damping=0.3d4ccccd',
        'bodycreate default_pose=0.0.0.0.0.0.3f800000',
        'bodycreate default_pose_copy=0.0.0.0.0.0.3f800000',
        'bodycreate default_velocity=0.0.0.0.0.0.3ecccccc',
        'bodycreate default_velocity_copy=0.0.0.0.0.0.3ecccccc',
        'bodycreate default_base_zero=0.0.0.0.0.0.0.0.0.0.0.0',
        'bodycreate default_mass_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate default_flags=100.4.0',
        'bodycreate default_frame=0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate default_stamp=2',
        'bodycreate default_zero=0.0.0.0.0.0.1',
        'bodycreate default_island=0.0.1.3ecccccc',
        'bodycreate default_island_tail=0.0.0.0',
        'bodycreate default_group=0.0.1.3ecccccc.0',
        'bodycreate default_saved_frame=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate default_bounds=7f7fffff.7f7fffff.7f7fffff.ff7fffff.ff7fffff.ff7fffff',
        'bodycreate default_links=body.self.0.self.self.0.self.0.0.0.0.0',
        'bodycreate default_getters=3e19999a.3e0f5c29.41700000',
        'bodycreate sdk_parameters_set=1.1.1',
        'bodycreate live_created=1',
        'bodycreate live_mass=41a00000',
        'bodycreate live_inverse_mass=3d4ccccd',
        'bodycreate live_sleep=3eb851ec.3d800000.41f20000',
        'bodycreate live_damping=0.3d4ccccd',
        'bodycreate live_pose=3fc00000.c0100000.40480000.3de9f140.be1bf62a.3e81f7ce.3f72c557',
        'bodycreate live_pose_copy=3fc00000.c0100000.40480000.3de9f140.be1bf62a.3e81f7ce.3f72c557',
        'bodycreate live_velocity=0.0.0.0.0.0.3ecccccc',
        'bodycreate live_velocity_copy=0.0.0.0.0.0.3ecccccc',
        'bodycreate live_base_zero=0.0.0.0.0.0.0.0.0.0.0.0',
        'bodycreate live_mass_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate live_flags=100.4.0',
        'bodycreate live_frame=3de9f140.be1bf62a.3e81f7ce.3f72c557.3f5321a9.bf0428d1.be6c6b58.3ee4b020.3f5853e1.be968434.3eb1982e.3e0eac62.3f6d7138.3fc00000.c0100000.40480000',
        'bodycreate live_stamp=2',
        'bodycreate live_zero=0.0.0.0.0.0.1',
        'bodycreate live_island=0.0.1.3ecccccc',
        'bodycreate live_island_tail=0.0.0.0',
        'bodycreate live_group=0.0.1.3ecccccc.0',
        'bodycreate live_saved_frame=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate live_bounds=7f7fffff.7f7fffff.7f7fffff.ff7fffff.ff7fffff.ff7fffff',
        'bodycreate live_links=body.self.0.self.self.0.self.0.0.0.0.0',
        'bodycreate live_getters=3f19999a.3e800000.41a00000',
        'bodycreate full_created=1',
        'bodycreate full_mass=40200000',
        'bodycreate full_inverse_mass=3ecccccd',
        'bodycreate full_inertia=3f000000.3fc00000.40400000',
        'bodycreate full_inverse_inertia=40000000.3f2aaaab.3eaaaaab',
        'bodycreate full_sleep=3db851ec.3d23d70b.42a20000',
        'bodycreate full_damping=3e000000.3e99999a',
        'bodycreate full_pose=3fc00000.c0100000.40480000.3de9f140.be1bf62a.3e81f7ce.3f72c557',
        'bodycreate full_pose_copy=3fc00000.c0100000.40480000.3de9f140.be1bf62a.3e81f7ce.3f72c557',
        'bodycreate full_velocity=3f800000.c0000000.40600000.3e800000.bf000000.3f400000.3f333333',
        'bodycreate full_velocity_copy=3f800000.c0000000.40600000.3e800000.bf000000.3f400000.3f333333',
        'bodycreate full_base_zero=0.0.0.0.0.0.0.0.0.0.0.0',
        'bodycreate full_mass_pose=3f72f9bb.3ea00b20.3d1c4b37.bea00b20.3f6ba634.3e7010b1.3d1c4b37.be7010b1.3f78ac79.3dcccccd.be4ccccd.3e99999a',
        'bodycreate full_flags=1.7.0',
        'bodycreate full_frame=3cbab08f.be2544a2.3da25bb7.3f7bc281.3f6f720d.be2733a5.bea0ae42.3e1822ea.3f7c842b.bd9034cd.3ea46176.3c9d909c.3f7265bc.3fcee824.c01d9a26.405a3ea4',
        'bodycreate full_world_inverse_inertia=3fe67057.3e36303d.3effb728.3e36303d.3f31c725.3dafc41d.3effb728.3dafc41d.3f01582c',
        'bodycreate full_stamp=2',
        'bodycreate full_zero=0.0.0.0.0.0.1',
        'bodycreate full_island=0.0.1.3ecccccc',
        'bodycreate full_island_tail=0.0.0.0',
        'bodycreate full_group=0.0.1.3ecccccc.0',
        'bodycreate full_saved_frame=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate full_bounds=7f7fffff.7f7fffff.7f7fffff.ff7fffff.ff7fffff.ff7fffff',
        'bodycreate full_links=body.self.0.self.self.0.self.0.0.0.0.0',
        'bodycreate full_getters=3e99999a.3e4ccccd.40200000',
        'bodycreate flat_created=1',
        'bodycreate flat_mass=3f800000',
        'bodycreate flat_inverse_mass=3f800000',
        'bodycreate flat_inertia=3f800000.0.40000000',
        'bodycreate flat_inverse_inertia=0.0.0',
        'bodycreate flat_sleep=3cb851ec.3ca0902e.42440000',
        'bodycreate flat_damping=0.3d4ccccd',
        'bodycreate flat_pose=0.0.0.0.0.0.3f800000',
        'bodycreate flat_pose_copy=0.0.0.0.0.0.3f800000',
        'bodycreate flat_velocity=0.0.0.0.0.0.3ecccccc',
        'bodycreate flat_velocity_copy=0.0.0.0.0.0.3ecccccc',
        'bodycreate flat_base_zero=0.0.0.0.0.0.0.0.0.0.0.0',
        'bodycreate flat_mass_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate flat_flags=100.4.0',
        'bodycreate flat_frame=0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate flat_world_inverse_inertia=0.0.0.0.0.0.0.0.0',
        'bodycreate flat_stamp=2',
        'bodycreate flat_zero=0.0.0.0.0.0.1',
        'bodycreate flat_island=0.0.1.3ecccccc',
        'bodycreate flat_island_tail=0.0.0.0',
        'bodycreate flat_group=0.0.1.3ecccccc.0',
        'bodycreate flat_saved_frame=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate flat_bounds=7f7fffff.7f7fffff.7f7fffff.ff7fffff.ff7fffff.ff7fffff',
        'bodycreate flat_links=body.self.0.self.self.0.self.0.0.0.0.0',
        'bodycreate flat_getters=3e19999a.3e0f5c29.3f800000',
        'bodycreate kinematic_created=1',
        'bodycreate kinematic_create_traffic=9.3 a.260.50.18.228.1c.260.20.c0.20 f.8.60.10',
        'bodycreate kinematic_mass=40400000',
        'bodycreate kinematic_inverse_mass=0',
        'bodycreate kinematic_inertia=3f800000.40000000.40800000',
        'bodycreate kinematic_inverse_inertia=0.0.0',
        'bodycreate kinematic_sleep=3cb851ec.3ca0902e.42440000',
        'bodycreate kinematic_damping=0.3d4ccccd',
        'bodycreate kinematic_pose=0.0.0.0.0.0.3f800000',
        'bodycreate kinematic_pose_copy=0.0.0.0.0.0.3f800000',
        'bodycreate kinematic_velocity=0.0.0.0.0.0.3ecccccc',
        'bodycreate kinematic_velocity_copy=0.0.0.0.0.0.3ecccccc',
        'bodycreate kinematic_base_zero=0.0.0.0.0.0.0.0.0.0.0.0',
        'bodycreate kinematic_mass_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate kinematic_flags=180.4.0',
        'bodycreate kinematic_frame=0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate kinematic_world_inverse_inertia=3f800000.0.0.0.3f000000.0.0.0.3e800000',
        'bodycreate kinematic_stamp=2',
        'bodycreate kinematic_zero=0.0.0.0.0.0.1',
        'bodycreate kinematic_island=0.0.1.3ecccccc',
        'bodycreate kinematic_island_tail=0.0.0.0',
        'bodycreate kinematic_group=0.0.1.3ecccccc.0',
        'bodycreate kinematic_saved_frame=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate kinematic_bounds=7f7fffff.7f7fffff.7f7fffff.ff7fffff.ff7fffff.ff7fffff',
        'bodycreate kinematic_links=body.self.0.self.self.0.self.other.0.0.0.0',
        'bodycreate kinematic_getters=3e19999a.3e0f5c29.40400000',
        'bodycreate kinematic_block=0',
        'bodycreate kinematic_flag=1',
        'bodycreate ids=0.1.2.3.4',
        'bodycreate release_traffic=3.5 a.20.8.8 f.10.18.260.1c.228',
        'bodycreate reused_created=1',
        'bodycreate reused_id=2',
        'bodycreate reused_mass=41200000',
        'bodycreate reused_inverse_mass=3dcccccd',
        'bodycreate reused_sleep=3cb851ec.3ca0902e.42440000',
        'bodycreate reused_damping=0.3d4ccccd',
        'bodycreate reused_pose=0.0.0.0.0.0.3f800000',
        'bodycreate reused_pose_copy=0.0.0.0.0.0.3f800000',
        'bodycreate reused_velocity=0.0.0.0.0.0.3ecccccc',
        'bodycreate reused_velocity_copy=0.0.0.0.0.0.3ecccccc',
        'bodycreate reused_base_zero=0.0.0.0.0.0.0.0.0.0.0.0',
        'bodycreate reused_mass_pose=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate reused_flags=100.4.0',
        'bodycreate reused_frame=0.0.0.3f800000.3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate reused_stamp=2',
        'bodycreate reused_zero=0.0.0.0.0.0.1',
        'bodycreate reused_island=0.0.1.3ecccccc',
        'bodycreate reused_island_tail=0.0.0.0',
        'bodycreate reused_group=0.0.1.3ecccccc.0',
        'bodycreate reused_saved_frame=3f800000.0.0.0.3f800000.0.0.0.3f800000.0.0.0',
        'bodycreate reused_bounds=7f7fffff.7f7fffff.7f7fffff.ff7fffff.ff7fffff.ff7fffff',
        'bodycreate reused_links=body.self.0.self.self.0.self.0.0.0.0.0',
        'bodycreate reused_getters=3e19999a.3e0f5c29.41200000',
        'bodycreate kinematic_release_traffic=0.6 a f.50.18.20.260.1c.228'
        # The setters (scene-raycast Task 4, setters: 000782, 000785/000787):
        # kinematic enter/leave on the zero-element tensor body and a mode-5
        # addForce on a drowsy body. The oracle's lines, appended as their own
        # statement so the line above is not edited.
        'bodycreate setters_enter_traffic=1.0 a.260 f',
        'bodycreate setters_enter_inverses=0.0.0.0',
        'bodycreate setters_enter_flags=180',
        'bodycreate setters_enter_block=0',
        'bodycreate setters_leave_traffic=0.1 a f.50',
        'bodycreate setters_leave_inverses=3f800000.3f800000.7f800000.3f000000',
        'bodycreate setters_leave_flags=100',
        'bodycreate setters_leave_block=0',
        'bodycreate setters_drowsy_created=1',
        'bodycreate setters_drowsy_wake=3e000000.3e000000',
        'bodycreate setters_mode5_wake=3ecccccc.3ecccccc',
        'bodycreate setters_mode5_accumulators=0.0.0.0.0.0.0.0.0.0.0.0',
        'bodycreate setters_mode5_velocity=0.0.0.0.0.0'
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
    # Scene-raycast block Task 3: the scene raycast differential. Every public
    # NxScene raycast (any/closest/all, bounds and shapes) over static and
    # dynamic box, sphere, capsule and plane shapes and two compounds: each
    # shapes type, collision groups, finite maximum distances, hint flags,
    # reports that stop the query, the error paths (non-unit direction, maximum
    # distance not positive), then after moving a dynamic actor and a static
    # shape, after releasing three actors and after adding one to a built
    # tree. Copied verbatim from the ORACLE side. Before this task the six
    # raycasts returned 0 on the candidate; the box's slot 5 (000949) differed
    # in the rotated box's impact words, every finite maximum distance missed
    # the static shapes (the vendored segment stab's float cross terms
    # overflowed against the plane's box), and a moved static shape was missed
    # at its new place (the static pruner's tree was never dropped).
    'NxPhysicsSceneRaycastTests' = @(
        'raycast create s_box created=1 shapes=1',
        'raycast create s_sphere created=1 shapes=1',
        'raycast create s_capsule created=1 shapes=1',
        'raycast create s_plane created=1 shapes=1',
        'raycast create s_hidden created=1 shapes=1',
        'raycast create s_rotated created=1 shapes=1',
        'raycast create d_box created=1 shapes=1',
        'raycast create d_sphere created=1 shapes=1',
        'raycast create d_capsule created=1 shapes=1',
        'raycast create d_rotated created=1 shapes=1',
        'raycast create s_compound created=1 shapes=2',
        'raycast create d_compound created=1 shapes=2',
        'raycast create s_late created=1',
        'raycast ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=1',
        'raycast ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=1',
        'raycast ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=s_box flags=00000013 shape=s_box impact=bf800000.00000000.00000000 distance=40800000',
        'raycast ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=s_box flags=00000017 shape=s_box impact=bf800000.00000000.00000000 normal=bf800000.00000000.00000000 distance=40800000',
        'raycast ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=5 calls=5',
        'raycast ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=1 flags=00000017 shape=sc_box impact=41ec0000.00000000.00000000 normal=bf800000.00000000.00000000 distance=420a0000 return=1',
        'raycast ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=2 flags=00000017 shape=s_rotated impact=41995556.b54d5556.00000000 normal=bf19999a.bf4ccccd.00000000 distance=41c15556 return=1',
        'raycast ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=3 flags=00000013 shape=s_capsule impact=41180000.00000000.00000000 distance=41680000 return=1',
        'raycast ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=4 flags=00000017 shape=s_sphere impact=40800000.00000000.00000000 normal=bf800000.00000000.00000000 distance=41100000 return=1',
        'raycast ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=5 flags=00000017 shape=s_box impact=bf800000.00000000.00000000 normal=bf800000.00000000.00000000 distance=40800000 return=1',
        'raycast ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=5',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=1',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=1',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=d_box flags=00000013 shape=d_box impact=bf000000.00000000.40a00000 distance=40900000',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=d_box flags=00000017 shape=d_box impact=bf000000.00000000.40a00000 normal=bf800000.00000000.00000000 distance=40900000',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=5 calls=5',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=1 flags=00000017 shape=dc_box impact=41ec0000.00000000.40a00000 normal=bf800000.00000000.00000000 distance=420a0000 return=1',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=2 flags=00000013 shape=d_capsule impact=41180000.00000000.40a00000 distance=41680000 return=1',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=3 flags=00000017 shape=d_rotated impact=419b0000.b4899999.40a00000 normal=bf4ccccc.3f19999a.00000000 distance=41c30000 return=1',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=4 flags=00000017 shape=d_box impact=bf000000.00000000.40a00000 normal=bf800000.00000000.00000000 distance=40900000 return=1',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=5 flags=00000017 shape=d_sphere impact=40880000.00000000.40a00000 normal=bf800000.00000000.00000000 distance=41140000 return=1',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=5',
        'raycast ray=x_rotated type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=0',
        'raycast ray=x_rotated type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=0',
        'raycast ray=x_rotated type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=00000000 flags=00000001 shape=00000000 distance_word=7f7fffff',
        'raycast ray=x_rotated type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=00000000 flags=00000001 shape=00000000 distance_word=7f7fffff',
        'raycast ray=x_rotated type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=0 calls=0',
        'raycast ray=x_rotated type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=0',
        'raycast ray=down_capsule type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=1',
        'raycast ray=down_capsule type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=1',
        'raycast ray=down_capsule type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=s_capsule flags=00000013 shape=s_capsule impact=41240000.3fc00000.00000000 distance=41080000',
        'raycast ray=down_capsule type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=s_capsule flags=00000013 shape=s_capsule impact=41240000.3fb76cf8.00000000 distance=41091261',
        'raycast ray=down_capsule type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=2 calls=2',
        'raycast ray=down_capsule type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=2',
        'raycast ray=up_plane type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=1',
        'raycast ray=up_plane type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=0',
        'raycast ray=up_plane type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=s_plane flags=00000013 shape=s_plane impact=40400000.c1200000.40400000 distance=00000000',
        'raycast ray=up_plane type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=00000000 flags=00000001 shape=00000000 distance_word=7f7fffff',
        'raycast ray=up_plane type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=1 calls=1',
        'raycast ray=up_plane type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=0',
        'raycast ray=inside_sphere type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=1',
        'raycast ray=inside_sphere type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=1',
        'raycast ray=inside_sphere type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=s_sphere flags=00000013 shape=s_sphere impact=40a00000.00000000.00000000 distance=00000000',
        'raycast ray=inside_sphere type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=s_sphere flags=00000017 shape=s_sphere impact=408ccccd.bf4ccccd.00000000 normal=bf199998.bf4cccce.00000000 distance=3f7fffff',
        'raycast ray=inside_sphere type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=1 calls=1',
        'raycast ray=inside_sphere type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=1',
        'raycast ray=inside_capsule type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=1',
        'raycast ray=inside_capsule type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=1',
        'raycast ray=inside_capsule type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=d_capsule flags=00000013 shape=d_capsule impact=41200000.00000000.40a00000 distance=00000000',
        'raycast ray=inside_capsule type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=d_capsule flags=00000013 shape=d_capsule impact=41200000.00000000.40900000 distance=3f000000',
        'raycast ray=inside_capsule type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=1 calls=1',
        'raycast ray=inside_capsule type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=1',
        'raycast ray=slant type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=1',
        'raycast ray=slant type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=1',
        'raycast ray=slant type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=d_box flags=00000013 shape=d_box impact=00000000.3f000000.40940000 distance=41160000',
        'raycast ray=slant type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=d_box flags=00000017 shape=d_box impact=00000000.3f000000.40940000 normal=00000000.3f800000.00000000 distance=41160000',
        'raycast ray=slant type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=2 calls=2',
        'raycast ray=slant type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=2',
        'raycast ray=miss_high type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=0',
        'raycast ray=miss_high type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=0',
        'raycast ray=miss_high type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=00000000 flags=00000001 shape=00000000 distance_word=7f7fffff',
        'raycast ray=miss_high type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=00000000 flags=00000001 shape=00000000 distance_word=7f7fffff',
        'raycast ray=miss_high type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=0 calls=0',
        'raycast ray=miss_high type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=0',
        'raycast ray=hidden type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=1',
        'raycast ray=hidden type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=1',
        'raycast ray=hidden type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=s_plane flags=00000013 shape=s_plane impact=41700000.c0a00000.00000000 distance=41700000',
        'raycast ray=hidden type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=s_plane flags=00000017 shape=s_plane impact=41700000.c0a00000.00000000 normal=00000000.3f800000.00000000 distance=41700000',
        'raycast ray=hidden type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=1 calls=1',
        'raycast ray=hidden type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=1',
        'raycast ray=compound_down type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=1 flags=00000017 shape=s_plane impact=41f00000.c0a00000.40a00000 normal=00000000.3f800000.00000000 distance=41700000 return=1',
        'raycast ray=compound_down type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=2 flags=00000017 shape=dc_box impact=41f00000.3f000000.40a00000 normal=00000000.3f800000.00000000 distance=41180000 return=1',
        'raycast ray=compound_down type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes on_hit=3 flags=00000017 shape=dc_sphere impact=41f00000.40200000.40a00000 normal=00000000.3f800000.00000000 distance=40f00000 return=1',
        'raycast ray=compound_down type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=3',
        'raycast ray=compound_down type=3 groups=00000800 max=7f7fffff hint=ffffffff all_shapes on_hit=1 flags=00000017 shape=dc_sphere impact=41f00000.40200000.40a00000 normal=00000000.3f800000.00000000 distance=40f00000 return=1',
        'raycast ray=compound_down type=3 groups=00000800 max=7f7fffff hint=ffffffff all_shapes result=0 calls=1',
        'raycast ray=compound_across type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=1 flags=00000013 shape=sc_box impact=41ec0000.00000000.00000000 distance=40900000 return=1',
        'raycast ray=compound_across type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=1 calls=1',
        'raycast after_release ray=compound_across type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=0 calls=0',
        'raycast ray=compound_high type=2 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=dc_sphere flags=00000017 shape=dc_sphere impact=41ec0000.40000000.40a00000 normal=bf800000.00000000.00000000 distance=40900000',
        'raycast ray=x_statics type=3 groups=00000002 max=7f7fffff hint=ffffffff all_shapes result=0 calls=1',
        'raycast ray=down_box type=3 groups=00000010 max=7f7fffff hint=ffffffff closest_shape result=s_plane flags=00000017 shape=s_plane impact=00000000.c0a00000.00000000 normal=00000000.3f800000.00000000 distance=41700000',
        'raycast ray=x_statics type=3 groups=ffffffff max=40e00000 hint=ffffffff all_shapes on_hit=1 flags=00000017 shape=s_box impact=bf800000.00000000.00000000 normal=bf800000.00000000.00000000 distance=40800000 return=1',
        'raycast ray=x_statics type=3 groups=ffffffff max=40e00000 hint=ffffffff all_shapes result=0 calls=1',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=40e00000 hint=ffffffff all_shapes on_hit=1 flags=00000017 shape=d_box impact=bf000000.00000000.40a00000 normal=bf800000.00000000.00000000 distance=40900000 return=1',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=40e00000 hint=ffffffff all_shapes result=0 calls=1',
        'raycast ray=down_box type=3 groups=ffffffff max=40e00000 hint=ffffffff all_shapes result=0 calls=0',
        'raycast ray=down_sphere type=3 groups=ffffffff max=40e00000 hint=ffffffff all_shapes result=0 calls=0',
        'raycast ray=x_statics type=3 groups=ffffffff max=40800000 hint=ffffffff closest_bounds result=s_box flags=00000013 shape=s_box impact=bf800000.00000000.00000000 distance=40800000',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=40800000 hint=ffffffff closest_bounds result=d_box flags=00000013 shape=d_box impact=bf000000.00000000.40a00000 distance=40900000',
        'raycast ray=down_box type=3 groups=ffffffff max=40800000 hint=ffffffff closest_bounds result=s_box flags=00000013 shape=s_box impact=00000000.3f800000.00000000 distance=41100000',
        'raycast ray=down_sphere type=3 groups=ffffffff max=40800000 hint=ffffffff closest_bounds result=d_sphere flags=00000013 shape=d_sphere impact=40a00000.3f400000.40a00000 distance=41140000',
        'raycast ray=inside_box type=3 groups=ffffffff max=40800000 hint=ffffffff closest_bounds result=s_box flags=00000013 shape=s_box impact=3e800000.00000000.00000000 distance=00000000',
        'raycast ray=diagonal type=3 groups=ffffffff max=40800000 hint=ffffffff closest_bounds result=s_box flags=00000013 shape=s_box impact=bf3ffffe.00000000.bf800000 distance=40700001',
        'raycast ray=x_statics type=3 groups=ffffffff max=3e800000 hint=ffffffff any_bounds result=0',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=3e800000 hint=ffffffff any_bounds result=0',
        'raycast ray=down_box type=3 groups=ffffffff max=3e800000 hint=ffffffff any_bounds result=1',
        'raycast ray=down_sphere type=3 groups=ffffffff max=3e800000 hint=ffffffff any_bounds result=1',
        'raycast ray=inside_box type=3 groups=ffffffff max=3e800000 hint=ffffffff any_bounds result=1',
        'raycast ray=diagonal type=3 groups=ffffffff max=3e800000 hint=ffffffff any_bounds result=0',
        'raycast ray=x_statics type=3 groups=ffffffff max=41180000 hint=ffffffff all_bounds result=2 calls=2',
        'raycast ray=x_dynamics type=3 groups=ffffffff max=41180000 hint=ffffffff all_bounds result=2 calls=2',
        'raycast ray=down_box type=3 groups=ffffffff max=41180000 hint=ffffffff all_bounds result=2 calls=2',
        'raycast ray=down_sphere type=3 groups=ffffffff max=41180000 hint=ffffffff all_bounds result=2 calls=2',
        'raycast ray=inside_box type=3 groups=ffffffff max=41180000 hint=ffffffff all_bounds result=3 calls=3',
        'raycast ray=diagonal type=3 groups=ffffffff max=41180000 hint=ffffffff all_bounds result=1 calls=1',
        'raycast ray=x_statics stop=1 all_bounds on_hit=1 flags=00000013 shape=sc_box impact=41ec0000.00000000.00000000 distance=420a0000 return=0',
        'raycast ray=x_statics stop=1 all_bounds result=1 calls=1',
        'raycast ray=x_statics stop=1 all_shapes on_hit=1 flags=00000017 shape=sc_box impact=41ec0000.00000000.00000000 normal=bf800000.00000000.00000000 distance=420a0000 return=0',
        'raycast ray=x_statics stop=1 all_shapes result=0 calls=1',
        'raycast ray=x_dynamics stop=1 all_bounds on_hit=1 flags=00000013 shape=dc_box impact=41ec0000.00000000.40a00000 distance=420a0000 return=0',
        'raycast ray=x_dynamics stop=1 all_bounds result=1 calls=1',
        'raycast ray=x_statics stop=2 all_shapes on_hit=1 flags=00000017 shape=sc_box impact=41ec0000.00000000.00000000 normal=bf800000.00000000.00000000 distance=420a0000 return=1',
        'raycast ray=x_statics stop=2 all_shapes on_hit=2 flags=00000017 shape=s_rotated impact=41995556.b54d5556.00000000 normal=bf19999a.bf4ccccd.00000000 distance=41c15556 return=0',
        'raycast ray=x_statics stop=2 all_shapes result=0 calls=2',
        'raycast ray=x_dynamics stop=2 all_shapes on_hit=1 flags=00000017 shape=dc_box impact=41ec0000.00000000.40a00000 normal=bf800000.00000000.00000000 distance=420a0000 return=1',
        'raycast ray=x_dynamics stop=2 all_shapes on_hit=2 flags=00000013 shape=d_capsule impact=41180000.00000000.40a00000 distance=41680000 return=0',
        'raycast ray=x_dynamics stop=2 all_shapes result=0 calls=2',
        'raycast report_error code=1 file=SceneRaycast.cpp line=327 message=NxRay direction not valid: must be unit vector.',
        'raycast report_error code=1 file=SceneRaycast.cpp line=345 message=NxRay direction not valid: must be unit vector.',
        'raycast report_error code=1 file=SceneRaycast.cpp line=411 message=NxRay direction not valid: must be unit vector.',
        'raycast report_error code=1 file=SceneRaycast.cpp line=443 message=NxRay direction not valid: must be unit vector.',
        'raycast report_error code=1 file=SceneRaycast.cpp line=368 message=NxRay direction not valid: must be unit vector.',
        'raycast report_error code=1 file=SceneRaycast.cpp line=386 message=NxRay direction not valid: must be unit vector.',
        'raycast ray=non_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=0',
        'raycast ray=non_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=0',
        'raycast ray=non_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=00000000 flags=cdcdcdcd shape=cdcdcdcd normal=cdcdcdcd.cdcdcdcd.cdcdcdcd face=cdcdcdcd distance_word=cdcdcdcd',
        'raycast ray=non_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=00000000 flags=cdcdcdcd shape=cdcdcdcd normal=cdcdcdcd.cdcdcdcd.cdcdcdcd face=cdcdcdcd distance_word=cdcdcdcd',
        'raycast ray=non_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=0 calls=0',
        'raycast ray=non_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=0',
        'raycast ray=zero_dir type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=0',
        'raycast ray=zero_dir type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=0',
        'raycast ray=zero_dir type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=00000000 flags=cdcdcdcd shape=cdcdcdcd normal=cdcdcdcd.cdcdcdcd.cdcdcdcd face=cdcdcdcd distance_word=cdcdcdcd',
        'raycast ray=zero_dir type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=00000000 flags=cdcdcdcd shape=cdcdcdcd normal=cdcdcdcd.cdcdcdcd.cdcdcdcd face=cdcdcdcd distance_word=cdcdcdcd',
        'raycast ray=zero_dir type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=0 calls=0',
        'raycast ray=zero_dir type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=0',
        'raycast ray=nearly_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=1',
        'raycast ray=nearly_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=1',
        'raycast ray=nearly_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=s_box flags=00000013 shape=s_box impact=bf800000.00000000.00000000 distance=40800000',
        'raycast ray=nearly_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=s_box flags=00000017 shape=s_box impact=bf800000.00000000.00000000 normal=bf800000.00000000.00000000 distance=40800000',
        'raycast ray=nearly_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=1 flags=00000013 shape=sc_box impact=41ec0000.00000000.00000000 distance=420a0000 return=1',
        'raycast ray=nearly_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=2 flags=00000013 shape=s_rotated impact=4190cccd.00000000.00000000 distance=41b8cccd return=1',
        'raycast ray=off_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=0',
        'raycast ray=off_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=0',
        'raycast ray=off_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=00000000 flags=cdcdcdcd shape=cdcdcdcd normal=cdcdcdcd.cdcdcdcd.cdcdcdcd face=cdcdcdcd distance_word=cdcdcdcd',
        'raycast ray=off_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=00000000 flags=cdcdcdcd shape=cdcdcdcd normal=cdcdcdcd.cdcdcdcd.cdcdcdcd face=cdcdcdcd distance_word=cdcdcdcd',
        'raycast ray=off_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=0 calls=0',
        'raycast ray=off_unit type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=0',
        'raycast ray=x_statics type=3 groups=ffffffff max=00000000 hint=ffffffff any_bounds result=0',
        'raycast ray=x_statics type=3 groups=ffffffff max=00000000 hint=ffffffff any_shape result=0',
        'raycast ray=x_statics type=3 groups=ffffffff max=00000000 hint=ffffffff closest_bounds result=00000000 flags=cdcdcdcd shape=cdcdcdcd normal=cdcdcdcd.cdcdcdcd.cdcdcdcd face=cdcdcdcd distance_word=cdcdcdcd',
        'raycast ray=x_statics type=3 groups=ffffffff max=00000000 hint=ffffffff closest_shape result=00000000 flags=cdcdcdcd shape=cdcdcdcd normal=cdcdcdcd.cdcdcdcd.cdcdcdcd face=cdcdcdcd distance_word=cdcdcdcd',
        'raycast ray=x_statics type=3 groups=ffffffff max=00000000 hint=ffffffff all_bounds result=0 calls=0',
        'raycast ray=x_statics type=3 groups=ffffffff max=00000000 hint=ffffffff all_shapes result=0 calls=0',
        'raycast ray=x_statics type=3 groups=ffffffff max=bf800000 hint=ffffffff any_bounds result=0',
        'raycast ray=x_statics type=3 groups=ffffffff max=bf800000 hint=ffffffff any_shape result=0',
        'raycast ray=x_statics type=3 groups=ffffffff max=bf800000 hint=ffffffff closest_bounds result=00000000 flags=cdcdcdcd shape=cdcdcdcd normal=cdcdcdcd.cdcdcdcd.cdcdcdcd face=cdcdcdcd distance_word=cdcdcdcd',
        'raycast ray=x_statics type=3 groups=ffffffff max=bf800000 hint=ffffffff closest_shape result=00000000 flags=cdcdcdcd shape=cdcdcdcd normal=cdcdcdcd.cdcdcdcd.cdcdcdcd face=cdcdcdcd distance_word=cdcdcdcd',
        'raycast ray=x_statics type=3 groups=ffffffff max=bf800000 hint=ffffffff all_bounds result=0 calls=0',
        'raycast ray=x_statics type=3 groups=ffffffff max=bf800000 hint=ffffffff all_shapes result=0 calls=0',
        'raycast moved d_sphere s_box',
        'raycast moved ray=x_moved_box type=1 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=1',
        'raycast moved ray=x_moved_box type=1 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=1',
        'raycast moved ray=x_moved_box type=1 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=s_box flags=00000013 shape=s_box impact=bf800000.00000000.40400000 distance=40800000',
        'raycast moved ray=x_moved_box type=1 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=s_box flags=00000017 shape=s_box impact=bf800000.00000000.40400000 normal=bf800000.00000000.00000000 distance=40800000',
        'raycast moved ray=x_moved_box type=1 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=1 flags=00000013 shape=s_box impact=bf800000.00000000.40400000 distance=40800000 return=1',
        'raycast moved ray=x_moved_box type=1 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=1 calls=1',
        'raycast moved ray=down_sphere type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=d_sphere flags=00000013 shape=d_sphere impact=40a00000.40100000.40a00000 distance=40f80000',
        'raycast moved ray=down_sphere type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=d_sphere flags=00000017 shape=d_sphere impact=40a00000.40100000.40a00000 normal=00000000.3f800000.00000000 distance=40f80000',
        'raycast released s_sphere d_box s_compound',
        'raycast after_release ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=1',
        'raycast after_release ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=1',
        'raycast after_release ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=s_capsule flags=00000013 shape=s_capsule impact=41180000.00000000.00000000 distance=41680000',
        'raycast after_release ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=s_capsule flags=00000013 shape=s_capsule impact=41180000.00000000.00000000 distance=41680000',
        'raycast after_release ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=1 flags=00000013 shape=s_rotated impact=4190cccd.00000000.00000000 distance=41b8cccd return=1',
        'raycast after_release ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=2 flags=00000013 shape=s_capsule impact=41180000.00000000.00000000 distance=41680000 return=1',
        'raycast after_release ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=1 flags=00000013 shape=dc_box impact=41ec0000.00000000.40a00000 distance=420a0000 return=1',
        'raycast after_release ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=2 flags=00000013 shape=d_capsule impact=41180000.00000000.40a00000 distance=41680000 return=1',
        'raycast after_release ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=3 flags=00000013 shape=d_rotated impact=41980000.00000000.40a00000 distance=41c00000 return=1',
        'raycast after_release ray=x_dynamics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds result=3 calls=3',
        'raycast after_release ray=compound_across type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=0',
        'raycast after_release ray=compound_across type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=0',
        'raycast after_release ray=compound_across type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=00000000 flags=00000001 shape=00000000 distance_word=7f7fffff',
        'raycast after_release ray=compound_across type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=00000000 flags=00000001 shape=00000000 distance_word=7f7fffff',
        'raycast after_release ray=compound_across type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_shapes result=0 calls=0',
        'raycast late ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_bounds result=1',
        'raycast late ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff any_shape result=1',
        'raycast late ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_bounds result=s_late flags=00000013 shape=s_late impact=40000000.00000000.00000000 distance=40e00000',
        'raycast late ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff closest_shape result=s_late flags=00000017 shape=s_late impact=40000000.00000000.00000000 normal=bf800000.00000000.00000000 distance=40e00000',
        'raycast late ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=1 flags=00000013 shape=s_rotated impact=4190cccd.00000000.00000000 distance=41b8cccd return=1',
        'raycast late ray=x_statics type=3 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=2 flags=00000013 shape=s_capsule impact=41180000.00000000.00000000 distance=41680000 return=1',
        'raycast late ray=x_statics type=1 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=1 flags=00000013 shape=s_rotated impact=4190cccd.00000000.00000000 distance=41b8cccd return=1',
        'raycast late ray=x_statics type=1 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=2 flags=00000013 shape=s_capsule impact=41180000.00000000.00000000 distance=41680000 return=1',
        'raycast late ray=x_statics type=1 groups=ffffffff max=7f7fffff hint=ffffffff all_bounds on_hit=3 flags=00000013 shape=s_late impact=40000000.00000000.00000000 distance=40e00000 return=1'
        # Scene-raycast Task 4, box hull: boxes created through the image's path
        # (000981 -> 000973) and resized through setDimensions (000983), their
        # dims, world bounds, hull words, facade slots 9/10 and raycasts. The
        # oracle side's lines, appended as their own statement.
        'box_resize s_resize grown is_box=1 get_dims=40000000.3f000000.40400000 world_bounds=42180000.bf000000.c0400000.42280000.3f000000.40400000',
        'box_resize d_resize grown is_box=1 get_dims=3f800000.40000000.3f400000 world_bounds=421c0000.c0000000.40880000.42240000.40000000.40b80000',
        'box_resize r_resize grown is_box=1 get_dims=40400000.3f000000.3fc00000 world_bounds=42173333.c02ccccc.41080000.4228cccd.402ccccc.41380000',
        'box_resize r_resize grown face=1 corners=00000004 list_a=00000001.00000005.00000006.00000002 list_b=00000001.00000008.00000005.00000009 plane=3f800000.00000000.00000000.c0400000 range=c0400000.40400000',
        'box_resize d_resize shrunk is_box=1 get_dims=3dcccccd.3e4ccccd.3e99999a world_bounds=421f999a.be4ccccd.40966666.42206666.3e4ccccd.40a9999a',
        'box_resize r_resize shrunk support dir=6 pose=1 face=4 feature_face=4 feature=00000000 bare=4',
        'raycast box_resize grown ray=x_resize_s type=3 groups=0000e000 max=7f7fffff hint=ffffffff closest_shape result=s_resize flags=00000017 shape=s_resize impact=42180000.00000000.00000000 normal=bf800000.00000000.00000000 distance=40400000',
        'raycast box_resize shrunk ray=x_resize_r type=3 groups=0000e000 max=7f7fffff hint=ffffffff closest_shape result=r_resize flags=00000017 shape=r_resize impact=421b5555.3e7ffff7.41200000 normal=bf19999a.bf4ccccc.00000000 distance=40755550'
    )
    # The scene visualisation differential (scene-raycast block Task 4,
    # visualisation: 000344 -> 000657 -> 000579/000020 -> 000766, the
    # Foundation's addBasis/addArrow/addOBB/addLine). Seven actors, the
    # visualisation parameters one at a time and together, a second call, the
    # scale back to 0, moved bodies and the Scene released. Copied verbatim from
    # the ORACLE side of the pinned pair: every create, stage-summary and
    # renderable-count line, every line of the world-axes, body-axes,
    # inertia-box and velocity stages, and the first three actors' actor-axes
    # lines. Before this task NxScene::visualize was a stub, and the candidate
    # Foundation's arrows and box corners differed from the oracle's by 1 ulp.
    'NxPhysicsSceneVisualizeTests' = @(
        'visualize empty renderables=0'
        'visualize create s_box created=1'
        'visualize create s_rotated created=1'
        'visualize create d_box created=1'
        'visualize create d_rotated created=1'
        'visualize create d_sphere created=1'
        'visualize create d_capsule created=1'
        'visualize create d_hidden created=1'
        'visualize created stage=scale_zero renderables=0'
        'visualize created stage=scale_only renderable=1 points=0 lines=0 triangles=0'
        'visualize created stage=scale_only renderables=1'
        'visualize created stage=world_axes renderable=1 points=0 lines=15 triangles=0'
        'visualize created stage=world_axes line=0 p0=00000000.00000000.00000000 p1=3fc00000.00000000.00000000 color=ffff0000'
        'visualize created stage=world_axes line=1 p0=3fc00000.00000000.00000000 p1=3fa33333.3e666667.00000000 color=ffff0000'
        'visualize created stage=world_axes line=2 p0=3fc00000.00000000.00000000 p1=3fa33333.be666667.00000000 color=ffff0000'
        'visualize created stage=world_axes line=3 p0=3fc00000.00000000.00000000 p1=3fa33333.00000000.3e666667 color=ffff0000'
        'visualize created stage=world_axes line=4 p0=3fc00000.00000000.00000000 p1=3fa33333.00000000.be666667 color=ffff0000'
        'visualize created stage=world_axes line=5 p0=00000000.00000000.00000000 p1=00000000.3fc00000.00000000 color=ff00ff00'
        'visualize created stage=world_axes line=6 p0=00000000.3fc00000.00000000 p1=be666667.3fa33333.00000000 color=ff00ff00'
        'visualize created stage=world_axes line=7 p0=00000000.3fc00000.00000000 p1=3e666667.3fa33333.00000000 color=ff00ff00'
        'visualize created stage=world_axes line=8 p0=00000000.3fc00000.00000000 p1=00000000.3fa33333.3e666667 color=ff00ff00'
        'visualize created stage=world_axes line=9 p0=00000000.3fc00000.00000000 p1=00000000.3fa33333.be666667 color=ff00ff00'
        'visualize created stage=world_axes line=10 p0=00000000.00000000.00000000 p1=00000000.00000000.3fc00000 color=ff0000ff'
        'visualize created stage=world_axes line=11 p0=00000000.00000000.3fc00000 p1=00000000.be666667.3fa33333 color=ff0000ff'
        'visualize created stage=world_axes line=12 p0=00000000.00000000.3fc00000 p1=00000000.3e666667.3fa33333 color=ff0000ff'
        'visualize created stage=world_axes line=13 p0=00000000.00000000.3fc00000 p1=3e666667.00000000.3fa33333 color=ff0000ff'
        'visualize created stage=world_axes line=14 p0=00000000.00000000.3fc00000 p1=be666667.00000000.3fa33333 color=ff0000ff'
        'visualize created stage=world_axes renderables=1'
        'visualize created stage=actor_axes renderable=1 points=0 lines=105 triangles=0'
        'visualize created stage=actor_axes line=0 p0=00000000.00000000.00000000 p1=3fc00000.00000000.00000000 color=ffff0000'
        'visualize created stage=actor_axes line=1 p0=3fc00000.00000000.00000000 p1=3fa33333.3e666667.00000000 color=ffff0000'
        'visualize created stage=actor_axes line=2 p0=3fc00000.00000000.00000000 p1=3fa33333.be666667.00000000 color=ffff0000'
        'visualize created stage=actor_axes line=3 p0=3fc00000.00000000.00000000 p1=3fa33333.00000000.3e666667 color=ffff0000'
        'visualize created stage=actor_axes line=4 p0=3fc00000.00000000.00000000 p1=3fa33333.00000000.be666667 color=ffff0000'
        'visualize created stage=actor_axes line=5 p0=00000000.00000000.00000000 p1=00000000.3fc00000.00000000 color=ff00ff00'
        'visualize created stage=actor_axes line=6 p0=00000000.3fc00000.00000000 p1=be666667.3fa33333.00000000 color=ff00ff00'
        'visualize created stage=actor_axes line=7 p0=00000000.3fc00000.00000000 p1=3e666667.3fa33333.00000000 color=ff00ff00'
        'visualize created stage=actor_axes line=8 p0=00000000.3fc00000.00000000 p1=00000000.3fa33333.3e666667 color=ff00ff00'
        'visualize created stage=actor_axes line=9 p0=00000000.3fc00000.00000000 p1=00000000.3fa33333.be666667 color=ff00ff00'
        'visualize created stage=actor_axes line=10 p0=00000000.00000000.00000000 p1=00000000.00000000.3fc00000 color=ff0000ff'
        'visualize created stage=actor_axes line=11 p0=00000000.00000000.3fc00000 p1=00000000.be666667.3fa33333 color=ff0000ff'
        'visualize created stage=actor_axes line=12 p0=00000000.00000000.3fc00000 p1=00000000.3e666667.3fa33333 color=ff0000ff'
        'visualize created stage=actor_axes line=13 p0=00000000.00000000.3fc00000 p1=3e666667.00000000.3fa33333 color=ff0000ff'
        'visualize created stage=actor_axes line=14 p0=00000000.00000000.3fc00000 p1=be666667.00000000.3fa33333 color=ff0000ff'
        'visualize created stage=actor_axes line=15 p0=40800000.00000000.00000000 p1=409ccccd.3f99999a.00000000 color=ffff0000'
        'visualize created stage=actor_axes line=16 p0=409ccccd.3f99999a.00000000 p1=4092b852.3f93d70b.00000000 color=ffff0000'
        'visualize created stage=actor_axes line=17 p0=409ccccd.3f99999a.00000000 p1=409e3d70.3f628f5d.00000000 color=ffff0000'
        'visualize created stage=actor_axes line=18 p0=409ccccd.3f99999a.00000000 p1=40987ae1.3f828f5c.3e666667 color=ffff0000'
        'visualize created stage=actor_axes line=19 p0=409ccccd.3f99999a.00000000 p1=40987ae1.3f828f5c.be666667 color=ffff0000'
        'visualize created stage=actor_axes line=20 p0=40800000.00000000.00000000 p1=40333333.3f666667.00000000 color=ff00ff00'
        'visualize created stage=actor_axes line=21 p0=40333333.3f666667.00000000 p1=4036147b.3f15c290.00000000 color=ff00ff00'
        'visualize created stage=actor_axes line=22 p0=40333333.3f666667.00000000 p1=40475c29.3f71eb86.00000000 color=ff00ff00'
        'visualize created stage=actor_axes line=23 p0=40333333.3f666667.00000000 p1=403eb852.3f43d70b.3e666667 color=ff00ff00'
        'visualize created stage=actor_axes line=24 p0=40333333.3f666667.00000000 p1=403eb852.3f43d70b.be666667 color=ff00ff00'
        'visualize created stage=actor_axes line=25 p0=40800000.00000000.00000000 p1=40800000.00000000.3fc00000 color=ff0000ff'
        'visualize created stage=actor_axes line=26 p0=40800000.00000000.3fc00000 p1=40800000.be666667.3fa33333 color=ff0000ff'
        'visualize created stage=actor_axes line=27 p0=40800000.00000000.3fc00000 p1=40800000.3e666667.3fa33333 color=ff0000ff'
        'visualize created stage=actor_axes line=28 p0=40800000.00000000.3fc00000 p1=40873333.00000000.3fa33333 color=ff0000ff'
        'visualize created stage=actor_axes line=29 p0=40800000.00000000.3fc00000 p1=4071999a.00000000.3fa33333 color=ff0000ff'
        'visualize created stage=actor_axes line=30 p0=00000000.3f800000.40a00000 p1=3fc00000.3f800000.40a00000 color=ffff0000'
        'visualize created stage=actor_axes line=31 p0=3fc00000.3f800000.40a00000 p1=3fa33333.3f9ccccd.40a00000 color=ffff0000'
        'visualize created stage=actor_axes line=32 p0=3fc00000.3f800000.40a00000 p1=3fa33333.3f466666.40a00000 color=ffff0000'
        'visualize created stage=actor_axes line=33 p0=3fc00000.3f800000.40a00000 p1=3fa33333.3f800000.40a73333 color=ffff0000'
        'visualize created stage=actor_axes line=34 p0=3fc00000.3f800000.40a00000 p1=3fa33333.3f800000.4098cccd color=ffff0000'
        'visualize created stage=actor_axes line=35 p0=00000000.3f800000.40a00000 p1=00000000.40200000.40a00000 color=ff00ff00'
        'visualize created stage=actor_axes line=36 p0=00000000.40200000.40a00000 p1=be666667.4011999a.40a00000 color=ff00ff00'
        'visualize created stage=actor_axes line=37 p0=00000000.40200000.40a00000 p1=3e666667.4011999a.40a00000 color=ff00ff00'
        'visualize created stage=actor_axes line=38 p0=00000000.40200000.40a00000 p1=00000000.4011999a.40a73333 color=ff00ff00'
        'visualize created stage=actor_axes line=39 p0=00000000.40200000.40a00000 p1=00000000.4011999a.4098cccd color=ff00ff00'
        'visualize created stage=actor_axes line=40 p0=00000000.3f800000.40a00000 p1=00000000.3f800000.40d00000 color=ff0000ff'
        'visualize created stage=actor_axes line=41 p0=00000000.3f800000.40d00000 p1=00000000.3f466666.40c8cccd color=ff0000ff'
        'visualize created stage=actor_axes line=42 p0=00000000.3f800000.40d00000 p1=00000000.3f9ccccd.40c8cccd color=ff0000ff'
        'visualize created stage=actor_axes line=43 p0=00000000.3f800000.40d00000 p1=3e666667.3f800000.40c8cccd color=ff0000ff'
        'visualize created stage=actor_axes line=44 p0=00000000.3f800000.40d00000 p1=be666667.3f800000.40c8cccd color=ff0000ff'
        'visualize created stage=actor_axes renderables=1'
        'visualize created stage=body_axes renderable=1 points=0 lines=12 triangles=0'
        'visualize created stage=body_axes line=0 p0=00000000.3f800000.40a00000 p1=3fc00000.3f800000.40a00000 color=00ff0000'
        'visualize created stage=body_axes line=1 p0=00000000.3f800000.40a00000 p1=00000000.40200000.40a00000 color=0000ff00'
        'visualize created stage=body_axes line=2 p0=00000000.3f800000.40a00000 p1=00000000.3f800000.40d00000 color=000000ff'
        'visualize created stage=body_axes line=3 p0=40a00000.40000000.40a00000 p1=40bccccd.404ccccc.40a00000 color=00ff0000'
        'visualize created stage=body_axes line=4 p0=40a00000.40000000.40a00000 p1=40733334.4039999a.40a00000 color=0000ff00'
        'visualize created stage=body_axes line=5 p0=40a00000.40000000.40a00000 p1=40a00000.40000000.40d00000 color=000000ff'
        'visualize created stage=body_axes line=6 p0=41200000.00000000.40a00000 p1=41380000.00000000.40a00000 color=00ff0000'
        'visualize created stage=body_axes line=7 p0=41200000.00000000.40a00000 p1=41200000.3fc00000.40a00000 color=0000ff00'
        'visualize created stage=body_axes line=8 p0=41200000.00000000.40a00000 p1=41200000.00000000.40d00000 color=000000ff'
        'visualize created stage=body_axes line=9 p0=41700000.40400000.c0000000 p1=41840000.40400000.c0000000 color=00ff0000'
        'visualize created stage=body_axes line=10 p0=41700000.40400000.c0000000 p1=41700000.4079999a.bf4cccce color=0000ff00'
        'visualize created stage=body_axes line=11 p0=41700000.40400000.c0000000 p1=41700000.3fe66667.bf8ccccc color=000000ff'
        'visualize created stage=body_axes renderables=1'
        'visualize created stage=mass_axes renderable=1 points=0 lines=48 triangles=0'
        'visualize created stage=mass_axes line=0 p0=ffc00000.ffc00000.ffc00000 p1=ffc00000.ffc00000.ffc00000 color=ffffffff'
        'visualize created stage=mass_axes line=1 p0=ffc00000.ffc00000.ffc00000 p1=ffc00000.ffc00000.ffc00000 color=ffffffff'
        'visualize created stage=mass_axes line=2 p0=ffc00000.ffc00000.ffc00000 p1=ffc00000.ffc00000.ffc00000 color=ffffffff'
        'visualize created stage=mass_axes line=3 p0=ffc00000.ffc00000.ffc00000 p1=ffc00000.ffc00000.ffc00000 color=ffffffff'
        'visualize created stage=mass_axes line=4 p0=ffc00000.ffc00000.ffc00000 p1=ffc00000.ffc00000.ffc00000 color=ffffffff'
        'visualize created stage=mass_axes line=5 p0=ffc00000.ffc00000.ffc00000 p1=ffc00000.ffc00000.ffc00000 color=ffffffff'
        'visualize created stage=mass_axes line=6 p0=ffc00000.ffc00000.ffc00000 p1=ffc00000.ffc00000.ffc00000 color=ffffffff'
        'visualize created stage=mass_axes line=7 p0=ffc00000.ffc00000.ffc00000 p1=ffc00000.ffc00000.ffc00000 color=ffffffff'
        'visualize created stage=mass_axes line=8 p0=ffc00000.ffc00000.ffc00000 p1=ffc00000.ffc00000.ffc00000 color=ffffffff'
        'visualize created stage=mass_axes line=9 p0=ffc00000.ffc00000.ffc00000 p1=ffc00000.ffc00000.ffc00000 color=ffffffff'
        'visualize created stage=mass_axes line=10 p0=ffc00000.ffc00000.ffc00000 p1=ffc00000.ffc00000.ffc00000 color=ffffffff'
        'visualize created stage=mass_axes line=11 p0=ffc00000.ffc00000.ffc00000 p1=ffc00000.ffc00000.ffc00000 color=ffffffff'
        'visualize created stage=mass_axes line=12 p0=408e08c2.3f3141b1.40880000 p1=40c32388.4039ed25.40880000 color=ffffffff'
        'visualize created stage=mass_axes line=13 p0=40c32388.4039ed25.40880000 p1=40b1f73e.4053af93.40880000 color=ffffffff'
        'visualize created stage=mass_axes line=14 p0=40b1f73e.4053af93.40880000 p1=4079b8f0.3f8c25b6.40880000 color=ffffffff'
        'visualize created stage=mass_axes line=15 p0=4079b8f0.3f8c25b6.40880000 p1=408e08c2.3f3141b1.40880000 color=ffffffff'
        'visualize created stage=mass_axes line=16 p0=4079b8f0.3f8c25b6.40b80000 p1=40b1f73e.4053af93.40b80000 color=ffffffff'
        'visualize created stage=mass_axes line=17 p0=40b1f73e.4053af93.40b80000 p1=40c32388.4039ed25.40b80000 color=ffffffff'
        'visualize created stage=mass_axes line=18 p0=40c32388.4039ed25.40b80000 p1=408e08c2.3f3141b1.40b80000 color=ffffffff'
        'visualize created stage=mass_axes line=19 p0=408e08c2.3f3141b1.40b80000 p1=4079b8f0.3f8c25b6.40b80000 color=ffffffff'
        'visualize created stage=mass_axes line=20 p0=40c32388.4039ed25.40880000 p1=40c32388.4039ed25.40b80000 color=ffffffff'
        'visualize created stage=mass_axes line=21 p0=40b1f73e.4053af93.40b80000 p1=40b1f73e.4053af93.40880000 color=ffffffff'
        'visualize created stage=mass_axes line=22 p0=4079b8f0.3f8c25b6.40880000 p1=4079b8f0.3f8c25b6.40b80000 color=ffffffff'
        'visualize created stage=mass_axes line=23 p0=408e08c2.3f3141b1.40b80000 p1=408e08c2.3f3141b1.40880000 color=ffffffff'
        'visualize created stage=mass_axes line=24 p0=40e00000.c007c3b6.40a00000 p1=41500000.c007c3b6.40a00000 color=ffffffff'
        'visualize created stage=mass_axes line=25 p0=41500000.c007c3b6.40a00000 p1=41500000.4007c3b6.40a00000 color=ffffffff'
        'visualize created stage=mass_axes line=26 p0=41500000.4007c3b6.40a00000 p1=40e00000.4007c3b6.40a00000 color=ffffffff'
        'visualize created stage=mass_axes line=27 p0=40e00000.4007c3b6.40a00000 p1=40e00000.c007c3b6.40a00000 color=ffffffff'
        'visualize created stage=mass_axes line=28 p0=40e00000.4007c3b6.40a00000 p1=41500000.4007c3b6.40a00000 color=ffffffff'
        'visualize created stage=mass_axes line=29 p0=41500000.4007c3b6.40a00000 p1=41500000.c007c3b6.40a00000 color=ffffffff'
        'visualize created stage=mass_axes line=30 p0=41500000.c007c3b6.40a00000 p1=40e00000.c007c3b6.40a00000 color=ffffffff'
        'visualize created stage=mass_axes line=31 p0=40e00000.c007c3b6.40a00000 p1=40e00000.4007c3b6.40a00000 color=ffffffff'
        'visualize created stage=mass_axes line=32 p0=41500000.c007c3b6.40a00000 p1=41500000.c007c3b6.40a00000 color=ffffffff'
        'visualize created stage=mass_axes line=33 p0=41500000.4007c3b6.40a00000 p1=41500000.4007c3b6.40a00000 color=ffffffff'
        'visualize created stage=mass_axes line=34 p0=40e00000.4007c3b6.40a00000 p1=40e00000.4007c3b6.40a00000 color=ffffffff'
        'visualize created stage=mass_axes line=35 p0=40e00000.c007c3b6.40a00000 p1=40e00000.c007c3b6.40a00000 color=ffffffff'
        'visualize created stage=mass_axes line=36 p0=41614d95.4031efa9.c074bb3a p1=417eb26b.4031efa9.c074bb3a color=ffffffff'
        'visualize created stage=mass_axes line=37 p0=417eb26b.4031efa9.c074bb3a p1=417eb26b.40960fe6.bfa3a0c0 color=ffffffff'
        'visualize created stage=mass_axes line=38 p0=417eb26b.40960fe6.bfa3a0c0 p1=41614d95.40960fe6.bfa3a0c0 color=ffffffff'
        'visualize created stage=mass_axes line=39 p0=41614d95.40960fe6.bfa3a0c0 p1=41614d95.4031efa9.c074bb3a color=ffffffff'
        'visualize created stage=mass_axes line=40 p0=41614d95.404e1057.be344c68 p1=417eb26b.404e1057.be344c68 color=ffffffff'
        'visualize created stage=mass_axes line=41 p0=417eb26b.404e1057.be344c68 p1=417eb26b.3fa7c066.c02e2fa0 color=ffffffff'
        'visualize created stage=mass_axes line=42 p0=417eb26b.3fa7c066.c02e2fa0 p1=41614d95.3fa7c066.c02e2fa0 color=ffffffff'
        'visualize created stage=mass_axes line=43 p0=41614d95.3fa7c066.c02e2fa0 p1=41614d95.404e1057.be344c68 color=ffffffff'
        'visualize created stage=mass_axes line=44 p0=417eb26b.4031efa9.c074bb3a p1=417eb26b.3fa7c066.c02e2fa0 color=ffffffff'
        'visualize created stage=mass_axes line=45 p0=417eb26b.404e1057.be344c68 p1=417eb26b.40960fe6.bfa3a0c0 color=ffffffff'
        'visualize created stage=mass_axes line=46 p0=41614d95.40960fe6.bfa3a0c0 p1=41614d95.404e1057.be344c68 color=ffffffff'
        'visualize created stage=mass_axes line=47 p0=41614d95.3fa7c066.c02e2fa0 p1=41614d95.4031efa9.c074bb3a color=ffffffff'
        'visualize created stage=mass_axes renderables=1'
        'visualize created stage=lin_velocity renderable=1 points=0 lines=15 triangles=0'
        'visualize created stage=lin_velocity line=0 p0=00000000.3f800000.40a00000 p1=3f19999a.400ccccd.40d9999a color=00ffffff'
        'visualize created stage=lin_velocity line=1 p0=3f19999a.400ccccd.40d9999a p1=3f028f5c.3fdeb205.40d6effc color=00ffffff'
        'visualize created stage=lin_velocity line=2 p0=3f19999a.400ccccd.40d9999a p1=3f028f5c.4013365a.40cafb8a color=00ffffff'
        'visualize created stage=lin_velocity line=3 p0=3f19999a.400ccccd.40d9999a p1=3f55a1c4.3ffc2b7c.40ce904f color=00ffffff'
        'visualize created stage=lin_velocity line=4 p0=3f19999a.400ccccd.40d9999a p1=3e3df3ce.4004799f.40d35b37 color=00ffffff'
        'visualize created stage=lin_velocity line=5 p0=40a00000.40000000.40a00000 p1=4091999a.40000000.40a1eb85 color=00ffffff'
        'visualize created stage=lin_velocity line=6 p0=4091999a.40000000.40a1eb85 p1=4093c290.3ff74896.40a1a1cb color=00ffffff'
        'visualize created stage=lin_velocity line=7 p0=4091999a.40000000.40a1eb85 p1=4093c290.40045bb5.40a1a1cb color=00ffffff'
        'visualize created stage=lin_velocity line=8 p0=4091999a.40000000.40a1eb85 p1=40940c4a.40000000.40a3cac0 color=00ffffff'
        'visualize created stage=lin_velocity line=9 p0=4091999a.40000000.40a1eb85 p1=409378d6.40000000.409f78d5 color=00ffffff'
        'visualize created stage=lin_velocity line=10 p0=41700000.40400000.c0000000 p1=41700000.3f999999.c0000000 color=00ffffff'
        'visualize created stage=lin_velocity line=11 p0=41700000.3f999999.c0000000 p1=417451ec.3fbc28f5.c0000000 color=00ffffff'
        'visualize created stage=lin_velocity line=12 p0=41700000.3f999999.c0000000 p1=416bae14.3fbc28f5.c0000000 color=00ffffff'
        'visualize created stage=lin_velocity line=13 p0=41700000.3f999999.c0000000 p1=41700000.3fbc28f5.bfdd70a4 color=00ffffff'
        'visualize created stage=lin_velocity line=14 p0=41700000.3f999999.c0000000 p1=41700000.3fbc28f5.c01147ae color=00ffffff'
        'visualize created stage=lin_velocity renderables=1'
        'visualize created stage=ang_velocity renderable=1 points=0 lines=15 triangles=0'
        'visualize created stage=ang_velocity line=0 p0=00000000.3f800000.40a00000 p1=3f333333.becccccb.40ab3333 color=00000000'
        'visualize created stage=ang_velocity line=1 p0=3f333333.becccccb.40ab3333 p1=3f4f6858.bda8c4fb.40a9851f color=00000000'
        'visualize created stage=ang_velocity line=2 p0=3f333333.becccccb.40ab3333 p1=3ec276fb.be985e1a.40a9851f color=00000000'
        'visualize created stage=ang_velocity line=3 p0=3f333333.becccccb.40ab3333 p1=3f124f38.be1279bd.40b1087f color=00000000'
        'visualize created stage=ang_velocity line=4 p0=3f333333.becccccb.40ab3333 p1=3f1e549e.be72a4f4.40a201be color=00000000'
        'visualize created stage=ang_velocity line=5 p0=40a00000.40000000.40a00000 p1=40a00000.40000000.40f9999a color=00000000'
        'visualize created stage=ang_velocity line=6 p0=40a00000.40000000.40f9999a p1=40a00000.3fca3d70.40ec28f6 color=00000000'
        'visualize created stage=ang_velocity line=7 p0=40a00000.40000000.40f9999a p1=40a00000.401ae148.40ec28f6 color=00000000'
        'visualize created stage=ang_velocity line=8 p0=40a00000.40000000.40f9999a p1=40ad70a4.40000000.40ec28f6 color=00000000'
        'visualize created stage=ang_velocity line=9 p0=40a00000.40000000.40f9999a p1=40928f5c.40000000.40ec28f6 color=00000000'
        'visualize created stage=ang_velocity line=10 p0=41700000.40400000.c0000000 p1=41833333.408ccccd.bf19999c color=00000000'
        'visualize created stage=ang_velocity line=11 p0=41833333.408ccccd.bf19999c p1=417eecc4.408e4f6f.bf4f5c2b color=00000000'
        'visualize created stage=ang_velocity line=12 p0=41833333.408ccccd.bf19999c p1=418393dc.407bb30e.bf4f5c2b color=00000000'
        'visualize created stage=ang_velocity line=13 p0=41833333.408ccccd.bf19999c p1=41805502.40815408.bf0354f8 color=00000000'
        'visualize created stage=ang_velocity line=14 p0=41833333.408ccccd.bf19999c p1=4182b53c.408ad4ee.bf8db1af color=00000000'
        'visualize created stage=ang_velocity renderables=1'
        'visualize created stage=joint_groups renderable=1 points=0 lines=0 triangles=0'
        'visualize created stage=joint_groups renderables=1'
        'visualize created stage=all renderable=1 points=0 lines=210 triangles=0'
        'visualize created stage=all renderables=1'
        'visualize again stage=all renderable=1 points=0 lines=210 triangles=0'
        'visualize again stage=all renderables=1'
        'visualize cleared stage=scale_zero renderable=1 points=0 lines=0 triangles=0'
        'visualize cleared stage=scale_zero renderables=1'
        'visualize moved d_box d_sphere'
        'visualize moved stage=all renderable=1 points=0 lines=210 triangles=0'
        'visualize moved stage=all renderables=1'
        'visualize scene_released renderables=0'
    )
    # Effector-and-coredump Task 2: the spring-and-damper effector differential.
    # NxScene's effector API and every NxSpringAndDamperEffector method over two
    # dynamic actors; the internal effector's slots 2 and 3 by index (each root's
    # +0x1f8 set to 0 on both sides); release, a release/create cycle, an actor
    # released under two effectors (the record's 0x100 notify), and the scene
    # released with two live effectors (000575). Each step prints the SDK
    # allocations and frees it made. Copied verbatim from the ORACLE side. The
    # first run differed only in the scene release's free order (the candidate
    # recycled a shape's id after freeing the shape; fixed in Scene.cpp) and in
    # lines since dropped (the record's +0x14 pad) or renamed (the lock links).
    # The root's +0x1f8 differed too: the candidate's record did not build
    # 000797's island (000760, then 000722's copy); fixed in the Task 2 review
    # and the island words +0x1bc..+0x200 registered (the two island lines).
    'NxPhysicsEffectorTests' = @(
        'effector start count=0 iterator=none',
        'effector actors=created,created',
        'effector fixture rec_a vt=physics observers=0 capacity=0 list=none',
        'effector fixture rec_b vt=physics observers=0 capacity=0 list=none',
        'effector fixture rec_a owner=body_a',
        'effector create allocs=4 sizes=68,18,8,8 frees=0 sizes=none',
        'effector create created=yes',
        'effector create np words=physics.00000000.physics.wlink.rlink.eff',
        'effector create internal vt=physics observers=0 capacity=0 list=none',
        'effector create internal words=00000000.scene.np.rec_a.rec_b.3e8aa393.3efc02a9.be96f0b6.bf0563b5.bcac7695.bcc20573.3f000000.3fa00000.40400000.42200000.425c0000.c0200000.3fe00000.40e00000.41180000',
        'effector create rec_a vt=physics observers=1 capacity=2 list=eff',
        'effector create rec_b vt=physics observers=1 capacity=2 list=eff',
        'effector create count=1 iterator=np',
        'effector create is=np slot6=np slot7=np userData_set=5eed0001',
        'effector create spring=3f000000.3fa00000.40400000.42200000.425c0000 damper=c0200000.3fe00000.40e00000.41180000',
        'effector set spring=3f400000.40000000.40900000.41480000.41f00000 damper=bfc00000.40500000.3f000000.40c00000',
        'effector set internal vt=physics observers=0 capacity=0 list=none',
        'effector set internal words=00000000.scene.np.rec_a.rec_b.3e8aa393.3efc02a9.be96f0b6.bf0563b5.bcac7695.bcc20573.3f400000.40000000.40900000.41480000.41f00000.bfc00000.40500000.3f000000.40c00000',
        'effector swap allocs=0 sizes=none frees=0 sizes=none',
        'effector swap internal vt=physics observers=0 capacity=0 list=none',
        'effector swap internal words=00000000.scene.np.rec_b.rec_a.c0112dd1.4035dc6d.3ffec990.3f96710b.bf82f9c0.3f47617b.3f400000.40000000.40900000.41480000.41f00000.bfc00000.40500000.3f000000.40c00000',
        'effector swap rec_a vt=physics observers=1 capacity=2 list=eff',
        'effector swap rec_b vt=physics observers=1 capacity=2 list=eff',
        'effector restore allocs=0 sizes=none frees=0 sizes=none',
        'effector restore internal vt=physics observers=0 capacity=0 list=none',
        'effector restore internal words=00000000.scene.np.rec_a.rec_b.3e8aa393.3efc02a9.be96f0b6.bf0563b5.bcac7695.bcc20573.3f400000.40000000.40900000.41480000.41f00000.bfc00000.40500000.3f000000.40c00000',
        'effector slots rec0 root=rec_a',
        'effector slots rec1 root=rec_b',
        'effector slots rec0 island=rec_a.00000000.00000000.00000001.3ecccccc.00000000.rec_a.00000000.00000000.00000000.00000000.rec_a.00000000.00000000.00000001.3ecccccc.00000000.rec_a wake=3ecccccc',
        'effector slots rec1 island=rec_b.00000000.00000000.00000001.3ecccccc.00000000.rec_b.00000000.00000000.00000000.00000000.rec_b.00000000.00000000.00000001.3ecccccc.00000000.rec_b wake=3ecccccc',
        'effector slots allocs=0 sizes=none frees=0 sizes=none',
        'effector slots slots internal changed none',
        'effector slots slots rec0 changed none',
        'effector slots slots rec1 changed none',
        'effector release allocs=0 sizes=none frees=2 sizes=18,68',
        'effector release count=0 iterator=none',
        'effector release rec_a vt=physics observers=0 capacity=2 list=none',
        'effector release rec_b vt=physics observers=0 capacity=2 list=none',
        'effector cycle1 allocs=2 sizes=68,18 frees=0 sizes=none',
        'effector cycle1 created=yes',
        'effector cycle1 np words=physics.00000000.physics.wlink.rlink.eff',
        'effector cycle1 internal vt=physics observers=0 capacity=0 list=none',
        'effector cycle1 internal words=00000000.scene.np.rec_a.rec_b.3e8aa393.3efc02a9.be96f0b6.bf0563b5.bcac7695.bcc20573.3f000000.3fa00000.40400000.42200000.425c0000.c0200000.3fe00000.40e00000.41180000',
        'effector cycle2 allocs=2 sizes=68,18 frees=0 sizes=none',
        'effector cycle2 created=yes',
        'effector cycle2 np words=physics.00000000.physics.wlink.rlink.eff',
        'effector cycle2 internal vt=physics observers=0 capacity=0 list=none',
        'effector cycle2 internal words=eff.scene.np.rec_b.rec_a.bfeed232.400bca1a.3f650d78.4033b2de.be8e4bce.3ecdcc20.3f000000.3fa00000.40400000.42200000.425c0000.c0200000.3fe00000.40e00000.41180000',
        'effector cycle2 count=2 iterator=np2.np1',
        'effector cycle2 rec_a vt=physics observers=2 capacity=2 list=eff1.eff2',
        'effector cycle2 rec_b vt=physics observers=2 capacity=2 list=eff1.eff2',
        'effector cycle_release1 allocs=0 sizes=none frees=2 sizes=18,68',
        'effector cycle_release1 count=1 iterator=np2',
        'effector cycle3 allocs=2 sizes=68,18 frees=0 sizes=none',
        'effector cycle3 created=yes',
        'effector cycle3 np words=physics.00000000.physics.wlink.rlink.eff',
        'effector cycle3 internal vt=physics observers=0 capacity=0 list=none',
        'effector cycle3 internal words=eff2.scene.np.rec_a.rec_b.3e8aa393.3efc02a9.be96f0b6.bf0563b5.bcac7695.bcc20573.3f000000.3fa00000.40400000.42200000.425c0000.c0200000.3fe00000.40e00000.41180000',
        'effector cycle3 count=2 iterator=np3.np2',
        'effector cycle3 rec_a vt=physics observers=2 capacity=2 list=eff2.eff3',
        'effector cycle3 rec_b vt=physics observers=2 capacity=2 list=eff2.eff3',
        'effector actor_release allocs=3 sizes=8,8,8 frees=6 sizes=18,8,260,1c,228,50',
        'effector actor_release e2 internal vt=physics observers=0 capacity=0 list=none',
        'effector actor_release e2 internal words=00000000.scene.np2.rec_b.00000000.bfeed232.400bca1a.3f650d78.4033b2de.be8e4bce.3ecdcc20.3f000000.3fa00000.40400000.42200000.425c0000.c0200000.3fe00000.40e00000.41180000',
        'effector actor_release e3 internal vt=physics observers=0 capacity=0 list=none',
        'effector actor_release e3 internal words=eff2.scene.np3.00000000.rec_b.3e8aa393.3efc02a9.be96f0b6.bf0563b5.bcac7695.bcc20573.3f000000.3fa00000.40400000.42200000.425c0000.c0200000.3fe00000.40e00000.41180000',
        'effector actor_release rec_b vt=physics observers=2 capacity=2 list=eff2.eff3',
        'effector actor_release count=2 iterator=np3.np2',
        'effector after_actor_release allocs=0 sizes=none frees=2 sizes=18,68',
        'effector after_actor_release count=1 iterator=np2',
        'effector after_actor_release rec_b vt=physics observers=1 capacity=2 list=eff2',
        'effector actors2=created,created',
        'effector live allocs=4 sizes=68,18,8,8 frees=0 sizes=none',
        'effector live created=yes',
        'effector live np words=physics.00000000.physics.wlink.rlink.eff',
        'effector live internal vt=physics observers=0 capacity=0 list=none',
        'effector live internal words=eff2.scene.np.rec_c.rec_d.3fdab1cb.bfa9b3b6.c009a579.3fcccccc.c0333333.c0366666.3f000000.3fa00000.40400000.42200000.425c0000.c0200000.3fe00000.40e00000.41180000',
        'effector live count=2 iterator=np4.np2',
        'effector scene_release allocs=3 sizes=18,18,18 frees=55 sizes=14,18,20,4,20,4,28,18,8,260,1c,228,50,18,8,260,1c,228,50,18,8,8,260,8,1c,8,228,50,18,68,18,68,400,400,400,400,400,400,400,400,400,400,400,400,a8,18,18,18,10,60,10,3c,18,18,710'
    )
    # Effector-and-coredump Task 4: the scene core-dump differential. Two populated
    # scenes dumped through NxPhysicsSDK::coreDump (000267 -> 004062) in text and
    # binary mode, with and without an addendum, the deadlock arm of 000267, one
    # scene and none; each .psc file printed back line by line with the date line
    # and the pointer tokens normalised (tests/PhysicsCoreDumpTests.cpp). Copied
    # verbatim from the ORACLE side: the setup and report lines, the returns and
    # line counts of every dump, the text dump's header, one material, constants,
    # group flags, the whole of scene A's asset and the trailer, samples of the
    # binary tokens, both addenda, and the one- and no-scene dumps. The first run
    # differed in scene A's bodies only: maxangularvelocity(7) where the oracle
    # prints the SDK's changed NX_MAX_ANGULAR_VELOCITY (fixed in Scene.cpp).
    # Task 5 (review) added scene C, dumped after the others in a new pointer
    # epoch: an unjointed actor created asleep, static capsules (one whose own
    # flags reach the trigger writer), consecutive equal actors and shapes for the
    # PsDefaultSettings lines, and a static three-shape actor; its first run found
    # the capsule's flags missing from the shape (fixed in Scene.cpp).
    # Those 95 lines sit before the list's last pre-Task-5 entry (the no_scene line
    # count). The final-review line after it is the outstanding SDK block count
    # at the pointer-epoch reset.
    'NxPhysicsCoreDumpTests' = @(
        'sdk materials=3 added=1,2',
        'sdk group_1_3=0 group_2_2=0',
        'scene a=created b=created',
        'scene actor ground=created',
        'scene actor wall block=created',
        'scene actor ball=created',
        'scene actor crate"q=created',
        'scene actor pill=created',
        'scene actor mover=created',
        'scene actor compound=created',
        'scene actor frozen one=created',
        'scene actor ghost=created',
        'scene actor unnamed=created',
        'scene joint prismatic=created',
        'scene joint revolute=created',
        'scene joint revolute_plain=created',
        'scene joint cylindrical=created',
        'scene joint spherical=created',
        'scene joint spherical_world=created',
        'scene joint point_on_line=created',
        'scene joint point_in_plane=created',
        'scene joint distance=created',
        'scene joint pulley=created',
        'scene joint fixed=created',
        'scene joint d6=created',
        'scene effector=created',
        'scene actor floor=created',
        'scene actor drop=created',
        'scene actor pair=created',
        'dump text coreDump binary=0 addendum=no returned=0',
        'dump text present=yes',
        'dump text line=0 text=################################################################<CR>',
        'dump text line=1 text=### Core Dump from Novodex Physics SDK<CR>',
        'dump text line=2 text=### Core Dump Generated on <date><CR>',
        'dump text line=3 text=### Contains 2 assets.<CR>',
        'dump text line=4 text=PsReset<CR>',
        'dump text line=5 text=PsVersion 1.4<CR>',
        'dump text line=6 text=PsNameSpace PhysicsSDK__P0<CR>',
        'dump text line=7 text=<CR>',
        'dump text line=8 text=<CR>',
        'dump text line=9 text=### Begin Material Definition ####<CR>',
        'dump text line=27 text=### Begin Material Definition ####<CR>',
        'dump text line=28 text=PsMatBegin mat2<CR>',
        'dump text line=29 text=PsMatDynamicFriction 0.375<CR>',
        'dump text line=30 text=PsMatStaticFriction 0.625<CR>',
        'dump text line=31 text=PsMatSpinFriction 0<CR>',
        'dump text line=32 text=PsMatRollFriction 0<CR>',
        'dump text line=33 text=PsMatRestitution 0.25<CR>',
        'dump text line=34 text=PsMatDynamicFrictionV 0.125<CR>',
        'dump text line=35 text=PsMatStaticFrictionV 0.300000012<CR>',
        'dump text line=36 text=PsMatDirOfAnisotropy 0 0 1<CR>',
        'dump text line=37 text=PsMatDirOfMotion 1 0 0<CR>',
        'dump text line=38 text=PsMatSpeedOfMotion 2.5<CR>',
        'dump text line=39 text=PsMatAnisotropic true<CR>',
        'dump text line=40 text=PsMatMovingSurface true<CR>',
        'dump text line=41 text=PsMatEnd<CR>',
        'dump text line=42 text=### End Material Definition ####<CR>',
        'dump text line=43 text=<CR>',
        'dump text line=44 text=<CR>',
        'dump text line=61 text=<CR>',
        'dump text line=62 text=PsAssetBegin Asset__P1<CR>',
        'dump text line=66 text=PsSetConstant NX_PENALTY_FORCE 0.75<CR>',
        'dump text line=70 text=PsSetConstant NX_BOUNCE_TRESHOLD -1.5<CR>',
        'dump text line=73 text=PsSetConstant NX_MAX_ANGULAR_VELOCITY 9<CR>',
        'dump text line=76 text=PsSetConstant NX_COLL_INFINITY fltmax<CR>',
        'dump text line=84 text=PsGroupCollisionFlag 1 FFFFFFF7<CR>',
        'dump text line=85 text=PsGroupCollisionFlag 2 FFFFFFFB<CR>',
        'dump text line=86 text=PsGroupCollisionFlag 3 FFFFFFFD<CR>',
        'dump text line=92 text=## Scene in initial configuration.<CR>',
        'dump text line=94 text=PsGravity 0.5 -9.81000042 0.25<CR>',
        'dump text line=95 text=PsJointEnd<CR>',
        'dump text line=96 text=PsJointEnd<CR>',
        'dump text line=97 text=PsJointEnd<CR>',
        'dump text line=98 text=PsJointLimitPlane 0 1 0 -0.25<CR>',
        'dump text line=99 text=PsJointEnd<CR>',
        'dump text line=100 text=PsJointBegin $__P2<CR>',
        'dump text line=101 text=### Point In Plane Joint<CR>',
        'dump text line=102 text=PsJointOffset frame(primary)   body(primary)   offset(-1,0.25,-2)<CR>',
        'dump text line=103 text=PsJointOffset frame(secondary) body(secondary) offset(0.103896126,0.311038971,2.225974083)<CR>',
        'dump text line=104 text=PsJointAxes   frame(primary)   body(primary)   xaxis(0,1,0) yaxis(0,0,1)<CR>',
        'dump text line=105 text=PsJointAxes   frame(secondary) body(secondary) xaxis(-0.155844167,0.958441556,-0.238961041) yaxis(-0.415584415,0.155844152,0.896103919)<CR>',
        'dump text line=106 text=PsDefaultSettings bodycollide(false) breakable(false) <CR>',
        'dump text line=107 text=PsJointLimit swing1(free) swing2(free) twist(free) linear(free,free,locked)<CR>',
        'dump text line=108 text=PsJointEnd<CR>',
        'dump text line=109 text=PsJointBegin $__P3<CR>',
        'dump text line=110 text=### Point On Line Joint<CR>',
        'dump text line=111 text=PsJointOffset frame(primary)   body(primary)   offset(0.157894701,-0.392105132,2.152631521)<CR>',
        'dump text line=112 text=PsJointOffset frame(secondary) body(secondary) offset(-1.719177961,-0.680136979,-1.180822015)<CR>',
        'dump text line=113 text=PsJointAxes   frame(primary)   body(primary)   xaxis(0.221052676,-0.618947327,0.753684223) yaxis(0.82105267,-0.298947304,-0.486315757)<CR>',
        'dump text line=114 text=PsJointAxes   frame(secondary) body(secondary) xaxis(0.950684905,-0.111780807,0.289315104) yaxis(0.26301375,-0.203835607,-0.943013668)<CR>',
        'dump text line=115 text=PsDefaultSettings bodycollide(false) breakable(false) <CR>',
        'dump text line=116 text=PsJointLimit swing1(free) swing2(free) twist(free) linear(free,locked,locked)<CR>',
        'dump text line=117 text=PsJointLimitPlane -0.000000038 -0.000000041 1 -1.999999762<CR>',
        'dump text line=118 text=PsJointEnd<CR>',
        'dump text line=119 text=PsJointBegin $__P4<CR>',
        'dump text line=120 text=### Spherical Joint<CR>',
        'dump text line=121 text=PsJointOffset frame(primary)   body(primary)   offset(1,4,2)<CR>',
        'dump text line=122 text=PsJointOffset frame(secondary) body(secondary) offset(0,1,0)<CR>',
        'dump text line=123 text=PsJointAxes   frame(primary)   body(primary)   xaxis(0,1,0) yaxis(0,0,1)<CR>',
        'dump text line=124 text=PsJointAxes   frame(secondary) body(secondary) xaxis(0,1,0) yaxis(0,0,1)<CR>',
        'dump text line=125 text=PsDefaultSettings bodycollide(false) breakable(false) <CR>',
        'dump text line=126 text=PsJointLimit swing1(free) swing2(free) twist(free) twistspring(none) swingspring(none) jointspring(none) projection(none) linear(locked,locked,locked)<CR>',
        'dump text line=127 text=PsJointEnd<CR>',
        'dump text line=128 text=PsJointBegin "shoulder joint___P5"<CR>',
        'dump text line=129 text=### Spherical Joint<CR>',
        'dump text line=130 text=PsJointOffset frame(primary)   body(primary)   offset(-1.168831229,-0.361688316,0.357792228)<CR>',
        'dump text line=131 text=PsJointOffset frame(secondary) body(secondary) offset(0.713043511,-0.697825968,-0.79347831)<CR>',
        'dump text line=132 text=PsJointAxes   frame(primary)   body(primary)   xaxis(-0.425974041,0.699740291,0.573506534) yaxis(0.896103919,0.238961041,0.374025971)<CR>',
        'dump text line=133 text=PsJointAxes   frame(secondary) body(secondary) xaxis(0.463768095,0.855072498,0.231884152) yaxis(0.884057999,-0.463768065,-0.057971034)<CR>',
        'dump text line=134 text=PsDefaultSettings bodycollide(false) breakable(false) name("shoulder joint") <CR>',
        'dump text line=135 text=PsJointLimit swing1(0.25,0.5,1) swing2(0.25,0.5,1) twist(-0.5,0.25,1,  0.375,0.125,1) twistspring(6,0.25,0.125) swingspring(4,0.75,0.0625) jointspring(3,0.5,0.25) projection(0.125) linear(locked,locked,locked)<CR>',
        'dump text line=136 text=PsJointEnd<CR>',
        'dump text line=137 text=PsJointBegin $__P6<CR>',
        'dump text line=138 text=### Cylindrical Joint<CR>',
        'dump text line=139 text=PsJointOffset frame(primary)   body(primary)   offset(-2.547945261,-0.075342514,0.04794509)<CR>',
        'dump text line=140 text=PsJointOffset frame(secondary) body(secondary) offset(1.090909123,1.190909147,1.972727299)<CR>',
        'dump text line=141 text=PsJointAxes   frame(primary)   body(primary)   xaxis(0.164383546,0.972602725,-0.164383546) yaxis(0.602739692,0.032876715,0.797260284)<CR>',
        'dump text line=142 text=PsJointAxes   frame(secondary) body(secondary) xaxis(-0.155844167,0.958441556,-0.238961041) yaxis(-0.415584415,0.155844152,0.896103919)<CR>',
        'dump text line=143 text=PsDefaultSettings bodycollide(false) breakable(100,250.5) <CR>',
        'dump text line=144 text=PsJointLimit twist(free) swing1(locked) swing2(locked) linear(free,locked,locked)<CR>',
        'dump text line=145 text=PsJointEnd<CR>',
        'dump text line=146 text=PsJointBegin $__P7<CR>',
        'dump text line=147 text=### Revolute Joint<CR>',
        'dump text line=148 text=PsJointOffset frame(primary)   body(primary)   offset(1.214492798,0.157971084,0.007246431)<CR>',
        'dump text line=149 text=PsJointOffset frame(secondary) body(secondary) offset(-4.6712327,0.145205364,2.271232605)<CR>',
        'dump text line=150 text=PsJointAxes   frame(primary)   body(primary)   xaxis(0.884057999,-0.463768065,-0.057971034) yaxis(0.336231858,0.544927478,0.768115997)<CR>',
        'dump text line=151 text=PsJointAxes   frame(secondary) body(secondary) xaxis(0.780821919,-0.230136961,-0.580821872) yaxis(0.602739692,0.032876715,0.797260284)<CR>',
        'dump text line=152 text=PsDefaultSettings bodycollide(false) breakable(false) <CR>',
        'dump text line=153 text=PsJointLimit twist(free) motor(false) twistspring(none) swing1(locked) swing2(locked) linear(locked,locked,locked)<CR>',
        'dump text line=154 text=PsJointEnd<CR>',
        'dump text line=155 text=PsJointBegin hinge___P8<CR>',
        'dump text line=156 text=### Revolute Joint<CR>',
        'dump text line=157 text=PsJointOffset frame(primary)   body(primary)   offset(-2.105263233,1.494736671,1.181578875)<CR>',
        'dump text line=158 text=PsJointOffset frame(secondary) body(secondary) offset(1.952173948,-1.491304159,-1.423913121)<CR>',
        'dump text line=159 text=PsJointAxes   frame(primary)   body(primary)   xaxis(-0.315789461,-0.315789461,0.894736826) yaxis(0.789473712,-0.610526204,0.063157901)<CR>',
        'dump text line=160 text=PsJointAxes   frame(secondary) body(secondary) xaxis(0.336231858,0.544927478,0.768115997) yaxis(0.884057999,-0.463768065,-0.057971034)<CR>',
        'dump text line=161 text=PsDefaultSettings bodycollide(true) breakable(false) name("hinge") <CR>',
        'dump text line=162 text=PsJointLimit twist(-0.25,0.5,0.75,  0.125,0.25,1) motor(2.5,30,true) twistspring(8,0.5,0.0625) swing1(locked) swing2(locked) linear(locked,locked,locked)<CR>',
        'dump text line=163 text=PsJointLimitPlane -0.000000029 1 -0.000000041 3.000000238<CR>',
        'dump text line=164 text=PsJointEnd<CR>',
        'dump text line=165 text=PsJointBegin $__P9<CR>',
        'dump text line=166 text=### Prismatic Joint<CR>',
        'dump text line=167 text=PsJointOffset frame(primary)   body(primary)   offset(1.590909123,0.040909097,0.172727257)<CR>',
        'dump text line=168 text=PsJointOffset frame(secondary) body(secondary) offset(-1.210526347,0.93947351,0.463157862)<CR>',
        'dump text line=169 text=PsJointAxes   frame(primary)   body(primary)   xaxis(-0.084675312,0.670753241,0.736831188) yaxis(0.993177295,-0.002690522,0.116583377)<CR>',
        'dump text line=170 text=PsJointAxes   frame(secondary) body(secondary) xaxis(0.284210503,-0.123789415,0.950736821) yaxis(0.736541331,-0.606635511,-0.299165726)<CR>',
        'dump text line=171 text=PsDefaultSettings bodycollide(false) breakable(false) <CR>',
        'dump text line=172 text=PsJointLimit swing1(locked) swing2(locked) twist(locked) linear(free,locked,locked)<CR>',
        'dump text line=173 text=PsJointEnd<CR>',
        'dump text line=174 text=PsPlane  plane(0,1,0,-0.5) localposition(0,0,0) localorientation(0,0,0,1) group(0) material(mat1) name(ground___P10) awake(false) position(0,0,0) orientation(0,0,0,1) static(true) <CR>',
        'dump text line=175 text=PsDefaultSettings localposition(0,0,0)<CR>',
        'dump text line=176 text=PsDefaultSettings localorientation(0,0,0,1)<CR>',
        'dump text line=177 text=PsDefaultSettings group(0)<CR>',
        'dump text line=178 text=PsBox sides(8,4,0.5) material(mat3) triggerevent(enter,leave,stay) name("wall block___P11") awake(false) position(-3,2,5) orientation(0,0.38176015,0,0.924261451) static(true) <CR>',
        'dump text line=179 text=PsSphere radius(0.625) material(mat1) name(ball___P12) position(0,1,0) orientation(0.101929434,0.203858867,-0.101929441,0.968329668) com(0,0,0) comrot(0,0,0,1) inertia(0.3125,0.3125,0.3125) mass(2.045000076) solvercount(4) velocity(0.5,-0.25,1) angularvelocity(0.300000012,0.699999988,-0.200000003) wakeupcounter(0.399999976) lineardamping(0) angulardamping(0.050000001) maxangularvelocity(9) <CR>',
        'dump text line=180 text=PsDefaultSettings wakeupcounter(0.399999976)<CR>',
        'dump text line=181 text=PsBox sides(2,1,1.5) localposition(0.125,-0.25,0.5) localorientation(0,0,0.382683486,0.923879504) material(mat2) group(3) name("crate"q___P13") position(3,0.5,-1) orientation(-0.20519565,0.102597833,0.307793438,0.923380554) com(0.125,-0.25,0.5) comrot(0,0,-0.382683486,0.923879504) inertia(2.34375,1.21875,1.875) mass(4.5) solvercount(7) velocity(0,0,0) angularvelocity(0,0,0) lineardamping(0.125) angulardamping(0.0625) maxangularvelocity(12.5) <CR>',
        'dump text line=182 text=PsDefaultSettings angularvelocity(0,0,0)<CR>',
        'dump text line=183 text=PsCapsule height(1.25) radius(0.375) localposition(0,0,0) localorientation(0,0,0,1) material(mat1) group(2) name(pill___P14) position(-2,0.5,1.5) orientation(0.323029101,-0.107676379,0.215352729,0.915249228) com(0,0,0) comrot(0,0,0,1) inertia(0.975000024,0.1875,0.975000024) mass(2.650000095) solvercount(4) velocity(-0.75,0.100000001,0.400000006) lineardamping(0) angulardamping(0.050000001) maxangularvelocity(9) locked(true,false,false,false,false,true) <CR>',
        'dump text line=184 text=PsDefaultSettings com(0,0,0)<CR>',
        'dump text line=185 text=PsDefaultSettings comrot(0,0,0,1)<CR>',
        'dump text line=186 text=PsDefaultSettings solvercount(4)<CR>',
        'dump text line=187 text=PsDefaultSettings lineardamping(0)<CR>',
        'dump text line=188 text=PsDefaultSettings angulardamping(0.050000001)<CR>',
        'dump text line=189 text=PsDefaultSettings maxangularvelocity(9)<CR>',
        'dump text line=190 text=PsDefaultSettings localposition(0,0,0)<CR>',
        'dump text line=191 text=PsDefaultSettings localorientation(0,0,0,1)<CR>',
        'dump text line=192 text=PsDefaultSettings material(mat1)<CR>',
        'dump text line=193 text=PsBox sides(1,1,4) group(0) name(mover___P15) position(1,3,2) orientation(0,0,0,1) density(1) inertia(5.666666985,5.666666985,0.666666687) mass(4) velocity(0,0,0) kinematic(true) <CR>',
        'dump text line=194 text=PsDefaultSettings velocity(0,0,0)<CR>',
        'dump text line=195 text=PsShapeBegin Shape1<CR>',
        'dump text line=196 text=PsDefaultSettings group(0)<CR>',
        'dump text line=197 text=PsBox sides(1,0.5,1) localposition(0,-0.5,0) <CR>',
        'dump text line=198 text=PsSphere radius(0.25) localposition(0,0.5,0) material(mat2) triggerevent(leave,) <CR>',
        'dump text line=199 text=PsCapsule height(0.75) radius(0.125) localposition(0.5,0,0) localorientation(0.707106888,0,0,0.707106709) material(mat1) group(5) <CR>',
        'dump text line=200 text=PsShapeEnd<CR>',
        'dump text line=201 text=PsShape Shape1 name(compound___P16) position(4,2,3) orientation(0.052342389,-0.31405434,0.10468477,0.94216305) com(0.044699065,-0.455300927,0) comrot(0,0,0.154588833,0.987978876) inertia(0.159999996,0.25,0.1875) mass(1.375) angularvelocity(0,1.5,0) <CR>',
        'dump text line=202 text=PsDefaultSettings group(0)<CR>',
        'dump text line=203 text=PsBox sides(0.600000024,0.600000024,0.600000024) name("frozen one___P17") position(-4,1,-4) orientation(0,0,0,1) com(0,0,0) comrot(0,0,0,1) inertia(0.01296,0.01296,0.01296) mass(0.216000006) angularvelocity(0,0,0) locked(true) collision(false) <CR>',
        'dump text line=204 text=PsDefaultSettings orientation(0,0,0,1)<CR>',
        'dump text line=205 text=PsDefaultSettings comrot(0,0,0,1)<CR>',
        'dump text line=206 text=PsDefaultSettings angularvelocity(0,0,0)<CR>',
        'dump text line=207 text=PsShapeBegin Shape2<CR>',
        'dump text line=208 text=PsShapeEnd<CR>',
        'dump text line=209 text=PsShape Shape2 name(ghost___P18) position(0,6,0) com(0,0.25,0) inertia(1,0.5,0.25) mass(2) <CR>',
        'dump text line=210 text=PsSphere radius(0.5) name($__P19) position(2,0.5,4) com(0,0,0) inertia(0.050000001,0.050000001,0.050000001) mass(0.5) wakeupcounter(0) <CR>',
        'dump text line=211 text=PsJoint $__P20 pill___P14 $__P19<CR>',
        'dump text line=212 text=PsJoint $__P21 compound___P16 $__P19<CR>',
        'dump text line=213 text=PsJoint $__P22 ball___P12 "crate"q___P13"<CR>',
        'dump text line=214 text=PsJoint rope___P23 ball___P12 $__P19<CR>',
        'dump text line=215 text=PsJoint $__P2 $__P19 ball___P12<CR>',
        'dump text line=216 text=PsJoint $__P3 "crate"q___P13" compound___P16<CR>',
        'dump text line=217 text=PsJoint $__P4 @world mover___P15<CR>',
        'dump text line=218 text=PsJoint "shoulder joint___P5" ball___P12 pill___P14<CR>',
        'dump text line=219 text=PsJoint $__P6 compound___P16 ball___P12<CR>',
        'dump text line=220 text=PsJoint $__P7 pill___P14 compound___P16<CR>',
        'dump text line=221 text=PsJoint hinge___P8 "crate"q___P13" pill___P14<CR>',
        'dump text line=222 text=PsJoint $__P9 ball___P12 "crate"q___P13"<CR>',
        'dump text line=223 text=PsDefaultSettings spring_dist_relaxed(1.250000)',
        'dump text line=224 text=PsDefaultSettings spring_dist_compress_saturate(0.500000) spring_dist_stretch_saturate(3.000000)',
        'dump text line=225 text=PsDefaultSettings spring_max_compress_force(40.000000) spring_max_stretch_force(55.000000)',
        'dump text line=226 text=PsDefaultSettings spring_vel_compress_saturate(-2.500000) spring_vel_stretch_saturate(1.750000)',
        'dump text line=227 text=PsDefaultSettings spring_damper_max_compress_force(7.000000) spring_damper_max_stretch_force(9.500000)',
        'dump text line=228 text=PsDefaultSettings spring_pos1(0.250000,1.500000,-0.300000)',
        'dump text line=229 text=PsDefaultSettings spring_pos2(-1.750000,0.200000,1.250000)',
        'dump text line=230 text=PsSpring ball___P12 pill___P14',
        'dump text line=231 text=PsAssetEnd<CR>',
        'dump text line=288 text=##################################################################################<CR>',
        'dump text line=289 text=<CR>',
        'dump text line=290 text=<CR>',
        'dump text line=291 text=PsSetScene 0<CR>',
        'dump text line=292 text=PsAsset Asset__P1 position(0,0,0) orientation(0,0,0,1)<CR>',
        'dump text line=293 text=<CR>',
        'dump text line=294 text=PsSetScene 1<CR>',
        'dump text line=295 text=PsAsset Asset__P24 position(0,0,0) orientation(0,0,0,1)<CR>',
        'dump text line=296 text=<CR>',
        'dump text line=297 text=PsSetScene 0<CR>',
        'dump text lines=298 normalised_bytes=16940',
        'dump text_addendum coreDump binary=0 addendum=yes returned=0',
        'dump text_addendum present=yes',
        'dump text_addendum line=297 text=PsSetScene 0<CR>',
        'dump text_addendum line=298 text=### Begin : User supplied addendum script.<CR>',
        'dump text_addendum line=299 text=PsSetScene 0<CR>',
        'dump text_addendum line=300 text=PsUserLine hello, world<CR>',
        'dump text_addendum line=301 text=### End   : User supplied addendum script.<CR>',
        'dump text_addendum lines=302 normalised_bytes=17079',
        'dump binary coreDump binary=1 addendum=no returned=0',
        'dump binary present=yes',
        'dump binary line=66 text=PsSetConstant NX_PENALTY_FORCE 0.7500$3f400000<CR>',
        'dump binary line=67 text=PsSetConstant NX_MIN_SEPARATION_FOR_PENALTY -0.0500$bd4ccccd<CR>',
        'dump binary line=94 text=PsGravity 0.5000$3f000000 -9.8100$c11cf5c3 0.2500$3e800000<CR>',
        'dump binary line=98 text=PsJointLimitPlane 0 1 0 -0.2500$be800000<CR>',
        'dump binary line=163 text=PsJointLimitPlane -0.0000$b2fa4576 1 -0.0000$b3309d79 3.0000$40400001<CR>',
        'dump binary line=167 text=PsJointOffset frame(primary)   body(primary)   offset(1.5909$3fcba2e9,0.0409$3d27904c,0.1727$3e30df6a)<CR>',
        'dump binary line=179 text=PsSphere radius(0.6250$3f200000) material(mat1) name(ball___P12) position(0,1,0) orientation(0.1019$3dd0c061,0.2039$3e50c061,-0.1019$bdd0c062,0.9683$3f77e474) com(0,0,0) comrot(0,0,0,1) inertia(0.3125$3ea00000,0.3125$3ea00000,0.3125$3ea00000) mass(2.0450$4002e148) solvercount(4) velocity(0.5000$3f000000,-0.2500$be800000,1) angularvelocity(0.3000$3e99999a,0.7000$3f333333,-0.2000$be4ccccd) wakeupcounter(0.4000$3ecccccc) lineardamping(0) angulardamping(0.0500$3d4ccccd) maxangularvelocity(9.0000$41100000) <CR>',
        'dump binary line=181 text=PsBox sides(2.0000$40000000,1,1.5000$3fc00000) localposition(0.1250$3e000000,-0.2500$be800000,0.5000$3f000000) localorientation(0,0,0.3827$3ec3ef17,0.9239$3f6c835e) material(mat2) group(3) name("crate"q___P13") position(3.0000$40400000,0.5000$3f000000,-1) orientation(-0.2052$be521ecf,0.1026$3dd21ed0,0.3078$3e9d971a,0.9234$3f6c62ab) com(0.1250$3e000000,-0.2500$be800000,0.5000$3f000000) comrot(0,0,-0.3827$bec3ef17,0.9239$3f6c835e) inertia(2.3438$40160000,1.2188$3f9c0000,1.8750$3ff00000) mass(4.5000$40900000) solvercount(7) velocity(0,0,0) angularvelocity(0,0,0) lineardamping(0.1250$3e000000) angulardamping(0.0625$3d800000) maxangularvelocity(12.5000$41480000) <CR>',
        'dump binary line=223 text=PsDefaultSettings spring_dist_relaxed(1.250000)',
        'dump binary line=230 text=PsSpring ball___P12 pill___P14',
        'dump binary lines=298 normalised_bytes=19479',
        'dump binary_addendum coreDump binary=1 addendum=yes returned=0',
        'dump binary_addendum present=yes',
        'dump binary_addendum line=297 text=PsSetScene 0<CR>',
        'dump binary_addendum line=298 text=### Begin : User supplied addendum script.<CR>',
        'dump binary_addendum line=299 text=# addendum for the binary dump<CR>',
        'dump binary_addendum line=300 text=### End   : User supplied addendum script.<CR>',
        'dump binary_addendum lines=301 normalised_bytes=19608',
        'deadlock before a_flag=0 b_flag=0',
        'report error code=2 file=\Epic\Novodex\SDKs\Physics\src\NpPhysicsSDK.cpp line=225 message=PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!',
        'dump deadlock coreDump binary=0 addendum=yes returned=0',
        'dump deadlock present=no',
        'deadlock after a_flag=0 a_owner_is_self=1 b_flag=1',
        'dump one_scene coreDump binary=0 addendum=no returned=0',
        'dump one_scene present=yes',
        'dump one_scene line=3 text=### Contains one Asset.<CR>',
        'dump one_scene line=6 text=PsNameSpace PhysicsSDK__P0<CR>',
        'dump one_scene line=62 text=PsAssetBegin Asset__P1<CR>',
        'dump one_scene line=94 text=PsGravity 0.5 -9.81000042 0.25<CR>',
        'dump one_scene line=230 text=PsSpring ball___P12 pill___P14',
        'dump one_scene line=231 text=PsAssetEnd<CR>',
        'dump one_scene line=232 text=##################################################################################<CR>',
        'dump one_scene line=233 text=<CR>',
        'dump one_scene line=234 text=<CR>',
        'dump one_scene line=235 text=PsSetScene 0<CR>',
        'dump one_scene line=236 text=PsAsset Asset__P1 position(0,0,0) orientation(0,0,0,1)<CR>',
        'dump one_scene line=237 text=<CR>',
        'dump one_scene line=238 text=PsSetScene 0<CR>',
        'dump one_scene lines=239 normalised_bytes=13916',
        'dump no_scene coreDump binary=1 addendum=yes returned=0',
        'dump no_scene present=yes',
        'dump no_scene line=59 text=PsMatEnd<CR>',
        'dump no_scene line=60 text=### End Material Definition ####<CR>',
        'dump no_scene line=61 text=<CR>',
        'dump no_scene line=62 text=<CR>',
        'dump no_scene line=63 text=PsSetScene 0<CR>',
        'dump no_scene line=64 text=### Begin : User supplied addendum script.<CR>',
        'dump no_scene line=65 text=<CR>',
        'dump no_scene line=66 text=### End   : User supplied addendum script.<CR>',
        'scene c=created',
        'dump pointer_epoch reset',
        'scene actor napper=created',
        'scene actor sensor=created',
        'scene actor sensor 2=created',
        'scene actor west=created',
        'scene actor west again=created',
        'scene actor pebble=created',
        'scene actor pebble 2=created',
        'scene actor twin=created',
        'scene actor twin 2=created',
        'scene actor brick=created',
        'scene actor brick 2=created',
        'scene actor gate=created',
        'dump scene_c coreDump binary=0 addendum=no returned=0',
        'dump scene_c present=yes',
        'dump scene_c line=3 text=### Contains one Asset.<CR>',
        'dump scene_c line=62 text=PsAssetBegin Asset__P1<CR>',
        'dump scene_c line=92 text=## Scene in initial configuration.<CR>',
        'dump scene_c line=95 text=PsSphere radius(0.5) localposition(0,0,0) localorientation(0,0,0,1) material(mat1) group(0) name(napper___P2) awake(false) position(-6,0.5,0) orientation(0,0,0,1) com(0,0,0) comrot(0,0,0,1) inertia(0.050000001,0.050000001,0.050000001) mass(0.5) solvercount(4) velocity(0,0,0) angularvelocity(0,0,0) wakeupcounter(0) lineardamping(0) angulardamping(0.050000001) maxangularvelocity(9) <CR>',
        'dump scene_c line=96 text=PsDefaultSettings localposition(0,0,0)<CR>',
        'dump scene_c line=97 text=PsDefaultSettings localorientation(0,0,0,1)<CR>',
        'dump scene_c line=98 text=PsDefaultSettings material(mat1)<CR>',
        'dump scene_c line=99 text=PsDefaultSettings group(0)<CR>',
        'dump scene_c line=100 text=PsCapsule height(2) radius(0.25) name(sensor___P3) awake(false) position(0,1,-6) orientation(0,0,0.382683486,0.923879504) static(true) <CR>',
        'dump scene_c line=101 text=PsDefaultSettings position(0,1,-6)<CR>',
        'dump scene_c line=102 text=PsDefaultSettings orientation(0,0,0.382683486,0.923879504)<CR>',
        'dump scene_c line=103 text=PsDefaultSettings radius(0.25)<CR>',
        'dump scene_c line=104 text=PsDefaultSettings height(2)<CR>',
        'dump scene_c line=105 text=PsCapsule triggerevent(enter,) name("sensor 2___P4") awake(false) static(true) <CR>',
        'dump scene_c line=106 text=PsPlane  plane(1,0,0,-20) name(west___P5) awake(false) position(0,0,0) orientation(0,0,0,1) static(true) <CR>',
        'dump scene_c line=107 text=PsDefaultSettings position(0,0,0)<CR>',
        'dump scene_c line=108 text=PsDefaultSettings orientation(0,0,0,1)<CR>',
        'dump scene_c line=109 text=PsDefaultSettings plane(1,0,0,-20)<CR>',
        'dump scene_c line=110 text=PsPlane  name("west again___P6") awake(false) static(true) <CR>',
        'dump scene_c line=111 text=PsSphere radius(0.375) material(mat3) name(pebble___P7) awake(false) position(3,0.375,3) static(true) <CR>',
        'dump scene_c line=112 text=PsDefaultSettings position(3,0.375,3)<CR>',
        'dump scene_c line=113 text=PsDefaultSettings radius(0.375)<CR>',
        'dump scene_c line=114 text=PsDefaultSettings material(mat3)<CR>',
        'dump scene_c line=115 text=PsSphere name("pebble 2___P8") awake(false) static(true) <CR>',
        'dump scene_c line=116 text=PsDefaultSettings comrot(0,0,0,1)<CR>',
        'dump scene_c line=117 text=PsBox sides(1,1.5,2.5) localposition(0,0.25,0) material(mat2) group(4) name(twin___P9) position(5,1,5) orientation(0,0.301131338,0,0.953582644) com(0,0.25,0) inertia(0.5,0.625,0.75) mass(3) solvercount(6) velocity(0.25,0,-0.5) angularvelocity(0,0.75,0) wakeupcounter(0.399999976) lineardamping(0.25) angulardamping(0.125) maxangularvelocity(5) <CR>',
        'dump scene_c line=118 text=PsDefaultSettings position(5,1,5)<CR>',
        'dump scene_c line=119 text=PsDefaultSettings orientation(0,0.301131338,0,0.953582644)<CR>',
        'dump scene_c line=120 text=PsDefaultSettings com(0,0.25,0)<CR>',
        'dump scene_c line=121 text=PsDefaultSettings inertia(0.5,0.625,0.75)<CR>',
        'dump scene_c line=122 text=PsDefaultSettings mass(3)<CR>',
        'dump scene_c line=123 text=PsDefaultSettings solvercount(6)<CR>',
        'dump scene_c line=124 text=PsDefaultSettings velocity(0.25,0,-0.5)<CR>',
        'dump scene_c line=125 text=PsDefaultSettings angularvelocity(0,0.75,0)<CR>',
        'dump scene_c line=126 text=PsDefaultSettings wakeupcounter(0.399999976)<CR>',
        'dump scene_c line=127 text=PsDefaultSettings lineardamping(0.25)<CR>',
        'dump scene_c line=128 text=PsDefaultSettings angulardamping(0.125)<CR>',
        'dump scene_c line=129 text=PsDefaultSettings maxangularvelocity(5)<CR>',
        'dump scene_c line=130 text=PsDefaultSettings sides(1,1.5,2.5)<CR>',
        'dump scene_c line=131 text=PsDefaultSettings localposition(0,0.25,0)<CR>',
        'dump scene_c line=132 text=PsDefaultSettings material(mat2)<CR>',
        'dump scene_c line=133 text=PsDefaultSettings group(4)<CR>',
        'dump scene_c line=134 text=PsBox name("twin 2___P10") <CR>',
        'dump scene_c line=135 text=PsBox sides(0.5,0.5,1) localposition(0,0,0) material(mat1) group(0) name(brick___P11) position(-3,2,-3) orientation(0,0,0,1) density(2) com(0,0,0) inertia(0.052083336,0.052083336,0.020833334) mass(0.5) solvercount(4) velocity(0,0,0) angularvelocity(0,0,0) lineardamping(0) angulardamping(0.050000001) maxangularvelocity(9) <CR>',
        'dump scene_c line=136 text=PsDefaultSettings orientation(0,0,0,1)<CR>',
        'dump scene_c line=137 text=PsDefaultSettings density(2)<CR>',
        'dump scene_c line=138 text=PsDefaultSettings com(0,0,0)<CR>',
        'dump scene_c line=139 text=PsDefaultSettings inertia(0.052083336,0.052083336,0.020833334)<CR>',
        'dump scene_c line=140 text=PsDefaultSettings mass(0.5)<CR>',
        'dump scene_c line=141 text=PsDefaultSettings solvercount(4)<CR>',
        'dump scene_c line=142 text=PsDefaultSettings velocity(0,0,0)<CR>',
        'dump scene_c line=143 text=PsDefaultSettings angularvelocity(0,0,0)<CR>',
        'dump scene_c line=144 text=PsDefaultSettings lineardamping(0)<CR>',
        'dump scene_c line=145 text=PsDefaultSettings angulardamping(0.050000001)<CR>',
        'dump scene_c line=146 text=PsDefaultSettings maxangularvelocity(9)<CR>',
        'dump scene_c line=147 text=PsDefaultSettings sides(0.5,0.5,1)<CR>',
        'dump scene_c line=148 text=PsDefaultSettings localposition(0,0,0)<CR>',
        'dump scene_c line=149 text=PsDefaultSettings material(mat1)<CR>',
        'dump scene_c line=150 text=PsDefaultSettings group(0)<CR>',
        'dump scene_c line=151 text=PsBox name("brick 2___P12") position(-3,3,-3) <CR>',
        'dump scene_c line=152 text=PsShapeBegin Shape1<CR>',
        'dump scene_c line=153 text=PsBox sides(0.5,4,0.5) localposition(-1,0,0) <CR>',
        'dump scene_c line=154 text=PsDefaultSettings sides(0.5,4,0.5)<CR>',
        'dump scene_c line=155 text=PsBox localposition(1,0,0) <CR>',
        'dump scene_c line=156 text=PsCapsule radius(0.200000003) localposition(0,2,0) localorientation(0,0,0.707106888,0.707106709) group(6) <CR>',
        'dump scene_c line=157 text=PsShapeEnd<CR>',
        'dump scene_c line=158 text=PsShape Shape1 name(gate___P13) awake(false) position(8,2,-2) static(true) <CR>',
        'dump scene_c lines=167 normalised_bytes=7270',
        'dump scene_c_binary coreDump binary=1 addendum=no returned=0',
        'dump scene_c_binary present=yes',
        'dump scene_c_binary line=95 text=PsSphere radius(0.5000$3f000000) localposition(0,0,0) localorientation(0,0,0,1) material(mat1) group(0) name(napper___P2) awake(false) position(-6.0000$c0c00000,0.5000$3f000000,0) orientation(0,0,0,1) com(0,0,0) comrot(0,0,0,1) inertia(0.0500$3d4ccccd,0.0500$3d4ccccd,0.0500$3d4ccccd) mass(0.5000$3f000000) solvercount(4) velocity(0,0,0) angularvelocity(0,0,0) wakeupcounter(0) lineardamping(0) angulardamping(0.0500$3d4ccccd) maxangularvelocity(9.0000$41100000) <CR>',
        'dump scene_c_binary line=100 text=PsCapsule height(2.0000$40000000) radius(0.2500$3e800000) name(sensor___P3) awake(false) position(0,1,-6.0000$c0c00000) orientation(0,0,0.3827$3ec3ef17,0.9239$3f6c835e) static(true) <CR>',
        'dump scene_c_binary line=105 text=PsCapsule triggerevent(enter,) name("sensor 2___P4") awake(false) static(true) <CR>',
        'dump scene_c_binary line=117 text=PsBox sides(1,1.5000$3fc00000,2.5000$40200000) localposition(0,0.2500$3e800000,0) material(mat2) group(4) name(twin___P9) position(5.0000$40a00000,1,5.0000$40a00000) orientation(0,0.3011$3e9a2de3,0,0.9536$3f741dfe) com(0,0.2500$3e800000,0) inertia(0.5000$3f000000,0.6250$3f200000,0.7500$3f400000) mass(3.0000$40400000) solvercount(6) velocity(0.2500$3e800000,0,-0.5000$bf000000) angularvelocity(0,0.7500$3f400000,0) wakeupcounter(0.4000$3ecccccc) lineardamping(0.2500$3e800000) angulardamping(0.1250$3e000000) maxangularvelocity(5.0000$40a00000) <CR>',
        'dump scene_c_binary line=135 text=PsBox sides(0.5000$3f000000,0.5000$3f000000,1) localposition(0,0,0) material(mat1) group(0) name(brick___P11) position(-3.0000$c0400000,2.0000$40000000,-3.0000$c0400000) orientation(0,0,0,1) density(2.0000$40000000) com(0,0,0) inertia(0.0521$3d555556,0.0521$3d555556,0.0208$3caaaaab) mass(0.5000$3f000000) solvercount(4) velocity(0,0,0) angularvelocity(0,0,0) lineardamping(0) angulardamping(0.0500$3d4ccccd) maxangularvelocity(9.0000$41100000) <CR>',
        'dump scene_c_binary line=153 text=PsBox sides(0.5000$3f000000,4.0000$40800000,0.5000$3f000000) localposition(-1,0,0) <CR>',
        'dump scene_c_binary line=156 text=PsCapsule radius(0.2000$3e4ccccd) localposition(0,2.0000$40000000,0) localorientation(0,0,0.7071$3f3504f5,0.7071$3f3504f2) group(6) <CR>',
        'dump scene_c_binary lines=167 normalised_bytes=8523',
        'scene c=released',
        'dump no_scene lines=67 normalised_bytes=1799',
        'allocator after_release outstanding=14'
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
        # float tape. The discrete families that compare exactly, and both tapes of
        # qhull_exact_output/qhull_exact_other, are registered whole; the other float
        # families, qhull_paths, qhull_paths_t4 and qhull_rotation are DIVERGENT, registered up to
        # the oracle digest, and held by kDivergentCeilings. The
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
        'thirdparty name=qhull_merge2 rva=0x0007d180 owner=phys_fn_003234 source=qhull.c,poly.c,poly2.c,merge.c,qset.c words=53209 oracle=d5c49873 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_merge2_x87 rva=0x0007d180 owner=phys_fn_003234 source=geom.c,geom2.c,merge.c words=62090 oracle=b399c974',
        'thirdparty name=qhull_merge rva=0x0007d180 owner=phys_fn_003234 source=qhull.c,poly.c,poly2.c,merge.c,qset.c words=84575 oracle=a0e11bc1 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_merge_x87 rva=0x0007d180 owner=phys_fn_003234 source=geom.c,geom2.c,merge.c words=77301 oracle=3639a851',
        'thirdparty name=qhull_random rva=0x00061490 owner=phys_fn_002550 source=geom2.c,global.c,qhull.c,merge.c words=10722 oracle=ba2b5e75 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_random_x87 rva=0x00061490 owner=phys_fn_002550 source=geom2.c,geom.c words=7184 oracle=1baa6368',
        'thirdparty name=qhull_direct rva=0x00068ce0 owner=phys_fn_002779 source=io.c,geom2.c,poly2.c,stat.c,qset.c words=127485 oracle=2cf4876b mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_direct_x87 rva=0x00068ce0 owner=phys_fn_002779 source=io.c,geom.c,geom2.c words=22629 oracle=3e023a50',
        'thirdparty name=qhull_exact_output rva=0x0006d800 owner=phys_fn_002866 source=io.c,geom2.c,poly2.c,stat.c words=55168 oracle=4f7bae0d mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_exact_output_x87 rva=0x0006d800 owner=phys_fn_002866 source=io.c,geom.c,geom2.c words=24566 oracle=3ae85f24 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_exact_other rva=0x0007d180 owner=phys_fn_003234 source=qhull.c,poly.c,poly2.c,merge.c,global.c,io.c,qset.c words=37571 oracle=cb2d8512 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_exact_other_x87 rva=0x0007d180 owner=phys_fn_003234 source=geom.c,geom2.c,merge.c,io.c words=18072 oracle=22bf6dbb mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=qhull_paths rva=0x0005c5c0 owner=phys_fn_002425 source=geom.c,qhull.c,poly2.c,merge.c,io.c words=9518 oracle=96cb5a62',
        'thirdparty name=qhull_paths_x87 rva=0x0005c5c0 owner=phys_fn_002425 source=geom.c,geom2.c,merge.c words=15524 oracle=216821ea',
        'thirdparty name=qhull_paths_t4 rva=0x0005dfb0 owner=phys_fn_002454 source=geom.c,qhull.c,poly2.c,merge.c,io.c words=17889 oracle=5bc3fc9a',
        'thirdparty name=qhull_paths_t4_x87 rva=0x0005dfb0 owner=phys_fn_002454 source=geom.c,geom2.c,merge.c words=2723 oracle=248026db',
        'thirdparty name=qhull_rotation rva=0x0005fec0 owner=phys_fn_002518 source=geom2.c,global.c,qhull.c,merge.c words=9980 oracle=e80e1851',
        'thirdparty name=qhull_rotation_x87 rva=0x0005fec0 owner=phys_fn_002518 source=geom2.c,geom.c words=8724 oracle=07282fca',
        'thirdparty coverage driven=77 divergent=27 words=1636017 layout_checks=47',
        'thirdparty oracle digest=52015450',
        # qhull-gap Task 4e: convex cooking. HullLibrary::CreateConvexHull/ReleaseResult
        # (0x0007ea10/0x0007e300) and phys_fn_002233 (0x00054920) called directly on both
        # sides, under 0x027f and 0x0f7f, with a zeroing recording allocator, each side's
        # QHULL_*.obj files in a directory of its own (units/convex-cooking-contract.md,
        # differentials A and B), and the host's size slot 003265 called directly. The exact
        # families are registered whole. hull_create_qhull
        # and hull_compute_qhull (two inputs that differ under 0x027f -- box: vendored qhull
        # (reproduced by hull_qhull_direct); clusters: not reproduced by qhull alone, open
        # (qhull-gap Task 5; candidates: allocation pattern, qh_gethash address hashing))
        # and the four _obj families (the OBJ text as tokens; the 2003 CRT prints -0.0
        # without its sign) are DIVERGENT, registered up to the oracle digest, and held by
        # kDivergentCeilings/kLengthCeilings. The pairs above keep printing where they were;
        # the pair below carries the totals.
        'thirdparty name=hull_host_size rva=0x0007e520 owner=phys_fn_003265 source=QhullHost.cpp words=15 oracle=c9faaedb mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=hull_create rva=0x0007ea10 owner=phys_fn_003279 source=QhullHost.cpp,Quantizer.cpp words=23542 oracle=1cf4b7ed mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=hull_create_qhull rva=0x0007ea10 owner=phys_fn_003279 source=QhullHost.cpp,Quantizer.cpp words=442 oracle=980dbb03',
        'thirdparty name=hull_create_obj rva=0x0007dea0 owner=phys_fn_003247 source=QhullHost.cpp words=5001 oracle=af04f879',
        'thirdparty name=hull_create_pc64 rva=0x0007ea10 owner=phys_fn_003279 source=QhullHost.cpp,Quantizer.cpp words=24047 oracle=c32b26d1 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=hull_create_pc64_obj rva=0x0007dea0 owner=phys_fn_003247 source=QhullHost.cpp words=5022 oracle=28e98259',
        'thirdparty name=hull_compute rva=0x00054920 owner=phys_fn_002233 source=TriangleMesh.cpp,QhullHost.cpp,Quantizer.cpp words=12128 oracle=8a6e6bdb mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=hull_compute_qhull rva=0x00054920 owner=phys_fn_002233 source=TriangleMesh.cpp,QhullHost.cpp,Quantizer.cpp words=454 oracle=dae9e9bd',
        'thirdparty name=hull_compute_obj rva=0x0007e050 owner=phys_fn_003251 source=QhullHost.cpp words=122 oracle=bc33134f',
        'thirdparty name=hull_compute_pc64 rva=0x00054920 owner=phys_fn_002233 source=TriangleMesh.cpp,QhullHost.cpp,Quantizer.cpp words=12633 oracle=f5a778d6 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=hull_compute_pc64_obj rva=0x0007e050 owner=phys_fn_003251 source=QhullHost.cpp words=122 oracle=4ebf7157',
        'thirdparty coverage driven=88 divergent=33 words=1719545 layout_checks=47',
        'thirdparty oracle digest=87804f45',
        # qhull-gap Task 5 (the Task 4e review): the byte digest of every QHULL_*.obj of
        # the Task 4e families after dropping the sign of a printed -0.000 (the two CRTs'
        # one known difference), exact and registered whole; and hull_qhull_direct(_x87),
        # the two inputs of hull_create_qhull run through qhull alone (nxQhullRun, "o"),
        # DIVERGENT, registered up to the oracle digest and held by kDivergentCeilings. The
        # pairs above keep printing where they were; the pair below carries the totals.
        'thirdparty name=hull_create_objbytes rva=0x0007dea0 owner=phys_fn_003247 source=QhullHost.cpp words=18 oracle=ef34c50f mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=hull_create_pc64_objbytes rva=0x0007dea0 owner=phys_fn_003247 source=QhullHost.cpp words=18 oracle=99ca1d01 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=hull_compute_objbytes rva=0x0007e050 owner=phys_fn_003251 source=QhullHost.cpp words=2 oracle=eebf5792 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=hull_compute_pc64_objbytes rva=0x0007e050 owner=phys_fn_003251 source=QhullHost.cpp words=2 oracle=16381ab0 mismatches=0 worst_ulp=0 verdict=exact',
        'thirdparty name=hull_qhull_direct rva=0x0007d180 owner=phys_fn_003234 source=qhull.c,poly.c,poly2.c,merge.c,geom.c,geom2.c,qset.c,mem.c,global.c words=812 oracle=12922cc6',
        'thirdparty name=hull_qhull_direct_x87 rva=0x0005c5c0 owner=phys_fn_002425 source=geom.c,geom2.c,merge.c words=410 oracle=27d5b5e1',
        'thirdparty coverage driven=94 divergent=35 words=1720807 layout_checks=47',
        'thirdparty oracle digest=5d3c9ff1'
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
    '4' = 188  # 34 for NxPhysicsAssetTests, 154 for NxPhysicsThirdPartyTests (67 + 29 from
               # vendored-correspondence Task 4 + 5 from its Task 5a + 32 from qhull-gap Task 1
               # + 13 from qhull-gap Task 4e + 8 from qhull-gap Task 5)
    '5' = 1046  # 126 object-layout, 1 shape-vtable and 744 public actor/pruner/box/scene lines
               # + 149 body-creation lines (scene-raycast Task 4)
               # + 13 setters lines (scene-raycast Task 4, setters)
               # + 9 dynamic-setter and 3 shape-vtable lines (scene-raycast Task 4, shape)
               # + 1 shape-vtable line (scene-raycast Task 4, box hull)
               # (RED on purpose: vtables family open)
    '6' = 856  # 3 oracle-descriptor + 118 oracle-joint + 118 staged-pair-joint + 6 tangent
               # + 12 joint-allocator + 146 joint-slot + 79 effector + 374 core-dump
    '7' = 1129  # the 118 + 12 + 146 STAGED-PAIR joint assertions + 207 + 8 scene-raycast + 185 scene-visualize
               # + 79 effector + 374 core-dump; the oracle-differential assertions
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
    'NxPhysicsBodyCreationTests',
    'NxPhysicsDynamicFirstTests',
    'NxPhysicsEmptySceneTests',
    'NxPhysicsEffectorTests',
    'NxPhysicsCoreDumpTests',
    'NxPhysicsCoreClusterTests',
    'NxFoundationTangentTests',
    'NxPhysicsExportTests',
    'NxPhysicsGeometryTests',
    'NxPhysicsJointAllocatorTests',
    'NxPhysicsJointSlotTests',
    'NxPhysicsJointStagedPairTests',
    'NxPhysicsKernelFuzzTests',
    'NxPhysicsSceneRaycastTests',
    'NxPhysicsSceneVisualizeTests',
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
