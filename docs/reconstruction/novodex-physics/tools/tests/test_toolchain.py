import copy
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import verify_toolchain  # noqa: E402


def make_ghidra_home(root, version="12.1.2", headless_body="rem headless\n"):
    (root / "Ghidra").mkdir(parents=True, exist_ok=True)
    (root / "support").mkdir(parents=True, exist_ok=True)
    (root / "Ghidra" / "application.properties").write_text(
        "application.name=Ghidra\n"
        f"application.version={version}\n"
        "application.release.name=PUBLIC\n",
        encoding="utf-8",
    )
    (root / "support" / "analyzeHeadless.bat").write_text(headless_body, encoding="utf-8")
    return root


class MeasureToolchainTests(unittest.TestCase):
    def measure(self, root):
        return verify_toolchain.measure_toolchain(
            root, verify_toolchain.DEFAULT_ANALYSIS_OPTIONS
        )

    def test_records_every_pinned_field(self):
        with tempfile.TemporaryDirectory() as directory:
            home = make_ghidra_home(Path(directory))
            measured = self.measure(home)
        self.assertEqual(measured["ghidra"]["version"], "12.1.2")
        self.assertTrue(measured["ghidra"]["headless_path"].endswith("analyzeHeadless.bat"))
        self.assertEqual(len(measured["ghidra"]["headless_sha256"]), 64)
        self.assertEqual(len(measured["capstone"]["package_sha256"]), 64)
        self.assertTrue(measured["capstone"]["version"])
        self.assertTrue(measured["python"]["version"])
        options = measured["analysis_options"]
        self.assertEqual(options["language_id"], "x86:LE:32:default")
        self.assertEqual(options["compiler_spec_id"], "windows")
        self.assertEqual(options["analysis_timeout_seconds"], 3600)
        self.assertIn("Decompiler Parameter ID", options["analyzers"])
        self.assertEqual(len(measured["analysis_options_sha256"]), 64)

    def test_rejects_ghidra_home_without_headless_launcher(self):
        with tempfile.TemporaryDirectory() as directory:
            home = make_ghidra_home(Path(directory))
            (home / "support" / "analyzeHeadless.bat").unlink()
            with self.assertRaisesRegex(ValueError, "analyzeHeadless.bat"):
                self.measure(home)

    def test_rejects_ghidra_home_without_application_properties(self):
        with tempfile.TemporaryDirectory() as directory:
            home = make_ghidra_home(Path(directory))
            (home / "Ghidra" / "application.properties").unlink()
            with self.assertRaisesRegex(ValueError, "application.properties"):
                self.measure(home)

    def test_rejects_application_properties_without_a_version(self):
        with tempfile.TemporaryDirectory() as directory:
            home = make_ghidra_home(Path(directory))
            (home / "Ghidra" / "application.properties").write_text(
                "application.name=Ghidra\n", encoding="utf-8"
            )
            with self.assertRaisesRegex(ValueError, "application.version"):
                self.measure(home)

    def test_analysis_options_hash_is_order_independent(self):
        options = copy.deepcopy(verify_toolchain.DEFAULT_ANALYSIS_OPTIONS)
        reordered = dict(reversed(list(options.items())))
        self.assertNotEqual(list(options), list(reordered))
        self.assertEqual(
            verify_toolchain.canonical_options_sha256(options),
            verify_toolchain.canonical_options_sha256(reordered),
        )

    def test_analysis_options_hash_changes_with_content(self):
        options = copy.deepcopy(verify_toolchain.DEFAULT_ANALYSIS_OPTIONS)
        options["analysis_timeout_seconds"] = 60
        self.assertNotEqual(
            verify_toolchain.canonical_options_sha256(options),
            verify_toolchain.canonical_options_sha256(
                verify_toolchain.DEFAULT_ANALYSIS_OPTIONS
            ),
        )


class TreeHashTests(unittest.TestCase):
    def test_hash_ignores_bytecode_caches(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "pkg").mkdir()
            (root / "pkg" / "__init__.py").write_text("x = 1\n", encoding="utf-8")
            before = verify_toolchain.tree_sha256(root)
            cache = root / "pkg" / "__pycache__"
            cache.mkdir()
            (cache / "__init__.cpython-313.pyc").write_bytes(b"\x00compiled")
            after = verify_toolchain.tree_sha256(root)
        self.assertEqual(before, after)

    def test_hash_covers_nested_content_and_names(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "lib").mkdir()
            (root / "lib" / "capstone.dll").write_bytes(b"binary")
            before = verify_toolchain.tree_sha256(root)
            (root / "lib" / "capstone.dll").write_bytes(b"changed")
            after_content = verify_toolchain.tree_sha256(root)
            (root / "lib" / "capstone.dll").rename(root / "lib" / "renamed.dll")
            after_name = verify_toolchain.tree_sha256(root)
        self.assertNotEqual(before, after_content)
        self.assertNotEqual(after_content, after_name)


class VerifyToolchainTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        home = make_ghidra_home(Path(self.directory.name))
        self.measured = verify_toolchain.measure_toolchain(
            home, verify_toolchain.DEFAULT_ANALYSIS_OPTIONS
        )

    def assertRejects(self, mutate, pattern):
        pin = copy.deepcopy(self.measured)
        mutate(pin)
        errors = verify_toolchain.verify_toolchain(pin, self.measured)
        self.assertTrue(errors, "expected at least one error")
        self.assertRegex("\n".join(errors), pattern)

    def test_matching_pin_has_no_errors(self):
        self.assertEqual(
            verify_toolchain.verify_toolchain(copy.deepcopy(self.measured), self.measured), []
        )

    def test_rejects_changed_ghidra_version(self):
        self.assertRejects(
            lambda pin: pin["ghidra"].__setitem__("version", "11.0.3"),
            r"ghidra\.version",
        )

    def test_rejects_changed_headless_path(self):
        self.assertRejects(
            lambda pin: pin["ghidra"].__setitem__("headless_path", "C:/other/analyzeHeadless.bat"),
            r"ghidra\.headless_path",
        )

    def test_rejects_changed_headless_hash(self):
        self.assertRejects(
            lambda pin: pin["ghidra"].__setitem__("headless_sha256", "0" * 64),
            r"ghidra\.headless_sha256",
        )

    def test_rejects_changed_capstone_version(self):
        self.assertRejects(
            lambda pin: pin["capstone"].__setitem__("version", "4.0.2"),
            r"capstone\.version",
        )

    def test_rejects_changed_capstone_package_hash(self):
        self.assertRejects(
            lambda pin: pin["capstone"].__setitem__("package_sha256", "0" * 64),
            r"capstone\.package_sha256",
        )

    def test_rejects_changed_analysis_options(self):
        self.assertRejects(
            lambda pin: pin["analysis_options"].__setitem__("analysis_timeout_seconds", 60),
            "analysis options do not match",
        )

    def test_rejects_changed_analysis_options_hash(self):
        self.assertRejects(
            lambda pin: pin.__setitem__("analysis_options_sha256", "0" * 64),
            "analysis-options hash",
        )

    def test_rejects_changed_schema_version(self):
        self.assertRejects(lambda pin: pin.__setitem__("schema_version", 99), "schema_version")


class VerifyToolchainCliTests(unittest.TestCase):
    def run_cli(self, *arguments):
        return subprocess.run(
            [sys.executable, str(TOOLS_DIR / "verify_toolchain.py"), *arguments],
            capture_output=True,
            text=True,
            check=False,
        )

    def test_written_pin_verifies_against_the_same_installation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            home = make_ghidra_home(root / "ghidra")
            pin = root / "analysis_toolchain.json"
            written = self.run_cli("--write-pin", "--ghidra-home", str(home), "--output", str(pin))
            self.assertEqual(written.returncode, 0, written.stderr)
            self.assertIn("ghidra=12.1.2", written.stdout)

            verified = self.run_cli("--pin", str(pin), "--ghidra-home", str(home))
            self.assertEqual(verified.returncode, 0, verified.stderr)
            self.assertIn("toolchain=pass", verified.stdout)
            self.assertIn("ghidra=12.1.2", verified.stdout)
            self.assertIn(
                f"capstone={json.loads(pin.read_text(encoding='utf-8'))['capstone']['version']}",
                verified.stdout,
            )

    def test_mutated_pin_fails_verification(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            home = make_ghidra_home(root / "ghidra")
            pin = root / "analysis_toolchain.json"
            self.run_cli("--write-pin", "--ghidra-home", str(home), "--output", str(pin))
            data = json.loads(pin.read_text(encoding="utf-8"))
            data["ghidra"]["version"] = "11.0.3"
            pin.write_text(json.dumps(data), encoding="utf-8")

            result = self.run_cli("--pin", str(pin), "--ghidra-home", str(home))
        self.assertEqual(result.returncode, 1)
        self.assertIn("error:", result.stderr)
        self.assertIn("11.0.3", result.stderr)
        self.assertNotIn("toolchain=pass", result.stdout)

    def test_missing_ghidra_installation_reports_cli_error_without_traceback(self):
        with tempfile.TemporaryDirectory() as directory:
            result = self.run_cli(
                "--write-pin",
                "--ghidra-home",
                str(Path(directory) / "absent"),
                "--output",
                str(Path(directory) / "pin.json"),
            )
        self.assertEqual(result.returncode, 2)
        self.assertIn("error:", result.stderr)
        self.assertNotIn("Traceback", result.stderr)


if __name__ == "__main__":
    unittest.main()
