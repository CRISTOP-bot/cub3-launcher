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
        self.assertIn('restore-keys:', windows)
        self.assertIn('cub3-vcpkg-installed-windows-2022-x64-v2-', windows)
        self.assertIn('actions/cache/save@v4', windows)
        self.assertIn("hashFiles('vcpkg.json')", windows)
        self.assertIn('--x-manifest-root=$PWD', windows)
        self.assertIn('--x-install-root=$installedDir', windows)
        self.assertIn('-DVCPKG_MANIFEST_INSTALL=OFF', windows)
        self.assertIn('timeout-minutes: 120', windows)
        self.assertIn("if ($LASTEXITCODE -ne 0) { throw 'Windows vcpkg dependency installation failed' }", windows)
        self.assertIn('ctest --test-dir build -C Release --output-on-failure', windows)

    def test_registry_and_manifest_are_pinned_to_the_same_verified_baseline(self):
        import json
        import re

        workflow = (ROOT / '.github/workflows/build.yml').read_text(encoding='utf-8')
        windows = workflow.split('  windows-release:', 1)[1]
        ref = re.search(r'repository: microsoft/vcpkg\s+ref: ([0-9a-f]{40})', windows)
        self.assertIsNotNone(ref, 'Windows must checkout an immutable vcpkg commit')
        manifest = json.loads((ROOT / 'vcpkg.json').read_text(encoding='utf-8'))
        self.assertEqual(manifest['builtin-baseline'], ref.group(1))
        # This vcpkg baseline downloads PCRE2's pinned sljit archive directly;
        # the newer port calls api.github.com/contents and fails on runner 403.
        self.assertEqual(ref.group(1), '2d82b86eeb840b04d593405a968f8aa03609681d')

    def test_zlib_is_installed_and_staged_for_the_native_region_reader(self):
        import json

        workflow = (ROOT / '.github/workflows/build.yml').read_text(encoding='utf-8')
        manifest = json.loads((ROOT / 'vcpkg.json').read_text(encoding='utf-8'))
        self.assertIn('zlib', manifest['dependencies'])
        self.assertIn('z.dll', workflow)
        self.assertIn('zlib1.dll', workflow)
        self.assertIn('zlib runtime DLL (z.dll or zlib1.dll)', workflow)
        self.assertIn('zlib1g-dev', workflow.split('Install build dependencies', 1)[1])
        self.assertIn('ZLIB::ZLIB', (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8'))
        self.assertIn('ZLIB::ZLIB', (ROOT / 'tests/CMakeLists.txt').read_text(encoding='utf-8'))

    def test_write_access_and_fork_contribution_rules_are_distinct(self):
        contributing = (ROOT / 'CONTRIBUTING.md').read_text(encoding='utf-8').lower()
        normalized = ' '.join(contributing.split())
        self.assertIn('personas con permiso de escritura en el repositorio pueden hacer commits', normalized)
        self.assertIn('quienes no tienen ese acceso deben contribuir desde un fork', normalized)
        self.assertIn('los cambios se integran mediante pr dirigido a `dev`', normalized)
        self.assertIn('no hagas push directo a `dev` ni a `main`', normalized)
        self.assertIn('el pr debe pasar la revisión y las validaciones requeridas', normalized)


if __name__ == '__main__':
    unittest.main()
