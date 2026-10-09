import pathlib,re,json,hashlib
root=pathlib.Path.cwd()
paths=sorted(p for base in ['Physics','Foundation','External'] for p in (root/base).rglob('*') if p.suffix in ('.h','.cpp','.c','.inl'))
def mask(s):
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',lambda m:re.sub(r'[^\n]',' ',m[0]),s,flags=re.S)
source={p.relative_to(root).as_posix():p.read_text(errors='replace') for p in paths}
clean={p:mask(s) for p,s in source.items()}
functions={}
for p,s in clean.items():
    # Keep offsets/newlines while preventing __declspec(...) and #if defined(...)
    # from consuming the actual signature. Dependencies are scanned in s below.
    signatures=re.sub(r'__declspec\s*\([^)]*\)|(?m:^\s*#[^\n]*)',
                      lambda m:re.sub(r'[^\n]',' ',m[0]),s)
    found=[]
    for m in re.finditer(r'([A-Za-z_~][\w:~]*)\s*\([^;{}]*?\)\s*(?:const\s*)?\{',signatures):
        if m[1] in ('if','for','while','switch','catch','__declspec','sizeof','defined'):continue
        start=m.end()-1; depth=1;end=start+1
        while end<len(s) and depth:
            if s[end]=='{':depth+=1
            if s[end]=='}':depth-=1
            end+=1
        declaration_start=max(signatures.rfind(c,0,m.start()) for c in ';{}')+1
        found.append((declaration_start,end,m[1],s[:m.start()].count('\n')+1))
    functions[p]=found
token=re.compile(r'\b(?:__asm|asm|__fastcall|__stdcall|__thiscall|_controlfp(?:_s)?|_control87|_FPU_GETCW|_FPU_SETCW)\b|__declspec\s*\(\s*naked\s*\)|\b(?:fnstcw|fstcw|fldcw|fistp|fisttp|rdtsc)\b')
files=[]
def assembly_text(body):
    """Extract MSVC blocks/line instructions, excluding ordinary C++ statements."""
    spans=[]
    for m in re.finditer(r'\b__asm\b',body):
        start=m.end()
        while start<len(body) and body[start].isspace():start+=1
        if start<len(body) and body[start]=='{':
            end=start+1;depth=1
            while end<len(body) and depth:
                if body[end]=='{':depth+=1
                if body[end]=='}':depth-=1
                end+=1
            spans.append(body[start+1:end-1])
        else:
            end=body.find('\n',start)
            spans.append(body[start:] if end<0 else body[start:end])
    return '\n'.join(spans)
def family(p):
    if 'External/' in p:return 'vendor-platform (Task 9; consumers Tasks 5-8)'
    if 'Foundation/' in p:return 'foundation-platform (Tasks 7/9)'
    if 'X87Sqrt' in p or 'JointAcos' in p:return 'shared-math (Task 3)'
    if '/core/' in p or any(x in p for x in ('Body','NpActor','Scene.cpp','SceneDump')):return 'dynamics-environment (Task 7)'
    if any(x in p for x in ('TriangleMeshPolygons','IceSupportMaps','ConvexHull','IceMesh','EdgeList','ContactConvex')):return 'mesh-support-convex (Task 5)'
    if any(x in p for x in ('Contact','NarrowPhase')):return 'contact-dispatch (Task 6)'
    if 'Quantizer' in p:return 'conversion (Task 3/5)'
    return 'geometry-ABI (Tasks 4/9)'
for p,s in clean.items():
    hits=list(token.finditer(s))
    if not hits:continue
    entries={}
    for m in hits:
        candidates=[f for f in functions[p] if f[0]<=m.start()<f[1]]
        f=min(candidates,key=lambda f:f[1]-f[0]) if candidates else None
        name=f[2] if f else '<declaration-or-file-scope>'
        key=(name,f[3] if f else s[:m.start()].count('\n')+1)
        e=entries.setdefault(key,{'symbol':name,'definition_line':key[1],'sites':[]})
        e['sites'].append({'line':s[:m.start()].count('\n')+1,'kind':m[0],'source_line':source[p].splitlines()[s[:m.start()].count('\n')].strip()})
        if f:
            body=assembly_text(s[f[0]:f[1]])
            e['resolution']='function-body'
            e['instructions']=sorted(set(re.findall(r'(?m)^\s*(?:\w+:\s*)?(f\w+|call|jmp|ret|rdtsc|_emit)\b',body)))
            e['assembly_call_targets']=sorted(set(t.strip() for t in re.findall(r'(?m)^\s*(?:call|jmp)\s+([^\n]+)',body)))
            e['continuation_labels']=re.findall(r'(?m)^\s*(\w+):',body)
            if '_emit' in e['instructions']:
                e['raw_emitted_bytes']=[int(v,0) for v in re.findall(r'\b_emit\s+(0x[0-9a-fA-F]+|\d+)',body)]
                e['raw_byte_decode_required']=True
        else:
            e['resolution']='declaration-macro-or-unresolved'
            e['unresolved_reason']='No enclosing function signature parsed; inspect the exact source sites and conditional declarations before conversion.'
    for e in entries.values():
        if e['symbol'].startswith('<'):continue
        pat=re.compile(r'\b'+re.escape(e['symbol'])+r'\b')
        refs=[]
        for q,t in clean.items():
            for m in pat.finditer(t):
                line=t[:m.start()].count('\n')+1
                if q==p and line==e['definition_line']:continue
                refs.append(f'{q}:{line}')
        e['source_reference_sites']=sorted(set(refs))
    active='source/header'
    if '/upstream/' in p:
        rel=p.split('/upstream/',1)[1]
        if p.startswith('External/opcode/'):rel=rel.removeprefix('Opcode/')
        overlay=root/('/'.join(p.split('/')[:2]))/'novodex'/rel
        if overlay.exists():active='shadowed by '+overlay.relative_to(root).as_posix()
        else:active='upstream effective tree (reachability follows External/CMakeLists.txt)'
    files.append({'path':p,'sha256':hashlib.sha256((root/p).read_bytes()).hexdigest(),'family':family(p),'selection':active,'entries':list(entries.values()),'conditional_lines':[{'line':i+1,'text':l.strip()} for i,l in enumerate(source[p].splitlines()) if re.match(r'\s*#\s*(if|else|elif)',l)]})
out=root/'docs/reconstruction/novodex-physics/evidence/portable-scalar-dependencies.json'
unresolved=[{'path':f['path'],'line':e['definition_line'],'sites':e['sites'],'reason':e['unresolved_reason']} for f in files for e in f['entries'] if e['resolution']!='function-body']
out.write_text(json.dumps({'schema_version':1,'revision':'a4838ddf282ee859471e1526145b616de5ddedb6','method':'comment/string-masked whole-tree token census; annotations/preprocessor masked for signature matching; instruction/edge/label extraction limited to assembly spans; symbols and references are source observations, not a compiler call graph; indirect closure is governed by the accompanying inventory contracts','unresolved_candidates':unresolved,'files':files},indent=2)+'\n')
table=['','## Exact source census','',f'{len(files)} files contain executable assembly, ABI declarations or FPU operations. The companion JSON retains every site, containing symbol, source references, assembly call/tail-jump targets, continuation labels and conditionals. Shadowed vendor files are explicitly marked; public headers are inventoried read-only.','', '| File | Owner / order | Symbols with dependencies |','|---|---|---|']
for f in files:
    symbols=', '.join('`'+e['symbol']+'` L'+str(e['definition_line']) for e in f['entries'])
    table.append(f"| `{f['path']}` | {f['family']} | {symbols} |")
(root/'build/task1-evidence').mkdir(parents=True,exist_ok=True)
(root/'build/task1-evidence/census.md').write_text('\n'.join(table)+'\n')
print('inventory_files',len(files),'entries',sum(len(f['entries']) for f in files),'unresolved_candidates',len(unresolved))
