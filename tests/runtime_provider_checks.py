"""Integration checks for provider routing; run in the full research runtime."""
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from utils.llm_backend_utils import is_openai_model
from utils.scholar_utils import s2_keyword_search


class ProviderRoutingIntegrationTests(unittest.TestCase):
    def test_openai_model_family_detection(self):
        for name in ("gpt-4o", "o1-preview", "o3-mini", "o4-mini"):
            self.assertTrue(is_openai_model(name), name)
        for name in ("gemini-3.1-pro-preview", "claude-sonnet", ""):
            self.assertFalse(is_openai_model(name), name)

    def test_semantic_scholar_keyword_search_filters_cutoff(self):
        class Response:
            status_code = 200

            @staticmethod
            def json():
                return {"data": [
                    {"title": "Paper before cutoff", "year": 2024, "publicationDate": "2024-10-15"},
                    {"title": "Paper after cutoff", "year": 2024, "publicationDate": "2024-11-02"},
                    {"title": "Paper without date", "year": 2022},
                ]}

        with patch("utils.scholar_utils.requests.get", return_value=Response()) as request:
            results = s2_keyword_search("graph neural networks", limit=15, cutoff_date="2024-11")
        self.assertEqual([item["title"] for item in results], [
            "Paper before cutoff", "Paper without date"
        ])
        self.assertEqual(request.call_args.kwargs["params"]["limit"], 15)
        self.assertEqual(request.call_args.kwargs["timeout"], 15)

    def test_openai_outline_uses_unified_backend(self):
        from methods.agents import outline_agent

        with tempfile.TemporaryDirectory() as temp:
            paths = []
            for name, body in (
                ("idea.md", "research idea"),
                ("experiment.md", "experimental evidence"),
                ("template.tex", "\\documentclass{article}"),
                ("guidelines.md", "conference rules"),
            ):
                path = Path(temp) / name
                path.write_text(body, encoding="utf-8")
                paths.append(str(path))
            expected = {"section_plan": []}
            with patch.object(
                outline_agent, "call_llm_with_text_prompt",
                return_value={"parsed_response": expected},
            ) as call:
                result = outline_agent.OutlineAgent("gpt-4o").run(*paths)
            self.assertEqual(result, expected)
            self.assertEqual(call.call_args.kwargs["model_name"], "gpt-4o")
            self.assertIn("research idea", call.call_args.kwargs["prompt"])

    def test_openai_literature_discovery_uses_semantic_scholar_not_gemini(self):
        from methods.agents import literature_review_agent as module

        with tempfile.TemporaryDirectory() as temp:
            paths = []
            for name in ("idea.md", "experiment.md", "template.tex", "guidelines.md"):
                path = Path(temp) / name
                path.write_text("context", encoding="utf-8")
                paths.append(str(path))
            agent = module.HybridLiteratureAgent(
                *paths[:2],
                latex_template_path=paths[2],
                conference_guidelines_path=paths[3],
                output_dir=str(Path(temp) / "out"),
                model_name="gpt-4o",
            )
            with patch.object(module, "s2_keyword_search", return_value=[
                {"title": "Verified candidate", "year": 2021}
            ]) as search, patch.object(module, "call_gemini_with_contents") as gemini:
                candidates = agent._discover_candidates(
                    {"focus": "graph networks", "context": "methods", "search_type": "targeted"},
                    {},
                    "2024-11",
                )
            self.assertEqual(candidates[0].title, "Verified candidate")
            self.assertEqual(candidates[0].year, 2021)
            search.assert_called_once()
            gemini.assert_not_called()

    def test_openai_literature_synthesis_uses_unified_backend(self):
        from methods.agents import literature_review_agent as module

        with tempfile.TemporaryDirectory() as temp:
            paths = []
            for name in ("idea.md", "experiment.md", "template.tex", "guidelines.md"):
                path = Path(temp) / name
                path.write_text("context", encoding="utf-8")
                paths.append(str(path))
            agent = module.HybridLiteratureAgent(
                *paths[:2],
                latex_template_path=paths[2],
                conference_guidelines_path=paths[3],
                output_dir=str(Path(temp) / "out"),
                model_name="gpt-4o",
            )
            with patch.object(module, "call_llm_with_text_prompt",
                              return_value={"parsed_response": "\\section{Related Work}"}) as call:
                result = agent._synthesize_content({}, [], "2024-11")
            self.assertEqual(result, "\\section{Related Work}")
            self.assertEqual(call.call_args.kwargs["model_name"], "gpt-4o")
            self.assertFalse(call.call_args.kwargs["return_json"])

    def test_peer_review_routes_through_unified_backend(self):
        from autoraters import agent_review

        with patch.object(agent_review, "call_llm_with_text_prompt",
                          return_value={"parsed_response": {"Overall": 8}}) as call:
            review = agent_review.perform_review_agentreview(
                "paper text", "gpt-4o", num_review_ensemble=1
            )
        self.assertEqual(review["Overall"], 8)
        self.assertEqual(call.call_args.kwargs["model_name"], "gpt-4o")


if __name__ == "__main__":
    unittest.main(verbosity=2)
