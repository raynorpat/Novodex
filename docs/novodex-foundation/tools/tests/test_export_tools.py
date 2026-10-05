import importlib.util
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]


def load_tool(name):
    path = TOOLS_DIR / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def make_pe32(path):
    image = bytearray(0x600)
    struct.pack_into("<I", image, 0x3C, 0x80)
    image[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", image, 0x84, 0x14C, 1, 0, 0, 0, 0xE0, 0x210E)

    optional = 0x98
    struct.pack_into("<H", image, optional, 0x10B)
    struct.pack_into("<II", image, optional + 96, 0x1000, 0x100)

    section = optional + 0xE0
    image[section:section + 8] = b".rdata\0\0"
    struct.pack_into("<IIII", image, section + 8, 0x400, 0x1000, 0x400, 0x200)

    export = 0x200
    struct.pack_into(
        "<IIHHIIIIIII",
        image,
        export,
        0,
        0,
        0,
        0,
        0x10A0,
        7,
        2,
        2,
        0x1060,
        0x1070,
        0x1080,
    )
    struct.pack_into("<II", image, 0x260, 0x1100, 0x1110)
    struct.pack_into("<II", image, 0x270, 0x10B0, 0x10B6)
    struct.pack_into("<HH", image, 0x280, 1, 0)
    image[0x2A0:0x2A9] = b"test.dll\0"
    image[0x2B0:0x2B6] = b"Alpha\0"
    image[0x2B6:0x2BB] = b"Beta\0"
    path.write_bytes(image)


class PeExportsTests(unittest.TestCase):
    def test_reads_named_exports_and_computes_ordinals(self):
        tool = load_tool("pe_exports")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.dll"
            make_pe32(path)
            self.assertEqual(tool.pe_named_exports(path), [(8, "Alpha"), (7, "Beta")])

    def test_rejects_non_pe32_optional_header(self):
        tool = load_tool("pe_exports")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.dll"
            make_pe32(path)
            image = bytearray(path.read_bytes())
            struct.pack_into("<H", image, 0x98, 0x20B)
            path.write_bytes(image)
            with self.assertRaisesRegex(ValueError, "PE32"):
                tool.pe_named_exports(path)

    def test_rejects_export_name_without_terminator_in_its_section(self):
        tool = load_tool("pe_exports")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.dll"
            make_pe32(path)
            image = bytearray(path.read_bytes())
            struct.pack_into("<I", image, 0x218, 1)
            struct.pack_into("<I", image, 0x188, 0xB6)
            image[0x2B5] = ord("X")
            path.write_bytes(image)
            with self.assertRaisesRegex(ValueError, "unterminated export name"):
                tool.pe_named_exports(path)


class CompareExportsTests(unittest.TestCase):
    def run_compare(self, oracle_text, rebuilt_text):
        with tempfile.TemporaryDirectory() as directory:
            oracle = Path(directory) / "oracle.txt"
            rebuilt = Path(directory) / "rebuilt.txt"
            oracle.write_text(oracle_text, encoding="utf-8")
            rebuilt.write_text(rebuilt_text, encoding="utf-8")
            return subprocess.run(
                [
                    sys.executable,
                    str(TOOLS_DIR / "compare_exports.py"),
                    "--oracle",
                    str(oracle),
                    "--rebuilt",
                    str(rebuilt),
                ],
                capture_output=True,
                text=True,
                check=False,
            )

    def test_missing_oracle_export_fails(self):
        result = self.run_compare("1 Alpha\n2 Beta\n", "9 Alpha\n10 Extra\n")
        self.assertEqual(result.returncode, 1)
        self.assertIn("oracle: 2", result.stdout)
        self.assertIn("rebuilt: 2", result.stdout)
        self.assertIn("MISSING Beta", result.stdout)
        self.assertIn("EXTRA Extra", result.stdout)

    def test_extra_exports_alone_succeed(self):
        result = self.run_compare("1 Alpha\n", "9 Alpha\n10 Extra\n")
        self.assertEqual(result.returncode, 0)
        self.assertIn("EXTRA Extra", result.stdout)

    def test_unreadable_input_reports_cli_error_without_traceback(self):
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "missing.txt"
            result = subprocess.run(
                [
                    sys.executable,
                    str(TOOLS_DIR / "compare_exports.py"),
                    "--oracle",
                    str(missing),
                    "--rebuilt",
                    str(missing),
                ],
                capture_output=True,
                text=True,
                check=False,
            )
        self.assertEqual(result.returncode, 2)
        self.assertIn("error:", result.stderr)
        self.assertNotIn("Traceback", result.stderr)


if __name__ == "__main__":
    unittest.main()
