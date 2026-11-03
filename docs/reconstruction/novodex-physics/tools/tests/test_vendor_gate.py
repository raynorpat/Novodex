import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import vendor_gate as vg  # noqa: E402

NL = chr(10)


class PathTests(unittest.TestCase):
    def test_upstream_and_overlay_paths_map_into_the_merged_tree(self):
        self.assertEqual(vg.merged_path('External/qhull/upstream/src/geom.c'), 'build/External/qhull-tree/geom.c')
        self.assertEqual(vg.merged_path('External/qhull/novodex/io.c'), 'build/External/qhull-tree/io.c')
        self.assertEqual(vg.merged_path('External/opcode/upstream/Opcode/Ice/IceAABB.cpp'), 'build/External/opcode-tree/Ice/IceAABB.cpp')
        self.assertEqual(vg.merged_path('External/opcode/novodex/OPC_RayCollider.cpp'), 'build/External/opcode-tree/OPC_RayCollider.cpp')
        self.assertEqual(vg.merged_path('External/opcode/novodex/Ice/IceOBB.cpp'), 'build/External/opcode-tree/Ice/IceOBB.cpp')

    def test_other_paths_are_not_vendor_paths(self):
        self.assertIsNone(vg.merged_path('Physics/src/Scene.cpp'))
        self.assertIsNone(vg.merged_path(''))

    def test_backslashes_are_accepted(self):
        self.assertEqual(vg.merged_path('External' + chr(92) + 'qhull' + chr(92) + 'novodex' + chr(92) + 'io.c'), 'build/External/qhull-tree/io.c')


class LineTests(unittest.TestCase):
    OUT = NL.join(['pair_directory=D:' + chr(92) + 'x', 'loaded module=NxPhysics.dll',
                   'thirdparty name=opcode_ray rva=0x1 words=9 oracle=aa mismatches=0 worst_ulp=0 verdict=exact',
                   'ULP_BEYOND opcode_ray_boundary 12',
                   'thirdparty name=qhull_hull rva=0x2 words=9 oracle=bb mismatches=0 worst_ulp=0 verdict=exact'])

    def test_lines_are_keyed_by_family_name(self):
        lines = vg.parse_lines(self.OUT)
        self.assertEqual(sorted(lines), ['opcode_ray', 'qhull_hull'])

    def test_changed_families(self):
        base = vg.parse_lines(self.OUT)
        now = vg.parse_lines(self.OUT.replace('oracle=aa mismatches=0', 'oracle=aa mismatches=3'))
        self.assertEqual(vg.changed_families(base, now), {'opcode_ray'})
        self.assertEqual(vg.changed_families(base, base), set())

    def test_a_vanished_family_counts_as_changed(self):
        base = vg.parse_lines(self.OUT)
        now = vg.parse_lines(self.OUT.replace('thirdparty name=qhull_hull', 'thirdparty name=qhull_other'))
        self.assertEqual(vg.changed_families(base, now), {'qhull_hull', 'qhull_other'})

    def test_detection_counts_only_the_rows_families(self):
        base = vg.parse_lines(self.OUT)
        now = vg.parse_lines(self.OUT.replace('oracle=aa', 'oracle=zz'))
        self.assertEqual(vg.detection(base, now, ['opcode_ray']), 1)
        self.assertIsNone(vg.detection(base, now, ['qhull_hull']))
        self.assertEqual(vg.detection(base, now, []), 1)    # a row with no named family sees any change


class FamilyTests(unittest.TestCase):
    def test_families_come_from_the_coverage_column_and_the_names_the_target_prints(self):
        diff = 'thirdparty:qhull_hull=exact+qhull_hull_x87=exact;thirdparty:qhull_hull_rotated=exact;asset:opcode_ray=exact'
        names = {'qhull_hull', 'qhull_hull_x87', 'opcode_ray', 'other'}
        self.assertEqual(vg.row_families(diff, names), ['opcode_ray', 'qhull_hull', 'qhull_hull_x87'])

    def test_packing_never_puts_overlapping_family_sets_together(self):
        rows = [{'id': 'a', 'families': ['f1', 'f2']}, {'id': 'b', 'families': ['f2']}, {'id': 'c', 'families': ['f3']},
                {'id': 'd', 'families': []}, {'id': 'e', 'families': []}]
        packs = vg.pack(rows, 10)
        for p in packs:
            seen = set()
            for r in p:
                self.assertFalse(seen & set(r['families']))
                seen |= set(r['families'])
        self.assertEqual(sorted(r['id'] for p in packs for r in p), ['a', 'b', 'c', 'd', 'e'])
        # a row with no named family sees every change, so it goes alone
        self.assertTrue(all(len(p) == 1 for p in packs if any(not r['families'] for r in p)))

    def test_pack_size_limit(self):
        rows = [{'id': str(i), 'families': ['f%d' % i]} for i in range(7)]
        self.assertEqual([len(p) for p in vg.pack(rows, 3)], [3, 3, 1])


class TreeCleanTests(unittest.TestCase):
    def test_a_merged_tree_file_that_differs_from_its_source_is_reported(self):
        d = Path(tempfile.mkdtemp())
        (d / 'up').mkdir()
        (d / 'ov').mkdir()
        (d / 'tree').mkdir()
        (d / 'up' / 'a.c').write_text('one', encoding='utf-8')
        (d / 'up' / 'b.c').write_text('two', encoding='utf-8')
        (d / 'ov' / 'b.c').write_text('over', encoding='utf-8')
        (d / 'tree' / 'a.c').write_text('one', encoding='utf-8')
        (d / 'tree' / 'b.c').write_text('over', encoding='utf-8')
        self.assertEqual(vg.dirty_tree_files(d / 'tree', d / 'up', d / 'ov'), [])
        (d / 'tree' / 'a.c').write_text('one MUTATED', encoding='utf-8')
        self.assertEqual(vg.dirty_tree_files(d / 'tree', d / 'up', d / 'ov'), ['a.c'])


if __name__ == '__main__':
    unittest.main()
