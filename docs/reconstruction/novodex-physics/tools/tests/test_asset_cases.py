"""cases/assets/formats.json against the transcript and the harness it came from.

The gate is the harness: it drives the shipped oracle and compares. This file
exists so the recorded case matrix cannot drift away from either the run that
produced it or the source that runs it. Everything checked here is recomputed
from the transcript or parsed out of the C++, not read out of the JSON, so
editing the JSON alone cannot make it pass.

The one thing that would make this whole file worthless is a fixture that
carries a pointer out of the oracle's process, because such a fixture means one
thing in the process that made it and nothing anywhere else. That is checked
structurally rather than trusted: every fixture is hex the harness decodes and
nothing else, and the harness is asserted to hold no other source of fixture
bytes.
"""

import json
import re
import unittest
from pathlib import Path

EVIDENCE_ROOT = Path(__file__).resolve().parents[2]
CASES_PATH = EVIDENCE_ROOT / 'cases' / 'assets' / 'formats.json'
TRANSCRIPT_PATH = EVIDENCE_ROOT / 'cases' / 'assets' / 'oracle-transcript.txt'
INVENTORY_PATH = EVIDENCE_ROOT / 'inventory.json'
HARNESS_PATH = Path(__file__).resolve().parents[5] / 'tests' / 'PhysicsAssetTests.cpp'

PMAP_RE = re.compile(r'^pmap case=(?P<id>\S+) dimension=(?P<dimension>\S+) bytes=(?P<bytes>\d+) '
                     r'accepted=(?P<accepted>\d+) errors=(?P<errors>\d+) line=(?P<line>0x[0-9a-f]{3}) '
                     r'resolution=(?P<resolution>\d+) cells=(?P<cells>\d+) grid=(?P<grid>[0-9a-f]{8})$')
MESH_RE = re.compile(r'^mesh case=(?P<id>\S+) dimension=(?P<dimension>\S+) '
                     r'accepted=(?P<accepted>\d+) dwords_read=(?P<dwords>\d+)$')
RELEASE_RE = re.compile(r'^release case=(?P<id>\S+) returned=(?P<returned>\d+) '
                        r'data_size=(?P<size>[0-9a-f]{8}) data=(?P<data>[0-9a-f]{8})$')
HEX_RE = re.compile(r'^(?:[0-9a-f]{2})+$')

# Every dimension the plan asks the fixtures to cover. A tag outside this set is
# a typo; a dimension with no case is a hole.
DIMENSIONS = {'minimal_valid', 'multi_element', 'malformed', 'truncated', 'boundary',
              'ownership', 'nonempty_cell_run', 'command_matrix'}

# The three the reconstruction replaces in Task 2.
CANDIDATE_FUNCTIONS = ('nxCandidatePMapLoad', 'nxCandidateMeshHeader', 'nxCandidateReleasePMap')


def load_transcript():
    pmap, mesh, release = {}, {}, {}
    for line in TRANSCRIPT_PATH.read_text().splitlines():
        if line.startswith('#'):
            continue
        match = PMAP_RE.match(line)
        if match:
            pmap[match.group('id')] = match.groupdict()
            continue
        match = MESH_RE.match(line)
        if match:
            mesh[match.group('id')] = match.groupdict()
            continue
        match = RELEASE_RE.match(line)
        if match:
            release['release.' + match.group('id')] = match.groupdict()
    return pmap, mesh, release


class AssetCases(unittest.TestCase):
    def setUp(self):
        self.document = json.loads(CASES_PATH.read_text())
        self.pmap, self.mesh, self.release = load_transcript()
        self.harness = HARNESS_PATH.read_text() if HARNESS_PATH.is_file() else None

    def test_transcript_covers_exactly_the_recorded_cases(self):
        self.assertEqual(sorted(self.pmap), sorted(c['id'] for c in self.document['pmap_cases']))
        self.assertEqual(sorted(self.mesh), sorted(c['id'] for c in self.document['mesh_cases']))
        self.assertEqual(sorted(self.release), sorted(c['id'] for c in self.document['release_cases']))

    def test_every_pmap_case_matches_the_transcript_word_for_word(self):
        for case in self.document['pmap_cases']:
            with self.subTest(case=case['id']):
                line = self.pmap[case['id']]
                self.assertEqual(case['dimension'], line['dimension'])
                self.assertEqual(case['byte_count'], int(line['bytes']))
                if case['bytes'] is not None:
                    self.assertEqual(case['byte_count'] * 2, len(case['bytes']))
                elif case['encoding']['kind'] == 'absolute_cell_run':
                    self.assertEqual(case['encoding']['command'], 31)
                    self.assertEqual(case['encoding']['coordinates'], [1, 2, 3])
                    self.assertEqual(case['encoding']['filled_sign_bits'], 32 ** 3)
                else:
                    self.assertEqual(case['encoding']['kind'], 'absolute_delta_command_matrix')
                    self.assertEqual(case['encoding']['commands'], [31] + list(range(26)) + list(range(26, 32)))
                    self.assertEqual(case['encoding']['filled_sign_bits'], 32 ** 3)
                self.assertEqual(case['oracle']['accepted'], int(line['accepted']))
                self.assertEqual(case['oracle']['errors'], int(line['errors']))
                self.assertEqual(case['oracle']['error_line'], line['line'])
                self.assertEqual(case['oracle']['resolution'], int(line['resolution']))
                self.assertEqual(case['oracle']['cells'], int(line['cells']))
                self.assertEqual(case['oracle']['grid'], line['grid'])

    def test_every_mesh_case_matches_the_transcript_word_for_word(self):
        for case in self.document['mesh_cases']:
            with self.subTest(case=case['id']):
                line = self.mesh[case['id']]
                self.assertEqual(case['dimension'], line['dimension'])
                self.assertEqual(case['oracle']['accepted'], int(line['accepted']))
                self.assertEqual(case['oracle']['dwords_read'], int(line['dwords']))

    def test_every_release_case_matches_the_transcript_word_for_word(self):
        for case in self.document['release_cases']:
            with self.subTest(case=case['id']):
                line = self.release[case['id']]
                self.assertEqual(case['oracle']['returned'], int(line['returned']))
                self.assertEqual(case['oracle']['data_size'], line['size'])
                self.assertEqual(case['oracle']['data'], line['data'])

    def test_fixtures_are_lower_case_hex_and_carry_no_pointer(self):
        # A fixture is bytes and only bytes. Nothing here is allowed to be a
        # value the oracle's own address space produced, which is why the
        # encoding is checked rather than assumed: a pointer cannot survive
        # being written as a literal hex byte string that another process reads.
        for case in self.document['pmap_cases'] + self.document['mesh_cases']:
            with self.subTest(case=case['id']):
                if case['bytes'] is not None:
                    self.assertRegex(case['bytes'], HEX_RE)
                else:
                    self.assertIsInstance(case['encoding'], dict)

    def test_every_dimension_the_plan_names_has_a_case(self):
        seen = {c['dimension'] for c in
                self.document['pmap_cases'] + self.document['mesh_cases'] + self.document['release_cases']}
        self.assertEqual(seen, DIMENSIONS)

    def test_every_probe_names_an_owned_phase_4_row_at_the_recorded_rva(self):
        inventory = json.loads(INVENTORY_PATH.read_text())
        rows = {row['id']: row for row in inventory['functions']}
        for probe in self.document['probes']:
            with self.subTest(probe=probe['name']):
                row = rows[probe['owner']]
                self.assertEqual(row['rva'], probe['rva'])
                self.assertEqual(row['phase'], 4)

    def test_the_probe_case_counts_are_the_case_counts(self):
        counts = {'pmap': len(self.document['pmap_cases']),
                  'mesh': len(self.document['mesh_cases']),
                  'release': len(self.document['release_cases'])}
        for probe in self.document['probes']:
            self.assertEqual(probe['cases'], counts[probe['name']])

    def test_the_harness_holds_the_same_bytes_and_the_same_answers(self):
        # The check that makes the two above worth anything: the fixture bytes
        # and the recorded answers have to appear in the source that runs them,
        # or this JSON is a description of a run nobody can reproduce.
        if self.harness is None:
            self.skipTest('the implementation repository is not present')
        for case in self.document['pmap_cases'] + self.document['mesh_cases']:
            with self.subTest(case=case['id']):
                self.assertIn('"%s"' % case['id'], self.harness)
                if case['bytes'] is not None:
                    self.assertIn(case['bytes'][:32], self.harness)
                else:
                    if case['encoding']['kind'] == 'absolute_cell_run':
                        self.assertIn('nxBuildPMapAbsoluteCellRun', self.harness)
                        self.assertIn('writeBits(0x1f, 5)', self.harness)
                    else:
                        self.assertIn('nxBuildPMapCommandMatrix', self.harness)
                        self.assertIn('writeBits(33, 32)', self.harness)
                        self.assertIn('for(unsigned code = 0; code < 26; ++code)', self.harness)

    def test_the_harness_takes_its_fixture_bytes_only_from_hex(self):
        # nxDecodeHex is the one door fixture bytes come through. If a second
        # one appears -- a file read, a resource, a pointer cast -- this fails,
        # because a fixture that is not a hex literal in this table is a fixture
        # this JSON does not describe.
        if self.harness is None:
            self.skipTest('the implementation repository is not present')
        self.assertEqual(self.harness.count('nxDecodeHex('), 3)
        for forbidden in ('CreateFileA', 'CreateFileW(fixture', 'fopen', 'FindResource'):
            self.assertNotIn(forbidden + '(', self.harness.replace('CreateFileW(path', ''))

    def test_the_candidate_side_is_never_handed_the_recorded_expectations(self):
        # A candidate that can read the oracle's recorded answers can agree with
        # the oracle on every case without parsing a byte, which makes this gate
        # green with nothing reconstructed.
        #
        # NxPMapFixture and NxMeshFixture carry expectAccepted, expectErrors,
        # expectErrorLine, expectCells, expectGrid and expectDwordsRead, so a
        # candidate handed one of them is handed the answers. The candidates
        # take fixture bytes and a length instead, and that is what is asserted
        # here -- structurally, on the signature and the body.
        #
        # This replaces a test that required a literal `return false;` in each
        # body. That one held only while the candidates were stubs: Task 2 has
        # to delete the `return false;` to make the gate green, and would have
        # deleted the test with it. This one is the property Task 2 must keep.
        if self.harness is None:
            self.skipTest('the implementation repository is not present')
        for name in CANDIDATE_FUNCTIONS:
            with self.subTest(candidate=name):
                marker = 'static bool %s(' % name
                self.assertIn(marker, self.harness)
                after = self.harness.split(marker)[-1]
                parameters = after.split(')')[0]
                for forbidden in ('NxPMapFixture', 'NxMeshFixture'):
                    self.assertNotIn(forbidden, parameters,
                        '%s takes a %s, which carries the oracle answers this gate compares '
                        'it against' % (name, forbidden))
                body = after.split('\n\t}')[0]
                self.assertNotIn('expect', body,
                    '%s names a recorded expectation; a candidate may only see fixture bytes'
                    % name)

    def test_the_harness_compares_every_result_field_it_prints_a_mismatch_for(self):
        # The pmap candidate comparison is a memcmp over the whole NxPMapResult,
        # so a run can fail on errorLine or resolution alone. Both were left out
        # of the mismatch line, which printed every field identical on both
        # sides and said nothing about why it failed.
        if self.harness is None:
            self.skipTest('the implementation repository is not present')
        marker = 'pmap CANDIDATE-MISMATCH case='
        self.assertEqual(self.harness.count(marker), 1)
        start = self.harness.index(marker)
        statement = self.harness[start:self.harness.index(');', start)]
        for field in ('accepted=%u/%u', 'errors=%u/%u', 'line=0x%03x/0x%03x',
                      'cells=%u/%u', 'grid=%08x/%08x', 'resolution=%u/%u'):
            self.assertIn(field, statement,
                'the pmap mismatch line does not print %s, which the memcmp compares' % field)


if __name__ == '__main__':
    unittest.main()
