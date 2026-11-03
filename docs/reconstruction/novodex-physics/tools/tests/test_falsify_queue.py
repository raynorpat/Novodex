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


if __name__ == '__main__':
    unittest.main()
