import tempfile
import unittest
from pathlib import Path

from yadavarifork.domain import PROPOSAL_SECTIONS, ProjectStore

class ProjectStoreTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.store = ProjectStore(Path(self.temp.name) / "db.sqlite3")

    def tearDown(self):
        self.temp.cleanup()

    def test_create_proposal_initializes_all_sections(self):
        project = self.store.create_project("آزمون", "fa", "no_api", "proposal")
        sections = self.store.load_sections(project.project_id)
        self.assertEqual(set(sections), {key for key, _ in PROPOSAL_SECTIONS})
        self.assertTrue(all(value == "" for value in sections.values()))

    def test_save_and_reload_individual_section(self):
        project = self.store.create_project("Test", "en", "no_api", "proposal")
        self.store.save_section(project.project_id, "problem_statement", "Evidence-based text")
        self.assertEqual(self.store.load_sections(project.project_id)["problem_statement"], "Evidence-based text")

    def test_reject_unknown_section(self):
        project = self.store.create_project("Test", "en", "no_api", "proposal")
        with self.assertRaises(ValueError):
            self.store.save_section(project.project_id, "invented", "x")

    def test_reject_empty_project_title(self):
        with self.assertRaises(ValueError):
            self.store.create_project("  ", "fa", "no_api", "proposal")

if __name__ == "__main__":
    unittest.main()
