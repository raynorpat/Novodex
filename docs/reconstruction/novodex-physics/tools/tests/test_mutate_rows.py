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


class HarnessCommandTest(unittest.TestCase):
    def test_extra_arguments_follow_the_oracle_directory_and_hash(self):
        cmd = mutate_rows.harness_command('T', ['--only'])
        self.assertEqual([Path(cmd[0]).name, cmd[1], cmd[2], cmd[3]],
                         ['T.exe', mutate_rows.ORACLE_DIR, mutate_rows.ORACLE_SHA, '--only'])
        self.assertEqual(len(mutate_rows.harness_command('T', None)), 3)


class CountCrashTest(unittest.TestCase):
    def test_a_crash_counts_only_when_asked(self):
        out = 'differential target=T oracle_exit=0 candidate_exit=-1073741819 both_exit_zero=False stdout_delta=26 stderr_exact=True'
        self.assertIsNone(mutate_rows.detect_staged(out))
        self.assertEqual(mutate_rows.detect_staged(out, count_crash=True), 26)
        # a crash with the output identical is still nothing
        same = 'differential target=T oracle_exit=0 candidate_exit=-1073741819 both_exit_zero=False stdout_delta=0 stderr_exact=True'
        self.assertIsNone(mutate_rows.detect_staged(same, count_crash=True))


class RepoRootTest(unittest.TestCase):
    def test_repo_is_the_checkout_root(self):
        # build/, Physics/ and the test exes live under it; a wrong root runs every command in the wrong directory
        self.assertTrue((mutate_rows.REPO / 'build').is_dir())
        self.assertTrue((mutate_rows.REPO / 'Physics' / 'src').is_dir())


class AtomicWriteTest(unittest.TestCase):
    def test_a_failed_write_leaves_the_original_intact(self):
        # a full disk truncated a source file mid-write; the target must be replaced whole or not at all
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'rows.cpp'
            p.write_bytes(b'original')

            def failing(tmp, data):
                tmp.write_bytes(data[:3])
                raise OSError(28, 'No space left on device')

            with self.assertRaises(OSError):
                mutate_rows.write_atomic(p, b'mutated text', writer=failing)
            self.assertEqual(p.read_bytes(), b'original')
            self.assertEqual([x.name for x in Path(d).iterdir()], ['rows.cpp'])

    def test_a_good_write_replaces_the_file_whole(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'rows.cpp'
            p.write_bytes(b'original')
            mutate_rows.write_atomic(p, b'mutated text')
            self.assertEqual(p.read_bytes(), b'mutated text')
            self.assertEqual([x.name for x in Path(d).iterdir()], ['rows.cpp'])


class ReplaceRetryTest(unittest.TestCase):
    def test_a_transiently_locked_target_is_retried(self):
        # Windows briefly denies os.replace while an indexer or scanner holds the file; a sweep once died with the source mutated
        real = mutate_rows.os.replace
        calls = []

        def flaky(a, b):
            calls.append(1)
            if len(calls) < 3:
                raise PermissionError(5, 'Access is denied')
            real(a, b)

        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'rows.cpp'
            p.write_bytes(b'original')
            mutate_rows.os.replace = flaky
            try:
                mutate_rows.write_atomic(p, b'mutated', pause=0)
            finally:
                mutate_rows.os.replace = real
            self.assertEqual(p.read_bytes(), b'mutated')
            self.assertEqual(len(calls), 3)


class RunTreeTest(unittest.TestCase):
    def test_a_timeout_kills_the_whole_process_tree(self):
        # subprocess.run only kills its direct child; a hung mutant test then kept the staged DLLs locked
        import subprocess
        code = ("import subprocess,sys,time;"
                "c=subprocess.Popen([sys.executable,'-c','import time;time.sleep(60)']);"
                "print(c.pid,flush=True);time.sleep(60)")
        pids = []
        with self.assertRaises(subprocess.TimeoutExpired) as ctx:
            mutate_rows.run_tree([sys.executable, '-c', code], timeout=3, cwd=str(TOOLS_DIR))
        out = ctx.exception.output or b''
        grandchild = int(out.split()[0]) if out.split() else None
        self.assertIsNotNone(grandchild)
        alive = subprocess.run(['tasklist', '/FI', 'PID eq %d' % grandchild], capture_output=True, text=True).stdout
        self.assertNotIn(str(grandchild), alive)


class BuildTargetsTest(unittest.TestCase):
    def test_staged_runs_also_rebuild_the_candidate_dll(self):
        # a staged pair stages build/Release/NxPhysics.dll, so a stale DLL would report nothing caught
        self.assertEqual(mutate_rows.build_targets('T', True), ['NxPhysics', 'T'])

    def test_a_harness_target_is_built_alone(self):
        self.assertEqual(mutate_rows.build_targets('T', False), ['T'])


class StaleLinkTest(unittest.TestCase):
    def test_the_candidate_dll_is_removed_so_msbuild_must_relink(self):
        # MSBuild's tracker once called the link up to date after NpShapeHandleRows.obj changed, so every mutation
        # of that file ran against the old DLL and none was ever detected
        with tempfile.TemporaryDirectory() as d:
            dll = Path(d) / 'build' / 'Release' / 'NxPhysics.dll'
            dll.parent.mkdir(parents=True)
            dll.write_bytes(b'old')
            mutate_rows.remove_stale_dll(Path(d))
            self.assertFalse(dll.exists())
            mutate_rows.remove_stale_dll(Path(d))


class StagedNoteTest(unittest.TestCase):
    def test_note_says_why_nothing_was_detected(self):
        crash = 'differential target=T oracle_exit=0 candidate_exit=-1073741819 both_exit_zero=False stdout_delta=40 stderr_exact=False\n'
        self.assertEqual(mutate_rows.staged_note(crash), 'crash(exit=-1073741819,delta=40)')
        same = 'differential target=T oracle_exit=0 candidate_exit=0 both_exit_zero=True stdout_delta=0 stderr_exact=True\n'
        self.assertEqual(mutate_rows.staged_note(same), 'same(exit=0,delta=0)')
        self.assertEqual(mutate_rows.staged_note('no summary line'), 'no-summary')


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

    def test_survey_tries_every_pick_and_reports_the_survivors(self):
        with tempfile.TemporaryDirectory() as d:
            p = self.source(d)
            calls = []

            def run(text):
                calls.append(text)
                return ('family', 3 if len(calls) == 2 else None)

            got = mutate_rows.survey(p, 0x1000, 0x100c, run, 6)
            self.assertEqual(len(calls), 3)
            self.assertEqual([m[1] for m in got], [None, 3, None])
            self.assertEqual(sum(1 for m in got if m[1]), 1)
            self.assertEqual(p.read_bytes(), SOURCE.encode())

    def test_survey_can_skip_the_row_head(self):
        with tempfile.TemporaryDirectory() as d:
            p = self.source(d)
            got = mutate_rows.survey(p, 0x1000, 0x100c, lambda t: ('family', None), 6, skip=1)
            self.assertEqual([m[0].split()[0] for m in got], ['0x00001004', '0x00001008'])

    def test_trace_sees_every_attempt(self):
        with tempfile.TemporaryDirectory() as d:
            p = self.source(d)
            seen = []
            mutate_rows.sweep(p, 0x1000, 0x100c, lambda t: ('family', None), 6,
                              trace=lambda addr, before, after, kind, n: seen.append((addr, kind, n)))
            self.assertEqual([a for a, _, _ in seen], [0x1000, 0x1004, 0x1008])
            self.assertTrue(all(k == 'family' and n is None for _, k, n in seen))

    def test_sweep_gives_up_at_the_limit(self):
        with tempfile.TemporaryDirectory() as d:
            p = self.source(d)
            calls = []
            got = mutate_rows.sweep(p, 0x1000, 0x100c, lambda t: (calls.append(t), ('family', None))[1], 2)
            self.assertIsNone(got)
            self.assertEqual(len(calls), 2)

    def test_build_is_restored_after_the_sweep(self):
        # the built binaries must match the restored source, or the next run measures the last mutant
        with tempfile.TemporaryDirectory() as d:
            p = self.source(d)
            restored = []
            mutate_rows.sweep(p, 0x1000, 0x100c, lambda t: ('family', None), 2, restore_build=lambda: restored.append(1))
            self.assertEqual(restored, [1])
            with self.assertRaises(OSError):
                mutate_rows.sweep(p, 0x1000, 0x100c, lambda t: (_ for _ in ()).throw(OSError('x')), 2,
                                  restore_build=lambda: restored.append(2))
            self.assertEqual(restored, [1, 2])

    def test_a_baseline_that_already_differs_is_not_a_detection(self):
        # without a control, a target that already reports a delta on the unmutated source "catches" the first mutant
        with tempfile.TemporaryDirectory() as d:
            p = self.source(d)
            runs = []
            restored = []

            def run(text):
                runs.append(text)
                return ('family', 3)

            with self.assertRaises(RuntimeError):
                mutate_rows.sweep(p, 0x1000, 0x100c, run, 6, restore_build=lambda: restored.append(1),
                                  control=lambda text: ('family', 3))
            self.assertEqual(runs, [])
            self.assertEqual(restored, [1])
            self.assertEqual(p.read_bytes(), SOURCE.encode())

    def test_a_clean_control_lets_the_sweep_run(self):
        with tempfile.TemporaryDirectory() as d:
            p = self.source(d)
            got = mutate_rows.sweep(p, 0x1000, 0x100c, lambda text: ('family', 2), 6, control=lambda text: ('family', None))
            self.assertEqual(got[2], 2)

    def test_the_restored_check_also_covers_the_caught_path(self):
        with tempfile.TemporaryDirectory() as d:
            p = self.source(d)
            real = mutate_rows.write_atomic
            calls = []

            def lying(path, data, **kw):
                calls.append(1)
                if len(calls) >= 2:  # no restore ever lands
                    return
                real(path, data, **kw)

            mutate_rows.write_atomic = lying
            try:
                with self.assertRaises(RuntimeError):
                    mutate_rows.sweep(p, 0x1000, 0x100c, lambda text: ('family', 1), 6)
            finally:
                mutate_rows.write_atomic = real

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
