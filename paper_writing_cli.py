# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import sys
import os
import shutil
import json
from datetime import datetime



def validate_runtime_prerequisites(requirements_path=None):
    """Return actionable missing runtime prerequisites before creating a run."""
    import importlib.metadata
    import re

    if requirements_path is None:
        requirements_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "requirements-research.txt")

    missing = []
    try:
        with open(requirements_path, encoding="utf-8") as requirements:
            packages = []
            for raw_line in requirements:
                line = raw_line.split("#", 1)[0].strip()
                if not line:
                    continue
                package = re.split(r"[<>=!~;\\[]", line, maxsplit=1)[0].strip()
                if package:
                    packages.append(package)
    except OSError as exc:
        missing.append(f"Cannot read requirements file: {exc}")
        packages = []

    for package in packages:
        try:
            importlib.metadata.distribution(package)
        except importlib.metadata.PackageNotFoundError:
            missing.append(f"Python package: {package}")

    for executable in ("pdflatex", "bibtex"):
        if shutil.which(executable) is None:
            missing.append(f"LaTeX executable: {executable}")

    return missing



def update_run_manifest(output_dir, status, stage, error=None):
    """Atomically persist non-secret lifecycle metadata for a single research run."""
    import uuid
    from utils.security_utils import redact_secrets

    os.makedirs(output_dir, exist_ok=True)
    manifest_path = os.path.join(output_dir, "run_manifest.json")
    manifest = {}
    if os.path.exists(manifest_path):
        with open(manifest_path, encoding="utf-8") as existing:
            manifest = json.load(existing)
        if not isinstance(manifest, dict):
            raise ValueError("Existing run manifest must contain a JSON object.")

    now = datetime.now().astimezone().isoformat(timespec="seconds")
    manifest.setdefault("schema_version", 1)
    manifest.setdefault("run_id", uuid.uuid4().hex)
    manifest.setdefault("created_at", now)
    manifest.update({
        "updated_at": now,
        "status": status,
        "stage": stage,
    })
    if error:
        manifest["error"] = redact_secrets(error)
    else:
        manifest.pop("error", None)

    temporary_path = manifest_path + ".tmp-" + uuid.uuid4().hex
    try:
        with open(temporary_path, "x", encoding="utf-8") as temporary:
            json.dump(manifest, temporary, ensure_ascii=False, indent=2)
            temporary.write("\n")
            temporary.flush()
            os.fsync(temporary.fileno())
        os.replace(temporary_path, manifest_path)
    finally:
        if os.path.exists(temporary_path):
            os.unlink(temporary_path)
    return manifest



def emit_event(enabled: bool, event: str, **details) -> None:
    """Emit optional machine-readable JSONL events for desktop integrations."""
    if not enabled:
        return
    safe_details = dict(details)
    if "error" in safe_details:
        from utils.security_utils import redact_secrets
        safe_details["error"] = redact_secrets(safe_details["error"])
    payload = {
        "event": event,
        "timestamp": datetime.now().isoformat(timespec="seconds"),
        **safe_details,
    }
    print(json.dumps(payload, ensure_ascii=False), flush=True)


def prepare_raw_materials(source_dir: str, target_dir: str) -> bool:
    """Copy source materials without ever overwriting existing user data.

    Returns False when source and target resolve to the same directory.
    Raises FileExistsError when the destination already exists.
    """
    source = os.path.realpath(source_dir)
    target = os.path.realpath(target_dir)
    if source == target:
        return False
    if not os.path.isdir(source):
        raise NotADirectoryError(f"Raw materials source is not a directory: {source_dir}")
    try:
        common_path = os.path.commonpath([source, target])
    except ValueError:
        # On Windows, source and destination may be on different drives.
        common_path = None
    if common_path == source:
        raise ValueError("Output raw_materials directory cannot be inside the source directory.")
    if os.path.lexists(target):
        raise FileExistsError(
            f"Refusing to overwrite existing raw materials: {target}. "
            "Choose a new output directory or move the existing data yourself."
        )
    shutil.copytree(source, target)
    return True


def main():
    parser = argparse.ArgumentParser(
        description="CLI for generating a single paper from the Paper Orchestra pipeline."
    )

    parser.add_argument(
        "--raw_materials_dir",
        required=True,
        help="Directory containing the raw materials (e.g. idea and experimental log).",
    )
    parser.add_argument(
        "--output_dir",
        default=None,
        help="Directory to output the generated paper. If not specified, an automatic folder is created.",
    )
    parser.add_argument(
        "--latex_template_dir",
        required=True,
        help="Directory containing the LaTeX template (e.g. cvpr2025/iclr2025).",
    )
    parser.add_argument(
        "--idea_filename",
        default="idea_sparse.md",
        help="Filename of the idea document within raw_materials. Default: idea_sparse.md",
    )
    parser.add_argument(
        "--experimental_log_filename",
        default="experimental_log.md",
        help="Filename of the experimental log within raw_materials. Default: experimental_log.md",
    )
    parser.add_argument(
        "--writer_model_name",
        default="gemini-3.1-pro-preview",
        help="LLM for writer and literature agents.",
    )
    parser.add_argument(
        "--reflection_model_name",
        default=None,
        help="LLM for reflection agents. Defaults to the selected writer model.",
    )
    parser.add_argument(
        "--research_cutoff",
        default=None,
        help="Year-Month cutoff date for literature review (e.g., 2024-11). If not provided, defaults to current Year-Month.",
    )
    parser.add_argument(
        "--use_plotting",
        type=lambda x: (str(x).lower() in ["true", "1", "yes"]),
        default=False,
        help="Enable plotting agent workflow to generate figures from code.",
    )
    parser.add_argument(
        "--plotting_model_name",
        default="gemini-3.1-pro-preview",
        help="LLM for the plotting agent.",
    )
    parser.add_argument(
        "--image_model_name",
        default="gemini-3-pro-image-preview",
        help="Vision model if required by plotting agent.",
    )
    parser.add_argument(
        "--plotting_max_critic_rounds",
        type=int,
        default=3,
        help="Maximum rounds of self-correction in the plotting agent.",
    )
    parser.add_argument(
        "--resume",
        action="store_true",
        help="Resume the newest attempt whose input fingerprint and artifacts validate.",
    )
    parser.add_argument(
        "--events_jsonl",
        action="store_true",
        help="Emit machine-readable JSON Lines lifecycle events for desktop clients.",
    )

    args = parser.parse_args()

    if args.research_cutoff is None:
        args.research_cutoff = datetime.now().strftime("%Y-%m")

    if args.reflection_model_name is None:
        args.reflection_model_name = args.writer_model_name

    if args.resume and (not args.output_dir or not os.path.isdir(args.output_dir)):
        message = "Resume requires an existing --output_dir from a previous run."
        print("Error: " + message, file=sys.stderr, flush=True)
        emit_event(args.events_jsonl, "pipeline_failed", stage="resume_preflight",
                   error=message)
        sys.exit(1)

    if not os.path.isdir(args.raw_materials_dir):
        message = (
            f"Raw materials directory does not exist or is not a directory: "
            f"{args.raw_materials_dir}"
        )
        print("Error: " + message, file=sys.stderr, flush=True)
        emit_event(args.events_jsonl, "pipeline_failed", stage="input_preflight",
                   error=message)
        sys.exit(1)

    missing_prerequisites = validate_runtime_prerequisites()
    if missing_prerequisites:
        details = "; ".join(missing_prerequisites)
        message = "Runtime preflight failed: " + details
        print(message, file=sys.stderr, flush=True)
        emit_event(args.events_jsonl, "pipeline_failed", stage="runtime_preflight",
                   error=message, missing=missing_prerequisites)
        sys.exit(2)

    if args.output_dir:
        base_dir = args.output_dir
    else:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        base_dir = f"./paper_output_{timestamp}"

    os.makedirs(base_dir, exist_ok=True)
    manifest = update_run_manifest(base_dir, "running", "preparing_inputs")

    target_raw_materials = os.path.join(base_dir, "raw_materials")
    try:
        if os.path.abspath(target_raw_materials) != os.path.abspath(args.raw_materials_dir):
            if args.resume and os.path.isdir(target_raw_materials):
                from utils.run_checkpoints import fingerprint_path
                if fingerprint_path(args.raw_materials_dir) != fingerprint_path(target_raw_materials):
                    raise ValueError(
                        "Raw materials differ from the existing run copy; refusing to resume with changed inputs."
                    )
            else:
                prepare_raw_materials(args.raw_materials_dir, target_raw_materials)
    except Exception as exc:
        update_run_manifest(base_dir, "failed", "preparing_inputs", error=exc)
        emit_event(args.events_jsonl, "pipeline_failed", stage="preparing_inputs", error=str(exc))
        raise

    if not args.use_plotting:
        has_figures = False
        figures_dir = os.path.join(args.raw_materials_dir, "figures")
        if os.path.exists(figures_dir):
            for f in os.listdir(figures_dir):
                if f.lower().endswith((".pdf", ".png", ".jpg", ".jpeg")):
                    has_figures = True
                    break

        if not has_figures:
            print("\n" + "=" * 80)
            print(
                f"⚠️ WARNING: No figures (.pdf, .png, .jpg) detected in the {figures_dir} directory."
            )
            print(
                "Since you are running WITHOUT plotting, we are using the provided materials directly."
            )
            print(
                f"Please make sure you provide all necessary figures inside {figures_dir}."
            )
            print("If figures are missing, compilation may fail.")
            print(
                f"If you don't have figures, you can use the plotting agent to generate them by running with --use_plotting."
            )
            print("=" * 80 + "\n")

    manifest = update_run_manifest(base_dir, "running", "paper_generation")
    emit_event(
        args.events_jsonl,
        "pipeline_started",
        stage="paper_generation",
        run_id=manifest["run_id"],
        mode="with_plotting" if args.use_plotting else "without_plotting",
        output_dir=os.path.abspath(base_dir),
    )
    try:
        if args.use_plotting:
            from methods.paper_writer_with_plotting import write_single_paper

            print(f"🚀 Starting Paper Generation (WITH PLOTTING) in {base_dir}")
            pipeline_result = write_single_paper(
                base_dir=base_dir,
                latex_template_dir=args.latex_template_dir,
                idea_filename=args.idea_filename,
                experimental_log_filename=args.experimental_log_filename,
                writer_model_name=args.writer_model_name,
                reflection_model_name=args.reflection_model_name,
                plotting_model_name=args.plotting_model_name,
                image_model_name=args.image_model_name,
                research_cutoff=args.research_cutoff,
                plotting_max_critic_rounds=args.plotting_max_critic_rounds,
                resume=args.resume,
            )
        else:
            from methods.paper_writer import write_single_paper

            provided_figures_dir = os.path.join(args.raw_materials_dir, "figures")
            print(
                f"🚀 Starting Paper Generation (WITHOUT PLOTTING), with figures stored in {provided_figures_dir}"
            )
            pipeline_result = write_single_paper(
                base_dir=base_dir,
                latex_template_dir=args.latex_template_dir,
                idea_filename=args.idea_filename,
                experimental_log_filename=args.experimental_log_filename,
                writer_model_name=args.writer_model_name,
                reflection_model_name=args.reflection_model_name,
                research_cutoff=args.research_cutoff,
                resume=args.resume,
            )
        if pipeline_result is not True:
            raise RuntimeError(
                "Paper generation pipeline reported failure after exhausting its retries."
            )
    except Exception as exc:
        update_run_manifest(base_dir, "failed", "paper_generation", error=exc)
        emit_event(args.events_jsonl, "pipeline_failed", stage="paper_generation", error=str(exc))
        raise

    update_run_manifest(base_dir, "completed", "completed")
    emit_event(
        args.events_jsonl,
        "pipeline_completed",
        stage="completed",
        run_id=manifest["run_id"],
        output_dir=os.path.abspath(base_dir),
    )


if __name__ == "__main__":
    main()
