import os
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

import tempfile
import unittest
from pathlib import Path
from PySide6.QtWidgets import QApplication
from yadavarifork.app import MainWindow
from yadavarifork.domain import ProjectStore

class AppSmokeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])

    def test_window_builds_and_has_four_steps(self):
        with tempfile.TemporaryDirectory() as tmp:
            window = MainWindow(ProjectStore(Path(tmp) / "db.sqlite3"))
            self.assertEqual(window.stack.count(), 4)
            self.assertEqual(window.stack.currentIndex(), 0)
            window.close()

if __name__ == "__main__":
    unittest.main()
