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



def frow(n, state='reconstructed', **extra):
    r = inv_row(n, state=state, phase='3', notes='Listing transcription with a static proof only; it awaits the testing pass (no differential, no mutation yet).')
    r.update(extra)
    return r


FMAN = {'doc': 'evidence/phase3-x.md', 'rows': {
    'phys_fn_000001': {'gate': 'NxPhysicsCollisionTests', 'family': 'fam', 'mutation': 'je -> jne at 0x1014', 'detected': 'mismatches=12'}}}
ORACLE = {'NxPhysicsCollisionTests': [3, 7]}
STAGED = {'NxPhysicsActorShapeMutationTests': [5, 6, 7], 'NxPhysicsEarlyTests': [2]}


def fclosure(*ids):
    return {'phase': 3, 'counts': {'deferred_reconstructed_not_falsified': len(ids), 'oracle_differential_falsified': 4,
                                   'differential_falsified': 2},
            'closed': [], 'deferred': [{'id': i, 'rva': '0x00001010', 'reason': 'reconstructed_not_falsified', 'driving_phases': []} for i in ids]}


def frun(manifest=FMAN, rows=None):
    rows = rows or [frow(1)]
    clo = {3: fclosure(*[r['id'] for r in rows])}
    return close_static_rows.falsify(manifest, {'functions': rows}, clo, ORACLE, STAGED)


class PublishMeasurementsTest(unittest.TestCase):
    def test_a_closed_row_is_named_in_the_phase_evidence_file_with_its_count(self):
        # validate_inventory reads each closed row's count back from the ledger's evidence_file
        import tempfile
        with tempfile.TemporaryDirectory() as d:
            (Path(d) / 'evidence').mkdir()
            doc = Path(d) / 'evidence' / 'p.md'
            rows = [inv_row(1, phase='4'), inv_row(2, phase='4')]
            m = {'doc': 'x', 'rows': {
                'phys_fn_000001': {'gate': 'G', 'family': 'f', 'mutation': 'je -> jne', 'detected': 'stdout_delta=26'},
                'phys_fn_000002': {'gate': 'G', 'family': 'f', 'mutation': 'jb -> jae', 'detected': 'stdout_delta=7'}}}
            doc.write_text('# Evidence\n- phys_fn_000002 already: 7\n', encoding='utf-8')
            close_static_rows.publish_measurements(m, {'functions': rows}, {4: {'evidence_file': 'evidence/p.md'}}, d)
            text = doc.read_text(encoding='utf-8')
            self.assertIn('phys_fn_000001', text)
            self.assertIn('stdout_delta=26', text)
            self.assertEqual(text.count('phys_fn_000002'), 1)  # already named with its count
            close_static_rows.publish_measurements(m, {'functions': rows}, {4: {}}, d)  # no evidence_file: untouched
            self.assertEqual(doc.read_text(encoding='utf-8'), text)


class FalsifiedModeTest(unittest.TestCase):
    def test_falsified_closes_the_row(self):
        inventory, closures = frun()
        r = inventory['functions'][0]
        self.assertEqual(r['state'], 'dynamically_gated')
        for needle in ('NxPhysicsCollisionTests', 'fam', 'je -> jne at 0x1014', 'mismatches=12'):
            self.assertIn(needle, r['dynamic_proof'])
        self.assertNotIn('awaits the testing pass', r['notes'])
        c = closures[3]
        self.assertEqual(c['deferred'], [])
        e = c['closed'][0]
        self.assertEqual((e['proof'], e['gate']), ('oracle_differential_falsified', 'NxPhysicsCollisionTests'))
        self.assertEqual(e['falsification'], {'mutation': 'je -> jne at 0x1014', 'detected': 'mismatches=12'})
        self.assertEqual(c['counts'], {'deferred_reconstructed_not_falsified': 0, 'oracle_differential_falsified': 5,
                                      'differential_falsified': 2})

    def test_a_row_driven_but_not_wired_is_not_recorded_as_wired(self):
        man = {'doc': 'x', 'rows': {'phys_fn_000001': dict(FMAN['rows']['phys_fn_000001'], wired=False)}}
        inventory, _ = frun(man)
        note = inventory['functions'][0]['notes']
        self.assertNotIn('wired and driven', note)
        self.assertIn('not wired', note)
        self.assertIn('wired and driven', frun()[0]['functions'][0]['notes'])  # the default is unchanged

    def test_falsified_refuses_a_mutation_outside_the_row(self):
        # the row is 0x1010..0x1020; a typo in a hand-edited manifest must not close it
        for mutation in ('je -> jne at 0x1004', 'je -> jne at 0x1020', 'je -> jne with no address'):
            m = {'doc': 'd', 'rows': {'phys_fn_000001': {'gate': 'NxPhysicsCollisionTests', 'family': 'f',
                                                        'mutation': mutation, 'detected': 'mismatches=3'}}}
            with self.assertRaises(ValueError, msg=mutation):
                frun(m)

    def test_falsified_kind_follows_the_detection(self):
        m = {'doc': 'd', 'rows': {'phys_fn_000001': {'gate': 'NxPhysicsActorShapeMutationTests', 'family': 'pair',
                                                    'mutation': 'm at 0x1014', 'detected': 'stdout_delta=4'}}}
        _, closures = frun(m)
        self.assertEqual(closures[3]['closed'][0]['proof'], 'differential_falsified')
        self.assertEqual(closures[3]['counts']['differential_falsified'], 3)

    def test_falsified_discharges_to_the_later_phase_that_runs_the_gate(self):
        m = {'doc': 'd', 'rows': {'phys_fn_000001': {'gate': 'NxPhysicsActorShapeMutationTests', 'family': 'pair',
                                                    'mutation': 'm at 0x1014', 'detected': 'stdout_delta=4'}}}
        _, closures = frun(m)
        e = closures[3]['closed'][0]
        self.assertEqual((e['discharged_by_phase'], e['driving_phases']), (5, [5, 6, 7]))

    def test_falsified_own_phase_gate_has_no_discharge(self):
        _, closures = frun()
        self.assertNotIn('discharged_by_phase', closures[3]['closed'][0])

    def test_falsified_refuses_a_gate_no_later_phase_runs(self):
        m = {'doc': 'd', 'rows': {'phys_fn_000001': {'gate': 'NxPhysicsEarlyTests', 'family': 'f',
                                                    'mutation': 'm at 0x1014', 'detected': 'stdout_delta=4'}}}
        with self.assertRaises(ValueError):
            frun(m)

    def test_program_counts_leave_phases_without_counts_alone(self):
        program = {'phases': [{'phase': 1, 'closed_functions': None, 'remaining_functions': None}]}
        close_static_rows.bump_program(program, {1: 0})
        self.assertEqual(program['phases'][0], {'phase': 1, 'closed_functions': None, 'remaining_functions': None})

    def test_gate_artifact_counts_follow_the_closures(self):
        artifact = {'counts': {'closed': 168, 'deferred': 224}, 'closure_ledger': 'x'}
        close_static_rows.bump_gate(artifact, 15)
        self.assertEqual(artifact['counts'], {'closed': 183, 'deferred': 209})

    def test_program_counts_follow_the_closures(self):
        program = {'phases': [{'phase': 3, 'closed_functions': 10, 'remaining_functions': 20}]}
        close_static_rows.bump_program(program, {3: 4})
        self.assertEqual(program['phases'][0], {'phase': 3, 'closed_functions': 14, 'remaining_functions': 16})


    def test_falsified_refuses_an_unregistered_gate(self):
        m = {'doc': 'd', 'rows': {'phys_fn_000001': dict(FMAN['rows']['phys_fn_000001'], gate='NxPhysicsNoSuchTests')}}
        with self.assertRaises(ValueError):
            frun(m)

    def test_falsified_refuses_a_gate_of_the_wrong_kind_for_the_detection(self):
        m = {'doc': 'd', 'rows': {'phys_fn_000001': dict(FMAN['rows']['phys_fn_000001'], detected='stdout_delta=3')}}
        with self.assertRaises(ValueError):
            frun(m)

    def test_falsified_refuses_a_zero_detection(self):
        m = {'doc': 'd', 'rows': {'phys_fn_000001': dict(FMAN['rows']['phys_fn_000001'], detected='mismatches=0')}}
        with self.assertRaises(ValueError):
            frun(m)

    def test_falsified_refuses_a_row_not_awaiting_the_testing_pass(self):
        with self.assertRaises(ValueError):
            frun(rows=[frow(1, notes='something else')])
        with self.assertRaises(ValueError):
            frun(rows=[frow(1, state='dynamically_gated')])


if __name__ == '__main__':
    unittest.main()


SRCMAN = {'doc': 'evidence/falsify-x.md', 'rows': {
    'phys_fn_000001': {'gate': 'NxPhysicsActorShapeMutationTests', 'family': 'NxPhysicsActorShapeMutationTests',
                       'mutation': 'Physics/src/core/RevoluteJoint.cpp line 935: determinant == 0.0f  ->  !(determinant == 0.0f)  (neg)',
                       'detected': 'stdout_delta=2842', 'source_mutation': True}}}


class SourceMutationTest(unittest.TestCase):
    def cpp_row(self, n=1, **extra):
        r = inv_row(n, state='reconstructed', phase='3', notes='', implementation='Physics/src/core/RevoluteJoint.cpp',
                    implementation_symbol='RevoluteJoint::row_slot6')
        r.update(extra)
        return r

    def run_source(self, manifest=SRCMAN, rows=None, ledger=None):
        rows = rows or [self.cpp_row()]
        clo = {3: ledger or fclosure(*[r['id'] for r in rows])}
        return close_static_rows.falsify(manifest, {'functions': rows}, clo, ORACLE, STAGED)

    def test_a_c_plus_plus_row_closes_on_a_source_mutation(self):
        inventory, closures = self.run_source()
        r = inventory['functions'][0]
        self.assertEqual(r['state'], 'dynamically_gated')
        self.assertIn('line 935', r['dynamic_proof'])
        self.assertIn('C++', r['notes'])
        e = closures[3]['closed'][0]
        self.assertEqual(e['proof'], 'differential_falsified')
        self.assertIn('line 935', e['falsification']['mutation'])
        self.assertEqual(closures[3]['deferred'], [])

    def test_a_source_mutation_needs_a_line_number(self):
        man = {'doc': 'd', 'rows': {'phys_fn_000001': dict(SRCMAN['rows']['phys_fn_000001'], mutation='a mutation somewhere')}}
        with self.assertRaises(ValueError):
            self.run_source(man)

    def test_without_the_flag_the_testing_pass_note_is_still_required(self):
        man = {'doc': 'd', 'rows': {'phys_fn_000001': dict(SRCMAN['rows']['phys_fn_000001'], mutation='je -> jne at 0x1014', detected='stdout_delta=4')}}
        del man['rows']['phys_fn_000001']['source_mutation']
        with self.assertRaises(ValueError):
            self.run_source(man)

    def test_a_vendored_entry_closes_and_its_count_follows(self):
        row = self.cpp_row(phase='4', third_party='qhull')
        ledger = {'phase': 4, 'counts': {'deferred_vendored_not_falsified': 1, 'differential_falsified': 0, 'oracle_differential_falsified': 0},
                  'closed': [], 'deferred': [{'id': row['id'], 'rva': '0x00001010', 'reason': 'vendored_not_falsified', 'driving_phases': []}]}
        man = {'doc': 'd', 'rows': {row['id']: dict(SRCMAN['rows']['phys_fn_000001'], gate='NxPhysicsActorShapeMutationTests')}}
        clo = {4: ledger}
        close_static_rows.falsify(man, {'functions': [row]}, clo, ORACLE, {'NxPhysicsActorShapeMutationTests': [4, 5, 6, 7]})
        self.assertEqual(clo[4]['counts']['deferred_vendored_not_falsified'], 0)
        self.assertEqual(clo[4]['counts']['differential_falsified'], 1)
        self.assertEqual(len(clo[4]['closed']), 1)


class StaleLedgerEntryTest(unittest.TestCase):
    def test_a_row_reconstructed_since_the_ledger_was_written_closes_from_its_not_reconstructed_entry(self):
        row = inv_row(1, state='reconstructed', phase='3', notes='', implementation='Physics/src/Foo.cpp')
        ledger = {'phase': 3, 'counts': {'deferred_not_reconstructed_in_phase': 1, 'differential_falsified': 0, 'oracle_differential_falsified': 0},
                  'closed': [], 'deferred': [{'id': row['id'], 'rva': '0x00001010', 'reason': 'not_reconstructed_in_phase', 'driving_phases': []}]}
        man = {'doc': 'd', 'rows': {row['id']: dict(SRCMAN['rows']['phys_fn_000001'], gate='NxPhysicsActorShapeMutationTests')}}
        clo = {3: ledger}
        close_static_rows.falsify(man, {'functions': [row]}, clo, ORACLE, {'NxPhysicsActorShapeMutationTests': [3, 5, 6, 7]})
        self.assertEqual(clo[3]['counts']['deferred_not_reconstructed_in_phase'], 0)
        self.assertEqual(len(clo[3]['closed']), 1)


class BlockedEntryTest(unittest.TestCase):
    def test_a_row_waiting_on_a_later_phase_closes_and_is_discharged_by_the_phase_that_runs_its_gate(self):
        row = inv_row(1, state='reconstructed', phase='4', notes='', implementation='Physics/src/Foo.cpp')
        ledger = {'phase': 4, 'counts': {'deferred_blocked_on_later_phase': 1, 'differential_falsified': 0, 'oracle_differential_falsified': 0},
                  'closed': [], 'deferred': [{'id': row['id'], 'rva': '0x00001010', 'reason': 'blocked_on_later_phase', 'driving_phases': [7],
                                              'blocked_on_type': 'Pruner', 'note': 'waits for phase 7'}]}
        man = {'doc': 'd', 'rows': {row['id']: dict(SRCMAN['rows']['phys_fn_000001'], gate='NxPhysicsCollisionTests', detected='mismatches=3', family='f')}}
        clo = {4: ledger}
        close_static_rows.falsify(man, {'functions': [row]}, clo, {'NxPhysicsCollisionTests': [3, 7]}, STAGED)
        e = clo[4]['closed'][0]
        self.assertEqual((e['discharged_by_phase'], e['driving_phases']), (7, [7]))
        self.assertEqual(clo[4]['counts']['deferred_blocked_on_later_phase'], 0)
        self.assertEqual(clo[4]['deferred'], [])
