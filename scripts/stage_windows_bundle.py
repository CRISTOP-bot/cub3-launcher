#!/usr/bin/env python3
"""Verify and stage the portable Windows build and its SHA-256 sidecar."""
from __future__ import annotations

import argparse
from hashlib import sha256
from pathlib import Path
import re
import shutil
import zipfile

ARCHIVE_NAME = 'cub3-launcher-windows-x64.zip'
CHECKSUM_NAME = ARCHIVE_NAME + '.sha256'


def stage_windows_bundle(source: Path, destination: Path) -> None:
    archive = source / ARCHIVE_NAME
    sidecar = source / CHECKSUM_NAME
    if not archive.is_file() or archive.stat().st_size == 0:
        raise ValueError('The Windows portable ZIP is missing or empty')
    if not zipfile.is_zipfile(archive):
        raise ValueError('The Windows portable build is not a valid ZIP archive')
    with zipfile.ZipFile(archive) as bundle:
        bad_member = bundle.testzip()
        if bad_member is not None:
            raise ValueError(f'The Windows ZIP contains a corrupt member: {bad_member}')

    if not sidecar.is_file():
        raise ValueError('The Windows ZIP SHA-256 sidecar is missing')
    fields = sidecar.read_text(encoding='ascii').strip().split()
    if (len(fields) != 2 or fields[1] != ARCHIVE_NAME
            or re.fullmatch(r'[0-9a-fA-F]{64}', fields[0]) is None):
        raise ValueError('The Windows ZIP SHA-256 sidecar has an unexpected format')
    digest = sha256(archive.read_bytes()).hexdigest()
    if digest.lower() != fields[0].lower():
        raise ValueError('The Windows portable ZIP failed SHA-256 verification')

    destination.mkdir(parents=True, exist_ok=True)
    shutil.copy2(archive, destination / ARCHIVE_NAME)
    shutil.copy2(sidecar, destination / CHECKSUM_NAME)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path, help='directory with the downloaded CI artifact files')
    parser.add_argument('destination', type=Path, help='site downloads directory')
    args = parser.parse_args()
    try:
        stage_windows_bundle(args.source, args.destination)
    except (OSError, UnicodeError, ValueError, zipfile.BadZipFile) as error:
        parser.error(str(error))
    print(f'Verified and staged {ARCHIVE_NAME} and {CHECKSUM_NAME}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
