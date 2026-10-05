#!/usr/bin/env python3
"""Compare named PE export dumps."""

import argparse
from pathlib import Path


def read_names(path):
    names = set()
    for line in Path(path).read_text(encoding="utf-8-sig").splitlines():
        fields = line.split(maxsplit=1)
        if fields:
            names.add(fields[-1])
    return names


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--oracle", required=True)
    parser.add_argument("--rebuilt", required=True)
    args = parser.parse_args()

    try:
        oracle = read_names(args.oracle)
        rebuilt = read_names(args.rebuilt)
    except (OSError, UnicodeError) as error:
        parser.exit(2, f"error: {error}\n")
    missing = sorted(oracle - rebuilt)
    extra = sorted(rebuilt - oracle)

    print(f"oracle: {len(oracle)}")
    print(f"rebuilt: {len(rebuilt)}")
    print(f"missing: {len(missing)}")
    print(f"extra: {len(extra)}")
    for name in missing[:50]:
        print(f"MISSING {name}")
    for name in extra[:50]:
        print(f"EXTRA {name}")
    return 1 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
