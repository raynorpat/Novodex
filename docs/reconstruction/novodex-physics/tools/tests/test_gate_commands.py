"""The phase plans invoke run_differential.ps1 as written; these run them.

Six plan files independently reach for `run_differential.ps1 -Phase N`. Each is
executed here to prove the parameter binds and the phase resolves to its
registered targets. Whether the differential then passes is not the point --
Some phases legitimately fail today, and Phase 4 has nothing registered -- the
point is that no plan carries a command PowerShell cannot bind.

The command lines are parsed out of the plan files rather than copied here, so a
plan that drifts is caught instead of a stale copy agreeing with itself.
"""

import re
import subprocess
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
REPO_DIR = Path(__file__).resolve().parents[5]
PLANS_DIR = REPO_DIR / "docs" / "superpowers" / "plans"
RUNNER = TOOLS_DIR / "run_differential.ps1"

COMMAND_PATTERN = re.compile(
    r"^powershell -NoProfile -File (?P<script>\S+) -Phase (?P<phase>[1-8])$",
    re.MULTILINE,
)

# What each phase in the plans must resolve to from gate_targets.ps1. A phase
# with nothing registered resolves to the empty list and skips.
PHASE_TARGETS = {
    "2": "NxPhysicsExportTests,NxPhysicsSDKTests,NxPhysicsCoreClusterTests",
    "3": "NxPhysicsGeometryTests,NxPhysicsKernelFuzzTests",
    "4": "",
    "5": "NxPhysicsActorLifecycleTests,NxPhysicsDynamicFirstTests,NxPhysicsEmptySceneTests,NxPhysicsActorNameTests,NxPhysicsActorMetadataTests,NxPhysicsActorBodyFlagTests",
    # Phase 6 has a registered STAGED-PAIR target: a closure is a mutation to a row's
    # implementation, and only a staged-pair target loads the rebuilt module, so the
    # closure schema needs one (evidence 11l).
    "6": "NxPhysicsJointStagedPairTests",
    # Phase 7 registered the same target for the same reason: five of the six rows
    # recorded in 11u are Phase 7's own and execute in that harness.
    "7": "NxPhysicsJointStagedPairTests",
}
# Phases 6 and 7 moved out of this set when NxPhysicsJointStagedPairTests was
# registered on them.
UNREGISTERED_PHASES = ("4",)
SKIPPED_EXIT = 3

BINDING_FAILURES = (
    "ParameterBindingException",
    "A positional parameter cannot be found",
    "Cannot process argument transformation",
    "does not belong to the set",
    "Parameter set cannot be resolved",
)


def plan_commands():
    """Every `run_differential.ps1 -Phase N` command line in the phase plans."""
    found = []
    for plan in sorted(PLANS_DIR.glob("*novodex-physics-phase*.md")):
        text = plan.read_text(encoding="utf-8")
        for match in COMMAND_PATTERN.finditer(text):
            found.append((plan.name, match.group("script"), match.group("phase")))
    return found


def run_powershell(*arguments):
    return subprocess.run(
        ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass", *arguments],
        cwd=REPO_DIR,
        capture_output=True,
        text=True,
    )


class PlanCommandTests(unittest.TestCase):
    def test_every_physics_phase_plan_carries_a_command(self):
        phases = sorted(phase for _, _, phase in plan_commands())
        self.assertEqual(phases, ["2", "3", "4", "5", "6", "7"])

    def test_every_named_script_exists(self):
        for plan, script, phase in plan_commands():
            with self.subTest(plan=plan, phase=phase):
                self.assertTrue((REPO_DIR / script).is_file(), f"{plan} names {script}")

    def test_every_command_binds_and_resolves_as_written(self):
        for plan, script, phase in plan_commands():
            with self.subTest(plan=plan, phase=phase):
                result = run_powershell("-File", str(REPO_DIR / script), "-Phase", phase)
                output = result.stdout + result.stderr
                for failure in BINDING_FAILURES:
                    self.assertNotIn(failure, output, f"{plan}: {output}")
                # A prefix match would pass with a target appended, which is
                # exactly how a new target reaches the gate unreviewed.
                self.assertIn(
                    f"phase={phase} resolved_targets={PHASE_TARGETS[phase]}\n",
                    output,
                    f"{plan}: {output}",
                )


class PhaseResolutionTests(unittest.TestCase):
    def test_unregistered_phase_skips_rather_than_passing(self):
        for phase in UNREGISTERED_PHASES:
            with self.subTest(phase=phase):
                result = run_powershell("-File", str(RUNNER), "-Phase", phase)
                self.assertIn(f"differential=skipped phase={phase}", result.stdout)
                self.assertEqual(result.returncode, SKIPPED_EXIT)

    def test_phase_and_targets_are_mutually_exclusive(self):
        result = run_powershell(
            "-File", str(RUNNER), "-Phase", "2", "-Targets", "NxPhysicsExportTests"
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Parameter set cannot be resolved", result.stdout + result.stderr)

    def test_unknown_phase_is_rejected(self):
        result = run_powershell("-File", str(RUNNER), "-Phase", "9")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("does not belong to the set", result.stdout + result.stderr)

    def test_unregistered_target_is_rejected(self):
        result = run_powershell("-File", str(RUNNER), "-Targets", "NxBogusTests")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(
            "test target is registered in gate_targets.ps1: NxBogusTests",
            result.stdout + result.stderr,
        )


if __name__ == "__main__":
    unittest.main()
