"""Behavioral checks for diagnostic lines that differential gates must reject."""

import subprocess
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
CHECKER = TOOLS_DIR / "assert_gate_transcript.ps1"


def run_checker(transcript: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            "powershell.exe",
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-Command",
            f"$ErrorActionPreference='Stop'; . '{CHECKER}'; Assert-NoCandidateFailures @('{transcript}') 'probe'; exit 0",
        ],
        capture_output=True,
        text=True,
    )


def test_zero_candidate_failures_are_accepted():
    result = run_checker("layout candidate failures=0 provisional=1")
    assert result.returncode == 0, result.stdout + result.stderr


def test_positive_candidate_failures_fail_the_gate():
    result = run_checker("layout candidate failures=3 provisional=1")
    assert result.returncode != 0
    assert "candidate failures=3" in result.stdout + result.stderr


def test_unrelated_transcript_lines_are_accepted():
    result = run_checker("layout candidate mismatches=0 mode=differential")
    assert result.returncode == 0, result.stdout + result.stderr
