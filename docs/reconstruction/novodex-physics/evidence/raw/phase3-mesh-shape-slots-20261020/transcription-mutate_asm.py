import subprocess, re, sys, os, time
REPO = r'D:\github\Novodex'
which = sys.argv[1]
if which == 'ray':
    SRC = REPO + r'\Physics\src\ShapeRaycast.cpp'; TGT = 'NxPhysicsCollisionTests'
    pat = r'name=shape_raycast_mesh .*? mismatches=(\d+)'
    muts = [(0x2927b,'add ecx, 0x454'),(0x29340,'test dword ptr [esp + 0x34], edi'),(0x293c1,'fadd dword ptr [esp + 0x14]'),
            (0x2948e,'jne 0x1002949b'),(0x294ba,'mov dword ptr [edi + 0x2c], 0x3a'),(0x294ac,'test byte ptr [esp + 0x9c], 8'),
            (0x2945b,'fadd dword ptr [esi + 0x34]'),(0x29298,'mov eax, dword ptr [edx + 0x74]'),(0x29365,'je 0x1002937c'),
            (0x29496,'mov eax, dword ptr [eax + edx*4 + 4]'),(0x2933e,'jne 0x10029367')]
else:
    SRC = REPO + r'\Physics\src\ObjectModel.cpp'; TGT = 'NxPhysicsShapeVtableTests'
    pat = r'mesh slot 7 prepared=\d sample=\d+ differs'
    muts = [(0x2963d,'fsub dword ptr [esi + 0x30]'),(0x296af,'movzx ebx, byte ptr [eax + edx + 1]'),(0x296ac,'mov edx, dword ptr [edi + 0x10]'),
            (0x29737,'mov ecx, dword ptr [eax + 0x6c]'),(0x296c4,'je 0x100296cd'),(0x2971b,'add ecx, 0x10'),(0x29625,'jne 0x10029737'),(0x2966a,'fmul dword ptr [esi + 0x14]')]
orig = open(SRC, encoding='utf-8', newline='').read()
for addr, new in muts:
    rx = re.compile(r'^(\t\t)[^\r\n]*?(\t\t// 0x%08x)(\r?)$' % addr, re.M)
    assert len(rx.findall(orig)) == 1, hex(addr)
    mut = rx.sub(lambda m: m.group(1) + new + m.group(2) + m.group(3), orig)
    open(SRC, 'w', encoding='utf-8', newline='').write(mut)
    try:
        r = subprocess.run('cmake --build build --config Release --target ' + TGT, shell=True, capture_output=True, text=True, cwd=REPO)
        if ': error' in r.stdout:
            print(hex(addr), new, 'BUILD FAIL', [x for x in r.stdout.splitlines() if ': error' in x][:1], flush=True); continue
        try:
            r = subprocess.run(['D:/github/Novodex/build/Release/' + TGT + '.exe', 'D:/FlamingEnt__/Unreal_3/Binaries',
                '4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c'], capture_output=True, text=True, cwd=REPO, timeout=240)
            rc = r.returncode
        except subprocess.TimeoutExpired:
            print(hex(addr), new, 'TIMEOUT (detected)', flush=True); continue
    finally:
        open(SRC, 'w', encoding='utf-8', newline='').write(orig)
    out = r.stdout + r.stderr
    if which == 'ray':
        m = re.search(pat, out); res = 'mismatches %s' % (m.group(1) if m else '?')
    else:
        res = 'slot7_failures %d' % len(re.findall(pat, out))
    print(hex(addr), new, 'exit', rc, res, flush=True)
