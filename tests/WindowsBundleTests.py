#!/usr/bin/env python3
"""Exercise Windows portable ZIP validation and website staging."""
from hashlib import sha256
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from stage_windows_bundle import (  # noqa: E402
    ARCHIVE_NAME,
    CHECKSUM_NAME,
    stage_windows_bundle,
)


class WindowsBundleTests(unittest.TestCase):
    def make_bundle(self, source: Path) -> Path:
        source.mkdir(parents=True, exist_ok=True)
        archive = source / ARCHIVE_NAME
        with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED) as bundle:
            bundle.writestr('cub3.exe', b'portable launcher test fixture')
        digest = sha256(archive.read_bytes()).hexdigest()
        (source / CHECKSUM_NAME).write_text(
            f'{digest}  {ARCHIVE_NAME}\r\n', encoding='ascii')
        return archive

    def test_verifies_and_stages_zip_and_windows_sidecar(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / 'artifact'
            destination = root / 'site' / 'downloads'
            archive = self.make_bundle(source)

            stage_windows_bundle(source, destination)

            staged = destination / ARCHIVE_NAME
            self.assertEqual(staged.read_bytes(), archive.read_bytes())
            self.assertEqual((destination / CHECKSUM_NAME).read_bytes(),
                             (source / CHECKSUM_NAME).read_bytes())
            self.assertEqual(sha256(staged.read_bytes()).hexdigest(),
                             (destination / CHECKSUM_NAME).read_text().split()[0])

    def test_rejects_a_checksum_mismatch_without_publishing_files(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / 'artifact'
            self.make_bundle(source)
            (source / CHECKSUM_NAME).write_text(f'{"0" * 64}  {ARCHIVE_NAME}\n')
            destination = root / 'site' / 'downloads'

            with self.assertRaisesRegex(ValueError, 'failed SHA-256 verification'):
                stage_windows_bundle(source, destination)
            self.assertFalse((destination / ARCHIVE_NAME).exists())

    def test_rejects_an_invalid_zip_without_publishing_files(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / 'artifact'
            source.mkdir()
            archive = source / ARCHIVE_NAME
            archive.write_bytes(b'not a zip')
            digest = sha256(archive.read_bytes()).hexdigest()
            (source / CHECKSUM_NAME).write_text(f'{digest}  {ARCHIVE_NAME}\n')
            destination = root / 'site' / 'downloads'

            with self.assertRaisesRegex(ValueError, 'not a valid ZIP archive'):
                stage_windows_bundle(source, destination)
            self.assertFalse((destination / ARCHIVE_NAME).exists())


if __name__ == '__main__':
    unittest.main()
