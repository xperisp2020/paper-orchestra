"""Domain models and local persistence; deliberately independent of Qt."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import json
import sqlite3
from typing import Iterable

PROPOSAL_SECTIONS: tuple[tuple[str, str], ...] = (
    ("title_fa", "عنوان فارسی"), ("title_en", "عنوان انگلیسی"),
    ("field", "رشته و گرایش"), ("main_question", "پرسش اصلی"),
    ("problem_statement", "بیان مسئله"), ("necessity", "اهمیت و ضرورت"),
    ("literature_review", "پیشینه پژوهش"), ("innovation", "نوآوری"),
    ("keywords", "کلیدواژه‌ها"), ("objectives", "اهداف"),
    ("beneficiaries", "بهره‌وران"), ("questions", "پرسش‌ها"),
    ("hypotheses", "فرضیه‌ها"), ("definitions", "تعاریف"),
    ("references", "منابع"), ("methodology", "روش‌شناسی"),
    ("data_collection", "روش گردآوری اطلاعات"), ("analysis", "روش تحلیل"),
    ("variables", "متغیرها"), ("timeline", "زمان‌بندی"),
    ("budget", "برآورد هزینه‌ها"),
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

    def _connect(self) -> sqlite3.Connection:
        connection = sqlite3.connect(self.database)
        connection.row_factory = sqlite3.Row
        return connection

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
