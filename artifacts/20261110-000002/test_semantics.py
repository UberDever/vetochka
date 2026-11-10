"""Regression checks for semantic losses found during WAL compaction review."""
from pathlib import Path
import re
import unittest

HERE = Path(__file__).resolve().parent


def entries() -> dict[str, str]:
    result = {}
    day = ''
    index = 0
    for line in (HERE / 'candidate.md').read_text(encoding='utf-8').splitlines():
        if line.startswith('### '):
            day, index = line[4:], 0
        elif line.startswith('- '):
            result[f'{day},{index}'] = line
            index += 1
    return result


class ReplaySemantics(unittest.TestCase):
    def test_reconstruction_contract_precedes_binder_exception(self) -> None:
        history = entries()
        contract = [(key, text) for key, text in history.items() if key.startswith('05.08.26,') and 'same node' in text]
        self.assertEqual(len(contract), 1, 'The reconstruction contract must be in the WAL, not only an artifact.')
        depth = [text for key, text in history.items() if key.startswith('24.09.26,') and 'one layer' in text]
        self.assertEqual(len(depth), 1)
        self.assertIn('own pith', depth[0])
        boundary = next(key for key, text in history.items() if 'Binder-boundary.' in text)
        self.assertLess(list(history).index(contract[0][0]), list(history).index(boundary))

    def test_rule3_supersession_does_not_remove_calculus_policy(self) -> None:
        history = entries()
        current = next(text for text in history.values() if 'Non-evaluating-Rule3.' in text)
        target = re.search(r'Supersession for \(([^,]+), (\d+)\)', current)
        previous = history[f'{target[1]},{target[2]}']
        self.assertNotIn('rules 0..3', previous)
        self.assertNotIn('branch-first', previous)
        self.assertTrue(any('rules 0..3' in text and 'branch-first' in text for text in history.values()))

    def test_influences_remain_pending(self) -> None:
        history = entries()
        task = [text for text in history.values() if text.startswith('- [ ] ') and all(word in text for word in ('triage', 'lambda', 'C', 'ML', 'Scheme')) and 'influence' in text.lower()]
        self.assertEqual(len(task), 1)
        self.assertIn('source', task[0])
        self.assertIn('level', task[0])

    def test_bytes_supersession_retains_list_notation(self) -> None:
        history = entries()
        current = next(text for text in history.values() if 'Substrate.' in text)
        target = re.search(r'Supersession for \(([^,]+), (\d+)\)', current)
        previous = history[f'{target[1]},{target[2]}']
        self.assertNotIn('Bare `~`', previous)
        self.assertNotIn('Proper lists', previous)
        self.assertTrue(any('right-nested' in text and 'ending in `~`' in text and '#superseded' not in text for text in history.values()))


if __name__ == '__main__':
    unittest.main()
