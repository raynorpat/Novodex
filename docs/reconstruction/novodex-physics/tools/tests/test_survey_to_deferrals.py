import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import survey_to_deferrals as sd  # noqa: E402

LOG = [
    'SURVEY phys_fn_000001 NxG tried=16 caught=15 survived=1 rejected=0',
    'SURVEY phys_fn_000002 NxG tried=10 caught=4 survived=6 rejected=2',
    'SURVEY phys_fn_000003 NxG tried=5 caught=0 survived=5 rejected=0',
    'SURVEY phys_fn_000004 NxG tried=0 caught=0 survived=0 rejected=3',
    'SURVEY phys_fn_000005 NxG tried=0 caught=0 survived=0 rejected=0',
]


class SurveyToDeferralsTest(unittest.TestCase):
    def test_classification(self):
        closable, deferred = sd.classify(LOG)
        self.assertEqual(closable, ['phys_fn_000001'])
        self.assertEqual(deferred['phys_fn_000002']['reason'], 'reconstructed_not_falsified')
        self.assertIn('caught 4 of 10', deferred['phys_fn_000002']['note'])
        self.assertEqual(deferred['phys_fn_000003']['reason'], 'reconstructed_not_falsified')
        self.assertEqual(deferred['phys_fn_000004']['reason'], 'not_independently_falsifiable')
        self.assertIn('3 source mutations', deferred['phys_fn_000004']['note'])

    def test_a_function_with_nothing_to_mutate_says_so(self):
        _, deferred = sd.classify(LOG)
        e = deferred['phys_fn_000005']
        self.assertEqual(e['reason'], 'not_independently_falsifiable')
        self.assertIn('no operator', e['note'])


class BuildFailureTest(unittest.TestCase):
    def test_a_row_whose_builds_failed_is_undecided_not_unfalsifiable(self):
        closable, deferred = sd.classify(['SURVEY phys_fn_000009 NxG tried=0 caught=0 survived=0 rejected=4 nobuild=4'])
        self.assertEqual(closable, [])
        self.assertNotIn('phys_fn_000009', deferred)

    def test_some_failed_builds_do_not_hide_the_other_rejections(self):
        _, deferred = sd.classify(['SURVEY phys_fn_000009 NxG tried=0 caught=0 survived=0 rejected=4 nobuild=1'])
        self.assertEqual(deferred['phys_fn_000009']['reason'], 'not_independently_falsifiable')


if __name__ == '__main__':
    unittest.main()
