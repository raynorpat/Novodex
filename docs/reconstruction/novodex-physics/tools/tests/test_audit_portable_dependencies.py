"""Behavior checks for the source census, independent of the repository snapshot."""
import contextlib
import io
import json
import os
import pathlib
import runpy
import tempfile
import unittest
from unittest.mock import patch

SCANNER = pathlib.Path(__file__).resolve().parents[1] / "audit_portable_dependencies.py"


class PortableDependencyAuditTests(unittest.TestCase):
    def audit(self, source):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            for name in ("Physics", "Foundation", "External", "docs/reconstruction/novodex-physics/evidence"):
                (root / name).mkdir(parents=True, exist_ok=True)
            (root / "Physics/sample.cpp").write_text(source)
            (root / "Physics/caller.cpp").write_text("void caller() { sample(1.0); bridge(); ordinary(1.0); }\n")
            previous = pathlib.Path.cwd()
            try:
                os.chdir(root)
                with contextlib.redirect_stdout(io.StringIO()), patch('sys.argv', ['audit']):
                    runpy.run_path(str(SCANNER), run_name="__main__")
            finally:
                os.chdir(previous)
            result=json.loads((root / "build/portable-audit/portable-scalar-dependencies.json").read_text())
            self.assertIsNone(result['revision'])
            self.assertIsNone(result['working_tree_dirty'])
            self.assertFalse((root / "docs/reconstruction/novodex-physics/evidence/portable-scalar-dependencies.json").exists())
            return result

    def test_naked_annotation_keeps_symbol_instructions_edges_and_label(self):
        result = self.audit('''static __declspec(naked) double __cdecl sample(double x)
{
    __asm {
        fld qword ptr [esp+4]
        fsqrt
        call bridge
        jmp continuation
        continuation: ret
    }
}
''')
        entries = result["files"][0]["entries"]
        sample = next((entry for entry in entries if entry["symbol"] == "sample"), None)
        self.assertIsNotNone(sample, "naked annotation must not swallow the function signature")
        self.assertEqual(sample["instructions"], ["call", "fld", "fsqrt", "jmp", "ret"])
        self.assertEqual(sample["assembly_call_targets"], ["bridge", "continuation"])
        self.assertEqual(sample["continuation_labels"], ["continuation"])
        self.assertIn("Physics/caller.cpp:1", sample["source_reference_sites"])
        self.assertEqual(len(entries), 1, "signature and body dependency sites belong to one symbol")

    def test_extern_c_and_ordinary_inline_signatures_keep_their_bodies(self):
        result = self.audit('''extern "C" __declspec(naked) void __cdecl bridge()
{
    __asm {
        call sample
        ret
    }
}
inline double ordinary(double x)
{
    __asm {
        fld x
        fsqrt
    }
    return x;
}
''')
        entries = {entry["symbol"]: entry for entry in result["files"][0]["entries"]}
        self.assertEqual(set(entries), {"bridge", "ordinary"})
        self.assertEqual(entries["bridge"]["assembly_call_targets"], ["sample"])
        self.assertEqual(entries["ordinary"]["instructions"], ["fld", "fsqrt"])
        self.assertIn("Physics/caller.cpp:1", entries["ordinary"]["source_reference_sites"])

    def test_raw_byte_body_keeps_emitted_bytes_and_decode_obligation(self):
        result = self.audit('''__declspec(naked) void sample()
{
    __asm {
        _emit 0xc3
    }
}
''')
        entry = result["files"][0]["entries"][0]
        self.assertEqual(entry["symbol"], "sample")
        self.assertEqual(entry["instructions"], ["_emit"])
        self.assertEqual(entry["raw_emitted_bytes"], [0xc3])
        self.assertTrue(entry["raw_byte_decode_required"])


if __name__ == "__main__":
    unittest.main()
