#!/usr/bin/env python3
"""Check the launcher icon assets and their Qt resource references."""
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
SVG = '{http://www.w3.org/2000/svg}'
ICONS = ('dashboard', 'library', 'downloads', 'mods', 'settings', 'play', 'refresh')


class AssetTests(unittest.TestCase):
    def test_icons_are_well_formed_and_use_a_consistent_canvas(self):
        for name in ICONS:
            with self.subTest(icon=name):
                svg = ET.parse(ROOT / 'assets' / 'icons' / f'{name}.svg').getroot()
                self.assertEqual(svg.tag, SVG + 'svg')
                self.assertEqual(svg.get('viewBox'), '0 0 24 24')
                self.assertEqual((svg.get('width'), svg.get('height')), ('24', '24'))
                self.assertEqual(svg.get('stroke'), 'currentColor')
                self.assertEqual(svg.get('stroke-width'), '1.8')
                self.assertTrue(any(node.tag in {SVG + 'path', SVG + 'circle'} for node in svg))
                self.assertFalse(any(node.tag in {SVG + 'script', SVG + 'image'} for node in svg))

    def test_qt_resources_keep_all_existing_icon_aliases(self):
        qrc = ET.parse(ROOT / 'resources' / 'cub3.qrc').getroot()
        aliases = {node.get('alias'): node.text for node in qrc.iter('file')}
        for name in (*ICONS, 'cub3'):
            with self.subTest(icon=name):
                self.assertEqual(aliases.get(f'icons/{name}.svg'), f'../assets/icons/{name}.svg')
                self.assertTrue((ROOT / 'resources' / aliases[f'icons/{name}.svg']).is_file())
        self.assertEqual(aliases.get('logos/cub3-primary.svg'), '../assets/logos/cub3-logo-01-island.svg')


if __name__ == '__main__':
    unittest.main()
