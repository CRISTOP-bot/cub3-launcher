#!/usr/bin/env python3
"""Guard the Windows CI cache and validation sequence without building Qt."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class WindowsWorkflowTests(unittest.TestCase):
    def test_cache_is_saved_after_install_before_build(self):
        workflow = (ROOT / '.github/workflows/build.yml').read_text(encoding='utf-8')
        windows = workflow.split('  windows-release:', 1)[1]
        names = (
            'Restore Windows vcpkg installation',
            'Install Windows dependencies with vcpkg',
            'Save Windows vcpkg installation',
            'Configure Windows CMake',
            'Build Windows Release',
            'Run Windows tests',
            'Prepare portable Windows bundle',
            'Upload portable Windows build',
        )
        positions = [windows.index('name: ' + name) for name in names]
        self.assertEqual(positions, sorted(positions))
        self.assertIn('actions/cache/restore@v4', windows)
        self.assertIn('actions/cache/save@v4', windows)
        self.assertIn("hashFiles('vcpkg.json')", windows)
        self.assertIn('--x-manifest-root=$PWD', windows)
        self.assertIn('--x-install-root=$installedDir', windows)
        self.assertIn('-DVCPKG_MANIFEST_INSTALL=OFF', windows)
        self.assertIn('timeout-minutes: 120', windows)
        self.assertIn("if ($LASTEXITCODE -ne 0) { throw 'Windows vcpkg dependency installation failed' }", windows)
        self.assertIn('ctest --test-dir build -C Release --output-on-failure', windows)


if __name__ == '__main__':
    unittest.main()
