import subprocess
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import close_static_rows  # noqa: E402
import mutate_cpp as mc  # noqa: E402
import survey_to_deferrals as sd  # noqa: E402
import vendor_gate as vg  # noqa: E402

NL = chr(10)


class FamilyNameTests(unittest.TestCase):
    def test_lastbit_grades_and_dotted_names_are_families(self):
        diff = 'thirdparty:adjacency_owner.plane_divergent=lastbit+opcode_ray=exact;thirdparty:qhull_hull=discrete'
        names = {'adjacency_owner.plane_divergent', 'opcode_ray', 'qhull_hull'}
        self.assertEqual(vg.row_families(diff, names), ['adjacency_owner.plane_divergent', 'opcode_ray', 'qhull_hull'])


class CrashTests(unittest.TestCase):
    class Mr:
        TIMEOUT = 5
        REPO = '.'
        ORACLE_DIR = 'D:/x'
        ORACLE_SHA = 'abc'

        def __init__(self, stdout, rc):
            self.out, self.rc = stdout, rc

        def harness_command(self, target, extra):
            return ['x.exe']

        def run_tree(self, cmd, timeout, cwd):
            return subprocess.CompletedProcess(cmd, self.rc, self.out, '')

    BASE = vg.parse_lines(NL.join(['thirdparty name=f1 oracle=aa', 'thirdparty name=f2 oracle=bb']))

    def spec(self, **kw):
        return dict({'target': 'NxPhysicsThirdPartyTests', 'lines': True, 'families': ['f2'], 'base_lines': self.BASE}, **kw)

    def test_a_truncated_run_after_a_crash_is_not_a_catch_unless_asked(self):
        crashed = self.Mr('thirdparty name=f1 oracle=aa', -1073741819)
        self.assertIsNone(mc.run_gate(crashed, self.spec()))
        self.assertEqual(mc.run_gate(crashed, self.spec(count_crash=True)), 1)

    def test_a_clean_exit_with_a_changed_line_is_still_a_catch(self):
        ok = self.Mr(NL.join(['thirdparty name=f1 oracle=aa', 'thirdparty name=f2 oracle=zz']), 0)
        self.assertEqual(mc.run_gate(ok, self.spec()), 1)


class ThresholdTests(unittest.TestCase):
    def closable(self, tried, caught, survived, limit=2):
        line = 'SURVEY phys_fn_000001 NxG tried=%d caught=%d survived=%d rejected=0' % (tried, caught, survived)
        return bool(sd.classify([line], limit)[0])

    def test_survivors_are_limited_to_a_quarter_of_the_mutations_tried(self):
        self.assertTrue(self.closable(16, 14, 2))
        self.assertTrue(self.closable(6, 5, 1, limit=1))
        self.assertFalse(self.closable(2, 1, 1))
        self.assertFalse(self.closable(3, 2, 1))
        self.assertTrue(self.closable(1, 1, 0))
        self.assertFalse(self.closable(16, 12, 4))


class SourceNameTests(unittest.TestCase):
    def test_a_mutation_in_the_overlay_names_the_overlay_file(self):
        repo = Path(__file__).resolve().parents[5]
        row = {'implementation': 'External/qhull/upstream/src/geom2.c'}
        self.assertEqual(vg.source_file(row, repo), 'External/qhull/novodex/geom2.c' if (repo / 'External/qhull/novodex/geom2.c').is_file() else 'External/qhull/upstream/src/geom2.c')

    def test_the_closer_accepts_the_shadowing_overlay_of_the_rows_upstream_file(self):
        row = {'id': 'phys_fn_000001', 'phase': '4', 'state': 'reconstructed', 'rva': '0x1', 'size': '9', 'notes': '',
               'implementation': 'External/qhull/upstream/src/geom2.c', 'third_party': 'qhull'}
        ledger = {'phase': 4, 'counts': {'deferred_vendored_not_falsified': 0, 'deferred_reconstructed_not_falsified': 1,
                                         'oracle_differential_falsified': 0, 'differential_falsified': 0},
                  'closed': [], 'deferred': [{'id': row['id'], 'rva': '0x1', 'reason': 'reconstructed_not_falsified', 'driving_phases': []}]}
        man = {'doc': 'd', 'rows': {row['id']: {'gate': 'G', 'family': 'f', 'detected': 'mismatches=2', 'source_mutation': True,
                                                'mutation': 'External/qhull/novodex/geom2.c line 976: a > b  ->  a >= b  (cmp)'}}}
        close_static_rows.falsify(man, {'functions': [row]}, {4: ledger}, {'G': [4]}, {})
        self.assertEqual(row['state'], 'dynamically_gated')


if __name__ == '__main__':
    unittest.main()
