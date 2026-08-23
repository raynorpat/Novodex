#!/usr/bin/env python3
"""Pin and verify the recursive Novodex Physics public-header tree."""

import argparse
import hashlib
import json
import sys
from pathlib import Path


SCHEMA_VERSION = 1
ALGORITHM = "SHA-256"


def _file_sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _scan(root):
    root = Path(root)
    if not root.is_dir():
        raise ValueError(f"{root} is not a directory")
    files = {}
    for path in root.rglob("*"):
        if path.is_file():
            files[path.relative_to(root).as_posix()] = path
    return files


def build_manifest(root) -> dict:
    """Hash every file under root, recording POSIX-relative paths in sorted order."""
    files = _scan(root)
    return {
        "schema_version": SCHEMA_VERSION,
        "algorithm": ALGORITHM,
        "root": Path(root).resolve().as_posix(),
        "file_count": len(files),
        "files": [
            {
                "path": relative,
                "size": files[relative].stat().st_size,
                "sha256": _file_sha256(files[relative]),
            }
            for relative in sorted(files)
        ],
    }


def verify_manifest(manifest: dict, root) -> list[str]:
    """Return every missing, unexpected, or byte-different header under root."""
    present = _scan(root)
    pinned = {entry["path"]: entry for entry in manifest["files"]}

    errors = []
    for relative in sorted(set(pinned) - set(present)):
        errors.append(f"missing header {relative}")
    for relative in sorted(set(present) - set(pinned)):
        errors.append(f"unexpected header {relative}")
    for relative in sorted(set(pinned) & set(present)):
        expected = pinned[relative]
        size = present[relative].stat().st_size
        if size != expected["size"]:
            errors.append(f"{relative} size {size} does not match pinned {expected['size']}")
            continue
        digest = _file_sha256(present[relative])
        if digest != expected["sha256"]:
            errors.append(f"{relative} sha256 {digest} does not match pinned {expected['sha256']}")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write-manifest", action="store_true", help="hash the tree into --output")
    parser.add_argument("--root", required=True, help="public-header root to scan recursively")
    parser.add_argument("--output", help="manifest to write with --write-manifest")
    parser.add_argument("--manifest", help="manifest to verify the tree against")
    args = parser.parse_args()

    if args.write_manifest and not args.output:
        parser.error("--write-manifest requires --output")
    if not args.write_manifest and not args.manifest:
        parser.error("--manifest is required without --write-manifest")

    try:
        if args.write_manifest:
            manifest = build_manifest(args.root)
            Path(args.output).write_text(
                json.dumps(manifest, indent=2) + "\n", encoding="utf-8",
                newline="\n"
            )
        else:
            manifest = json.loads(Path(args.manifest).read_text(encoding="utf-8"))
            errors = verify_manifest(manifest, args.root)
            if errors:
                for error in errors:
                    print(f"error: {error}", file=sys.stderr)
                return 1
    except (OSError, ValueError) as error:
        parser.exit(2, f"error: {error}\n")

    print("public_headers=written" if args.write_manifest else "public_headers=pass")
    print(f"files={manifest['file_count']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
