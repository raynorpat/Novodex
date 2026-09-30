import sys
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import work_units  # noqa: E402


def row(n, rva, size=0x10, kind="code", state="discovered", phase=6):
    return {"id": f"phys_fn_{n:06d}", "rva": rva, "size": size, "kind": kind,
            "state": state, "phase": phase}


def fn(n):
    return f"phys_fn_{n:06d}"


class BuildUnitsTest(unittest.TestCase):
    def setUp(self):
        # A.cpp is evidenced over rows 1-2, B.cpp at row 6; rows 3-5 lie between.
        self.rows = [row(1, 0x1000), row(2, 0x1010), row(3, 0x1020), row(4, 0x1030),
                     row(5, 0x1040), row(6, 0x1050),
                     row(7, 0x1060, kind="compiler_artifact", state="classified", phase=8)]
        self.seeds = {0x1000: "A.cpp", 0x1010: "A.cpp", 0x1050: "B.cpp"}

    def unit(self, units, name):
        return next(u for u in units if u["unit"] == name)

    def test_row_with_an_edge_into_the_left_unit_joins_it(self):
        units = work_units.build_units(self.rows, self.seeds, {(fn(3), fn(1))})
        self.assertIn(fn(3), self.unit(units, "A.cpp")["inferred_extent"])

    def test_growth_follows_a_chain_of_edges(self):
        edges = {(fn(3), fn(1)), (fn(4), fn(3))}
        units = work_units.build_units(self.rows, self.seeds, edges)
        self.assertEqual(self.unit(units, "A.cpp")["inferred_extent"],
                         [fn(1), fn(2), fn(3), fn(4)])

    def test_a_row_touching_both_units_stops_growth(self):
        edges = {(fn(3), fn(1)), (fn(3), fn(6))}
        units = work_units.build_units(self.rows, self.seeds, edges)
        self.assertNotIn(fn(3), self.unit(units, "A.cpp")["inferred_extent"])
        self.assertIn(fn(3), self.unit(units, "gap:A.cpp..B.cpp")["ambiguous_rows"])

    def test_the_right_unit_grows_downward(self):
        units = work_units.build_units(self.rows, self.seeds, {(fn(6), fn(5))})
        self.assertEqual(self.unit(units, "B.cpp")["inferred_extent"], [fn(5), fn(6)])

    def test_unconnected_rows_are_ambiguous(self):
        units = work_units.build_units(self.rows, self.seeds, set())
        gap = self.unit(units, "gap:A.cpp..B.cpp")
        self.assertEqual(gap["inferred_extent"], [])
        self.assertEqual(gap["ambiguous_rows"], [fn(3), fn(4), fn(5)])
        self.assertIsNone(gap["evidenced_span"])

    def test_partition_covers_every_code_row_exactly_once(self):
        units = work_units.build_units(self.rows, self.seeds, {(fn(3), fn(1))})
        seen = [i for u in units for i in u["inferred_extent"] + u["ambiguous_rows"]]
        self.assertEqual(sorted(seen), [fn(n) for n in range(1, 7)])

    def test_rows_before_the_first_span_form_a_start_gap(self):
        rows = [row(9, 0x0f00)] + self.rows
        units = work_units.build_units(rows, self.seeds, set())
        self.assertEqual(self.unit(units, "gap:<start>..A.cpp")["ambiguous_rows"], [fn(9)])
        self.assertEqual(units[0]["unit"], "gap:<start>..A.cpp")

    def test_interleaved_spans_are_rejected(self):
        seeds = {0x1000: "A.cpp", 0x1030: "A.cpp", 0x1010: "B.cpp"}
        with self.assertRaises(ValueError):
            work_units.build_units(self.rows, seeds, set())

    def test_counts_are_per_state_bytes_and_phase(self):
        rows = list(self.rows)
        rows[1] = row(2, 0x1010, state="reconstructed")
        units = work_units.build_units(rows, self.seeds, set())
        a = self.unit(units, "A.cpp")
        self.assertEqual(a["rows"], {"discovered": 1, "reconstructed": 1})
        self.assertEqual(a["bytes"], {"discovered": 0x10, "reconstructed": 0x10})
        self.assertEqual(a["phases"], {"6": 2})
        self.assertEqual(a["evidenced_span"], ["0x00001000", "0x00001010"])


class LoadEdgesTest(unittest.TestCase):
    def test_reads_quoted_dot_edges_and_ignores_nodes(self):
        dot = ('digraph g {\n  "phys_fn_000001" [rva="0x00001000",phase=2];\n'
               '  "phys_fn_000001" -> "phys_fn_000002";\n}\n')
        self.assertEqual(work_units.load_edges(dot), {(fn(1), fn(2))})


class SourceSeedTest(unittest.TestCase):
    def test_resolves_a_reference_to_the_second_byte_of_a_file_string(self):
        value = "=\\Epic\\Novodex\\SDKs\\Physics\\src\\ContactMeshHeightfield.cpp"
        ghidra = {
            "strings": [{"rva": "0x00107cff", "length": len(value), "value": value}],
            "references": [{"from_rva": "0x00045f70", "to_rva": "0x00107d00"}],
        }
        seeds = work_units.source_seeds(ghidra, [row(7, 0x45f70)])
        self.assertEqual(seeds, {0x45f70: "ContactMeshHeightfield.cpp"})


if __name__ == "__main__":
    unittest.main()
