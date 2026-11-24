#!/usr/bin/env python3
"""Generate a complete, source-conscious proposal draft through a configured LLM.

This helper never fabricates bibliographic records and writes no output until
all requested scientific sections have passed validation.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
import tempfile
from pathlib import Path
from typing import Any, Callable

ALLOWED_SCIENTIFIC_KEYS = (
    "title_fa", "title_en", "thesis_specifications", "main_question",
    "problem_statement", "necessity", "literature_review", "innovation",
    "keywords", "objectives", "beneficiaries", "questions", "hypotheses",
    "definitions", "references_fa", "references_en", "references",
    "azad_challenge", "azad_goals", "azad_strategy", "azad_constellation",
    "azad_landscape", "research_program", "research_goal", "research_relation",
    "methodology", "data_collection", "research_tools", "analysis", "timeline",
    "variables", "personnel_cost", "materials_equipment", "miscellaneous_costs",
    "total_costs", "budget",
)
PROTECTED_KEYS = {
    "degree", "field", "specialization", "supervisor", "consultant",
    "student_name", "student_id", "student_information",
    "supervisor_information", "academic_records", "previous_theses",
    "archive_number", "approval_date", "supervisor_confirmation",
    "consultant_confirmation", "project_information", "student_commitment",
    "supervisor_commitment", "researcher_commitment", "committee_opinion",
    "scientific_evaluation", "group_council_minutes",
    "faculty_research_council_opinion", "final_approval",
}
BATCH_SIZE = 6


def _emit(event: str, **fields: Any) -> None:
    print(json.dumps({"event": event, **fields}, ensure_ascii=False), flush=True)


def _extract_sections(response: Any) -> dict[str, Any]:
    if isinstance(response, dict) and "parsed_response" in response:
        response = response["parsed_response"]
    if isinstance(response, dict) and isinstance(response.get("sections"), dict):
        response = response["sections"]
    if not isinstance(response, dict):
        raise ValueError("The AI provider did not return a JSON object of proposal sections.")
    return response


def _build_prompt(payload: dict[str, Any], batch: list[dict[str, str]]) -> str:
    language = payload["language"]
    title = payload["project_title"].strip()
    source_records = payload.get("sources") or []
    safe_sources = []
    for source in source_records[:80]:
        if not isinstance(source, dict):
            continue
        record = {
            key: str(source.get(key, "")).strip()[:1200]
            for key in ("title", "authors", "year", "doi", "url", "venue", "notes")
        }
        if record["title"]:
            safe_sources.append(record)
    keys = [item["key"] for item in batch]
    labels = [{"key": item["key"], "label": item["label"]} for item in batch]
    lang_rule = (
        "Write all proposal content in formal academic Persian. title_en must be a faithful English translation."
        if language == "fa"
        else "Write all proposal content in formal academic English. title_fa must be a faithful Persian translation."
    )
    return (
        "You are an experienced academic proposal-writing assistant. Produce a rigorous, "
        "coherent, discipline-aware proposal draft.\n"
        f"Project title supplied by the researcher: {title}\n"
        f"Output language: {language}. {lang_rule}\n"
        "The title is the only confirmed topic context. Do not invent the student's identity, "
        "supervisor, university, degree, field, approvals, dates, archive numbers, signatures, "
        "or administrative decisions. Those are entered manually later and are not requested here.\n"
        "Do not invent empirical results, collected data, experiments, statistics, or completed work. "
        "Describe proposed methods and expected contributions as plans, not as completed findings. "
        "Use cautious, testable claims and make the research question, objectives, hypotheses, "
        "methodology, variables, and analysis logically consistent.\n"
        "Bibliographic integrity is mandatory: cite only the source records supplied below. "
        "Never fabricate authors, article titles, journals, years, DOIs, URLs, or quotations. "
        "If no suitable source is supplied, explicitly state that the literature review needs "
        "verified sources and provide a focused search plan; do not create fake references.\n"
        "Return ONLY valid JSON with exactly this shape: "
        '{"sections":{"key":"complete section text"}}. No markdown fences or extra keys.\n'
        f"Required keys (JSON): {json.dumps(keys, ensure_ascii=False)}\n"
        f"Section labels (JSON): {json.dumps(labels, ensure_ascii=False)}\n"
        "Write substantive but concise sections (normally 100–180 words each, shorter for titles, "
        "keywords, and definitions). Use clear paragraphs or numbered lists where appropriate. "
        "Do not return empty strings or generic placeholders.\n"
        f"Verified/user-supplied source metadata (JSON): {json.dumps(safe_sources, ensure_ascii=False)}"
    )


def generate_proposal_sections(
    payload: dict[str, Any],
    model_name: str,
    llm_call: Callable[..., Any] | None = None,
) -> dict[str, str]:
    if not isinstance(payload, dict):
        raise ValueError("Proposal input must be a JSON object.")
    language = payload.get("language")
    title = payload.get("project_title")
    if language not in ("fa", "en") or not isinstance(title, str) or not title.strip():
        raise ValueError("A non-empty project title and language 'fa' or 'en' are required.")
    requested = payload.get("sections")
    if not isinstance(requested, list) or not requested:
        raise ValueError("No scientific proposal sections were supplied.")
    normalized: list[dict[str, str]] = []
    seen: set[str] = set()
    for item in requested:
        if not isinstance(item, dict):
            raise ValueError("Invalid proposal section descriptor.")
        key = str(item.get("key", "")).strip()
        label = str(item.get("label", key)).strip()
        if key in PROTECTED_KEYS or key not in ALLOWED_SCIENTIFIC_KEYS:
            raise ValueError(f"Protected or unsupported proposal section requested: {key or '(empty)'}")
        if key in seen:
            raise ValueError(f"Duplicate proposal section requested: {key}")
        if not label:
            raise ValueError(f"Missing label for proposal section: {key}")
        seen.add(key)
        normalized.append({"key": key, "label": label})
    if "title_fa" not in seen or "title_en" not in seen:
        raise ValueError("Both Persian and English proposal title fields must be generated.")
    if llm_call is None:
        # Lazy import keeps validation and unit tests independent of provider SDKs.
        from utils.llm_backend_utils import call_llm_with_text_prompt
        llm_call = call_llm_with_text_prompt

    completed: dict[str, str] = {}
    batches = [normalized[i:i + BATCH_SIZE] for i in range(0, len(normalized), BATCH_SIZE)]
    for index, batch in enumerate(batches, start=1):
        prompt = _build_prompt(payload, batch)
        response = llm_call(
            prompt=prompt,
            model_name=model_name,
            generation_configs={"temperature": 0.2},
            check_parsed_response_not_none=True,
            return_json=True,
        )
        sections = _extract_sections(response)
        expected = {item["key"] for item in batch}
        if set(sections) != expected:
            missing = sorted(expected - set(sections))
            extra = sorted(set(sections) - expected)
            raise ValueError(
                f"AI response batch {index} did not match the requested section keys "
                f"(missing={missing}, unexpected={extra})."
            )
        for key, value in sections.items():
            if not isinstance(value, str):
                raise ValueError(f"AI response for {key} was not text.")
            clean = value.strip()
            if len(clean) < 8 or re.search(r"(?i)\b(TODO|TBD|N/?A|lorem ipsum)\b", clean):
                raise ValueError(f"AI returned an incomplete draft for section: {key}.")
            completed[key] = clean
        _emit("proposal_generation_progress", completed_batches=index, total_batches=len(batches))
    if set(completed) != seen:
        raise ValueError("The AI response did not complete every requested proposal section.")
    return completed


def _atomic_write(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=path.name + ".", suffix=".tmp", dir=str(path.parent))
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as stream:
            json.dump(payload, stream, ensure_ascii=False, indent=2)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    except Exception:
        try:
            os.unlink(temporary)
        except OSError:
            pass
        raise


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, help="JSON input file")
    parser.add_argument("--output", required=True, help="JSON output file")
    parser.add_argument("--model", required=True, help="Gemini, OpenAI, or OpenRouter model ID")
    args = parser.parse_args(argv)
    try:
        input_path = Path(args.input).resolve()
        output_path = Path(args.output).resolve()
        if input_path == output_path:
            raise ValueError("Input and output paths must be different.")
        payload = json.loads(input_path.read_text(encoding="utf-8"))
        sections = generate_proposal_sections(payload, args.model.strip())
        result = {
            "schema_version": 1,
            "model": args.model.strip(),
            "language": payload["language"],
            "project_title": payload["project_title"].strip(),
            "sections": sections,
        }
        _atomic_write(output_path, result)
        _emit("proposal_generation_completed", section_count=len(sections), output=str(output_path))
        return 0
    except Exception as exc:
        # Never print raw provider responses or credentials to CI/UI logs.
        try:
            from utils.security_utils import redact_secrets
            message = redact_secrets(str(exc))
        except Exception:
            message = "The AI proposal generation request failed."
        _emit("proposal_generation_failed", error_type=type(exc).__name__, message=message[:320])
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
