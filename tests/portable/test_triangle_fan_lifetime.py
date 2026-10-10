"""Check the active compiled fan body has a real vendor Triangle lifetime.

The numeric oracle cannot distinguish calling nonvirtual methods through a
layout-compatible cast from using a constructed Triangle. This source contract
therefore consumes compiler-preprocessed backend0 code in addition to the
actual-source numeric test, and rejects that specific historical UB.
"""
import argparse
import pathlib
import re


def fan_body(source):
    starts = list(re.finditer(r'\bNxRayInflatedTriangleFan\s*\([^;{}]*\)\s*\{', source))
    if len(starts) != 1:
        raise ValueError('expected exactly one active fan definition')
    start = starts[0].end()
    depth = 1
    for end in range(start, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if not depth:
            return source[start:end]
    raise ValueError('unterminated active fan definition')


parser = argparse.ArgumentParser()
parser.add_argument('preprocessed_geometry', type=pathlib.Path)
args = parser.parse_args()
body = fan_body(args.preprocessed_geometry.read_text(errors='replace'))
if re.search(r'\breinterpret_cast\s*<\s*IceMaths\s*::\s*Triangle\s*\*', body):
    raise SystemExit('FAIL: active scalar fan calls Triangle through unrelated storage')
# This is a narrow regression for the observed cast, not a complete C++ lifetime
# proof. The actual automatic object and typed transfers are audited in review;
# compiler execution and the numeric fixtures cover their full call behavior.
print('triangle_fan_lifetime: historical unrelated-storage Triangle cast absent in active scalar source')
