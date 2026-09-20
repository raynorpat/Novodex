# The one registry of differential test targets per reconstruction phase.
# run_phase_gate.ps1 and run_differential.ps1 both dot-source this file and
# neither keeps a second list, so registering a target is a one-line edit here.
# The phase plans address it through `run_differential.ps1 -Phase N`.
#
# Phase 1 is evidence-only. Phases 5-8 have registered nothing yet: a phase with
# no targets cannot be gated, so both runners report it skipped and exit 3
# rather than reporting a pass. Phase 4 registers an oracle differential and no
# staged-pair differential, which is why run_phase_gate.ps1 counts all three
# lists before it decides a phase is ungated.

$NxPhaseTestTargets = [ordered] @{
    '1' = @()
    '2' = @('NxPhysicsExportTests', 'NxPhysicsSDKTests', 'NxPhysicsCoreClusterTests')
    '3' = @('NxPhysicsGeometryTests', 'NxPhysicsKernelFuzzTests')
    '4' = @()
    '5' = @()
    '6' = @()
    '7' = @()
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
    '5' = @('NxPhysicsObjectLayoutTests')
    '6' = @('NxPhysicsJointDescTests')
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
    # The Phase 6 joint-descriptor differential. Two cases over the two exported
    # rows, printing the whole descriptor surface before and after each call. The
    # degenerate zero and NaN axes are quarantined in the harness with the reason
    # recorded there (evidence/phase6-joints.md 7p); these three lines pin the
    # finite path, which is exact.
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
        # phys_fn_001377 gates on its default-word half only. Under 0x0f7f it
        # differs on 13 of 2,940,000, and every one is hit.worldImpact or the
        # distance derived from it -- fields NxRaySphereIntersect writes, not
        # fields this row computes. The recovered matrix has just put that
        # export inside the simulation step for the first time, which is the
        # third time this program has had a Task 2 row reopened by a
        # reachability discovery. The count is registered so it fails if it
        # moves either way, including toward zero.
        'collision name=shape_raycast_sphere index=- rva=0x00027c70 owner=phys_fn_001377 checks=5880000 oracle=6bee7065d00d060e',
        'collision coverage name=shape_raycast_sphere hits=54437 wrote_normal=27073 aimed=22568 behind=5666 default_mismatches=0 simulate_mismatches=13',
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
        'collision name=shape_raycast_capsule index=- rva=0x00022480 owner=phys_fn_001010 checks=5880000 oracle=28ac6dc0d51aa6bd',
        'collision coverage name=shape_raycast_capsule hits=32821 untouched_normal=32821 aimed=22571 zero_axis=15030 default_mismatches=0 simulate_mismatches=242',

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
        'collision name=contact_capsule_capsule index=21 rva=0x0003d9d0 owner=phys_fn_001775 checks=1073192 oracle=d94c81f08538ddac',
        'collision coverage name=contact_capsule_capsule emitted=20238 f00=25362 f01=25044 f10=24804 f11=25386 swept_emitted=10174 seeded_normal=10008 parallel=37380 zero_axis=42726 coincident=3830 beyond_end=10224 c1=18889 c2=1305 c3=20 c4=24 default_mismatches=0 simulate_mismatches=43',

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
        'thirdparty name=segment_sqrdist.grid rva=0x000f0560 owner=phys_fn_005493 source=Ice/IceSegment.cpp:29 words=60000 oracle=e2c89342 mismatches=9356 worst_ulp=67 verdict=divergent',
        'thirdparty name=segment_sqrdist.wide rva=0x000f0560 owner=phys_fn_005493 source=Ice/IceSegment.cpp:29 words=40000 oracle=4dbf889d mismatches=15538 worst_ulp=8420 verdict=divergent',
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

        'thirdparty coverage driven=16 divergent=2 words=193028 layout_checks=47',
        'thirdparty oracle digest=b87c3219'
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
    '4' = 100  # 34 for NxPhysicsAssetTests, 66 for NxPhysicsThirdPartyTests
    '5' = 126  # was 124: +2 for the shapeleaf family (row + candidate drive)
               # (RED on purpose: vtables family open)
    '6' = 3   # the Phase 6 joint-descriptor differential's three lines
    '7' = 0
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
    'NxPhysicsCoreClusterTests',
    'NxPhysicsExportTests',
    'NxPhysicsGeometryTests',
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
    'NxPhysicsObjectLayoutTests',
    'NxPhysicsThirdPartyTests'
)
$NxSkippedExitCode = 3
