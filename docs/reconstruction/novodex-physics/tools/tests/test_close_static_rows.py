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

    def test_count_that_would_go_negative_raises(self):
        rows = [inv_row(1), inv_row(2)]
        c = closure('phys_fn_000001', 'phys_fn_000002')
        c['counts']['deferred_not_reconstructed_in_phase'] = 1
        with self.assertRaises(ValueError):
            close_static_rows.close(MANIFEST, {'functions': rows}, {3: c}, MAP)

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


VMANIFEST = {'source': 'External/qhull/novodex/QhullRows.cpp', 'compare': 'evidence/vendored-rows-qhull-listing-compare.py',
             'doc': 'evidence/vendored-correspondence.md', 'instructions': 40, 'mappings': 3,
             'rows': {'phys_fn_000001': {'symbol': 'qh_one', 'what': 'part one', 'group': '0x00001010', 'family': 'qhull_hull'},
                      'phys_fn_000002': {'symbol': 'qh_one', 'what': 'part two', 'group': '0x00001010', 'family': 'qhull_hull'}}}
VMAP = {'_qh_one': ('0x10001010', 'QhullRows.obj')}
EXACT = {('qhull', '0x00001010'): {'family_best': 'exact', 'differential': 'thirdparty:qhull_hull=exact+qhull_hull_x87=exact'}}


def vrun(manifest=VMANIFEST, rows=None, coverage=EXACT):
    rows = rows or [inv_row(1, third_party='qhull', phase='4'), inv_row(2, third_party='qhull', phase='4')]
    c = {'phase': 4, 'counts': {'deferred_vendored_not_falsified': len(rows), 'deferred_reconstructed_not_falsified': 5},
         'deferred': [{'id': r['id'], 'reason': 'vendored_not_falsified', 'driving_phases': []} for r in rows]}
    return close_static_rows.close(manifest, {'functions': rows}, {4: c}, VMAP, coverage=coverage)


class VendoredModeTest(unittest.TestCase):
    def test_vendored_records_static_and_dynamic_proof(self):
        inventory, closures = vrun()
        r = inventory['functions'][0]
        self.assertEqual(r['state'], 'reconstructed')
        self.assertIn('NX_VENDORED_LISTING_ROWS', r['static_proof'])
        self.assertIn('qhull_hull', r['dynamic_proof'])
        self.assertIn('family_best=exact', r['dynamic_proof'])
        self.assertEqual(closures[4]['counts'], {'deferred_vendored_not_falsified': 0, 'deferred_reconstructed_not_falsified': 7})
        self.assertEqual({e['reason'] for e in closures[4]['deferred']}, {'reconstructed_not_falsified'})

    def test_vendored_proof_lists_the_divergent_families_the_group_also_ran_in(self):
        cov = {('qhull', '0x00001010'): {'family_best': 'exact', 'differential':
               'thirdparty:qhull_hull=exact+qhull_hull_x87=exact;thirdparty:qhull_output=exact+qhull_output_x87=lastbit;'
               'thirdparty:qhull_paths=discrete'}}
        inventory, _ = vrun(coverage=cov)
        proof = inventory['functions'][0]['dynamic_proof']
        self.assertIn('qhull_output_x87 (lastbit)', proof)
        self.assertIn('qhull_paths (discrete)', proof)
        self.assertIn('vendored-correspondence.md', proof)

    def test_vendored_refuses_a_family_whose_execution_has_a_divergent_sibling(self):
        with self.assertRaises(ValueError):
            vrun(coverage={('qhull', '0x00001010'): {'family_best': 'exact', 'differential':
                 'thirdparty:qhull_hull=exact+qhull_hull_x87=discrete;thirdparty:qhull_other=exact+qhull_other_x87=exact'}})

    def test_vendored_refuses_a_family_credited_only_through_an_inlined_copy(self):
        with self.assertRaises(ValueError):
            vrun(coverage={('qhull', '0x00001010'): {'family_best': 'exact', 'differential':
                 'thirdparty[via-inline:_qh_caller]:qhull_hull=exact+qhull_hull_x87=exact'}})

    def test_vendored_refuses_inexact_group(self):
        with self.assertRaises(ValueError):
            vrun(coverage={('qhull', '0x00001010'): {'family_best': 'discrete', 'differential': 'thirdparty:qhull_hull=exact'}})

    def test_vendored_refuses_family_not_in_group_differential(self):
        for diff in ('thirdparty:qhull_other=exact', 'thirdparty:qhull_hull=discrete+qhull_other=exact'):
            with self.assertRaises(ValueError):
                vrun(coverage={('qhull', '0x00001010'): {'family_best': 'exact', 'differential': diff}})

    def test_vendored_split_rows_need_their_group(self):
        inventory, _ = vrun()
        self.assertEqual({r['state'] for r in inventory['functions']}, {'reconstructed'})
        m = dict(VMANIFEST, rows=dict(VMANIFEST['rows'], phys_fn_000003={'symbol': 'qh_one', 'what': 'part three',
                                                                         'group': '0x00009990', 'family': 'qhull_hull'}))
        rows = [inv_row(n, third_party='qhull', phase='4') for n in (1, 2, 3)]
        with self.assertRaises(ValueError):
            vrun(m, rows)

    def test_vendored_refuses_first_party_row(self):
        with self.assertRaises(ValueError):
            vrun(rows=[inv_row(1, phase='4'), inv_row(2, third_party='qhull', phase='4')])


if __name__ == '__main__':
    unittest.main()
