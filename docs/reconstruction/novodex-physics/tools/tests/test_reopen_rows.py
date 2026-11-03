import copy
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import reopen_rows  # noqa: E402


def base_state():
    inv = {'functions': [{'id': 'phys_fn_000001', 'phase': '4', 'state': 'reconstructed', 'rva': '0x1', 'size': '9',
                          'notes': 'before', 'dynamic_proof': 'old proof'}]}
    ledger = {'phase': 4, 'counts': {'deferred_blocked_on_later_phase': 1, 'oracle_differential_falsified': 2, 'differential_falsified': 0},
              'closed': [], 'deferred': [{'id': 'phys_fn_000001', 'rva': '0x1', 'size': 9, 'reason': 'blocked_on_later_phase',
                                          'driving_phases': [7], 'blocked_on_type': 'Pruner'}]}
    return inv, {4: ledger}


def closed_state():
    inv, clo = copy.deepcopy(base_state())
    inv['functions'][0].update(state='dynamically_gated', notes='after', dynamic_proof='new proof')
    clo[4]['deferred'] = []
    clo[4]['counts'] = {'deferred_blocked_on_later_phase': 0, 'oracle_differential_falsified': 3, 'differential_falsified': 0}
    clo[4]['closed'] = [{'id': 'phys_fn_000001', 'rva': '0x1', 'proof': 'oracle_differential_falsified', 'gate': 'G',
                         'falsification': {'mutation': 'm', 'detected': 'mismatches=1'}, 'discharged_by_phase': 7, 'driving_phases': [7]}]
    return inv, clo


class ReopenTest(unittest.TestCase):
    def test_a_closed_row_goes_back_to_its_base_state(self):
        base_inv, base_clo = base_state()
        inv, clo = closed_state()
        program = {'phases': [{'phase': 4, 'closed_functions': 10, 'remaining_functions': 5}]}
        reopen_rows.reopen(['phys_fn_000001'], inv, clo, base_inv, base_clo, program)
        self.assertEqual(inv['functions'][0], base_inv['functions'][0])
        self.assertEqual(clo[4]['closed'], [])
        self.assertEqual(clo[4]['deferred'], base_clo[4]['deferred'])
        self.assertEqual(clo[4]['counts'], base_clo[4]['counts'])
        self.assertEqual(program['phases'][0], {'phase': 4, 'closed_functions': 9, 'remaining_functions': 6})

    def test_a_row_that_is_not_closed_is_refused(self):
        base_inv, base_clo = base_state()
        inv, clo = base_state()
        with self.assertRaises(ValueError):
            reopen_rows.reopen(['phys_fn_000001'], inv, clo, base_inv, base_clo, {'phases': []})

    def test_the_published_evidence_line_is_removed(self):
        nl = chr(10)
        text = nl.join(['# Evidence', '', '## Testing pass', '', '- `phys_fn_000001`: m; `G` mismatches=1.',
                        '- `phys_fn_000002`: other; `G` mismatches=2.', ''])
        out = reopen_rows.drop_evidence_lines(text, ['phys_fn_000001'])
        self.assertNotIn('phys_fn_000001', out)
        self.assertIn('phys_fn_000002', out)


if __name__ == '__main__':
    unittest.main()
