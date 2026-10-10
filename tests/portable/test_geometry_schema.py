import copy
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'docs/reconstruction/novodex-physics/tools/geometry_fixture_schema.py'
spec = importlib.util.spec_from_file_location('schema', SCRIPT)
schema = importlib.util.module_from_spec(spec)
spec.loader.exec_module(schema)

class SchemaTests(unittest.TestCase):
    def test_immutable_words_and_void(self):
        old = json.loads((ROOT/'tests/portable/fixtures/geometry-shipped-oracle.json').read_text())
        new = schema.convert(old)
        self.assertEqual(len(new['records']), 299)
        self.assertEqual(sum(r['return']['kind']=='void' for r in new['records']), 28)
        for before, after in zip(old['records'], new['records']):
            self.assertEqual(before['case'], after['case'])
            self.assertEqual(before['input_u32_hex'], after['input_u32_hex'])
            self.assertEqual(before['output_u32_hex'], after['output_u32_hex'])
            self.assertEqual(after['return'], {'kind':'void'} if before['return_u32_hex']=='void'
                             else {'kind':'u32','word_hex':before['return_u32_hex']})
        schema.validate(new)

    def test_rejects_malformed_return(self):
        value = schema.convert(json.loads((ROOT/'tests/portable/fixtures/geometry-shipped-oracle.json').read_text()))
        for ret in ({'kind':'void','word_hex':'00000000'}, {'kind':'u32','word_hex':'void'},
                    {'kind':'u32'}, {'kind':'float','word_hex':'00000000'}):
            bad=copy.deepcopy(value); bad['records'][0]['return']=ret
            with self.assertRaises(ValueError): schema.validate(bad)

if __name__ == '__main__': unittest.main()
