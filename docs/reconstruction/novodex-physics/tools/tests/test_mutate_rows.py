import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import mutate_rows  # noqa: E402


SOURCE = ('\t\tje L1\t\t// 0x00001000\r\n'
          '\t\tmov eax, dword ptr [ecx + 0x8]\t\t// 0x00001004\r\n'
          '\t\tcmp eax, 5\t\t// 0x00001008\r\n')


class CandidatesTest(unittest.TestCase):
    def test_candidates_only_inside_the_row_range(self):
        found = mutate_rows.candidates(SOURCE, 0x1004, 0x1008)
        self.assertEqual([a for a, _, _ in found], [0x1004])

    def test_each_mutation_kind(self):
        self.assertEqual(mutate_rows.mutate('je L1'), 'jne L1')
        self.assertEqual(mutate_rows.mutate('mov eax, dword ptr [ecx + 0x8]'), 'mov eax, dword ptr [ecx + 0xc]')
        self.assertEqual(mutate_rows.mutate('mov eax, dword ptr [ecx]'), 'mov eax, dword ptr [ecx + 4]')
        self.assertEqual(mutate_rows.mutate('cmp eax, 5'), 'cmp eax, 6')
        self.assertEqual(mutate_rows.mutate('shl eax, 2'), 'shl eax, 3')
        self.assertEqual(mutate_rows.mutate('fadd st(0), st(1)'), 'fsub st(0), st(1)')
        self.assertIsNone(mutate_rows.mutate('ret'))


class DataFlowMutationTest(unittest.TestCase):
    def test_decimal_offsets_are_mutated(self):
        self.assertEqual(mutate_rows.mutate('lea esi, [esp + 8]'), 'lea esi, [esp + 12]')

    def test_stack_frame_arithmetic_is_not_a_candidate(self):
        # sub/add esp only unbalance the stack: a crash, which says nothing about the row's data flow
        self.assertIsNone(mutate_rows.mutate('sub esp, 0x24'))
        self.assertIsNone(mutate_rows.mutate('add esp, 0xc'))

    def test_pick_spreads_attempts_across_the_row(self):
        self.assertEqual(mutate_rows.pick(list(range(12)), 3), [0, 4, 8])
        self.assertEqual(mutate_rows.pick([1, 2], 6), [1, 2])


class DetectionTest(unittest.TestCase):
    def test_detect_family_mismatches(self):
        out = 'thirdparty name=fam rva=0x1 owner=x words=9 oracle=ab mismatches=12 worst_ulp=0 verdict=FAILED\n'
        self.assertEqual(mutate_rows.detect_family(out, 'fam'), 12)
        self.assertIsNone(mutate_rows.detect_family(out, 'other'))

    def test_detect_family_zero_is_not_a_detection(self):
        out = 'x name=fam mismatches=0 verdict=exact\n'
        self.assertIsNone(mutate_rows.detect_family(out, 'fam'))

    def test_detect_staged_delta(self):
        out = 'differential target=T oracle_exit=0 candidate_exit=0 both_exit_zero=True stdout_delta=7 stderr_exact=True\n'
        self.assertEqual(mutate_rows.detect_staged(out), 7)

    def test_staged_crash_is_not_a_detection(self):
        out = 'differential target=T oracle_exit=0 candidate_exit=-1073741819 both_exit_zero=False stdout_delta=40 stderr_exact=False\n'
        self.assertIsNone(mutate_rows.detect_staged(out))
        out = 'differential target=T oracle_exit=0 candidate_exit=3221225477 both_exit_zero=False stdout_delta=40 stderr_exact=False\n'
        self.assertIsNone(mutate_rows.detect_staged(out))


class RepoRootTest(unittest.TestCase):
    def test_repo_is_the_checkout_root(self):
        # build/, Physics/ and the test exes live under it; a wrong root runs every command in the wrong directory
        self.assertTrue((mutate_rows.REPO / 'build').is_dir())
        self.assertTrue((mutate_rows.REPO / 'Physics' / 'src').is_dir())


class BuildTargetsTest(unittest.TestCase):
    def test_staged_runs_also_rebuild_the_candidate_dll(self):
        # a staged pair stages build/Release/NxPhysics.dll, so a stale DLL would report nothing caught
        self.assertEqual(mutate_rows.build_targets('T', True), ['NxPhysics', 'T'])

    def test_a_harness_target_is_built_alone(self):
        self.assertEqual(mutate_rows.build_targets('T', False), ['T'])


class SweepTest(unittest.TestCase):
    def source(self, d):
        p = Path(d) / 'rows.cpp'
        p.write_bytes(SOURCE.encode())
        return p

    def test_sweep_stops_at_the_first_caught(self):
        with tempfile.TemporaryDirectory() as d:
            p = self.source(d)
            calls = []

            def run(text):
                calls.append(text)
                return ('family', len(calls) - 1 if len(calls) > 1 else None)

            got = mutate_rows.sweep(p, 0x1000, 0x100c, run, 6)
            self.assertEqual(len(calls), 2)
            self.assertEqual(got[2], 1)
            self.assertEqual(p.read_bytes(), SOURCE.encode())

    def test_sweep_gives_up_at_the_limit(self):
        with tempfile.TemporaryDirectory() as d:
            p = self.source(d)
            calls = []
            got = mutate_rows.sweep(p, 0x1000, 0x100c, lambda t: (calls.append(t), ('family', None))[1], 2)
            self.assertIsNone(got)
            self.assertEqual(len(calls), 2)

    def test_source_is_restored_after_a_failed_run(self):
        with tempfile.TemporaryDirectory() as d:
            p = self.source(d)

            def run(text):
                raise OSError('build exploded')

            with self.assertRaises(OSError):
                mutate_rows.sweep(p, 0x1000, 0x100c, run, 3)
            self.assertEqual(p.read_bytes(), SOURCE.encode())


if __name__ == '__main__':
    unittest.main()
