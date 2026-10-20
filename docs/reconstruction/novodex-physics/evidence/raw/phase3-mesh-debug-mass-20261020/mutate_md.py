import subprocess, re, sys, os, time
REPO = r'D:\github\Novodex'
SRC = REPO + r'\Physics\src\ObjectModel.cpp'; TGT = 'NxPhysicsCollisionTests'
pat = r'name=mesh_debug .*? mismatches=(\d+)'
muts = [(0x27f22,'jne L28e01'),(0x27f4b,'jp L28e00'),(0x27f89,'cmp dword ptr [ebx + 0x28], ecx'),(0x28000,'mov ecx, dword ptr [ebx + 0x14]'),
 (0x28022,'fmul dword ptr [esi + 0x10]'),(0x28104,'push 0xff8041'),(0x281b0,'fdiv dword ptr [nxMDK101041ec_4]'),(0x2825b,'push 0x4080fe'),
 (0x28316,'mov eax, dword ptr [ebx + 0x3c]'),(0x28460,'push 0xff00fe00'),(0x28561,'jb L2878f'),(0x28709,'jne L28716'),
 (0x2870f,'mov bl, byte ptr [eax + edx + 8]'),(0x28716,'test bl, 2'),(0x2879d,'je L287a6'),(0x28961,'mov eax, dword ptr [edi + 4]'),
 (0x289f8,'je L28a01'),(0x28b16,'push 0x2266ab'),(0x28b73,'mov edi, dword ptr [eax + 0x20]'),(0x28d23,'fmul dword ptr [nxMDK101043cc_4]'),(0x28d83,'push 0xaa6623')]
which = 'md'
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
                '4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c', '--mesh-debug-only'], capture_output=True, text=True, cwd=REPO, timeout=240)
            rc = r.returncode
        except subprocess.TimeoutExpired:
            print(hex(addr), new, 'TIMEOUT (detected)', flush=True); continue
    finally:
        open(SRC, 'w', encoding='utf-8', newline='').write(orig)
    out = r.stdout + r.stderr
    m = re.search(pat, out); res = 'mismatches %s' % (m.group(1) if m else '?')
    print(hex(addr), new, 'exit', rc, res, flush=True)
