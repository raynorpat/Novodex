def main(path):
    parts = []
    total = 0
    for sym, lo, hi, rows, decl in ROWS:
        body, n = gen(lo, hi)
        total += n
        parts.append('// %s (0x%08x, %d B), %d instructions\n%s\n\t{\n\t__asm\n\t\t{\n%s\n\t\t}\n\t}\n' % (rows, lo, hi - lo, n, decl, body))
    consts = ''
    for k, (ad, n, v) in sorted(KSEEN.items()):
        consts += 'static const unsigned %s%s = 0x%x%s;\n' % ('long long ' if n == 8 else '', k, v, 'ull' if n == 8 else '')
    for va_, nm_ in STRINGS.items():
        d_ = f[off(va_ - 0x10000000):off(va_ - 0x10000000) + 400]
        txt_ = d_[:d_.index(b'\0')].decode('latin1')
        consts += 'static const char %s[] = %s;\n' % (nm_, json.dumps(txt_))
    open(path, 'w').write(consts + '\n' + '\n'.join(parts))
    print(total, 'instructions in', len(ROWS), 'rows')


if __name__ == '__main__':
    main(sys.argv[1])
