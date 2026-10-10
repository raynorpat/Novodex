"""Lossless schema adaptation of immutable shipped geometry evidence, not a new oracle."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

WORD = re.compile(r'[0-9a-f]{8}\Z')

def validate(value):
    if value.get('schema_version') != 2 or value.get('provenance_kind') != 'shipped-oracle':
        raise ValueError('expected discriminated shipped-oracle schema 2')
    cases=set()
    for r in value['records']:
        if r['case'] in cases: raise ValueError('duplicate case')
        cases.add(r['case'])
        for field in ('input_u32_hex','output_u32_hex'):
            if any(not isinstance(w,str) or not WORD.fullmatch(w) for w in r[field]):
                raise ValueError('invalid u32 word')
        ret=r['return']
        if ret == {'kind':'void'}: continue
        if set(ret) != {'kind','word_hex'} or ret['kind'] != 'u32' or not WORD.fullmatch(ret['word_hex']):
            raise ValueError('invalid discriminated return')

def convert(old):
    result={k:v for k,v in old.items() if k!='records'}
    result['schema_version']=2
    result['records']=[]
    for r in old['records']:
        item={k:v for k,v in r.items() if k!='return_u32_hex'}
        word=r['return_u32_hex']
        item['return']={'kind':'void'} if word=='void' else {'kind':'u32','word_hex':word}
        result['records'].append(item)
    validate(result)
    return result

def header(value, destination):
    validate(value)
    rows=[]
    for r in value['records']:
        words=lambda field: ','.join('0x'+w+'u' for w in r[field]) or '0'
        ret=r['return']
        rows.append('{"%s",{%s},%d,%s,0x%su,{%s},%d}' %
                    (r['case'],words('input_u32_hex'),len(r['input_u32_hex']),
                     'true' if ret['kind']=='void' else 'false',ret.get('word_hex','00000000'),
                     words('output_u32_hex'),len(r['output_u32_hex'])))
    destination.write_text('// Generated from validated schema; original words preserved.\n'
        'struct GeometryFixtureRow { const char* name; unsigned in[65]; unsigned nin; bool returnsVoid; '
        'unsigned result; unsigned out[26]; unsigned nout; };\n'
        'static const GeometryFixtureRow geometryRows[]={\n'+',\n'.join(rows)+'\n};\n')

def main():
    p=argparse.ArgumentParser()
    sub=p.add_subparsers(dest='command',required=True)
    adapt=sub.add_parser('adapt')
    adapt.add_argument('--source',type=Path,required=True)
    adapt.add_argument('--output',type=Path,required=True)
    adapt.add_argument('--capture-id',required=True)
    adapt.add_argument('--reference-revision',required=True)
    emit=sub.add_parser('header')
    emit.add_argument('--source',type=Path,required=True)
    emit.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    if args.command=='header':
        header(json.loads(args.source.read_text()),args.output); return
    root=Path(__file__).resolve().parents[4]
    revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
    if revision!=args.reference_revision: p.error('explicit reference revision must match actual HEAD')
    if args.output.exists(): p.error('refusing to replace existing evidence destination')
    original=root/'tests/portable/fixtures/geometry-shipped-oracle.json'
    digest=hashlib.sha256(args.source.read_bytes()).hexdigest()
    manifest=json.loads((original.parent/'manifest.json').read_text())
    expected=next(f['sha256'] for f in manifest['fixtures'] if f['path']==original.name)
    if digest!=expected: p.error('source does not match immutable Task 1 oracle fixture hash')
    value=convert(json.loads(args.source.read_text()))
    value['schema_adaptation']={'capture_id':args.capture_id,'adapter_revision':revision,
        'adapter_dirty':bool(subprocess.check_output(['git','status','--porcelain'],cwd=root,text=True).strip()),
        'original_fixture':original.name,'original_sha256':digest,
        'reference_revision':manifest['reference_revision'],
        'oracle_identity':manifest['oracle'],
        'adapter_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'meaning':'lossless return-discriminator adaptation; no production code executed; no new oracle answers'}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with args.output.open('x') as out: out.write(json.dumps(value,indent=2)+'\n')
    print('adapted',len(value['records']),'records; preserved all input/output/return words')

if __name__=='__main__': main()
