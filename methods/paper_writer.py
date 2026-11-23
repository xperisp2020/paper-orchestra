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

import os
import inspect
from utils.security_utils import redact_secrets
import os.path as osp
import shutil
import traceback
import json
import tqdm
from concurrent.futures import ProcessPoolExecutor, as_completed

from methods.agents.outline_agent import OutlineAgent
from methods.agents.literature_review_agent import HybridLiteratureAgent
from methods.agents.section_writing_agent import SectionWritingAgent
from methods.agents.content_refinement_agent import ContentRefinementAgent

from utils.common_utils import (
    create_log_folder,
)
from utils.safe_workspace import create_new_workspace, create_next_workspace, ensure_paths_absent, publish_file_no_overwrite
from utils.run_checkpoints import CheckpointStore, create_or_resume_attempt, fingerprint_inputs, fingerprint_path


def write_single_paper(
    base_dir,
    latex_template_dir,
    idea_filename="idea_dense.md",
    experimental_log_filename="experimental_log.md",
    writer_model_name: str = "gemini-3.1-pro-preview",
    reflection_model_name: str = None,
    research_cutoff: str = "2024-11",
    resume: bool = False,
):
    if reflection_model_name is None:
        reflection_model_name = writer_model_name

    pdf_file = osp.join(base_dir, "final_paper.pdf")
    ensure_paths_absent((
        osp.join(base_dir, "latex_writeup"),
        osp.join(base_dir, "content_refinement_workdir"),
        osp.join(base_dir, "outline.json"),
        osp.join(base_dir, "literature_agent_output"),
    ))

    materials_dir = osp.join(base_dir, "raw_materials")
    old_figure_dir = osp.join(materials_dir, "figures")

    checkpoint_fingerprint = fingerprint_inputs({
        "raw_materials": fingerprint_path(materials_dir),
        "latex_template": fingerprint_path(latex_template_dir),
        "idea_filename": idea_filename,
        "experimental_log_filename": experimental_log_filename,
        "writer_model": writer_model_name,
        "reflection_model": reflection_model_name,
        "research_cutoff": research_cutoff,
        "agent_sources": {
            "outline": fingerprint_path(inspect.getsourcefile(OutlineAgent)),
            "literature": fingerprint_path(inspect.getsourcefile(HybridLiteratureAgent)),
            "section_writing": fingerprint_path(inspect.getsourcefile(SectionWritingAgent)),
            "refinement": fingerprint_path(inspect.getsourcefile(ContentRefinementAgent)),
        },
        "checkpoint_engine": fingerprint_path(inspect.getsourcefile(CheckpointStore)),
        "writer_source": fingerprint_path(__file__),
        "prompt_sources": (
            fingerprint_path(osp.join(osp.dirname(osp.dirname(__file__)), "methods", "prompts"))
            if osp.isdir(osp.join(osp.dirname(osp.dirname(__file__)), "methods", "prompts"))
            else None
        ),
        "pipeline": "without_plotting_v2",
    })
    if osp.lexists(pdf_file):
        existing_attempt, existing_store, resumed = create_or_resume_attempt(
            base_dir, checkpoint_fingerprint, resume=resume, create_if_missing=False
        )
        staged_pdf = (
            osp.join(existing_attempt, "content_refinement_workdir", "final_refined_paper.pdf")
            if resumed else ""
        )
        published_pdf_valid = False
        if (
            resume and resumed and existing_store.is_stage_valid("refinement")
            and not osp.islink(pdf_file) and osp.isfile(pdf_file)
            and staged_pdf and not osp.islink(staged_pdf) and osp.isfile(staged_pdf)
        ):
            with open(pdf_file, "rb") as stream:
                has_pdf_signature = stream.read(5) == b"%PDF-"
            published_pdf_valid = (
                has_pdf_signature and fingerprint_path(pdf_file) == fingerprint_path(staged_pdf)
            )
        if published_pdf_valid:
            return True
        raise FileExistsError(
            "A final_paper.pdf already exists but is not verified against a completed checkpoint."
        )
    max_n_tries = 3
    cur_try = 0
    succeed = False

    while (not succeed) and (cur_try < max_n_tries):
        checkpoint_run_lock = None
        checkpoint_lock_acquired = False
        try:
            assert osp.exists(
                materials_dir
            ), f"Please make sure '{materials_dir}' exists!"

            # Every retry gets an isolated directory. Failed partial outputs are
            # retained for diagnosis and never mixed into the next attempt.
            attempt_dir, checkpoint_store, resumed_attempt = create_or_resume_attempt(
                base_dir, checkpoint_fingerprint, resume=(resume and cur_try == 0)
            )
            checkpoint_run_lock = checkpoint_store.run_lock()
            checkpoint_run_lock.__enter__()
            checkpoint_lock_acquired = True
            latex_writeup_dir = osp.join(attempt_dir, "latex_writeup")
            content_refinement_workdir = osp.join(attempt_dir, "content_refinement_workdir")
            outline_file_path = osp.join(attempt_dir, "outline.json")
            outline_initial_path = osp.join(attempt_dir, "outline_initial.json")
            checkpoint_inputs_dir = osp.join(attempt_dir, "checkpoint_inputs")
            literature_agent_output_dir = osp.join(attempt_dir, "literature_agent_output")
            new_figure_dir = osp.join(latex_writeup_dir, "figures")
            figures_dir = new_figure_dir

            if not osp.isdir(latex_writeup_dir):
                shutil.copytree(latex_template_dir, latex_writeup_dir)
            if osp.exists(old_figure_dir):
                shutil.copytree(old_figure_dir, new_figure_dir, dirs_exist_ok=True)

            guidelines_path = osp.join(latex_writeup_dir, "guidelines.md")
            latex_template_path = osp.join(latex_writeup_dir, "template.tex")

            idea_file_path = osp.join(materials_dir, idea_filename)
            experimental_log_file_path = osp.join(
                materials_dir, experimental_log_filename
            )

            assert osp.exists(
                guidelines_path
            ), f"No guidelines file detected at '{guidelines_path}'!"
            assert osp.exists(
                latex_template_path
            ), f"No template file detected at '{latex_template_path}'!"
            assert osp.exists(
                idea_file_path
            ), f"No idea file detected at '{idea_file_path}'!"
            assert osp.exists(
                experimental_log_file_path
            ), f"No experimental log file detected at '{experimental_log_file_path}'!"

            prepare_inputs_valid = checkpoint_store.is_stage_valid("prepare_inputs")
            if resumed_attempt and not prepare_inputs_valid:
                raise RuntimeError("Checkpoint input snapshot is invalid; preserving this attempt and starting a fresh one.")
            if not prepare_inputs_valid:
                os.makedirs(checkpoint_inputs_dir, exist_ok=True)
                shutil.copy2(latex_template_path, osp.join(checkpoint_inputs_dir, "template.tex"))
                shutil.copy2(guidelines_path, osp.join(checkpoint_inputs_dir, "guidelines.md"))
                checkpoint_store.record_stage("prepare_inputs", [
                    "checkpoint_inputs/template.tex", "checkpoint_inputs/guidelines.md"
                ])

            ###############################################################################################
            # Step 1: Call outline agent
            if not checkpoint_store.is_stage_valid("outline"):
                shutil.copy2(osp.join(checkpoint_inputs_dir, "template.tex"), latex_template_path)
                outline_agent = OutlineAgent(
                    model_name=writer_model_name, cutoff_date=research_cutoff
                )
                outline_agent.run(
                    idea_file=idea_file_path,
                    experimental_log_file=experimental_log_file_path,
                    latex_template_file=latex_template_path,
                    conference_guidelines_file=guidelines_path,
                    output_filepath=outline_file_path,
                )
                shutil.copy2(outline_file_path, outline_initial_path)
                checkpoint_store.record_stage("outline", ["outline_initial.json"])

            ###############################################################################################
            # Step 2: Call HybridLiteratureAgent
            if not checkpoint_store.is_stage_valid("literature"):
                shutil.copy2(osp.join(checkpoint_inputs_dir, "template.tex"), latex_template_path)
                shutil.copy2(outline_initial_path, outline_file_path)
                literature_agent = HybridLiteratureAgent(
                    idea_path=idea_file_path,
                    experimental_log_path=experimental_log_file_path,
                    latex_template_path=latex_template_path,
                    conference_guidelines_path=guidelines_path,
                    output_dir=literature_agent_output_dir,
                    model_name=writer_model_name,
                    max_workers=3,
                )
                literature_agent.run(
                    outline_path=outline_file_path,
                    cutoff_date=research_cutoff,
                )
                shutil.copy(
                    osp.join(literature_agent_output_dir, "outline_v1.json"),
                    outline_file_path,
                )
                shutil.copy(
                    osp.join(literature_agent_output_dir, "updated_template.tex"),
                    osp.join(latex_writeup_dir, "template.tex"),
                )
                shutil.copy(
                    osp.join(literature_agent_output_dir, "references.bib"),
                    osp.join(latex_writeup_dir, "references.bib"),
                )
                checkpoint_store.record_stage("literature", [
                    "literature_agent_output/outline_v1.json",
                    "literature_agent_output/updated_template.tex",
                    "literature_agent_output/references.bib",
                    "literature_agent_output/citation_map.json",
                    "outline.json",
                    "latex_writeup/template.tex",
                    "latex_writeup/references.bib",
                ])

            ###############################################################################################
            # Step 3. Call SectionWritingAgent
            citation_map_path = osp.join(
                literature_agent_output_dir, "citation_map.json"
            )
            raw_draft_tex_path = osp.join(latex_writeup_dir, "raw_draft_paper.tex")
            if not checkpoint_store.is_stage_valid("draft"):
                section_writing_agent = SectionWritingAgent(
                    outline_path=outline_file_path,
                    template_path=osp.join(latex_writeup_dir, "template.tex"),
                    idea_path=idea_file_path,
                    experimental_log_path=experimental_log_file_path,
                    citation_map_path=citation_map_path,
                    figures_info_path=osp.join(figures_dir, "info.json"),
                    guidelines_path=guidelines_path,
                    model_name=writer_model_name,
                )
                section_writing_agent.run(output_path=raw_draft_tex_path)
                checkpoint_store.record_stage("draft", ["latex_writeup/raw_draft_paper.tex"])

            ###############################################################################################
            # Step 4: Content Reflection
            if not osp.isdir(content_refinement_workdir):
                shutil.copytree(latex_writeup_dir, content_refinement_workdir)

            content_refinment_agent_output_pdf_path = osp.join(
                content_refinement_workdir, "final_refined_paper.pdf"
            )
            if not checkpoint_store.is_stage_valid("refinement"):
                content_refinement_agent = ContentRefinementAgent(
                    experimental_log_path=experimental_log_file_path,
                    citation_map_path=citation_map_path,
                    guidelines_path=guidelines_path,
                    model_name=reflection_model_name,
                    max_reflections=3,
                    work_dir=content_refinement_workdir,
                )
                content_refinment_agent_output_pdf_path = content_refinement_agent.run(
                    texfile_path=raw_draft_tex_path
                )
                checkpoint_store.record_stage("refinement", [
                    osp.relpath(content_refinment_agent_output_pdf_path, attempt_dir).replace(osp.sep, "/")
                ])
            assert osp.exists(
                content_refinment_agent_output_pdf_path
            ), f"Content Refinement Agent failed to produce final PDF at {content_refinment_agent_output_pdf_path}!"

            final_pdf_path = osp.join(base_dir, "final_paper.pdf")
            print(
                f" >> Copying final PDF from  {content_refinment_agent_output_pdf_path} to {final_pdf_path}...\n"
            )
            publish_file_no_overwrite(content_refinment_agent_output_pdf_path, final_pdf_path)
            print(" >> Final PDF saved to:\n", final_pdf_path)

            succeed = True
            return True

        except Exception as exc:
            if isinstance(exc, RuntimeError) and "Checkpoint is locked" in str(exc):
                raise
            print(f"[Try {cur_try+1}/{max_n_tries}] EXCEPTION in perform_writeup:")
            print(redact_secrets(traceback.format_exc()))
            cur_try += 1
            if cur_try < max_n_tries:
                print("Try again now...")
        finally:
            if checkpoint_lock_acquired and checkpoint_run_lock is not None:
                checkpoint_run_lock.__exit__(None, None, None)

    print(f"Writeup failed after {max_n_tries}! Stop here.")
    return False


def process_paper_task(
    paper,
    output_dir,
    latex_template_dir,
    idea_filename,
    experimental_log_filename,
    writer_model_name,
    research_cutoff,
):
    """
    Worker function to process a single paper.
    Returns a dictionary with results to be handled by the main thread.
    """
    try:
        paper_id = paper["paper_id"]
        print(f"Starting processing for paper '{paper_id}'...")

        paper_dest_dir = create_next_workspace(output_dir, str(paper_id))
        final_paper_path = osp.join(paper_dest_dir, "final_paper.pdf")

        result = {
            "paper_id": paper_id,
            "status": "failed",
            "paper_path": "",
        }

        if True:
            content_dest_dir = osp.join(paper_dest_dir, "raw_materials")
            os.makedirs(content_dest_dir, exist_ok=True)
            print(
                f"Copying content_folder from {paper['content_folder']} to {content_dest_dir}..."
            )
            shutil.copytree(
                src=paper["content_folder"], dst=content_dest_dir, dirs_exist_ok=True
            )
            print(f"Copying original paper to {content_dest_dir}...")
            shutil.copy(
                src=paper["pdf_path"],
                dst=osp.join(content_dest_dir, "original_paper.pdf"),
            )

            success = write_single_paper(
                base_dir=paper_dest_dir,
                latex_template_dir=latex_template_dir,
                idea_filename=idea_filename,
                experimental_log_filename=experimental_log_filename,
                writer_model_name=writer_model_name,
                research_cutoff=research_cutoff,
            )

            if success:
                result["status"] = "success"
                result["paper_path"] = final_paper_path
                print(f"Paper {paper_id} successfully written!")
            else:
                print(f"Paper {paper_id} writing failed.")
                return result

        return result

    except Exception as e:
        print(f"CRITICAL ERROR in worker for paper {paper['paper_id']}: {redact_secrets(e)}")
        print(redact_secrets(traceback.format_exc()))
        return {"paper_id": paper["paper_id"], "status": "error", "error": str(e)}


def _load_papers_from_config(
    process_root_folder: bool,
    root_input_folder: str,
    paper_info_path: str,
    n_papers: int,
):
    """Loads a list of paper dictionaries from either a root folder or a JSON file."""
    if process_root_folder:
        if not root_input_folder:
            raise ValueError(
                "Please provide root_input_folder if process_root_folder is True."
            )
        all_papers_candidates = []
        all_paper_ids_raw = os.listdir(root_input_folder)
        all_paper_ids = [
            paper_id
            for paper_id in all_paper_ids_raw
            if os.path.isdir(os.path.join(root_input_folder, paper_id))
        ]
        for paper_id in all_paper_ids:
            content_folder = os.path.join(root_input_folder, paper_id, "raw_materials")
            pdf_path = os.path.join(root_input_folder, paper_id, f"{paper_id}.pdf")
            if not os.path.exists(content_folder) or not os.path.exists(pdf_path):
                continue
            paper_entry = {
                "paper_id": paper_id,
                "content_folder": content_folder,
                "pdf_path": pdf_path,
            }
            all_papers_candidates.append(paper_entry)
    else:
        if not paper_info_path:
            raise ValueError(
                "Please provide paper_info_path if process_root_folder is False."
            )
        with open(paper_info_path, "r") as f:
            all_papers_candidates = json.load(f)

    if n_papers < 0:
        n_papers = len(all_papers_candidates)
    else:
        n_papers = min(n_papers, len(all_papers_candidates))

    return all_papers_candidates[:n_papers]


def _load_logs(output_dir: str):
    """Loads existing logs to support resuming a batch job."""
    paper_writing_log_path = osp.join(output_dir, "paper_writing_log.json")
    write_log = {}
    if osp.exists(paper_writing_log_path):
        with open(paper_writing_log_path, "r") as f:
            write_log = json.load(f)
        print(f"Loaded existing writing log with {len(write_log)} entries.")

    return write_log


def _filter_and_prepare_papers(
    all_papers, write_log, process_root_folder, paper_info_path
):
    """Filters out completed papers and resolves paths."""
    papers_to_process = []
    base_info_dir = ""
    if (not process_root_folder) and (paper_info_path != ""):
        base_info_dir = osp.dirname(paper_info_path)

    for paper in all_papers:
        # Check if already successfully processed in log
        if (
            paper["paper_id"] in write_log
            and write_log[paper["paper_id"]].get("status") == "success"
            and osp.exists(write_log[paper["paper_id"]].get("paper_path", ""))
        ):
            continue

        if not osp.isabs(paper["content_folder"]):
            paper["content_folder"] = osp.join(base_info_dir, paper["content_folder"])
        if not osp.isabs(paper["pdf_path"]):
            paper["pdf_path"] = osp.join(base_info_dir, paper["pdf_path"])

        papers_to_process.append(paper)

    return papers_to_process


def run_batch_writeup(
    latex_template_dir: str,
    process_root_folder: bool = True,
    root_input_folder: str = "",
    paper_info_path: str = "",
    output_dir: str = "",
    output_dir_prefix: str = "",
    date_str: str = "",
    idea_filename: str = "idea_sparse.md",
    experimental_log_filename: str = "experimental_log.md",
    writer_model_name: str = "gemini-3.1-pro-preview",
    research_cutoff: str = "2024-11",
    n_papers=-1,
    max_workers=4,
):
    if output_dir == "":
        output_dir = create_log_folder(prefix=output_dir_prefix, date_str=date_str)
    else:
        print(f"Resuming from output_dir = {output_dir}")
    os.makedirs(output_dir, exist_ok=True)

    all_papers = _load_papers_from_config(
        process_root_folder, root_input_folder, paper_info_path, n_papers
    )
    write_log = _load_logs(output_dir)

    papers_to_process = _filter_and_prepare_papers(
        all_papers, write_log, process_root_folder, paper_info_path
    )

    print(
        f"Starting batch writeup for {len(papers_to_process)} papers (skipped {len(all_papers) - len(papers_to_process)} already done)."
    )
    print(f"Parallel execution with max_workers={max_workers}")

    paper_writing_log_path = osp.join(output_dir, "paper_writing_log.json")
    with ProcessPoolExecutor(max_workers=max_workers) as executor:
        futures = {
            executor.submit(
                process_paper_task,
                paper,
                output_dir,
                latex_template_dir,
                idea_filename,
                experimental_log_filename,
                writer_model_name,
                research_cutoff,
            ): paper["paper_id"]
            for paper in papers_to_process
        }

        for future in tqdm.tqdm(
            as_completed(futures), total=len(futures), desc="Writing Papers"
        ):
            paper_id = futures[future]
            try:
                result = future.result()

                write_log[paper_id] = {
                    "status": result["status"],
                    "paper_path": result.get("paper_path", ""),
                }

                with open(paper_writing_log_path, "w") as f:
                    json.dump(write_log, f, indent=4)

            except Exception as e:
                print(f"Exception handling result for paper {paper_id}: {redact_secrets(e)}")

    success_count = sum(
        1 for entry in write_log.values() if entry.get("status") == "success"
    )
    error_count = sum(
        1 for entry in write_log.values() if entry.get("status") in ["failed", "error"]
    )

    print("\n" + "=" * 50)
    print("BATCH PROCESSING COMPLETE")
    print(f"Total Papers: {len(all_papers)}")
    print(f"Successfully written: {success_count}")
    print(f"Failed/Error: {error_count}")
    print(f"Logs saved to: {output_dir}")
    print("=" * 50 + "\n")

    return paper_writing_log_path
