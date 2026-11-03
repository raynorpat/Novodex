import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import falsify_queue as fq


def row(rid, state='reconstructed', kind='code', **kw):
    base = {'id': rid, 'rva': '0x00001000', 'size': 16, 'kind': kind, 'state': state, 'phase': 4,
            'implementation': 'Physics/src/Foo.cpp', 'implementation_symbol': 'Foo::bar()',
            'dynamic_proof': 'NxPhysicsFooTests exercises it', 'third_party': None, 'source': None}
    base.update(kw)
    return base


class QueueTests(unittest.TestCase):
    def test_queue_keeps_only_unfinished_code_rows(self):
        inv = {'functions': [row('a', 'reconstructed'), row('b', 'discovered'), row('c', 'statically_reviewed'),
                             row('d', 'dynamically_gated'), row('e', 'closed'), row('f', 'classified'),
                             row('g', 'reconstructed', kind='data')]}
        ids = [r['id'] for r in fq.build_queue(inv)]
        self.assertEqual(ids, ['a', 'b', 'c'])

    def test_queue_parses_numbers_and_target(self):
        r = fq.build_queue({'functions': [row('a', rva='0x000f2090', size='199')]})[0]
        self.assertEqual((r['rva'], r['size'], r['target'], r['symbol']), (0xf2090, 199, 'NxPhysicsFooTests', 'Foo::bar()'))

    def test_group_of_joints(self):
        self.assertEqual(fq.group_of(row('a', implementation='Physics/src/D6Joint.cpp')), 'joints')
        self.assertEqual(fq.group_of(row('a', implementation='Physics/src/NpFixedJoint.cpp')), 'joints')

    def test_group_of_vendored(self):
        self.assertEqual(fq.group_of(row('a', third_party='qhull')), 'vendored_qhull')
        self.assertEqual(fq.group_of(row('a', third_party='opcode')), 'vendored_opcode')

    def test_group_of_no_driver_and_no_mapping(self):
        self.assertEqual(fq.group_of(row('a', implementation=None, implementation_symbol=None, dynamic_proof=None)), 'no_mapping')
        self.assertEqual(fq.group_of(row('a', dynamic_proof=None)), 'no_driver')

    def test_group_of_other_first_party(self):
        self.assertEqual(fq.group_of(row('a', implementation='Physics/src/Whatever.cpp')), 'other_first_party')
        self.assertEqual(fq.group_of(row('a', implementation='Physics/src/ContactGeneration.cpp')), 'collision')
        self.assertEqual(fq.group_of(row('a', implementation='Physics/src/Quantizer.cpp')), 'mesh')
        self.assertEqual(fq.group_of(row('a', implementation='Physics/src/opcode/PrunerOctree.cpp')), 'pruner')
        self.assertEqual(fq.group_of(row('a', implementation='Physics/src/core/SceneDump.cpp')), 'scene')

    def test_batches_never_share_a_target(self):
        rows = [{'id': str(i), 'target': 'A' if i < 3 else 'B'} for i in range(6)]
        for batch in fq.batches(rows, 4):
            targets = [r['target'] for r in batch]
            self.assertEqual(len(targets), len(set(targets)))
        self.assertEqual(sorted(r['id'] for b in fq.batches(rows, 4) for r in b), sorted(r['id'] for r in rows))



class PlanTests(unittest.TestCase):
    ROWS = [{'id': 'a', 'target': 'NxS1', 'group': 'joints'}, {'id': 'b', 'target': 'NxS2', 'group': 'joints'},
            {'id': 'c', 'target': 'NxS1', 'group': 'joints'}, {'id': 'd', 'target': None, 'group': 'joints'},
            {'id': 'e', 'target': 'NxO1', 'group': 'joints'}]

    def test_plans_hold_only_staged_rows_with_distinct_targets(self):
        plans, unplanned = fq.plans(self.ROWS, staged={'NxS1', 'NxS2'}, size=4)
        self.assertEqual([[r['id'] for r in p] for p in plans], [['a', 'b'], ['c']])
        self.assertTrue(all(r['staged'] for p in plans for r in p))
        self.assertEqual({u['id']: u['why'] for u in unplanned}, {'d': 'no gated target named', 'e': 'target NxO1 is not a staged-pair gate'})



class OracleOnlyPlanTests(unittest.TestCase):
    def test_oracle_only_digest_targets_are_planned_and_family_targets_are_not(self):
        rows = [{'id': 'a', 'target': 'NxJ', 'group': 'joints'}, {'id': 'b', 'target': 'NxPhysicsCollisionTests', 'group': 'joints'}]
        plans, unplanned = fq.plans(rows, staged=set(), size=4, oracle={'NxJ', 'NxPhysicsCollisionTests'})
        self.assertEqual(plans, [[{'id': 'a', 'target': 'NxJ', 'oracle_only': True, 'count_crash': True}]])
        self.assertEqual(unplanned, [{'id': 'b', 'why': 'target NxPhysicsCollisionTests needs a family (its output is read by family)'}])



class FamilyTests(unittest.TestCase):
    FAMS = {'NxPhysicsCollisionTests': {'convex_mesh_faces', 'box_box.random', 'build'}}

    def row(self, rid, proof):
        return {'id': rid, 'target': 'NxPhysicsCollisionTests', 'group': 'collision', 'dynamic_proof': proof}

    def test_family_from_the_proof_text(self):
        self.assertEqual(fq.family_of(self.row('a', 'oracle differential convex_mesh_faces (NxPhysicsCollisionTests: 0018)'), self.FAMS), 'convex_mesh_faces')

    def test_generic_words_do_not_make_a_family(self):
        self.assertIsNone(fq.family_of(self.row('a', 'we build and run it'), self.FAMS))

    def test_two_candidate_families_are_ambiguous(self):
        self.assertIsNone(fq.family_of(self.row('a', 'convex_mesh_faces and box_box.random both run it'), self.FAMS))

    def test_plans_use_the_family_and_batch_by_it(self):
        rows = [self.row('a', 'convex_mesh_faces x'), self.row('b', 'convex_mesh_faces y'), self.row('c', 'box_box.random z'), self.row('d', 'nothing')]
        plans, unplanned = fq.plans(rows, staged=set(), size=4, oracle={'NxPhysicsCollisionTests'}, families=self.FAMS)
        self.assertEqual([sorted(r['id'] for r in p) for p in plans], [['a', 'c'], ['b']])
        self.assertEqual(plans[0][0]['family'] in ('convex_mesh_faces', 'box_box.random'), True)
        self.assertEqual([u['id'] for u in unplanned], ['d'])



class VendoredPlanTests(unittest.TestCase):
    COV = {('qhull', '0x00001000'): 'thirdparty:qh_a=exact+qh_a_x87=exact', ('qhull', '0x00002000'): 'thirdparty:qh_a=exact',
           ('opcode', '0x00003000'): 'thirdparty:op_b=exact'}
    NAMES = {'qh_a', 'qh_a_x87', 'op_b'}

    def rows(self):
        return [{'id': 'a', 'rva': 0x1000, 'third_party': 'qhull', 'group': 'vendored_qhull'},
                {'id': 'b', 'rva': 0x2000, 'third_party': 'qhull', 'group': 'vendored_qhull'},
                {'id': 'c', 'rva': 0x3000, 'third_party': 'opcode', 'group': 'vendored_opcode'},
                {'id': 'd', 'rva': 0x4000, 'third_party': 'opcode', 'group': 'vendored_opcode'}]

    def test_plans_name_the_families_and_keep_overlapping_rows_apart(self):
        plans = fq.vendored_plans(self.rows(), self.COV, self.NAMES, 5)
        by = {r['id']: r for p in plans for r in p}
        self.assertEqual(by['a']['families'], ['qh_a', 'qh_a_x87'])
        self.assertEqual(by['c']['families'], ['op_b'])
        self.assertEqual(by['d']['families'], [])
        self.assertTrue(all(r['lines'] and r['target'] == 'NxPhysicsThirdPartyTests' for p in plans for r in p))
        together = [sorted(r['id'] for r in p) for p in plans]
        self.assertFalse(any({'a', 'b'} <= set(t) for t in together))
        self.assertTrue(any(t == ['d'] for t in together))


if __name__ == '__main__':
    unittest.main()
