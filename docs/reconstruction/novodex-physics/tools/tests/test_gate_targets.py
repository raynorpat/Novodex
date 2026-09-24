"""Structural checks on the coverage registration in gate_targets.ps1.

Why this file exists. run_phase_gate.ps1 enforces $NxRequiredCoverageLines by
substring match, requiring each registered line to appear at least twice in the
differential transcript -- once per pair. That makes the registration itself
load-bearing, and nothing checked it.

It was wrong. The first version pinned bare counts, and `hits=34575` happens to
be emitted by both NxSegmentAABBIntersect.aimed and NxSegmentBoxIntersect.aimed.
Either export alone satisfied the requirement, so de-aiming the segment/AABB
generator -- dropping it from 34575 to 92 -- left the gate passing. A coverage
assertion that cannot fail for the export it names is worse than none, because
it reads as protection.

These tests are cheap and they close that class of error rather than the single
instance: a registered line must be unique, and every generator that reports
coverage must be registered.
"""

import collections
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
GATE_TARGETS = TOOLS_DIR / "gate_targets.ps1"
RUN_DIFFERENTIAL = TOOLS_DIR / "run_differential.ps1"
FUZZ_SOURCE = Path(r"D:/github/Novodex/tests/PhysicsKernelFuzzTests.cpp")
COLLISION_SOURCE = Path(r"D:/github/Novodex/tests/PhysicsCollisionTests.cpp")
ASSET_SOURCE = Path(r"D:/github/Novodex/tests/PhysicsAssetTests.cpp")
THIRDPARTY_SOURCE = Path(r"D:/github/Novodex/tests/PhysicsThirdPartyTests.cpp")

# The blocks NxPhysicsCollisionTests drives directly rather than through a
# dispatch-matrix slot, so they have no entry in the driven table to parse. The
# last two are Task 2 exports that the recovered matrix puts inside the
# simulation step, driven here because their own differential runs under the CRT
# default control word only.
COLLISION_DIRECT_BLOCKS = ("box_corner", "sphere_box_data",
                           "step_ray_tri", "step_smooth_normals",
                           "contact_plane_sphere", "contact_emit",
                           "shape_raycast_plane", "contact_plane_capsule",
                           "shape_raycast_sphere", "contact_sphere_capsule",
                           "segment_segment", "shape_raycast_capsule",
                           "contact_capsule_capsule", "contact_plane_box",
                           "shape_owner", "ccd_guard",
                           "contact_sphere_sphere", "sphere_box_contact",
                           "contact_sphere_box", "box_quad_depth",
                           "box_clip.random", "box_clip.aimed",
                           "box_axis.random", "box_axis.aimed",
                           "box_shim", "contact_box_box")


def registered_lines():
    """The string literals inside $NxRequiredCoverageLines, per target."""
    text = GATE_TARGETS.read_text(encoding="utf-8")
    start = text.index("$NxRequiredCoverageLines")
    body = text[start : text.index("\n}", start)]
    targets = {}
    current = None
    for line in body.splitlines():
        header = re.match(r"\s*'([^']+)'\s*=\s*@\(", line)
        if header:
            current = header.group(1)
            targets[current] = []
            continue
        if current is None:
            continue
        stripped = line.strip()
        if stripped.startswith("#"):
            continue
        for literal in re.findall(r"'((?:[^']|'')*)'", line):
            targets[current].append(literal.replace("''", "'"))
    return targets


class RegisteredCoverageLines(unittest.TestCase):
    def test_at_least_one_target_is_registered(self):
        # A guard on the parser rather than on the registry: if the shape of
        # gate_targets.ps1 changes so that nothing parses, every other test in
        # this file would pass vacuously.
        targets = registered_lines()
        self.assertTrue(targets, "no coverage targets parsed out of gate_targets.ps1")
        for target, lines in targets.items():
            self.assertTrue(lines, "%s registered no coverage lines" % target)

    def test_registered_lines_are_unique_within_a_target(self):
        for target, lines in registered_lines().items():
            duplicates = {line for line in lines if lines.count(line) > 1}
            self.assertFalse(duplicates,
                "%s registers the same coverage line twice: %s" % (target, sorted(duplicates)))

    def test_no_registered_line_is_a_substring_of_another(self):
        """The collision that actually happened, generalised.

        run_phase_gate.ps1 matches with Contains, so a registered line that is a
        substring of another is satisfied by the other one's transcript line and
        cannot fail on its own account.
        """
        for target, lines in registered_lines().items():
            for shorter in lines:
                for longer in lines:
                    if shorter is not longer and shorter in longer:
                        self.fail("%s registers %r, which is a substring of %r, so it "
                                  "cannot fail independently" % (target, shorter, longer))

    def test_every_coverage_line_the_harness_prints_is_registered(self):
        """Every nxCoverage call in the harness must have a registered line.

        This is the half that catches a *new* generator being added without a
        registration, which is how coverage silently lapses in the first place.
        """
        if not FUZZ_SOURCE.exists():
            self.skipTest("implementation tree not present at %s" % FUZZ_SOURCE)
        source = FUZZ_SOURCE.read_text(encoding="utf-8")
        emitted = set(re.findall(r'nxCoverage\("([^"]+)"', source))
        self.assertTrue(emitted, "no nxCoverage calls found; has the harness changed shape?")
        registered = registered_lines().get("NxPhysicsKernelFuzzTests", [])
        for name in sorted(emitted):
            needle = "fuzz coverage name=%s reached=" % name
            self.assertTrue(any(line.startswith(needle) for line in registered),
                "the harness reports coverage for %s but gate_targets.ps1 registers no "
                "line for it, so losing that generator would not fail the gate" % name)

    def test_registered_coverage_names_still_exist_in_the_harness(self):
        """And the other half: a registration left behind after its generator
        was removed would pass forever without covering anything."""
        if not FUZZ_SOURCE.exists():
            self.skipTest("implementation tree not present at %s" % FUZZ_SOURCE)
        source = FUZZ_SOURCE.read_text(encoding="utf-8")
        emitted = set(re.findall(r'nxCoverage\("([^"]+)"', source))
        for line in registered_lines().get("NxPhysicsKernelFuzzTests", []):
            match = re.match(r"fuzz coverage name=(\S+) reached=", line)
            if match:
                self.assertIn(match.group(1), emitted,
                    "gate_targets.ps1 registers coverage for %s but the harness no longer "
                    "reports it" % match.group(1))


FORMAT_SPEC = re.compile(r"%(?:0[0-9]+)?(?:l{0,2}[udx]|s|c|f|g|e|016llx)")
PRINTF_CALL = re.compile(r'printf\(\s*"((?:[^"\\]|\\.)*)"')


def harness_formats(source_path):
    """Every literal printf format the harness can emit, split into the fixed
    text around its conversions."""
    text = source_path.read_text(encoding="utf-8")
    formats = []
    for literal in PRINTF_CALL.findall(text):
        fmt = literal.replace("\\n", "\n").replace("\\t", "\t").replace('\\"', '"')
        fmt = fmt.rstrip("\n")
        pieces = []
        index = 0
        for match in FORMAT_SPEC.finditer(fmt):
            pieces.append(re.escape(fmt[index:match.start()]))
            pieces.append(r"\S+" if match.group(0).endswith("s") else r"[-+0-9a-fA-Fx.]+")
            index = match.end()
        pieces.append(re.escape(fmt[index:]))
        formats.append((fmt, [p for p in pieces if p]))
    return formats


def is_prefix_of_format(line, pieces):
    """Could this registration be a prefix of a line that format prints?

    Truncating the pattern after each piece and asking for a full match is
    enough: the pieces alternate fixed text and one conversion, so a
    registration that stops inside a number still matches the truncation that
    ends with the preceding fixed text plus a partial one.
    """
    for count in range(1, len(pieces) + 1):
        pattern = "".join(pieces[:count])
        if re.fullmatch(pattern, line):
            return True
        # A registration may also stop part way through a fixed run.
        if count == len(pieces) and re.fullmatch(pattern + r".*", line):
            return True
    return False


class RegistrationsAreCheckedAgainstTheHarness(unittest.TestCase):
    """A registered line must be a prefix of exactly one line the harness prints.

    Two rules in one, and both were violated by a registration that shipped.

    *Prefix*, because `run_phase_gate.ps1` matches with Contains, and a
    registration taken from the middle of a line carries none of the context
    that makes it identify that line. `hits=89978` was such a registration.

    *Exactly one*, because nine export lines in PhysicsKernelFuzzTests.cpp end
    with ` hits=%u`, so that registration was satisfied by any of them and could
    not fail on account of the one it was written for. That is the same defect
    the file's own comment describes as fixed -- it was fixed for the twelve
    `fuzz coverage name=` lines and not for this one.

    The reason it survived is that every other test here compares registrations
    only against each other. This one is the only test that looks at what the
    harness can actually print.
    """

    CASES = (("NxPhysicsKernelFuzzTests", FUZZ_SOURCE),
             ("NxPhysicsCollisionTests", COLLISION_SOURCE),
             ("NxPhysicsAssetTests", ASSET_SOURCE),
             ("NxPhysicsThirdPartyTests", THIRDPARTY_SOURCE))

    def test_every_registration_is_a_prefix_of_exactly_one_printed_line(self):
        registry = registered_lines()
        for target, source in self.CASES:
            if not source.exists():
                continue
            formats = harness_formats(source)
            self.assertTrue(formats, "no printf formats parsed out of %s" % source)
            for line in registry.get(target, []):
                matching = [fmt for fmt, pieces in formats if is_prefix_of_format(line, pieces)]
                self.assertNotEqual(len(matching), 0,
                    "%s registers %r, which is not a prefix of any line %s can print, so it "
                    "identifies nothing" % (target, line, source.name))
                # A format whose %s could absorb text another format spells out
                # is the less specific of the two, and the specific one is the
                # line the registration was written for. Ties are the real
                # ambiguity: `hits=89978` matched nine formats, all equally
                # specific, and none of them more so than the others.
                specificity = [len(FORMAT_SPEC.sub("", fmt)) for fmt in matching]
                best = max(specificity)
                ambiguous = [fmt for fmt, score in zip(matching, specificity) if score == best]
                self.assertEqual(len(ambiguous), 1,
                    "%s registers %r, which is a prefix of %d equally specific lines %s can "
                    "print, so it cannot fail on account of the one it was written for: %s"
                    % (target, line, len(ambiguous), source.name, ambiguous))


def phase_lists(variable):
    """The phase -> target-name mapping out of one registry in gate_targets.ps1."""
    text = GATE_TARGETS.read_text(encoding="utf-8")
    start = text.index("$%s" % variable)
    body = text[start : text.index("\n}", start)]
    lists = {}
    for line in body.splitlines():
        match = re.match(r"\s*'([0-9]+)'\s*=\s*@\((.*)\)", line)
        if match:
            lists[match.group(1)] = re.findall(r"'([A-Za-z0-9_]+)'", match.group(2))
    return lists


def registered_targets(variable):
    """The literal names in one $NxRegistered* list in gate_targets.ps1."""
    text = GATE_TARGETS.read_text(encoding="utf-8")
    start = text.index("$%s = @(" % variable)
    body = text[start : text.index("\n)", start)]
    return re.findall(r"'([A-Za-z0-9_]+)'", body)


REGISTRIES = (("NxPhaseTestTargets", "NxRegisteredTestTargets"),
              ("NxPhaseStaticProofTargets", "NxRegisteredStaticProofTargets"),
              ("NxPhaseOracleDifferentialTargets", "NxRegisteredOracleDifferentialTargets"))


class TargetRegistry(unittest.TestCase):
    """The registry has to be independent of the phase maps it checks.

    run_phase_gate.ps1 asserts "selected test target is registered", "selected
    static-proof target is registered" and "selected oracle-differential target
    is registered", and run_differential.ps1 asserts the first of the three.
    All four read a $NxRegistered* list that was BUILT by flattening the phase
    maps, so each one compared a value against a set containing that value.
    Putting 'NxNotInTheRegistry' on a phase list printed a pass. Four checks
    that could not fire.
    """

    def test_the_registries_are_not_derived_from_the_phase_maps(self):
        text = GATE_TARGETS.read_text(encoding="utf-8")
        for _, registry in REGISTRIES:
            start = text.index("$%s = @(" % registry)
            body = text[start : text.index("\n)", start)]
            self.assertNotIn(".Values", body,
                "$%s is derived from the phase maps, so every check that reads it compares a "
                "value against a set built from that value" % registry)
            self.assertTrue(registered_targets(registry),
                "$%s parses to no names; has the shape of the file changed?" % registry)

    def test_every_name_on_a_phase_list_is_in_its_registry(self):
        for variable, registry in REGISTRIES:
            known = registered_targets(registry)
            for phase, names in phase_lists(variable).items():
                for name in names:
                    self.assertIn(name, known,
                        "phase %s registers %s under $%s but it is not in $%s"
                        % (phase, name, variable, registry))

    def test_every_registered_name_is_on_a_phase_list(self):
        """A name in the registry that no phase runs is a target nothing gates."""
        for variable, registry in REGISTRIES:
            used = {name for names in phase_lists(variable).values() for name in names}
            for name in registered_targets(registry):
                self.assertIn(name, used,
                    "$%s lists %s but no phase runs it" % (registry, name))

    def _run_differential(self, directory, phase):
        return subprocess.run(
            ["powershell.exe", "-NoProfile", "-NonInteractive", "-File",
             str(Path(directory) / "run_differential.ps1"), "-Phase", phase],
            capture_output=True, text=True, timeout=180)

    def test_an_unregistered_target_is_rejected(self):
        """The check firing, not the data being consistent.

        run_differential.ps1 tests the registry before it stages anything or
        builds anything, so this drives the real runner over a copied registry
        with one name added to a phase list and not to the registry. The control
        is the same copy unmodified: it must reach the skip, which is what says
        the failure below is the registry check and not the copy being broken.
        """
        if shutil.which("powershell.exe") is None:
            self.skipTest("powershell.exe is not on PATH")
        with tempfile.TemporaryDirectory() as directory:
            shutil.copy2(RUN_DIFFERENTIAL, Path(directory) / "run_differential.ps1")
            registry = GATE_TARGETS.read_text(encoding="utf-8")
            shutil.copy2(GATE_TARGETS, Path(directory) / "gate_targets.ps1")

            control = self._run_differential(directory, "8")
            self.assertEqual(control.returncode, 3, control.stdout + control.stderr)
            self.assertIn("differential=skipped phase=8", control.stdout)

            mutated = registry.replace("    '8' = @()\n", "    '8' = @('NxNotInTheRegistry')\n", 1)
            self.assertNotEqual(mutated, registry, "the phase-8 staged-pair list was not found")
            (Path(directory) / "gate_targets.ps1").write_text(mutated, encoding="utf-8")

            rejected = self._run_differential(directory, "8")
            transcript = rejected.stdout + rejected.stderr
            # On the exact requirement, not on the exit code: the copy is
            # outside the evidence tree, so a run that got past the registry
            # would fail a few lines later on program.json and a bare
            # `returncode != 0` would pass for the wrong reason. Against the
            # derived registry this printed `pass: test target is registered in
            # gate_targets.ps1: NxNotInTheRegistry` and went on.
            self.assertIn("GATE FAILED: requirement not met: test target is registered in "
                          "gate_targets.ps1: NxNotInTheRegistry", transcript,
                "an unregistered target was accepted: %s" % transcript)
            self.assertNotIn("pass: test target is registered in gate_targets.ps1: "
                             "NxNotInTheRegistry", transcript)
            self.assertNotEqual(rejected.returncode, 0)


def coverage_floor():
    text = GATE_TARGETS.read_text(encoding="utf-8")
    start = text.index("$NxPhaseCoverageFloor")
    body = text[start : text.index("\n}", start)]
    return {phase: int(value)
            for phase, value in re.findall(r"'([0-9]+)'\s*=\s*([0-9]+)", body)}


class CoverageFloor(unittest.TestCase):
    """The floor that turns an emptied target list into a failure.

    Every coverage assertion in run_phase_gate.ps1 sits inside a loop over a
    target list. Emptying `$NxPhaseOracleDifferentialTargets['3']` evaluated 0
    of 37 assertions and Phase 3 still printed status=pass -- the "nothing
    registered cannot be gated" guard reads only the staged-pair list, and the
    python suite was unchanged at 491 because nothing referenced the variable.

    The floor is checked here as well as in the runner, so lowering it to match
    a lost registration takes an edit to gate_targets.ps1 and an edit to this
    file, and the two show up together.
    """

    # Pinned independently of the registry. Raising this is fine; lowering it is
    # the edit that has to be justified.
    MINIMUM = {"3": 103, "4": 101, "5": 180}

    def test_the_floor_is_at_least_what_this_task_recorded(self):
        floor = coverage_floor()
        for phase, minimum in self.MINIMUM.items():
            self.assertIn(phase, floor)
            self.assertGreaterEqual(floor[phase], minimum,
                "phase %s's coverage floor is %d, below the %d assertions recorded when it was "
                "set; lowering it hides registrations that stopped being evaluated"
                % (phase, floor[phase], minimum))

    def test_the_floor_matches_what_the_phase_actually_registers(self):
        registry = registered_lines()
        lists = {}
        for variable in ("NxPhaseTestTargets", "NxPhaseStaticProofTargets",
                         "NxPhaseOracleDifferentialTargets"):
            for phase, names in phase_lists(variable).items():
                lists.setdefault(phase, []).extend(names)
        floor = coverage_floor()
        for phase, expected in floor.items():
            actual = sum(len(registry.get(name, [])) for name in lists.get(phase, []))
            self.assertEqual(actual, expected,
                "phase %s registers %d coverage lines across its targets but its floor says %d; "
                "one of them was changed without the other" % (phase, actual, expected))

    def test_every_registered_target_is_on_a_phase_list(self):
        """A registration for a target no phase runs is evaluated zero times."""
        lists = set()
        for variable in ("NxPhaseTestTargets", "NxPhaseStaticProofTargets",
                         "NxPhaseOracleDifferentialTargets"):
            for names in phase_lists(variable).values():
                lists.update(names)
        for target in registered_lines():
            self.assertIn(target, lists,
                "%s registers coverage lines but is on no phase's target list, so none of them "
                "is ever evaluated" % target)


def collision_driven_names():
    """The dispatch-matrix entries NxPhysicsCollisionTests drives."""
    source = COLLISION_SOURCE.read_text(encoding="utf-8")
    start = source.index("static const NxDrivenEntry nxDriven[]")
    body = source[start : source.index("};", start)]
    return re.findall(r'\{\s*"([A-Za-z0-9_]+)"\s*,', body)


class OracleDifferentialCoverageLines(unittest.TestCase):
    """The same two directions as above, for the oracle-side differential.

    This registration is load-bearing in a way the staged-pair one is not. An
    oracle differential compares in-process, so a wrong reconstruction fails on
    its own exit code -- but a harness that has quietly stopped exercising
    anything reports no mismatches and exits 0. The digests are the only thing
    that can catch that, and only if every generator has one.
    """

    def setUp(self):
        if not COLLISION_SOURCE.exists():
            self.skipTest("implementation tree not present at %s" % COLLISION_SOURCE)
        self.registered = registered_lines().get("NxPhysicsCollisionTests", [])

    def test_the_target_is_registered_at_all(self):
        self.assertTrue(self.registered,
            "NxPhysicsCollisionTests registers no coverage lines, so a run that "
            "checked nothing would still pass the gate")

    def test_every_driven_row_has_a_digest_and_a_coverage_line(self):
        names = list(collision_driven_names())
        self.assertTrue(names, "no driven entries found; has the harness changed shape?")
        blocks = ["%s.%s" % (name, kind) for name in names for kind in ("random", "aimed")]
        blocks += list(COLLISION_DIRECT_BLOCKS)
        for block in blocks:
            digest = "collision name=%s " % block
            coverage = "collision coverage name=%s " % block
            self.assertTrue(any(line.startswith(digest) for line in self.registered),
                "the harness drives %s but gate_targets.ps1 registers no digest line for "
                "it, so losing that generator would not fail the gate" % block)
            self.assertTrue(any(line.startswith(coverage) for line in self.registered),
                "the harness reports coverage for %s but gate_targets.ps1 registers no "
                "line for it" % block)

    def test_every_registration_names_a_block_the_harness_still_drives(self):
        names = list(collision_driven_names())
        live = {"%s.%s" % (name, kind) for name in names for kind in ("random", "aimed")}
        live |= set(COLLISION_DIRECT_BLOCKS)
        for line in self.registered:
            match = re.match(r"collision (?:coverage )?name=(\S+) ", line)
            if match:
                self.assertIn(match.group(1), live,
                    "gate_targets.ps1 registers %s but the harness no longer drives it"
                    % match.group(1))

    def test_every_digest_registration_is_oracle_side(self):
        """A registered digest line has to end at the oracle's digest.

        The harness prints `oracle=<a> candidate=<b> mismatches=<n>` on one
        line. Registering the whole line would pin the reconstruction's own
        output as well, and a reconstruction can always be made to print
        whatever is pinned; only the oracle half is evidence that the shipped
        DLL was called with these inputs. So a registration that reaches past
        `oracle=<digest>` is rejected here rather than left to be noticed.
        """
        for line in self.registered:
            if not line.startswith("collision name="):
                continue
            self.assertRegex(line, r" oracle=[0-9a-f]{16}$",
                "%r must end at the oracle digest: a registration that includes the "
                "candidate side can be satisfied by the reconstruction" % line)


class ThirdPartyReconstructedOwners(unittest.TestCase):
    """A driven family whose owner is NovodeX code has to be a reconstruction.

    NxPhysicsThirdPartyTests drives two different kinds of row. Most of its
    families own a VENDORED row: the census marks it `third_party` and the
    candidate side is qhull's or OPCODE's own source, so `discovered` is the
    right state for it and reconstructing it would be wrong. The families P4
    Task 2b added own rows with no `third_party` label and no upstream source at
    all -- they are NovodeX code inside the library spans, and the candidate side
    of those families is a reconstruction this repository wrote.

    Nothing tied the two together. The inventory could revert those rows to
    `discovered`, or drop the source path that says which file reconstructs them,
    and every gate would stay green because the differential does not read the
    inventory and the inventory does not read the differential. This is the
    join: for every `owner=` the registry names, if the census does not call that
    row third-party then it must stand at least at `reconstructed` and must say
    where its reconstruction lives.
    """

    RECONSTRUCTED_OR_BETTER = ("reconstructed", "statically_reviewed",
                               "dynamically_gated", "closed")

    @classmethod
    def setUpClass(cls):
        import json
        cls.registered = registered_lines().get("NxPhysicsThirdPartyTests", [])
        inventory = json.loads((TOOLS_DIR.parent / "inventory.json").read_text(encoding="utf-8"))
        cls.rows = {row["id"]: row for row in inventory["functions"]}

    def owners(self):
        found = []
        for line in self.registered:
            match = re.search(r" owner=(phys_fn_[0-9]{6}) ", line)
            if match:
                found.append((match.group(1), line))
        return found

    def test_the_registry_names_owners_at_all(self):
        """Guards the two tests below against a regex that stopped matching."""
        self.assertGreaterEqual(len(self.owners()), 16,
            "gate_targets.ps1 registers fewer owner= lines than the families it lists; "
            "the checks below would then be vacuous")

    def test_every_non_third_party_owner_is_reconstructed(self):
        checked = 0
        for identifier, line in self.owners():
            self.assertIn(identifier, self.rows,
                "gate_targets.ps1 names %s, which is not a census row" % identifier)
            row = self.rows[identifier]
            if "third_party" in row:
                continue
            checked += 1
            self.assertIn(row["state"], self.RECONSTRUCTED_OR_BETTER,
                "%s is driven by NxPhysicsThirdPartyTests (%r) and is not third-party, so its "
                "candidate side is a reconstruction -- but the census leaves it %r"
                % (identifier, line, row["state"]))
            self.assertTrue(row["source"],
                "%s is a reconstructed row driven by NxPhysicsThirdPartyTests but records no "
                "source, so nothing says which file the differential is proving" % identifier)
        self.assertGreater(checked, 0,
            "no driven family owns a non-third-party row; either the registry lost the P4 "
            "Task 2b families or the census started calling them third-party")


class OwnersSitAtTheAddressTheyClaim(unittest.TestCase):
    """`owner=` and `rva=` on the same registered line have to be the same row.

    They were not. `thirdparty name=radixsort rva=0x000e32c0
    owner=phys_fn_005141` named a row at 0x000e2d20 -- AABB::Add, a different
    translation unit in a different source file -- while the row actually at
    0x000e32c0 is phys_fn_005157, RadixSort::RadixSort. Every existing check
    passed: the id is a real census row, the census calls it third-party, the
    RVA is a real row too. Nothing compared the two, so the one artefact whose
    job is to bind a proof to a census row bound it to the wrong one, and the
    census would have recorded the proof against a function the differential
    never ran.

    This reads every registered line in the file, not only Phase 4's, because
    the failure is in the shape of the line and not in the family.
    """

    OWNER = re.compile(r" owner=(phys_fn_[0-9]{6})")
    RVA = re.compile(r" rva=(0x[0-9a-f]{8})")

    @classmethod
    def setUpClass(cls):
        import json
        inventory = json.loads((TOOLS_DIR.parent / "inventory.json").read_text(encoding="utf-8"))
        cls.rows = {row["id"]: row for row in inventory["functions"]}
        cls.by_rva = {}
        for row in inventory["functions"]:
            cls.by_rva.setdefault(row["rva"], row["id"])

    def test_every_line_naming_both_names_one_row(self):
        checked = 0
        for target, lines in sorted(registered_lines().items()):
            for line in lines:
                owner = self.OWNER.search(line)
                rva = self.RVA.search(line)
                if not owner or not rva:
                    continue
                checked += 1
                self.assertIn(owner.group(1), self.rows,
                    "%s registers %s, which is not a census row: %r"
                    % (target, owner.group(1), line))
                self.assertEqual(self.rows[owner.group(1)]["rva"], rva.group(1),
                    "%s registers owner=%s rva=%s, but %s is at %s and the row at %s is %s; "
                    "the line binds its proof to a different function than it drives"
                    % (target, owner.group(1), rva.group(1), owner.group(1),
                       self.rows[owner.group(1)]["rva"], rva.group(1),
                       self.by_rva.get(rva.group(1), "no row")))
        self.assertGreaterEqual(checked, 54,
            "only %d registered lines carry both owner= and rva=; either the registry lost "
            "them or one of the two regexes stopped matching, and this check went vacuous"
            % checked)


class AssetDifferentialRowsAreReconstructed(unittest.TestCase):
    """The `asset rows` registration has to name census rows that stand at a
    reconstruction, for the same reason ThirdPartyReconstructedOwners exists.

    NxPhysicsAssetTests names four rows on one registered line -- the PMap
    header reader, the payload loader, the mesh header reader and NxReleasePMap.
    Nothing tied that line to the census: the inventory could revert all four to
    `discovered`, or drop the source path that says which file reconstructs
    them, and every gate would stay green, because the differential does not
    read the inventory and the inventory does not read the differential. That is
    the same join the third-party families needed and the same reason.

    The line is `asset rows name=phys_fn_NNNNNN ...`, so the pattern here is
    `name=phys_fn_NNNNNN` rather than `owner=`; the first assertion guards the
    other two against a regex that stopped matching.
    """

    RECONSTRUCTED_OR_BETTER = ("reconstructed", "statically_reviewed",
                               "dynamically_gated", "closed")
    ROW = re.compile(r"=(phys_fn_[0-9]{6})")

    @classmethod
    def setUpClass(cls):
        import json
        registered = registered_lines().get("NxPhysicsAssetTests", [])
        cls.named = []
        for line in registered:
            if line.startswith("asset rows "):
                cls.named = [(identifier, line) for identifier in cls.ROW.findall(line)]
        inventory = json.loads((TOOLS_DIR.parent / "inventory.json").read_text(encoding="utf-8"))
        cls.rows = {row["id"]: row for row in inventory["functions"]}

    def test_the_registry_names_asset_rows_at_all(self):
        self.assertGreaterEqual(len(self.named), 4,
            "gate_targets.ps1 no longer registers an `asset rows` line naming four census "
            "rows; the check below would be vacuous")

    def test_every_named_asset_row_is_reconstructed(self):
        for identifier, line in self.named:
            self.assertIn(identifier, self.rows,
                "gate_targets.ps1 names %s, which is not a census row" % identifier)
            row = self.rows[identifier]
            self.assertIn(row["state"], self.RECONSTRUCTED_OR_BETTER,
                "%s is driven by NxPhysicsAssetTests (%r) but the census leaves it %r"
                % (identifier, line, row["state"]))
            self.assertTrue(row["source"],
                "%s is driven by NxPhysicsAssetTests but records no source, so nothing says "
                "which file the differential is proving" % identifier)


class RetypedArtifactsStayCode(unittest.TestCase):
    """The 63 rows P4 Task 3 retyped out of `compiler_artifact` must stay code.

    Their old classification proof was `no product translation unit reaches
    these bytes and none is named above the last one that does, at 0x000e9100`.
    It is false for every one of the 63: 35 are the target of a direct
    `call`/`jmp rel32` from another censused row and 28 have their address
    written into the OPCODE vtable pool at .rdata:0x0011b5a4-0x0011bcf8, which
    is an indirect dispatch a reach heuristic built on direct calls cannot see.
    The split was published as 27/36 -- right total, wrong halves, and the
    halves swapped -- so it is asserted below rather than only described here.
    While they carried `compiler_artifact`, validate_inventory.py refused any
    row among them that recorded product source, so 8,919 bytes could not be
    closed at all.

    evidence/phase4-artifact-retype.csv carries the reference that reaches each
    one. A MISSING OR SHORT FILE IS A FAILURE, not an empty result: an empty
    file would make `every row named is code` vacuously true, which is the shape
    of unfailable check this programme keeps finding.
    """

    @classmethod
    def setUpClass(cls):
        import csv
        import json
        path = TOOLS_DIR.parent / "evidence" / "phase4-artifact-retype.csv"
        cls.entries = list(csv.DictReader(path.read_text(encoding="utf-8").splitlines()))
        inventory = json.loads((TOOLS_DIR.parent / "inventory.json").read_text(encoding="utf-8"))
        cls.rows = {row["id"]: row for row in inventory["functions"]}

    def test_the_evidence_file_still_carries_all_63(self):
        self.assertEqual(len(self.entries), 63,
            "phase4-artifact-retype.csv records %d rows, not the 63 P4 Task 3 retyped"
            % len(self.entries))
        self.assertEqual(sorted({entry["reached_by"] for entry in self.entries}),
                         ["direct_call", "vtable_slot"])

    def test_the_published_split_is_the_one_the_csv_records(self):
        # The escalation, both gate records, the pmap evidence and the Phase 8
        # plan all quote this split. It was 27/36 in every one of them against a
        # CSV that says 35/28, and nothing compared the two.
        split = collections.Counter(entry["reached_by"] for entry in self.entries)
        self.assertEqual(split["direct_call"], 35)
        self.assertEqual(split["vtable_slot"], 28)

    def test_every_retyped_row_is_code_at_the_recorded_address(self):
        for entry in self.entries:
            self.assertIn(entry["id"], self.rows,
                "%s is not a census row" % entry["id"])
            row = self.rows[entry["id"]]
            self.assertEqual(row["rva"], entry["rva"],
                "%s is censused at %s and the retype evidence names %s"
                % (entry["id"], row["rva"], entry["rva"]))
            self.assertEqual(row["size"], int(entry["size"]),
                "%s is %d bytes and the retype evidence names %s"
                % (entry["id"], row["size"], entry["size"]))
            self.assertEqual(row["kind"], "code",
                "%s is reached (%s) and must not carry kind %r: while it does, "
                "validate_inventory.py rejects any reconstruction recorded against it"
                % (entry["id"], entry["evidence"], row["kind"]))
            self.assertTrue(entry["evidence"],
                "%s records no reference that reaches it" % entry["id"])

if __name__ == "__main__":
    unittest.main()
