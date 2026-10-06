"""cases/geometry.json against the oracle transcript it claims to record.

The differential is the gate; this file exists so the recorded case matrix
cannot drift away from the harness that produced it. Everything checked here is
recomputed from the transcript rather than read out of the JSON, so editing the
JSON alone cannot make it pass.
"""

import json
import re
import unittest
from pathlib import Path

EVIDENCE_ROOT = Path(__file__).resolve().parents[2]
CASES_PATH = EVIDENCE_ROOT / 'cases' / 'geometry.json'
TRANSCRIPT_PATH = EVIDENCE_ROOT / 'cases' / 'geometry-oracle-transcript.txt'
INVENTORY_PATH = EVIDENCE_ROOT / 'inventory.json'
HARNESS_PATH = Path('D:/github/Novodex/tests/PhysicsGeometryTests.cpp')

CASE_RE = re.compile(r'^case=(?P<id>[A-Za-z0-9]+\.\d\d) in=(?P<in>\S+) '
                     r'ret=(?P<ret>\S+) out=(?P<out>\S+)$')
EXPORT_RE = re.compile(r'^export name=(?P<name>\w+) ordinal=(?P<ordinal>\d+) present=(?P<present>\d)$')
WORD_RE = re.compile(r'^[0-9a-f]{8}$')

# Every dimension the brief asks the matrix to cover, and the tag that carries
# it. A tag outside this set is a typo; a dimension with no case is a hole.
DIMENSIONS = {
    'hit', 'miss', 'tangent', 'reversed_direction', 'zero_length',
    'non_normalized', 'degenerate', 'aliasing', 'null_optional_output',
    'null_optional_input', 'undocumented_domain', 'undocumented_bool_encoding',
    'non_finite', 'nominal', 'overflow', 'underflow', 'rotated',
}

# Poison is 0xcdcd0000 + the word's offset in the buffer, so an untouched guard
# word is always of this shape and an ascending run of them is a ladder no
# uninitialised stack produces.
POISON_RE = re.compile(r'^cdcd00[0-9a-f]{2}$')


def load_transcript():
    cases = {}
    exports = {}
    for line in TRANSCRIPT_PATH.read_text().splitlines():
        if line.startswith('#'):
            continue
        match = CASE_RE.match(line)
        if match:
            cases[match.group('id')] = (match.group('in').split(','),
                                        match.group('ret'),
                                        [] if match.group('out') == '-' else match.group('out').split(','))
            continue
        match = EXPORT_RE.match(line)
        if match:
            exports[match.group('name')] = (int(match.group('ordinal')), match.group('present'))
    return cases, exports


class GeometryCases(unittest.TestCase):
    def setUp(self):
        self.document = json.loads(CASES_PATH.read_text())
        self.cases = self.document['cases']
        self.transcript, self.exports = load_transcript()

    def test_transcript_covers_exactly_the_recorded_cases(self):
        self.assertEqual(sorted(self.transcript), sorted(case['id'] for case in self.cases))
        self.assertEqual(len(self.cases), len(self.transcript))

    def test_every_case_matches_the_transcript_word_for_word(self):
        for case in self.cases:
            with self.subTest(case=case['id']):
                inputs, ret, out = self.transcript[case['id']]
                self.assertEqual(case['inputs'], inputs)
                self.assertEqual(case['oracle']['ret'], ret)
                self.assertEqual(case['oracle']['out'], out)

    def test_words_are_lower_case_eight_digit_hexadecimal(self):
        # Decimal does not round-trip and the differential is bit-exact, so a
        # value that is not exactly eight hex digits is a value that was not
        # recorded exactly.
        for case in self.cases:
            for word in list(case['inputs']) + list(case['oracle']['out']):
                self.assertRegex(word, WORD_RE)
            self.assertTrue(case['oracle']['ret'] == 'void' or WORD_RE.match(case['oracle']['ret']))

    def test_case_ids_are_dense_and_grouped_by_export(self):
        seen = {}
        for case in self.cases:
            export, index = case['id'].rsplit('.', 1)
            self.assertEqual(export, case['export'])
            seen.setdefault(export, []).append(int(index))
        for export, indices in seen.items():
            self.assertEqual(indices, list(range(len(indices))), export)

    def test_input_width_matches_the_declared_layout(self):
        layouts = {e['name']: sum(f['words'] for f in e['input_layout'])
                   for e in self.document['exports']}
        counts = {e['name']: e['cases'] for e in self.document['exports']}
        actual = {}
        for case in self.cases:
            self.assertEqual(len(case['inputs']), layouts[case['export']], case['id'])
            actual[case['export']] = actual.get(case['export'], 0) + 1
        self.assertEqual(actual, counts)

    def test_output_width_matches_the_declared_layout_plus_guards(self):
        for export in self.document['exports']:
            width = sum(f['words'] + f['guard_words'] for f in export['output_layout'])
            for case in self.cases:
                if case['export'] == export['name']:
                    self.assertEqual(len(case['oracle']['out']), width, case['id'])

    def test_integer_columns_hold_integers(self):
        # A float bit pattern in a selector or bool column reads as a large
        # integer and silently turns the case into a different case.
        for export in self.document['exports']:
            at = 0
            for field in export['input_layout']:
                if field['encoding'] == 'integer':
                    for case in self.cases:
                        if case['export'] != export['name']:
                            continue
                        for word in case['inputs'][at:at + field['words']]:
                            self.assertLess(int(word, 16), 0x10, (case['id'], field['field']))
                at += field['words']

    def test_every_phase_three_export_is_covered(self):
        inventory = json.loads(INVENTORY_PATH.read_text(encoding='utf-8'))
        functions = {f['id']: f for f in inventory['functions']}
        expected = sorted(e['name'] for e in inventory['exports']
                          if functions[e['function_id']]['phase'] == 3)
        self.assertEqual(sorted(e['name'] for e in self.document['exports']), expected)
        self.assertEqual(sorted(self.exports), expected)
        for export in self.document['exports']:
            ordinal, present = self.exports[export['name']]
            self.assertEqual(ordinal, export['ordinal'])
            self.assertEqual(present, '1')

    def test_dimensions_come_from_the_vocabulary_and_are_all_exercised(self):
        used = {case['dimension'] for case in self.cases}
        self.assertEqual(used - DIMENSIONS, set())
        self.assertEqual(DIMENSIONS - used, set())

    def test_every_output_carrying_export_has_an_aliasing_or_null_case(self):
        with_outputs = {e['name'] for e in self.document['exports'] if e['output_layout']}
        covered = {case['export'] for case in self.cases
                   if case['dimension'] in ('aliasing', 'null_optional_output')}
        # The two inertia tensors take a single write-only vector and nothing to
        # alias it with, so they are the documented exceptions.
        self.assertEqual(with_outputs - covered,
                         {'NxComputeBoxInertiaTensor', 'NxComputeSphereInertiaTensor'})

    def test_the_harness_declares_the_same_case_counts(self):
        source = HARNESS_PATH.read_text()
        for export in self.document['exports']:
            table = re.search(r'static const NxU32 nxCases_%s\[\]\[(\d+)\] =\s*\{(.*?)\n\t\};'
                              % export['name'], source, re.S)
            self.assertIsNotNone(table, export['name'])
            self.assertEqual(int(table.group(1)),
                             sum(f['words'] for f in export['input_layout']))
            self.assertEqual(table.group(2).count('\n\t{ '), export['cases'])

    def test_the_harness_declares_the_same_input_layout(self):
        # test_integer_columns_hold_integers reads the encoding out of this
        # file, so without this check re-tagging a column as float would
        # silently switch that check off instead of failing it.
        source = HARNESS_PATH.read_text()
        for export in self.document['exports']:
            comment = re.search(r'^// %s: (.*)$' % export['name'], source, re.M)
            self.assertIsNotNone(comment, export['name'])
            declared = [(f['field'], f['words'], f['encoding'])
                        for f in export['input_layout']]
            parsed = []
            for part in comment.group(1).split(', '):
                field, rest = part.split('[', 1)
                words, encoding = rest.split(']:', 1)
                parsed.append((field, int(words), encoding))
            self.assertEqual(parsed, declared, export['name'])

    def test_guard_words_are_a_poison_ladder_or_an_input(self):
        # Blocks the failure this task hit once already: an output buffer that
        # is never gathered prints uninitialised stack, which reads as a
        # plausible result. Stack cannot spell 0xcdcd00NN in ascending order.
        layouts = {e['name']: e['output_layout'] for e in self.document['exports']}
        for case in self.cases:
            with self.subTest(case=case['id']):
                inputs = set(case['inputs'])
                at = 0
                for field in layouts[case['export']]:
                    at += field['words']
                    # Per segment, because an aliasing case can aim two output
                    # pointers at one buffer and see the same offsets twice.
                    previous = None
                    for word in case['oracle']['out'][at:at + field['guard_words']]:
                        if POISON_RE.match(word):
                            offset = int(word[-2:], 16)
                            if previous is not None:
                                self.assertEqual(offset, previous + 1, (case['id'], word))
                            previous = offset
                        else:
                            # The only way a guard is not poison is an aliasing
                            # case, where it is a word of the input the output
                            # was aimed into.
                            self.assertEqual(case['dimension'], 'aliasing', (case['id'], word))
                            self.assertIn(word, inputs, (case['id'], word))
                    at += field['guard_words']

    def test_aliasing_cases_reach_the_write_and_null_cases_do_not(self):
        # An aliasing case whose declared output is still all poison never
        # exercised the alias: the call returned before writing. Its guard words
        # come from the input either way, so only the declared words prove it.
        layouts = {e['name']: e['output_layout'] for e in self.document['exports']}
        for case in self.cases:
            if case['dimension'] not in ('aliasing', 'null_optional_output'):
                continue
            with self.subTest(case=case['id']):
                declared = []
                at = 0
                for field in layouts[case['export']]:
                    declared += case['oracle']['out'][at:at + field['words']]
                    at += field['words'] + field['guard_words']
                self.assertTrue(declared, case['id'])
                written = [word for word in declared if not POISON_RE.match(word)]
                if case['dimension'] == 'aliasing':
                    self.assertTrue(written, case['id'])
                else:
                    self.assertEqual(written, [], case['id'])


if __name__ == '__main__':
    unittest.main()
