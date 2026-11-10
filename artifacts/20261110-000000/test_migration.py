"""Regression checks for the one-time migration; no dependencies required."""
import runpy
import unittest
from datetime import date
from pathlib import Path

migration = runpy.run_path(str(Path(__file__).with_name('migrate.py')))


class MigrationTests(unittest.TestCase):
    def test_iso_section_date(self) -> None:
        match = migration['DATE_HEADING'].match('## 2026-09-24 (user drunk; review sober)')
        self.assertIsNotNone(match)
        self.assertEqual(migration['parse_day'](match[1]), date(2026, 9, 24))

    def test_selected_source_lines_are_not_decisions(self) -> None:
        self.assertNotIn('decision', migration['kinds']('Selected source lines: 12–18.', ''))

    def test_explicit_decision_keeps_its_kind(self) -> None:
        self.assertIn('decision', migration['kinds']('Decision (user): use this model.', ''))
        self.assertIn('proposal', migration['kinds']('Proposed name (agent): value model.', ''))
        self.assertIn('evidence', migration['kinds']('Evidence (agent): the test passed.', ''))

    def test_document_collection_date(self) -> None:
        source = migration['Source'](
            {'source': 'tasks/20260529-103914/apply-evidence.md', 'matches_HEAD': False, 'git_history': []},
            b'# Evidence\n\nGathered 2026-09-25 from tasks and docs.\n',
            'artifacts/20260529-103914/apply-evidence.md',
        )
        self.assertEqual(migration['date_basis'](source)[0], date(2026, 9, 25))


if __name__ == '__main__':
    unittest.main()
