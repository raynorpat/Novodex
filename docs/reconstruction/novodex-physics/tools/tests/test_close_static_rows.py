import sys
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import close_static_rows  # noqa: E402


def inv_row(n, state='discovered', **extra):
    r = {'id': 'phys_fn_%06d' % n, 'rva': '0x%08x' % (0x1000 + 0x10 * n), 'size': '16', 'kind': 'code',
         'phase': '3', 'state': state, 'source': None, 'static_proof': None, 'dynamic_proof': None, 'notes': ''}
    r.update(extra)
    return r


def closure(*ids):
    return {'phase': 3, 'counts': {'deferred_not_reconstructed_in_phase': len(ids), 'deferred_reconstructed_not_falsified': 5},
            'deferred': [{'id': i, 'reason': 'not_reconstructed_in_phase', 'driving_phases': []} for i in ids]}


MANIFEST = {'source': 'Physics/src/x/XRows.cpp', 'compare': 'evidence/phase3-x-listing-compare.py', 'doc': 'evidence/phase3-x.md',
            'instructions': 40, 'mappings': 3,
            'rows': {'phys_fn_000001': {'symbol': 'nxXOne', 'what': 'does one thing'},
                     'phys_fn_000002': {'symbol': 'nxXTwo', 'what': 'does another'}}}
MAP = {'?nxXOne@@YAXXZ': ('0x10001000', 'XRows.obj'), '?nxXTwo@@YAXXZ': ('0x10001040', 'XRows.obj')}


def run(manifest=MANIFEST, rows=None, mp=MAP):
    rows = rows or [inv_row(1), inv_row(2)]
    inventory = {'functions': rows}
    closures = {3: closure(*[r['id'] for r in rows if r['state'] == 'discovered'])}
    return close_static_rows.close(manifest, inventory, closures, mp)


class CloseStaticRowsTest(unittest.TestCase):
    def test_marks_reconstructed_with_static_proof_only(self):
        inventory, _ = run()
        r = inventory['functions'][0]
        self.assertEqual(r['state'], 'reconstructed')
        self.assertEqual(r['source'], MANIFEST['source'])
        self.assertEqual(r['implementation_symbol'], '?nxXOne@@YAXXZ')
        self.assertEqual(r['candidate_map_reference'], {'path': 'build/Release/NxPhysics.map', 'symbol': '?nxXOne@@YAXXZ',
                                                        'address': '0x10001000', 'object': 'XRows.obj'})
        for needle in (MANIFEST['compare'], '40 instructions', '3 call mappings', r['rva'], 'does one thing'):
            self.assertIn(needle, r['static_proof'])
        self.assertFalse(r['dynamic_proof'])
        self.assertIn('awaits the testing pass', r['notes'])

    def test_closure_counts_follow_the_move(self):
        _, closures = run()
        c = closures[3]
        self.assertEqual(c['counts']['deferred_not_reconstructed_in_phase'], 0)
        self.assertEqual(c['counts']['deferred_reconstructed_not_falsified'], 7)
        self.assertEqual({e['reason'] for e in c['deferred']}, {'reconstructed_not_falsified'})
        self.assertTrue(all('awaits the testing pass' in e['note'] for e in c['deferred']))

    def test_blocked_on_later_phase_entry_moves_with_its_count(self):
        rows = [inv_row(1), inv_row(2)]
        inventory = {'functions': rows}
        c = closure('phys_fn_000001', 'phys_fn_000002')
        c['deferred'][0]['reason'] = 'blocked_on_later_phase'
        c['counts'] = {'deferred_not_reconstructed_in_phase': 1, 'deferred_blocked_on_later_phase': 1,
                       'deferred_reconstructed_not_falsified': 0}
        _, closures = close_static_rows.close(MANIFEST, inventory, {3: c}, MAP)
        self.assertEqual(closures[3]['counts'], {'deferred_not_reconstructed_in_phase': 0, 'deferred_blocked_on_later_phase': 0,
                                                 'deferred_reconstructed_not_falsified': 2})

    def test_split_rows_share_one_symbol(self):
        m = dict(MANIFEST, rows={'phys_fn_000001': {'symbol': 'nxXOne', 'what': 'first part'},
                                 'phys_fn_000002': {'symbol': 'nxXOne', 'what': 'second part'}})
        inventory, _ = run(m)
        a, b = inventory['functions']
        self.assertEqual(a['implementation_symbol'], b['implementation_symbol'])
        self.assertEqual(a['candidate_map_reference'], b['candidate_map_reference'])

    def test_refuses_vendored_rows(self):
        with self.assertRaises(ValueError):
            run(rows=[inv_row(1, third_party='qhull'), inv_row(2)])

    def test_refuses_rows_not_discovered(self):
        with self.assertRaises(ValueError):
            run(rows=[inv_row(1, state='reconstructed'), inv_row(2)])

    def test_missing_map_symbol_raises(self):
        with self.assertRaisesRegex(ValueError, 'nxXTwo'):
            run(mp={'?nxXOne@@YAXXZ': ('0x10001000', 'XRows.obj')})


if __name__ == '__main__':
    unittest.main()
