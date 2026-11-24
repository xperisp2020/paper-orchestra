import json
import unittest

from scripts.generate_proposal import (
    ALLOWED_SCIENTIFIC_KEYS,
    PROTECTED_KEYS,
    generate_proposal_sections,
)


class ProposalGeneratorTests(unittest.TestCase):
    def payload(self):
        return {
            "language": "fa",
            "project_title": "ارزیابی اثر فناوری آموزشی بر یادگیری",
            "sections": [
                {"key": "title_fa", "label": "عنوان فارسی"},
                {"key": "title_en", "label": "English title"},
                {"key": "problem_statement", "label": "بیان مسئله"},
                {"key": "literature_review", "label": "پیشینه پژوهش"},
                {"key": "methodology", "label": "روش‌شناسی"},
                {"key": "references_fa", "label": "منابع فارسی"},
            ],
            "sources": [],
        }

    def test_generates_every_requested_section_in_batches(self):
        calls = []

        def fake_llm_call(**kwargs):
            calls.append(kwargs)
            prompt = kwargs["prompt"]
            marker = "Required keys (JSON): "
            keys_line = next(line for line in prompt.splitlines() if line.startswith(marker))
            keys = json.loads(keys_line[len(marker):])
            return {
                "parsed_response": {
                    "sections": {
                        key: "متن علمی کامل و منسجم برای بخش " + key
                        for key in keys
                    }
                }
            }

        result = generate_proposal_sections(self.payload(), "nvidia/nemotron:free", fake_llm_call)
        self.assertEqual(set(result), {item["key"] for item in self.payload()["sections"]})
        self.assertEqual(len(calls), 1)
        self.assertTrue(all(kwargs["return_json"] for kwargs in calls))

    def test_rejects_identity_and_administrative_fields(self):
        payload = self.payload()
        payload["sections"].append({"key": "student_name", "label": "نام دانشجو"})
        with self.assertRaisesRegex(ValueError, "Protected or unsupported"):
            generate_proposal_sections(payload, "model", lambda **_: {})

    def test_rejects_incomplete_model_response(self):
        payload = self.payload()

        def incomplete(**kwargs):
            return {"parsed_response": {"sections": {"title_fa": "عنوان"}}}

        with self.assertRaisesRegex(ValueError, "did not match"):
            generate_proposal_sections(payload, "model", incomplete)

    def test_protected_fields_are_not_scientific_generation_targets(self):
        self.assertIn("student_information", PROTECTED_KEYS)
        self.assertIn("archive_number", PROTECTED_KEYS)
        self.assertNotIn("student_information", ALLOWED_SCIENTIFIC_KEYS)
        self.assertNotIn("archive_number", ALLOWED_SCIENTIFIC_KEYS)

    def test_rejects_missing_bilingual_titles(self):
        payload = self.payload()
        payload["sections"] = [item for item in payload["sections"] if item["key"] != "title_en"]
        with self.assertRaisesRegex(ValueError, "Both Persian and English"):
            generate_proposal_sections(payload, "model", lambda **_: {})


if __name__ == "__main__":
    unittest.main()
