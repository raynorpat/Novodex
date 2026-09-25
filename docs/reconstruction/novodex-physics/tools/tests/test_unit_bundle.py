import sys
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import unit_bundle  # noqa: E402


def ins(rva, mnemonic, operands="", indirect=False):
    return {"rva": f"0x{rva:08x}", "size": 1, "bytes": "90", "mnemonic": mnemonic,
            "operands": operands, "flow": "call" if indirect else "sequential",
            "indirect": indirect, "target_rva": None, "stack_delta": 0}


def oracles(ghidra_functions=(), supplement_functions=(), data_objects=(), edges=()):
    inventory = {"functions": [
        {"id": "phys_fn_000001", "rva": "0x00001000", "size": 3, "kind": "code",
         "state": "discovered", "phase": 6, "source": None},
        {"id": "phys_fn_000002", "rva": "0x00001010", "size": 2, "kind": "code",
         "state": "reconstructed", "phase": 6, "source": "x"}],
        "data_objects": list(data_objects)}
    ghidra = {"functions": list(ghidra_functions),
              "strings": [{"rva": "0x00002000", "value": "Joint.cpp"}],
              "references": [{"from_rva": "0x00001001", "to_rva": "0x00002000"}]}
    capstone = {"instructions": [ins(0x1000, "push", "ebp"),
                                 ins(0x1001, "call", "dword ptr [eax + 0x14]", True),
                                 ins(0x1002, "ret"), ins(0x1010, "ret")]}
    return unit_bundle.Oracles(inventory, ghidra, capstone,
                               {"functions": list(supplement_functions)}, set(edges))


class DecompileSourceTest(unittest.TestCase):
    def test_manifest_decompile_wins(self):
        o = oracles(ghidra_functions=[{"rva": "0x00001000", "decompiler_status": "ok",
                                       "decompiler_c": "void f(void) {}"}],
                    supplement_functions=[{"rva": "0x00001000", "status": "ok",
                                           "decompiler_c": "other"}])
        self.assertEqual(unit_bundle.decompile_source(0x1000, 3, o),
                         ("ghidra manifest", "void f(void) {}"))

    def test_supplement_used_when_manifest_has_none(self):
        o = oracles(supplement_functions=[{"rva": "0x00001000", "status": "ok",
                                           "decompiler_c": "void g(void) {}"}])
        self.assertEqual(unit_bundle.decompile_source(0x1000, 3, o),
                         ("ghidra supplement", "void g(void) {}"))

    def test_failed_supplement_falls_back_to_disassembly(self):
        o = oracles(supplement_functions=[{"rva": "0x00001000", "status": "decompile_failed",
                                           "decompiler_c": None}])
        label, text = unit_bundle.decompile_source(0x1000, 3, o)
        self.assertEqual(label, "capstone disassembly")
        self.assertIn("0x00001000  push ebp", text)
        self.assertNotIn("0x00001010", text)


class RenderUnitTest(unittest.TestCase):
    def test_bundle_names_rows_edges_indirect_calls_strings_and_dispatch(self):
        o = oracles(data_objects=[{"id": "phys_data_000009", "rva": "0x00003000",
                                   "notes": '{"slots":3,"targets":["0x0000f000","0x0000f004","0x00001010"]}'}],
                    edges={("phys_fn_000001", "phys_fn_000002"),
                           ("phys_fn_000001", "phys_fn_000077")})
        unit = {"unit": "core\\A.cpp", "evidenced_span": ["0x00001000", "0x00001000"],
                "inferred_extent": ["phys_fn_000001", "phys_fn_000002"], "ambiguous_rows": [],
                "rows": {}, "bytes": {}, "phases": {}}
        text = unit_bundle.render_unit(unit, {"phys_fn_000077": "Other.cpp"}, o)
        self.assertIn("# core\\A.cpp", text)
        self.assertIn("## phys_fn_000001 (0x00001000, 3 B, discovered)", text)
        self.assertIn("phys_data_000009 slot 2 -> phys_fn_000002", text)
        self.assertIn("0x00001001  call dword ptr [eax + 0x14]", text)
        self.assertIn("Joint.cpp", text)
        self.assertIn("Other.cpp: phys_fn_000077", text)

    def test_unit_filename_is_flat_and_safe(self):
        self.assertEqual(unit_bundle.unit_filename("core\\RevoluteJoint.cpp"),
                         "core__RevoluteJoint.cpp.md")
        self.assertEqual(unit_bundle.unit_filename("gap:A.cpp..B.cpp"),
                         "gap__A.cpp__to__B.cpp.md")


if __name__ == "__main__":
    unittest.main()
