import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import verify_public_headers  # noqa: E402


def make_header_tree(root):
    (root / "fluids").mkdir(parents=True, exist_ok=True)
    (root / "NxPhysics.h").write_text("#define NX_PHYSICS_SDK 1\n", encoding="utf-8")
    (root / "NxActor.h").write_text("class NxActor {};\n", encoding="utf-8")
    (root / "fluids" / "NxFluid.h").write_text("class NxFluid {};\n", encoding="utf-8")
    return root


class BuildManifestTests(unittest.TestCase):
    def test_records_every_file_recursively_with_posix_paths(self):
        with tempfile.TemporaryDirectory() as directory:
            root = make_header_tree(Path(directory))
            manifest = verify_public_headers.build_manifest(root)
        paths = [entry["path"] for entry in manifest["files"]]
        self.assertEqual(paths, ["NxActor.h", "NxPhysics.h", "fluids/NxFluid.h"])
        self.assertEqual(manifest["file_count"], 3)
        self.assertEqual(manifest["algorithm"], "SHA-256")
        for entry in manifest["files"]:
            self.assertEqual(len(entry["sha256"]), 64)
            self.assertGreater(entry["size"], 0)

    def test_manifest_is_deterministic(self):
        with tempfile.TemporaryDirectory() as directory:
            root = make_header_tree(Path(directory))
            self.assertEqual(
                verify_public_headers.build_manifest(root),
                verify_public_headers.build_manifest(root),
            )

    def test_rejects_a_missing_root(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, "not a directory"):
                verify_public_headers.build_manifest(Path(directory) / "absent")


class VerifyManifestTests(unittest.TestCase):
    def assertRejects(self, mutate, pattern):
        with tempfile.TemporaryDirectory() as directory:
            root = make_header_tree(Path(directory))
            manifest = verify_public_headers.build_manifest(root)
            mutate(root)
            errors = verify_public_headers.verify_manifest(manifest, root)
        self.assertTrue(errors, "expected at least one error")
        self.assertRegex("\n".join(errors), pattern)

    def test_unchanged_tree_verifies(self):
        with tempfile.TemporaryDirectory() as directory:
            root = make_header_tree(Path(directory))
            manifest = verify_public_headers.build_manifest(root)
            self.assertEqual(verify_public_headers.verify_manifest(manifest, root), [])

    def test_rejects_missing_file(self):
        self.assertRejects(
            lambda root: (root / "NxActor.h").unlink(),
            "missing header NxActor.h",
        )

    def test_rejects_missing_nested_file(self):
        self.assertRejects(
            lambda root: (root / "fluids" / "NxFluid.h").unlink(),
            "missing header fluids/NxFluid.h",
        )

    def test_rejects_extra_file(self):
        self.assertRejects(
            lambda root: (root / "NxExtra.h").write_text("//\n", encoding="utf-8"),
            "unexpected header NxExtra.h",
        )

    def test_rejects_extra_nested_file(self):
        self.assertRejects(
            lambda root: (root / "fluids" / "NxExtra.h").write_text("//\n", encoding="utf-8"),
            "unexpected header fluids/NxExtra.h",
        )

    def test_rejects_byte_different_file_of_the_same_size(self):
        self.assertRejects(
            lambda root: (root / "NxActor.h").write_text("class NxActor {}:\n", encoding="utf-8"),
            r"NxActor\.h sha256",
        )

    def test_rejects_resized_file(self):
        self.assertRejects(
            lambda root: (root / "NxActor.h").write_text("class NxActor { int a; };\n", encoding="utf-8"),
            r"NxActor\.h size",
        )


class VerifyPublicHeadersCliTests(unittest.TestCase):
    def run_cli(self, *arguments):
        return subprocess.run(
            [sys.executable, str(TOOLS_DIR / "verify_public_headers.py"), *arguments],
            capture_output=True,
            text=True,
            check=False,
        )

    def test_written_manifest_verifies_against_the_same_tree(self):
        with tempfile.TemporaryDirectory() as directory:
            root = make_header_tree(Path(directory) / "include")
            manifest = Path(directory) / "public_header_hashes.json"
            written = self.run_cli(
                "--write-manifest", "--root", str(root), "--output", str(manifest)
            )
            self.assertEqual(written.returncode, 0, written.stderr)
            self.assertIn("files=3", written.stdout)
            self.assertEqual(
                json.loads(manifest.read_text(encoding="utf-8"))["file_count"], 3
            )

            verified = self.run_cli("--manifest", str(manifest), "--root", str(root))
            self.assertEqual(verified.returncode, 0, verified.stderr)
            self.assertIn("public_headers=pass", verified.stdout)

    def test_changed_tree_fails_verification(self):
        with tempfile.TemporaryDirectory() as directory:
            root = make_header_tree(Path(directory) / "include")
            manifest = Path(directory) / "public_header_hashes.json"
            self.run_cli("--write-manifest", "--root", str(root), "--output", str(manifest))
            (root / "fluids" / "NxFluid.h").unlink()

            result = self.run_cli("--manifest", str(manifest), "--root", str(root))
        self.assertEqual(result.returncode, 1)
        self.assertIn("error:", result.stderr)
        self.assertIn("fluids/NxFluid.h", result.stderr)
        self.assertNotIn("public_headers=pass", result.stdout)

    def test_missing_root_reports_cli_error_without_traceback(self):
        with tempfile.TemporaryDirectory() as directory:
            result = self.run_cli(
                "--write-manifest",
                "--root",
                str(Path(directory) / "absent"),
                "--output",
                str(Path(directory) / "manifest.json"),
            )
        self.assertEqual(result.returncode, 2)
        self.assertIn("error:", result.stderr)
        self.assertNotIn("Traceback", result.stderr)


if __name__ == "__main__":
    unittest.main()
