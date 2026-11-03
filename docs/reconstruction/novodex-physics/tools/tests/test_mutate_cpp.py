import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import mutate_cpp as mc

SRC = '''#include <x.h>
// a < b in a comment
int A::f(int x)
{
	const char* s = "a < b";
	if(x < 3 && x != 7)
		return x + 2;
	return 0;
}

int A::g(int y) const
{
	return y * 3;
}

// phys_fn_000123 (0x00001000, 99 B)
// notes about it
static int helper(int q)
{
	for(int i = 0; i < q; ++i) q -= 4;
	return q;
}
'''


class SpanTests(unittest.TestCase):
    def body(self, text, span):
        return text[span[0]:span[1]]

    def test_span_finds_member_definition(self):
        span = mc.find_function_span(SRC, 'A::f(int)')
        self.assertTrue(self.body(SRC, span).startswith('{') and self.body(SRC, span).endswith('}'))
        self.assertIn('x + 2', self.body(SRC, span))
        self.assertNotIn('y * 3', self.body(SRC, span))

    def test_span_const_member(self):
        span = mc.find_function_span(SRC, 'A::g(int) const')
        self.assertIn('y * 3', self.body(SRC, span))

    def test_span_ambiguous_overload_returns_none(self):
        text = 'int B::h(int a) { return a; }\nint B::h(float a) { return 1; }\n'
        self.assertIsNone(mc.find_function_span(text, 'B::h'))
        self.assertIsNotNone(mc.find_function_span(text, 'B::h(float)'))

    def test_span_ignores_calls_and_comments(self):
        text = 'void k() { A::f(1); }\n// A::f(int) { fake }\nint A::f(int x) { return 2; }\n'
        span = mc.find_function_span(text, 'A::f(int)')
        self.assertEqual(text[span[0]:span[1]], '{ return 2; }')

    def test_span_by_marker(self):
        span = mc.find_span_by_marker(SRC, 'phys_fn_000123')
        self.assertIn('q -= 4', SRC[span[0]:span[1]])
        self.assertIsNone(mc.find_span_by_marker(SRC, 'phys_fn_999999'))


class MutationTests(unittest.TestCase):
    def setUp(self):
        self.span = mc.find_function_span(SRC, 'A::f(int)')
        self.muts = mc.cpp_mutations(SRC, *self.span)

    def test_mutations_skip_comments_and_strings(self):
        for m in mc.cpp_mutations(SRC, 0, len(SRC)):
            line = SRC.count('\n', 0, m[0])
            self.assertNotIn(line, (1, 4))

    def test_kinds_present(self):
        kinds = {m[3] for m in self.muts}
        self.assertTrue({'cmp', 'arith', 'const', 'neg'} <= kinds, kinds)

    def test_apply_changes_exactly_one_site(self):
        m = self.muts[0]
        out = mc.apply_mutation(SRC, m)
        self.assertNotEqual(out, SRC)
        off, old, new = m[0], m[1], m[2]
        self.assertEqual(out[:off], SRC[:off])
        self.assertEqual(out[off:off + len(new)], new)
        self.assertEqual(out[off + len(new):], SRC[off + len(old):])

    def test_apply_refuses_stale_mutation(self):
        m = self.muts[0]
        with self.assertRaises(ValueError):
            mc.apply_mutation('x' * len(SRC), m)

    def test_spread_is_even_and_deterministic(self):
        items = [(i, 'a', 'b', 'cmp') for i in range(20)]
        got = mc.spread(items, 5)
        self.assertEqual(len(got), 5)
        self.assertEqual(got, mc.spread(items, 5))
        self.assertEqual((got[0], got[-1]), (items[0], items[-1]))
        self.assertEqual(mc.spread(items, 50), items)


if __name__ == '__main__':
    unittest.main()
