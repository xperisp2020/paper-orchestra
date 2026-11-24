import os
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

import tempfile
import unittest
from pathlib import Path
from PySide6.QtCore import Qt
from PySide6.QtWidgets import QApplication
from yadavarifork.app import MainWindow
from yadavarifork.domain import ProjectStore

class AppSmokeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])


    def test_language_switch_refreshes_controls_and_section_labels(self):
        with tempfile.TemporaryDirectory() as tmp:
            store = ProjectStore(Path(tmp) / "db.sqlite3")
            window = MainWindow(store)
            project = store.create_project("Test", "fa", "no_api", "proposal")
            window.project_id = project.project_id
            window._load_project_sections()
            window.language_combo.setCurrentIndex(1)
            window._choose_language()

            self.assertEqual(window.language, "en")
            self.assertEqual(window.ai_next_button.text(), "Continue")
            self.assertEqual(window.workflow_back_button.text(), "Back")
            self.assertEqual(window.title_form_label.text(), "Project title")
            self.assertEqual(window.section_list.item(0).text(), "Persian title")
            self.assertEqual(window.layoutDirection(), Qt.LayoutDirection.LeftToRight)
            window.close()

    def test_back_navigation_returns_to_previous_wizard_page(self):
        with tempfile.TemporaryDirectory() as tmp:
            window = MainWindow(ProjectStore(Path(tmp) / "db.sqlite3"))
            window.stack.setCurrentIndex(1)
            window.ai_back_button.click()
            self.assertEqual(window.stack.currentIndex(), 0)
            window.stack.setCurrentIndex(2)
            window.workflow_back_button.click()
            self.assertEqual(window.stack.currentIndex(), 1)
            window.close()

    def test_window_builds_and_has_four_steps(self):
        with tempfile.TemporaryDirectory() as tmp:
            window = MainWindow(ProjectStore(Path(tmp) / "db.sqlite3"))
            self.assertEqual(window.stack.count(), 4)
            self.assertEqual(window.stack.currentIndex(), 0)
            window.close()

if __name__ == "__main__":
    unittest.main()
