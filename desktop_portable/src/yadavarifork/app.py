"""Qt desktop wizard for the first portable implementation slice."""
from __future__ import annotations
import os
from pathlib import Path
import sys

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QApplication, QComboBox, QDialog, QFormLayout, QHBoxLayout, QLabel,
    QLineEdit, QListWidget, QMainWindow, QMessageBox, QPushButton,
    QStackedWidget, QTextEdit, QVBoxLayout, QWidget,
)

from .domain import PROPOSAL_SECTIONS, ProjectStore

PERSIAN = {
    "app": "یادآور پژوهش", "language": "زبان برنامه را انتخاب کنید",
    "next": "ادامه", "back": "بازگشت", "ai": "روش استفاده از هوش مصنوعی را انتخاب کنید",
    "free": "کاملاً رایگان و بدون API", "free_note": "ابزارهای محلی و قواعد قطعی؛ بدون تولید متن توسط مدل زبانی.",
    "tier": "سرویس‌های دارای طرح رایگان", "tier_note": "اتصال به سرویس‌های رایگان، در صورت دسترسی و با محدودیت سهمیه.",
    "paid": "سرویس‌های پولی", "paid_note": "اتصال اختیاری به ارائه‌دهندگان پولی؛ کلیدها بعداً امن‌سازی می‌شوند.",
    "workflow": "چه کاری می‌خواهید انجام دهید؟", "proposal": "تکمیل پروپوزال",
    "research": "انجام پژوهش", "title": "عنوان پروژه", "create": "ایجاد پروژه",
    "save": "ذخیره بخش", "saved": "ذخیره شد", "research_soon": "اتصال عامل‌های پژوهش در گام بعدی توسعه انجام می‌شود.",
    "required": "لطفاً عنوان پروژه را وارد کنید.", "sections": "بخش‌های پروپوزال",
}
ENGLISH = {
    "app": "Yadavarifork Research", "language": "Choose the application language",
    "next": "Continue", "back": "Back", "ai": "Choose the AI usage mode", "free": "Free, no API",
    "free_note": "Local tools and deterministic rules; no language-model generation.",
    "tier": "Providers with free tiers", "tier_note": "Optional access to free-tier services, subject to availability and quotas.",
    "paid": "Paid AI providers", "paid_note": "Optional paid-provider integration; secure key storage will be added later.",
    "workflow": "What would you like to do?", "proposal": "Complete a proposal",
    "research": "Conduct research", "title": "Project title", "create": "Create project",
    "save": "Save section", "saved": "Saved", "research_soon": "Research-agent integration is scheduled for a subsequent development step.",
    "required": "Please enter a project title.", "sections": "Proposal sections",
}


EN_SECTION_LABELS = {
    "title_fa": "Persian title",
    "title_en": "English title",
    "degree": "Degree level",
    "field": "Academic field",
    "specialization": "Specialization",
    "supervisor": "Supervisor",
    "consultant": "Advisor",
    "student_name": "Student name",
    "student_id": "Student ID",
    "archive_number": "Archive number",
    "approval_date": "Approval date",
    "student_information": "Student information",
    "supervisor_information": "Supervisor information",
    "academic_records": "Academic records",
    "previous_theses": "Previous theses",
    "thesis_specifications": "Thesis specifications",
    "main_question": "Main research question",
    "problem_statement": "Problem statement",
    "necessity": "Significance and necessity",
    "literature_review": "Literature review",
    "innovation": "Original contribution",
    "keywords": "Keywords",
    "objectives": "Objectives",
    "beneficiaries": "Beneficiaries",
    "questions": "Research questions",
    "hypotheses": "Hypotheses",
    "definitions": "Definitions",
    "references_fa": "Persian references",
    "references_en": "Non-Persian references",
    "references": "Additional references (legacy field)",
    "azad_challenge": "Azad monitoring challenge",
    "azad_goals": "Azad monitoring goals",
    "azad_strategy": "Azad monitoring strategy",
    "azad_constellation": "Azad monitoring constellation",
    "azad_landscape": "Azad monitoring landscape",
    "research_program": "Research program",
    "research_goal": "Research goal",
    "research_relation": "Research-program alignment",
    "methodology": "Methodology",
    "data_collection": "Data collection",
    "research_tools": "Research instruments",
    "analysis": "Analysis method",
    "timeline": "Timeline",
    "variables": "Variables",
    "personnel_cost": "Personnel costs",
    "materials_equipment": "Materials and equipment",
    "miscellaneous_costs": "Miscellaneous costs",
    "total_costs": "Total costs",
    "budget": "Overall budget (legacy field)",
    "supervisor_confirmation": "Supervisor confirmation",
    "consultant_confirmation": "Advisor confirmation",
    "project_information": "Project information",
    "student_commitment": "Student undertaking",
    "supervisor_commitment": "Supervisor undertaking",
    "researcher_commitment": "Researcher undertakings",
    "committee_opinion": "Specialist committee opinion",
    "scientific_evaluation": "Scientific evaluation",
    "group_council_minutes": "Department council minutes",
    "faculty_research_council_opinion": "Faculty research council opinion",
    "final_approval": "Final approval",
}

class MainWindow(QMainWindow):
    def __init__(self, store: ProjectStore):
        super().__init__()
        self.store = store
        self.language = "fa"
        self.ai_mode = "no_api"
        self.workflow = "proposal"
        self.project_id: int | None = None
        self.texts = PERSIAN
        self.page_headings: list[QLabel] = []
        self.setWindowTitle(self.texts["app"])
        self.resize(1080, 720)
        self.stack = QStackedWidget()
        self.setCentralWidget(self.stack)
        self._build_language_page()
        self._build_ai_page()
        self._build_workflow_page()
        self._build_editor_page()
        self.stack.setCurrentIndex(0)

    def t(self, key: str) -> str:
        return self.texts[key]

    def _page(self, heading: str) -> tuple[QWidget, QVBoxLayout]:
        page = QWidget()
        layout = QVBoxLayout(page)
        label = QLabel(heading)
        label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        label.setStyleSheet("font-size: 24px; font-weight: 600; padding: 24px;")
        layout.addWidget(label)
        self.page_headings.append(label)
        self.stack.addWidget(page)
        return page, layout

    def _build_language_page(self):
        page, layout = self._page(self.t("language"))
        self.language_combo = QComboBox()
        self.language_combo.addItem("فارسی", "fa")
        self.language_combo.addItem("English", "en")
        layout.addWidget(self.language_combo)
        self.language_next_button = QPushButton(self.t("next"))
        self.language_next_button.clicked.connect(self._choose_language)
        layout.addWidget(self.language_next_button)
        layout.addStretch()

    def _choose_language(self):
        self.language = str(self.language_combo.currentData())
        self.texts = PERSIAN if self.language == "fa" else ENGLISH
        self.setWindowTitle(self.t("app"))
        self.stack.setCurrentIndex(1)
        self._refresh_labels()

    def _build_ai_page(self):
        page, layout = self._page(self.t("ai"))
        self.ai_combo = QComboBox()
        for label, value in ((self.t("free"), "no_api"), (self.t("tier"), "free_tier"), (self.t("paid"), "paid")):
            self.ai_combo.addItem(label, value)
        self.ai_description = QLabel(self.t("free_note"))
        self.ai_description.setWordWrap(True)
        self.ai_combo.currentIndexChanged.connect(self._update_ai_description)
        layout.addWidget(self.ai_combo)
        layout.addWidget(self.ai_description)
        navigation = QHBoxLayout()
        self.ai_back_button = QPushButton(self.t("back"))
        self.ai_back_button.clicked.connect(lambda: self.stack.setCurrentIndex(0))
        self.ai_next_button = QPushButton(self.t("next"))
        self.ai_next_button.clicked.connect(lambda: self.stack.setCurrentIndex(2))
        navigation.addWidget(self.ai_back_button)
        navigation.addWidget(self.ai_next_button)
        layout.addLayout(navigation)
        layout.addStretch()

    def _update_ai_description(self):
        notes = {"no_api": "free_note", "free_tier": "tier_note", "paid": "paid_note"}
        self.ai_description.setText(self.t(notes[str(self.ai_combo.currentData())]))

    def _build_workflow_page(self):
        page, layout = self._page(self.t("workflow"))
        self.workflow_combo = QComboBox()
        self.workflow_combo.addItem(self.t("proposal"), "proposal")
        self.workflow_combo.addItem(self.t("research"), "research")
        self.project_title = QLineEdit()
        self.project_title.setPlaceholderText(self.t("title"))
        form = QFormLayout()
        self.title_form_label = QLabel(self.t("title"))
        form.addRow(self.title_form_label, self.project_title)
        layout.addWidget(self.workflow_combo)
        layout.addLayout(form)
        navigation = QHBoxLayout()
        self.workflow_back_button = QPushButton(self.t("back"))
        self.workflow_back_button.clicked.connect(lambda: self.stack.setCurrentIndex(1))
        self.create_project_button = QPushButton(self.t("create"))
        self.create_project_button.clicked.connect(self._create_project)
        navigation.addWidget(self.workflow_back_button)
        navigation.addWidget(self.create_project_button)
        layout.addLayout(navigation)
        layout.addStretch()

    def _create_project(self):
        title = self.project_title.text().strip()
        if not title:
            QMessageBox.warning(self, self.t("app"), self.t("required"))
            return
        self.ai_mode = str(self.ai_combo.currentData())
        self.workflow = str(self.workflow_combo.currentData())
        project = self.store.create_project(title, self.language, self.ai_mode, self.workflow)
        self.project_id = project.project_id
        if self.workflow == "research":
            QMessageBox.information(self, self.t("app"), self.t("research_soon"))
            return
        self._load_project_sections()
        self.stack.setCurrentIndex(3)

    def _build_editor_page(self):
        page, layout = self._page(self.t("sections"))
        editor_row = QHBoxLayout()
        self.section_list = QListWidget()
        self.section_editor = QTextEdit()
        self.section_list.currentRowChanged.connect(self._select_section)
        editor_row.addWidget(self.section_list, 1)
        editor_row.addWidget(self.section_editor, 3)
        layout.addLayout(editor_row)
        navigation = QHBoxLayout()
        self.editor_back_button = QPushButton(self.t("back"))
        self.editor_back_button.clicked.connect(lambda: self.stack.setCurrentIndex(2))
        self.editor_save_button = QPushButton(self.t("save"))
        self.editor_save_button.clicked.connect(self._save_section)
        navigation.addWidget(self.editor_back_button)
        navigation.addWidget(self.editor_save_button)
        layout.addLayout(navigation)

    def _load_project_sections(self):
        self.section_list.clear()
        labels = EN_SECTION_LABELS if self.language == "en" else dict(PROPOSAL_SECTIONS)
        for key, _label in PROPOSAL_SECTIONS:
            self.section_list.addItem(labels[key])
            self.section_list.item(self.section_list.count() - 1).setData(Qt.ItemDataRole.UserRole, key)
        if self.section_list.count():
            self.section_list.setCurrentRow(0)

    def _select_section(self, row: int):
        if self.project_id is None or row < 0:
            return
        key = self.section_list.item(row).data(Qt.ItemDataRole.UserRole)
        self.section_editor.setPlainText(self.store.load_sections(self.project_id).get(key, ""))

    def _save_section(self):
        row = self.section_list.currentRow()
        if self.project_id is None or row < 0:
            return
        key = self.section_list.item(row).data(Qt.ItemDataRole.UserRole)
        self.store.save_section(self.project_id, key, self.section_editor.toPlainText())
        QMessageBox.information(self, self.t("app"), self.t("saved"))

    def _refresh_labels(self):
        headings = ("language", "ai", "workflow", "sections")
        for heading, label in zip(headings, self.page_headings):
            label.setText(self.t(heading))
        self.language_next_button.setText(self.t("next"))
        self.ai_back_button.setText(self.t("back"))
        self.ai_next_button.setText(self.t("next"))
        self.workflow_back_button.setText(self.t("back"))
        self.create_project_button.setText(self.t("create"))
        self.editor_back_button.setText(self.t("back"))
        self.editor_save_button.setText(self.t("save"))
        self.title_form_label.setText(self.t("title"))
        self.ai_combo.setItemText(0, self.t("free"))
        self.ai_combo.setItemText(1, self.t("tier"))
        self.ai_combo.setItemText(2, self.t("paid"))
        self.workflow_combo.setItemText(0, self.t("proposal"))
        self.workflow_combo.setItemText(1, self.t("research"))
        self.project_title.setPlaceholderText(self.t("title"))
        if self.project_id is not None:
            self._load_project_sections()
        self.setLayoutDirection(
            Qt.LayoutDirection.RightToLeft if self.language == "fa"
            else Qt.LayoutDirection.LeftToRight
        )
        self._update_ai_description()


def data_directory() -> Path:
    # Keep portable data beside the executable; source runs use the project directory.
    if getattr(sys, "frozen", False):
        return Path(sys.executable).resolve().parent / "data"
    return Path(os.environ.get("YADAVARIFORK_DATA", Path.home() / ".yadavarifork"))


def main() -> int:
    app = QApplication(sys.argv)
    store = ProjectStore(data_directory() / "projects.sqlite3")
    window = MainWindow(store)
    window.show()
    return app.exec()

if __name__ == "__main__":
    raise SystemExit(main())
