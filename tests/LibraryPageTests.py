#!/usr/bin/env python3
"""Contract checks for the installed-instance UI and safe local actions."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class LibraryPageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = (ROOT / 'src/ui/pages/LibraryPage.cpp').read_text(encoding='utf-8')
        cls.header = (ROOT / 'src/ui/pages/LibraryPage.h').read_text(encoding='utf-8')
        cls.model = (ROOT / 'src/ui/pages/LibraryModel.h').read_text(encoding='utf-8')
        cls.theme = (ROOT / 'src/ui/styles/Theme.cpp').read_text(encoding='utf-8')

    def test_search_and_sort_are_exposed_and_accessible(self):
        self.assertIn('Buscar nombre, versión o cargador', self.source)
        self.assertIn('setClearButtonEnabled(true)', self.source)
        self.assertIn('InstallationSort::RecentlyAdded', self.source)
        self.assertIn('InstallationSort::Name', self.source)
        self.assertIn('InstallationSort::Version', self.source)
        self.assertIn('itemSelectionChanged', self.source)
        self.assertIn('filterAndSortInstallations(m_installations', self.source)
        self.assertIn('setAccessibleName(QStringLiteral("Buscar instalaciones', self.source)
        self.assertIn('QLineEdit#LibrarySearchField', self.theme)
        self.assertIn('QComboBox#LibrarySortSelector', self.theme)

    def test_search_and_sort_logic_is_case_insensitive_and_natural(self):
        self.assertIn('Qt::CaseInsensitive', self.model)
        self.assertIn('naturalCompare', self.model)
        self.assertIn('a.isDigit() && b.isDigit()', self.model)
        self.assertNotIn('QCollator', self.model)
        self.assertIn('std::stable_sort', self.model)
        self.assertIn('installation.version.contains', self.model)
        self.assertIn('installation.loader.contains', self.model)
        self.assertIn('left.createdAt > right.createdAt', self.model)

    def test_folder_action_opens_only_an_existing_local_directory(self):
        self.assertIn('QDesktopServices::openUrl(QUrl::fromLocalFile', self.source)
        self.assertIn('info.isDir()', self.source)
        self.assertIn('QFileInfo(item->data(Qt::UserRole + 1).toString()).isDir()', self.source)
        self.assertNotIn('QFile::remove', self.source)
        self.assertNotIn('QDir().removeRecursively', self.source)

    def test_query_reads_the_existing_schema_without_migration(self):
        self.assertIn('SELECT id, name, version, loader, directory, created_at', self.source)
        self.assertIn('FROM installations', self.source)
        self.assertNotIn('CREATE TABLE', self.source)
        self.assertNotIn('ALTER TABLE', self.source)

    def test_empty_state_and_new_profile_status_remain_honest(self):
        self.assertIn('newButton->setEnabled(false)', self.source)
        self.assertIn('La creación de perfiles aún no está disponible.', self.source)
        self.assertIn('No encontramos instalaciones que coincidan.', self.source)
        self.assertIn('Aún no tienes instalaciones.', self.source)

    def test_cmake_discovers_a_real_qt_unit_test_for_the_model(self):
        test = ROOT / 'tests/LibraryModelTests.cpp'
        self.assertTrue(test.is_file())
        self.assertIn('QTEST_GUILESS_MAIN(LibraryModelTests)', test.read_text(encoding='utf-8'))
        cmake = (ROOT / 'tests/CMakeLists.txt').read_text(encoding='utf-8')
        self.assertRegex(cmake, r'file\(GLOB CUB3_TEST_SOURCES\s+"\$\{CMAKE_CURRENT_SOURCE_DIR\}/\*\.cpp"')

    def test_workflow_runs_page_contract_checks(self):
        workflow = (ROOT / '.github/workflows/build.yml').read_text(encoding='utf-8')
        self.assertIn('python3 tests/LibraryPageTests.py', workflow)


if __name__ == '__main__':
    unittest.main()
