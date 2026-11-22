"""Domain models and local persistence; deliberately independent of Qt."""
from __future__ import annotations

from dataclasses import dataclass
from contextlib import contextmanager
from collections.abc import Iterator
from pathlib import Path
import json
import sqlite3
from typing import Iterable

PROPOSAL_SECTIONS: tuple[tuple[str, str], ...] = (
    ("title_fa", "عنوان فارسی"),
    ("title_en", "عنوان انگلیسی"),
    ("degree", "مقطع تحصیلی"),
    ("field", "رشته تحصیلی"),
    ("specialization", "گرایش"),
    ("supervisor", "استاد راهنما"),
    ("consultant", "استاد مشاور"),
    ("student_name", "نام دانشجو"),
    ("student_id", "شماره دانشجویی"),
    ("archive_number", "شماره بایگانی"),
    ("approval_date", "تاریخ تصویب"),
    ("student_information", "اطلاعات دانشجو"),
    ("supervisor_information", "اطلاعات استاد راهنما"),
    ("academic_records", "سوابق تحصیلی"),
    ("previous_theses", "پایان‌نامه‌های پیشین"),
    ("thesis_specifications", "مشخصات پایان‌نامه"),
    ("main_question", "پرسش اصلی"),
    ("problem_statement", "بیان مسئله"),
    ("necessity", "اهمیت و ضرورت"),
    ("literature_review", "پیشینه پژوهش"),
    ("innovation", "نوآوری"),
    ("keywords", "کلیدواژه‌ها"),
    ("objectives", "اهداف"),
    ("beneficiaries", "بهره‌وران"),
    ("questions", "پرسش‌ها"),
    ("hypotheses", "فرضیه‌ها"),
    ("definitions", "تعاریف"),
    ("references_fa", "منابع فارسی"),
    ("references_en", "منابع غیرفارسی"),
    ("references", "منابع تکمیلی (سازگاری با پرونده‌های قبلی)"),
    ("azad_challenge", "چالش پایش آزاد"),
    ("azad_goals", "اهداف پایش آزاد"),
    ("azad_strategy", "راهبرد پایش آزاد"),
    ("azad_constellation", "منظومه پایش آزاد"),
    ("azad_landscape", "منظر پایش آزاد"),
    ("research_program", "برنامه پژوهشی"),
    ("research_goal", "هدف پژوهش"),
    ("research_relation", "ارتباط پژوهش با برنامه"),
    ("methodology", "روش‌شناسی"),
    ("data_collection", "روش گردآوری اطلاعات"),
    ("research_tools", "ابزارهای پژوهش"),
    ("analysis", "روش تحلیل"),
    ("timeline", "زمان‌بندی"),
    ("variables", "متغیرها"),
    ("personnel_cost", "هزینه پرسنلی"),
    ("materials_equipment", "مواد و وسایل"),
    ("miscellaneous_costs", "هزینه‌های متفرقه"),
    ("total_costs", "جمع کل هزینه‌ها"),
    ("budget", "برآورد کلی هزینه‌ها (سازگاری با پرونده‌های قبلی)"),
    ("supervisor_confirmation", "تأیید استاد راهنما"),
    ("consultant_confirmation", "تأیید استاد مشاور"),
    ("project_information", "اطلاعات فرم طرح"),
    ("student_commitment", "تعهدنامه دانشجو"),
    ("supervisor_commitment", "تعهدنامه استاد راهنما"),
    ("researcher_commitment", "سایر تعهدات پژوهش"),
    ("committee_opinion", "نظر کمیته تخصصی"),
    ("scientific_evaluation", "ارزیابی علمی"),
    ("group_council_minutes", "صورت‌جلسه شورای گروه"),
    ("faculty_research_council_opinion", "نظر شورای پژوهشی دانشکده"),
    ("final_approval", "تأیید نهایی"),
)
@dataclass(frozen=True)
class Project:
    project_id: int
    title: str
    language: str
    ai_mode: str
    workflow: str

class ProjectStore:
    """SQLite store that never deletes user project files implicitly."""
    def __init__(self, database: Path):
        self.database = Path(database)
        self.database.parent.mkdir(parents=True, exist_ok=True)
        self._initialize()

    @contextmanager
    def _connect(self) -> Iterator[sqlite3.Connection]:
        """Yield a transaction-scoped connection and always release its file handle."""
        connection = sqlite3.connect(self.database)
        connection.row_factory = sqlite3.Row
        try:
            yield connection
            connection.commit()
        except BaseException:
            connection.rollback()
            raise
        finally:
            connection.close()

    def _initialize(self) -> None:
        with self._connect() as db:
            db.execute("PRAGMA foreign_keys=ON")
            db.execute("""CREATE TABLE IF NOT EXISTS projects (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                title TEXT NOT NULL,
                language TEXT NOT NULL,
                ai_mode TEXT NOT NULL,
                workflow TEXT NOT NULL,
                created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
            )""")
            db.execute("""CREATE TABLE IF NOT EXISTS sections (
                project_id INTEGER NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
                section_key TEXT NOT NULL,
                content TEXT NOT NULL DEFAULT '',
                PRIMARY KEY(project_id, section_key)
            )""")
            # Add newly introduced form fields to existing projects without replacing content.
            for section_key, _label in PROPOSAL_SECTIONS:
                db.execute(
                    "INSERT OR IGNORE INTO sections(project_id, section_key, content) "
                    "SELECT id, ?, '' FROM projects WHERE workflow='proposal'",
                    (section_key,),
                )

    def create_project(self, title: str, language: str, ai_mode: str, workflow: str) -> Project:
        clean_title = title.strip()
        if not clean_title:
            raise ValueError("Project title cannot be empty")
        with self._connect() as db:
            cursor = db.execute(
                "INSERT INTO projects(title, language, ai_mode, workflow) VALUES (?, ?, ?, ?)",
                (clean_title, language, ai_mode, workflow),
            )
            project_id = int(cursor.lastrowid)
            if workflow == "proposal":
                db.executemany(
                    "INSERT INTO sections(project_id, section_key, content) VALUES (?, ?, '')",
                    ((project_id, key) for key, _ in PROPOSAL_SECTIONS),
                )
        return Project(project_id, clean_title, language, ai_mode, workflow)

    def save_section(self, project_id: int, section_key: str, content: str) -> None:
        valid_keys = {key for key, _ in PROPOSAL_SECTIONS}
        if section_key not in valid_keys:
            raise ValueError(f"Unknown proposal section: {section_key}")
        with self._connect() as db:
            exists = db.execute("SELECT 1 FROM projects WHERE id=?", (project_id,)).fetchone()
            if not exists:
                raise KeyError(f"Project {project_id} does not exist")
            db.execute(
                "INSERT INTO sections(project_id, section_key, content) VALUES (?, ?, ?) "
                "ON CONFLICT(project_id, section_key) DO UPDATE SET content=excluded.content",
                (project_id, section_key, content),
            )
            db.execute("UPDATE projects SET updated_at=CURRENT_TIMESTAMP WHERE id=?", (project_id,))

    def load_sections(self, project_id: int) -> dict[str, str]:
        with self._connect() as db:
            rows = db.execute(
                "SELECT section_key, content FROM sections WHERE project_id=?", (project_id,)
            ).fetchall()
        return {row["section_key"]: row["content"] for row in rows}

    def list_projects(self) -> list[Project]:
        with self._connect() as db:
            rows = db.execute(
                "SELECT id,title,language,ai_mode,workflow FROM projects ORDER BY updated_at DESC"
            ).fetchall()
        return [Project(row["id"], row["title"], row["language"], row["ai_mode"], row["workflow"]) for row in rows]
