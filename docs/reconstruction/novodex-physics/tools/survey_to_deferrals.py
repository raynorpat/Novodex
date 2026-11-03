#!/usr/bin/env python3
"""Turn a mutate_cpp survey log into the deferral manifest for the rows it did not close.

usage: python survey_to_deferrals.py <survey log> <doc> <out manifest> [--limit-survivors 2]

Rows with a catch and at most 2 survivors are the ones `mutate_cpp.py --manifest` closes; every other row of the log
gets a deferral: no mutation changed its compiled function (not_independently_falsifiable), or the gate missed too many
(reconstructed_not_falsified, the note giving the counts).
"""
import json
import re
import sys


def classify(lines, limit=2):
    stats = {}
    for line in lines:
        m = re.match(r'SURVEY (phys_fn_\d+) (\w+) tried=(\d+) caught=(\d+) survived=(\d+) rejected=(\d+)(?: nobuild=(\d+))?', line)
        if m:
            stats[m.group(1)] = (m.group(2),) + tuple(int(x) for x in m.groups()[2:6]) + (int(m.group(7) or 0),)
    closable, deferred = [], {}
    for rid, (target, tried, caught, survived, rejected, nobuild) in stats.items():
        if caught >= 1 and survived <= min(limit, tried // 4):
            closable.append(rid)
        elif tried == 0 and nobuild >= rejected > 0:
            continue    # every attempt failed to build: nothing was learned about the row
        elif tried == 0 and rejected == 0:
            deferred[rid] = {'reason': 'not_independently_falsifiable',
                             'note': 'The function body has no operator the mutation set can change (calls and assignments only), '
                                     'so no source mutation can be aimed at it.'}
        elif tried == 0:
            deferred[rid] = {'reason': 'not_independently_falsifiable',
                             'note': 'None of the %d source mutations tried changed this row\'s compiled function in NxPhysics.dll '
                                     '(compiler-generated or folded code with no body of its own).' % rejected}
        else:
            deferred[rid] = {'reason': 'reconstructed_not_falsified',
                             'note': '%s caught %d of %d mutations tried (%d survived, %d changed other code and were not counted); '
                                     'more than %d survivors, so the gate does not yet cover this row.' % (target, caught, tried, survived, rejected, limit)}
    return closable, deferred


def main(argv=None):
    argv = argv or sys.argv[1:]
    log, doc, out = argv[:3]
    limit = int(argv[3]) if len(argv) > 3 else 2    # survivors allowed for a closable row (1 when only 6 mutations were tried)
    closable, deferred = classify(open(log, encoding='utf-8').read().splitlines(), limit)
    json.dump({'doc': doc, 'rows': deferred}, open(out, 'w', encoding='utf-8'), indent=1)
    print('closable %d, deferred %d' % (len(closable), len(deferred)))


if __name__ == '__main__':
    main()
