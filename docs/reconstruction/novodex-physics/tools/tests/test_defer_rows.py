import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import defer_rows  # noqa: E402


def inv_row(n, **extra):
    r = {'id': 'phys_fn_%06d' % n, 'rva': '0x00001000', 'size': '16', 'kind': 'code', 'phase': '6', 'state': 'reconstructed', 'notes': ''}
    r.update(extra)
    return r


def ledger(*ids):
    return {'phase': 6, 'counts': {'deferred_reconstructed_not_falsified': len(ids), 'deferred_homeless_shared_code': 1},
            'closed': [], 'deferred': [{'id': i, 'rva': '0x00001000', 'reason': 'reconstructed_not_falsified', 'driving_phases': []} for i in ids]}


def run(spec, rows=None, clo=None):
    rows = rows or [inv_row(1), inv_row(2)]
    clo = clo or {6: ledger(*[r['id'] for r in rows])}
    manifest = {'doc': 'evidence/falsify-x.md', 'rows': {'phys_fn_000001': spec}}
    defer_rows.defer(manifest, {'functions': rows}, clo)
    return rows, clo


class DeferRowsTest(unittest.TestCase):
    def test_a_row_moves_to_the_new_reason_with_its_note(self):
        rows, clo = run({'reason': 'not_independently_falsifiable', 'note': 'an adjustor thunk with no body of its own'})
        e = [d for d in clo[6]['deferred'] if d['id'] == 'phys_fn_000001'][0]
        self.assertEqual((e['reason'], e['note']), ('not_independently_falsifiable', 'an adjustor thunk with no body of its own'))
        self.assertEqual(clo[6]['counts'], {'deferred_reconstructed_not_falsified': 1, 'deferred_homeless_shared_code': 1,
                                            'deferred_not_independently_falsifiable': 1})
        self.assertIn('adjustor thunk', rows[0]['notes'])
        self.assertIn('evidence/falsify-x.md', rows[0]['notes'])

    def test_the_same_reason_only_updates_the_note(self):
        rows, clo = run({'reason': 'reconstructed_not_falsified', 'note': '5 caught of 16, 4 survived'})
        self.assertEqual(clo[6]['counts']['deferred_reconstructed_not_falsified'], 2)
        e = [d for d in clo[6]['deferred'] if d['id'] == 'phys_fn_000001'][0]
        self.assertEqual(e['note'], '5 caught of 16, 4 survived')

    def test_an_unknown_reason_is_refused(self):
        with self.assertRaises(ValueError):
            run({'reason': 'because', 'note': 'x'})

    def test_a_note_is_required(self):
        with self.assertRaises(ValueError):
            run({'reason': 'homeless_shared_code', 'note': ''})

    def test_a_row_that_is_not_deferred_is_refused(self):
        with self.assertRaises(ValueError):
            run({'reason': 'homeless_shared_code', 'note': 'x'}, clo={6: ledger('phys_fn_000002')})

    def test_a_vendored_row_may_not_take_a_native_reason_at_discovered(self):
        rows = [inv_row(1, third_party='qhull', state='discovered')]
        with self.assertRaises(ValueError):
            run({'reason': 'homeless_shared_code', 'note': 'x'}, rows=rows)


class KeepReasonTest(unittest.TestCase):
    def test_keep_leaves_the_reason_and_updates_the_note(self):
        rows = [inv_row(1), inv_row(2)]
        clo = {6: ledger('phys_fn_000001', 'phys_fn_000002')}
        clo[6]['deferred'][0]['reason'] = 'blocked_on_later_phase'
        clo[6]['counts']['deferred_blocked_on_later_phase'] = 1
        clo[6]['counts']['deferred_reconstructed_not_falsified'] = 1
        manifest = {'doc': 'd', 'rows': {'phys_fn_000001': {'reason': 'keep', 'note': 'no gate family is named by its proof'}}}
        defer_rows.defer(manifest, {'functions': rows}, clo)
        e = clo[6]['deferred'][0]
        self.assertEqual((e['reason'], e['note']), ('blocked_on_later_phase', 'no gate family is named by its proof'))
        self.assertEqual(clo[6]['counts']['deferred_blocked_on_later_phase'], 1)
        self.assertIn('blocked_on_later_phase', rows[0]['notes'])


if __name__ == '__main__':
    unittest.main()
