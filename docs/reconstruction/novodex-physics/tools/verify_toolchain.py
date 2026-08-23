#!/usr/bin/env python3
"""Pin and verify the Ghidra and Capstone toolchain used to census NxPhysics.dll."""

import argparse
import hashlib
import json
import platform
import sys
from pathlib import Path

import capstone


SCHEMA_VERSION = 1

# The pinned headless analysis contract. Every extractor must verify this pin
# before producing evidence, so any change here invalidates existing evidence.
DEFAULT_ANALYSIS_OPTIONS = {
    "processor": "x86",
    "language_id": "x86:LE:32:default",
    "compiler_spec_id": "windows",
    "analysis_timeout_seconds": 3600,
    "analyzers": {
        # Guessing at undiscovered code makes results order-dependent.
        "Aggressive Instruction Finder": False,
        "Decompiler Parameter ID": True,
        "Demangler Microsoft": True,
        "Non-Returning Functions - Discovered": True,
        "Windows x86 PE Exception Handling": True,
        "Windows x86 PE RTTI Analyzer": True,
    },
}

# Fields the pin refuses to see change. Ghidra is pinned to an explicit
# installation, so its path is part of the identity; the Capstone package moves
# with the Python environment, so only its version and content are pinned.
VERIFIED_FIELDS = (
    ("ghidra", ("version", "headless_path", "headless_sha256")),
    ("capstone", ("version", "package_sha256")),
)


def _file_sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def tree_sha256(root) -> str:
    """Hash a directory's files by relative path and content, ignoring bytecode caches."""
    root = Path(root)
    entries = sorted(
        (path.relative_to(root).as_posix(), path)
        for path in root.rglob("*")
        if path.is_file() and "__pycache__" not in path.parts
    )
    digest = hashlib.sha256()
    for relative, path in entries:
        digest.update(relative.encode("utf-8"))
        digest.update(b"\0")
        digest.update(_file_sha256(path).encode("ascii"))
    return digest.hexdigest()


def canonical_options_sha256(analysis_options) -> str:
    """Hash analysis options independently of key order."""
    canonical = json.dumps(analysis_options, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(canonical.encode("utf-8")).hexdigest()


def _read_property(path, key):
    for line in path.read_text(encoding="utf-8").splitlines():
        name, separator, value = line.partition("=")
        if separator and name.strip() == key:
            return value.strip()
    raise ValueError(f"{path} does not define {key}")


def measure_toolchain(ghidra_home, analysis_options) -> dict:
    """Measure the installed Ghidra and Capstone toolchain against an analysis contract."""
    home = Path(ghidra_home).resolve()
    properties = home / "Ghidra" / "application.properties"
    headless = home / "support" / "analyzeHeadless.bat"
    for required in (properties, headless):
        if not required.is_file():
            raise ValueError(f"{required} is missing; {home} is not a Ghidra installation")

    package_root = Path(capstone.__file__).resolve().parent
    return {
        "schema_version": SCHEMA_VERSION,
        "ghidra": {
            "home": str(home),
            "version": _read_property(properties, "application.version"),
            "headless_path": str(headless),
            "headless_sha256": _file_sha256(headless),
        },
        "capstone": {
            "version": capstone.__version__,
            "package_root": str(package_root),
            "package_sha256": tree_sha256(package_root),
        },
        "python": {"version": platform.python_version()},
        "analysis_options": analysis_options,
        "analysis_options_sha256": canonical_options_sha256(analysis_options),
    }


def verify_toolchain(pin: dict, measured: dict) -> list[str]:
    """Return every difference between a recorded pin and a fresh measurement."""
    errors = []
    if pin.get("schema_version") != measured["schema_version"]:
        errors.append(
            f"schema_version is {measured['schema_version']} but the pin records "
            f"{pin.get('schema_version')!r}"
        )
    for section, fields in VERIFIED_FIELDS:
        recorded = pin.get(section) or {}
        for field in fields:
            actual = measured[section][field]
            if recorded.get(field) != actual:
                errors.append(
                    f"{section}.{field} is {actual!r} but the pin records {recorded.get(field)!r}"
                )
    if pin.get("analysis_options") != measured["analysis_options"]:
        errors.append("analysis options do not match the pin")
    if pin.get("analysis_options_sha256") != measured["analysis_options_sha256"]:
        errors.append(
            f"analysis-options hash is {measured['analysis_options_sha256']} but the pin "
            f"records {pin.get('analysis_options_sha256')!r}"
        )
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write-pin", action="store_true", help="measure the toolchain into --output")
    parser.add_argument("--ghidra-home", required=True, help="pinned Ghidra installation root")
    parser.add_argument("--output", help="pin to write with --write-pin")
    parser.add_argument("--pin", help="pin to verify the installed toolchain against")
    args = parser.parse_args()

    if args.write_pin and not args.output:
        parser.error("--write-pin requires --output")
    if not args.write_pin and not args.pin:
        parser.error("--pin is required without --write-pin")

    try:
        measured = measure_toolchain(args.ghidra_home, DEFAULT_ANALYSIS_OPTIONS)
        if args.write_pin:
            Path(args.output).write_text(json.dumps(measured, indent=2) + "\n",
                                         encoding="utf-8", newline="\n")
        else:
            pin = json.loads(Path(args.pin).read_text(encoding="utf-8"))
            errors = verify_toolchain(pin, measured)
            if errors:
                for error in errors:
                    print(f"error: {error}", file=sys.stderr)
                return 1
    except (OSError, ValueError) as error:
        parser.exit(2, f"error: {error}\n")

    print("toolchain=written" if args.write_pin else "toolchain=pass")
    print(f"ghidra={measured['ghidra']['version']}")
    print(f"capstone={measured['capstone']['version']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
