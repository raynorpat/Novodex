import subprocess, re, sys, os, time
REPO = r'D:\github\Novodex'
SRC = REPO + r'\Physics\src\ObjectModel.cpp'; TGT = 'NxPhysicsCollisionTests'
pat = r'name=mesh_mass .*? mismatches=(\d+)'
muts = [(0x54bd8,'jnp L54e52'),(0x54bc1,'fld dword ptr [edi + 0xb4]'),(0x54bde,'mov ecx, dword ptr [edi + 0x8]'),(0x54c05,'mov eax, dword ptr [edi + 0x10]'),(0x54c0c,'push 0x40000000'),(0x54c29,'jne L54e5b'),(0x54c4b,'mov dword ptr [edi + 0xdc], eax'),(0x54c6d,'test eax, 0x206'),(0x54de1,'jnp L54e50'),(0x54e1d,'fstp dword ptr [esi + 0xc]'),(0x54e0e,'fstp dword ptr [esi + 4]'),(0x54dec,'push 0x2f8'),(0x54df6,'push 0xcf'),(0x28e16,'test byte ptr [esi + 0xde], 3'),(0x28e2e,'mov ecx, dword ptr [eax + 4]'),(0x28e31,'lea edx, [eax + 0x2c]'),(0x28e42,'add esi, 0x70'),(0x28e2c,'jne L28e6f'),(0x28e66,'mov al, 0'),(0x1c5ae,'add edi, 0x28'),(0x538ea,'mov ecx, 2'),(0x538e4,'lea eax, [ecx + 0x34]')]
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
                '4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c', '--mesh-mass-only'], capture_output=True, text=True, cwd=REPO, timeout=240)
            rc = r.returncode
        except subprocess.TimeoutExpired:
            print(hex(addr), new, 'TIMEOUT (detected)', flush=True); continue
    finally:
        open(SRC, 'w', encoding='utf-8', newline='').write(orig)
    out = r.stdout + r.stderr
    m = re.search(pat, out); res = 'mismatches %s' % (m.group(1) if m else '?')
    print(hex(addr), new, 'exit', rc, res, flush=True)
