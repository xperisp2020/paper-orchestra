#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStringList>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <optional>

#include "project_store.h"
#include "document_export.h"

class WelcomeWindow final : public QMainWindow {
public:
    explicit WelcomeWindow(ProjectStore &store, QWidget *parent = nullptr)
        : QMainWindow(parent), store_(store) {
        setObjectName(QStringLiteral("welcomeWindow"));
        setMinimumSize(860, 600);
        pages_ = new QStackedWidget;
        pages_->setObjectName(QStringLiteral("wizardPages"));

        auto *languagePage = makePage(QStringLiteral("languagePage"));
        language_ = new QComboBox;
        language_->setObjectName(QStringLiteral("languageSelector"));
        language_->addItem(QStringLiteral("فارسی"), QStringLiteral("fa"));
        language_->addItem(QStringLiteral("English"), QStringLiteral("en"));
        static_cast<QVBoxLayout *>(languagePage->layout())->addWidget(language_);
        static_cast<QVBoxLayout *>(languagePage->layout())->addStretch();
        pages_->addWidget(languagePage);

        auto *aiPage = makePage(QStringLiteral("aiPage"));
        aiMode_ = new QComboBox;
        aiMode_->setObjectName(QStringLiteral("aiModeSelector"));
        aiMode_->addItem(QStringLiteral("کاملاً رایگان، بدون API"), QStringLiteral("no_api"));
        aiMode_->addItem(QStringLiteral("سرویس‌های دارای طرح رایگان"), QStringLiteral("free_tier"));
        aiMode_->addItem(QStringLiteral("سرویس‌های پولی"), QStringLiteral("paid"));
        aiDescription_ = new QLabel;
        aiDescription_->setWordWrap(true);
        static_cast<QVBoxLayout *>(aiPage->layout())->addWidget(aiMode_);
        static_cast<QVBoxLayout *>(aiPage->layout())->addWidget(aiDescription_);
        static_cast<QVBoxLayout *>(aiPage->layout())->addStretch();
        pages_->addWidget(aiPage);

        auto *taskPage = makePage(QStringLiteral("taskPage"));
        taskMode_ = new QComboBox;
        taskMode_->setObjectName(QStringLiteral("taskModeSelector"));
        taskMode_->addItem(QStringLiteral("تکمیل پروپوزال"), QStringLiteral("proposal"));
        taskMode_->addItem(QStringLiteral("انجام پژوهش"), QStringLiteral("research"));
        projectTitle_ = new QLineEdit;
        projectTitle_->setObjectName(QStringLiteral("projectTitle"));
        taskStatus_ = new QLabel;
        taskStatus_->setWordWrap(true);
        static_cast<QVBoxLayout *>(taskPage->layout())->addWidget(taskMode_);
        static_cast<QVBoxLayout *>(taskPage->layout())->addWidget(projectTitle_);
        static_cast<QVBoxLayout *>(taskPage->layout())->addWidget(taskStatus_);
        static_cast<QVBoxLayout *>(taskPage->layout())->addStretch();
        pages_->addWidget(taskPage);

        auto *summaryPage = makePage(QStringLiteral("summaryPage"));
        summary_ = new QLabel;
        summary_->setObjectName(QStringLiteral("selectionSummary"));
        summary_->setWordWrap(true);
        summaryStatus_ = new QLabel;
        summaryStatus_->setWordWrap(true);
        static_cast<QVBoxLayout *>(summaryPage->layout())->addWidget(summary_);
        static_cast<QVBoxLayout *>(summaryPage->layout())->addWidget(summaryStatus_);
        static_cast<QVBoxLayout *>(summaryPage->layout())->addStretch();
        pages_->addWidget(summaryPage);

        auto *editorPage = makePage(QStringLiteral("proposalEditorPage"));
        auto *editorRow = new QHBoxLayout;
        sectionList_ = new QListWidget;
        sectionList_->setObjectName(QStringLiteral("proposalSectionList"));
        sectionEditor_ = new QTextEdit;
        sectionEditor_->setObjectName(QStringLiteral("proposalSectionEditor"));
        editorRow->addWidget(sectionList_, 1);
        editorRow->addWidget(sectionEditor_, 3);
        static_cast<QVBoxLayout *>(editorPage->layout())->addLayout(editorRow);
        editorStatus_ = new QLabel;
        editorStatus_->setWordWrap(true);
        static_cast<QVBoxLayout *>(editorPage->layout())->addWidget(editorStatus_);
        auto *exportRow = new QHBoxLayout;
        saveSection_ = new QPushButton;
        saveSection_->setObjectName(QStringLiteral("saveSectionButton"));
        exportPdf_ = new QPushButton;
        exportDocx_ = new QPushButton;
        exportPng_ = new QPushButton;
        printDocument_ = new QPushButton;
        exportRow->addWidget(saveSection_);
        exportRow->addWidget(exportPdf_);
        exportRow->addWidget(exportDocx_);
        exportRow->addWidget(exportPng_);
        exportRow->addWidget(printDocument_);
        static_cast<QVBoxLayout *>(editorPage->layout())->addLayout(exportRow);
        pages_->addWidget(editorPage);

        auto *navigation = new QWidget;
        auto *buttons = new QHBoxLayout(navigation);
        back_ = new QPushButton;
        back_->setObjectName(QStringLiteral("backButton"));
        next_ = new QPushButton;
        next_->setObjectName(QStringLiteral("nextButton"));
        buttons->addWidget(back_);
        buttons->addStretch();
        buttons->addWidget(next_);

        auto *root = new QWidget;
        auto *rootLayout = new QVBoxLayout(root);
        rootLayout->addWidget(pages_, 1);
        rootLayout->addWidget(navigation);
        setCentralWidget(root);

        connect(back_, &QPushButton::clicked, this, [this] {
            if (pages_->currentIndex() > 0)
                pages_->setCurrentIndex(pages_->currentIndex() - 1);
            updateNavigation();
        });
        connect(next_, &QPushButton::clicked, this, [this] {
            const int index = pages_->currentIndex();
            if (index < 2) {
                pages_->setCurrentIndex(index + 1);
            } else if (index == 2) {
                taskStatus_->clear();
                updateSummary();
                pages_->setCurrentIndex(3);
            } else if (index == 3) {
                createOrOpenProject();
            }
            updateNavigation();
        });
        connect(language_, &QComboBox::currentIndexChanged, this, [this](int) {
            applyLanguage();
        });
        connect(aiMode_, &QComboBox::currentIndexChanged, this, [this](int) {
            updateAiDescription();
        });
        connect(sectionList_, &QListWidget::currentRowChanged, this, [this](int) {
            loadSelectedSection();
        });
        connect(saveSection_, &QPushButton::clicked, this, [this] {
            saveSelectedSection();
        });
        connect(exportPdf_, &QPushButton::clicked, this, [this] { exportPdf(); });
        connect(exportDocx_, &QPushButton::clicked, this, [this] { exportDocx(); });
        connect(exportPng_, &QPushButton::clicked, this, [this] { exportPng(); });
        connect(printDocument_, &QPushButton::clicked, this, [this] { printDocument(); });

        applyLanguage();
        updateNavigation();
    }

    bool runSmokeTest() {
        if (!language_ || !aiMode_ || !taskMode_ || !pages_ || pages_->count() != 5)
            return false;

        language_->setCurrentIndex(1);
        if (layoutDirection() != Qt::LeftToRight)
            return false;
        language_->setCurrentIndex(0);
        if (layoutDirection() != Qt::RightToLeft)
            return false;

        const QStringList aiIds{QStringLiteral("no_api"), QStringLiteral("free_tier"), QStringLiteral("paid")};
        const QStringList taskIds{QStringLiteral("proposal"), QStringLiteral("research")};
        for (int aiIndex = 0; aiIndex < aiIds.size(); ++aiIndex) {
            for (int taskIndex = 0; taskIndex < taskIds.size(); ++taskIndex) {
                aiMode_->setCurrentIndex(aiIndex);
                taskMode_->setCurrentIndex(taskIndex);
                pages_->setCurrentIndex(1);
                next_->click();
                next_->click();
                if (pages_->currentIndex() != 3 ||
                    !summary_->text().contains(aiMode_->currentText()) ||
                    !summary_->text().contains(taskMode_->currentText()))
                    return false;
                back_->click();
                if (pages_->currentIndex() != 2)
                    return false;
                back_->click();
                if (pages_->currentIndex() != 1)
                    return false;
            }
        }

        language_->setCurrentIndex(0);
        aiMode_->setCurrentIndex(0);
        taskMode_->setCurrentIndex(0);
        projectTitle_->setText(QStringLiteral("SQLite proposal smoke test"));
        pages_->setCurrentIndex(2);
        next_->click();
        if (pages_->currentIndex() != 3)
            return false;
        next_->click();
        if (pages_->currentIndex() != 4 || !projectId_)
            return false;

        int problemRow = -1;
        for (int row = 0; row < sectionList_->count(); ++row) {
            if (sectionList_->item(row)->data(Qt::UserRole).toString() ==
                QStringLiteral("problem_statement")) {
                problemRow = row;
                break;
            }
        }
        if (problemRow < 0)
            return false;
        sectionList_->setCurrentRow(problemRow);
        sectionEditor_->setPlainText(QStringLiteral("Persisted by GUI smoke test"));
        saveSection_->click();
        QString storageError;
        const auto saved = store_.loadSections(*projectId_, &storageError);
        if (saved.value(QStringLiteral("problem_statement")) !=
            QStringLiteral("Persisted by GUI smoke test"))
            return false;

        QTemporaryDir exportData;
        if (!exportData.isValid())
            return false;
        const auto labels = currentSectionLabels();
        const QString pdfPath = exportData.filePath(QStringLiteral("proposal.pdf"));
        const QString docxPath = exportData.filePath(QStringLiteral("proposal.docx"));
        const QString pngDirectory = exportData.filePath(QStringLiteral("proposal_png"));
        if (!DocumentExport::toPdf(saved, labels, projectTitle_->text(), QStringLiteral("fa"),
                                   pdfPath, &storageError)) {
            qCritical().noquote() << "PDF export failed:" << storageError;
            return false;
        }
        if (!DocumentExport::toDocx(saved, labels, projectTitle_->text(), QStringLiteral("fa"),
                                    docxPath, &storageError)) {
            qCritical().noquote() << "DOCX export failed:" << storageError;
            return false;
        }
        if (!DocumentExport::toPngPages(saved, labels, projectTitle_->text(), QStringLiteral("fa"),
                                        pngDirectory, &storageError)) {
            qCritical().noquote() << "PNG export failed:" << storageError;
            return false;
        }
        QFile pdfFile(pdfPath);
        if (!pdfFile.open(QIODevice::ReadOnly) || !pdfFile.read(4).startsWith("%PDF")) {
            qCritical() << "PDF signature validation failed:" << pdfPath;
            return false;
        }
        QFile docxFile(docxPath);
        if (!docxFile.open(QIODevice::ReadOnly)) {
            qCritical() << "DOCX output cannot be opened:" << docxPath;
            return false;
        }
        const QByteArray docxBytes = docxFile.readAll();
        if (!docxBytes.startsWith("PK") ||
            !docxBytes.contains("word/document.xml") ||
            !docxBytes.contains(QByteArray::fromHex("504b0506"))) {
            qCritical() << "DOCX ZIP structure validation failed:" << docxBytes.size();
            return false;
        }
        if (QDir(pngDirectory).entryList({QStringLiteral("*.png")}, QDir::Files).isEmpty()) {
            qCritical() << "PNG page output is empty:" << pngDirectory;
            return false;
        }
        if (DocumentExport::toPdf(saved, labels, projectTitle_->text(), QStringLiteral("fa"),
                                  pdfPath, &storageError))
            return false;

        language_->setCurrentIndex(1);
        pages_->setCurrentIndex(4);
        updateNavigation();
        if (!summary_->text().contains(QStringLiteral("Language: English")))
            return false;
        return true;
    }

private:
    QWidget *makePage(const QString &name) {
        auto *page = new QWidget;
        page->setObjectName(name);
        auto *layout = new QVBoxLayout(page);
        auto *heading = new QLabel;
        heading->setObjectName(name + QStringLiteral("Heading"));
        heading->setWordWrap(true);
        heading->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: 600;"));
        layout->addWidget(heading);
        headings_.append(heading);
        return page;
    }

    QString sectionLabel(const QString &key, bool persian) const {
        static const QMap<QString, QPair<QString, QString>> labels = {
            {QStringLiteral("title_fa"), {QStringLiteral("عنوان فارسی"), QStringLiteral("Persian title")}},
            {QStringLiteral("title_en"), {QStringLiteral("عنوان انگلیسی"), QStringLiteral("English title")}},
            {QStringLiteral("degree"), {QStringLiteral("مقطع تحصیلی"), QStringLiteral("Degree level")}},
            {QStringLiteral("field"), {QStringLiteral("رشته تحصیلی"), QStringLiteral("Academic field")}},
            {QStringLiteral("specialization"), {QStringLiteral("گرایش"), QStringLiteral("Specialization")}},
            {QStringLiteral("supervisor"), {QStringLiteral("استاد راهنما"), QStringLiteral("Supervisor")}},
            {QStringLiteral("consultant"), {QStringLiteral("استاد مشاور"), QStringLiteral("Advisor")}},
            {QStringLiteral("student_name"), {QStringLiteral("نام دانشجو"), QStringLiteral("Student name")}},
            {QStringLiteral("student_id"), {QStringLiteral("شماره دانشجویی"), QStringLiteral("Student ID")}},
            {QStringLiteral("archive_number"), {QStringLiteral("شماره بایگانی"), QStringLiteral("Archive number")}},
            {QStringLiteral("approval_date"), {QStringLiteral("تاریخ تصویب"), QStringLiteral("Approval date")}},
            {QStringLiteral("student_information"), {QStringLiteral("اطلاعات دانشجو"), QStringLiteral("Student information")}},
            {QStringLiteral("supervisor_information"), {QStringLiteral("اطلاعات استاد راهنما"), QStringLiteral("Supervisor information")}},
            {QStringLiteral("academic_records"), {QStringLiteral("سوابق تحصیلی"), QStringLiteral("Academic records")}},
            {QStringLiteral("previous_theses"), {QStringLiteral("پایان‌نامه‌های پیشین"), QStringLiteral("Previous theses")}},
            {QStringLiteral("thesis_specifications"), {QStringLiteral("مشخصات پایان‌نامه"), QStringLiteral("Thesis specifications")}},
            {QStringLiteral("main_question"), {QStringLiteral("پرسش اصلی"), QStringLiteral("Main research question")}},
            {QStringLiteral("problem_statement"), {QStringLiteral("بیان مسئله"), QStringLiteral("Problem statement")}},
            {QStringLiteral("necessity"), {QStringLiteral("اهمیت و ضرورت"), QStringLiteral("Significance and necessity")}},
            {QStringLiteral("literature_review"), {QStringLiteral("پیشینه پژوهش"), QStringLiteral("Literature review")}},
            {QStringLiteral("innovation"), {QStringLiteral("نوآوری"), QStringLiteral("Original contribution")}},
            {QStringLiteral("keywords"), {QStringLiteral("کلیدواژه‌ها"), QStringLiteral("Keywords")}},
            {QStringLiteral("objectives"), {QStringLiteral("اهداف"), QStringLiteral("Objectives")}},
            {QStringLiteral("beneficiaries"), {QStringLiteral("بهره‌وران"), QStringLiteral("Beneficiaries")}},
            {QStringLiteral("questions"), {QStringLiteral("پرسش‌ها"), QStringLiteral("Research questions")}},
            {QStringLiteral("hypotheses"), {QStringLiteral("فرضیه‌ها"), QStringLiteral("Hypotheses")}},
            {QStringLiteral("definitions"), {QStringLiteral("تعاریف"), QStringLiteral("Definitions")}},
            {QStringLiteral("references_fa"), {QStringLiteral("منابع فارسی"), QStringLiteral("Persian references")}},
            {QStringLiteral("references_en"), {QStringLiteral("منابع غیرفارسی"), QStringLiteral("Non-Persian references")}},
            {QStringLiteral("references"), {QStringLiteral("منابع تکمیلی (سازگاری با پرونده‌های قبلی)"), QStringLiteral("Additional references (legacy field)")}},
            {QStringLiteral("azad_challenge"), {QStringLiteral("چالش پایش آزاد"), QStringLiteral("Azad monitoring challenge")}},
            {QStringLiteral("azad_goals"), {QStringLiteral("اهداف پایش آزاد"), QStringLiteral("Azad monitoring goals")}},
            {QStringLiteral("azad_strategy"), {QStringLiteral("راهبرد پایش آزاد"), QStringLiteral("Azad monitoring strategy")}},
            {QStringLiteral("azad_constellation"), {QStringLiteral("منظومه پایش آزاد"), QStringLiteral("Azad monitoring constellation")}},
            {QStringLiteral("azad_landscape"), {QStringLiteral("منظر پایش آزاد"), QStringLiteral("Azad monitoring landscape")}},
            {QStringLiteral("research_program"), {QStringLiteral("برنامه پژوهشی"), QStringLiteral("Research program")}},
            {QStringLiteral("research_goal"), {QStringLiteral("هدف پژوهش"), QStringLiteral("Research goal")}},
            {QStringLiteral("research_relation"), {QStringLiteral("ارتباط پژوهش با برنامه"), QStringLiteral("Research-program alignment")}},
            {QStringLiteral("methodology"), {QStringLiteral("روش‌شناسی"), QStringLiteral("Methodology")}},
            {QStringLiteral("data_collection"), {QStringLiteral("روش گردآوری اطلاعات"), QStringLiteral("Data collection")}},
            {QStringLiteral("research_tools"), {QStringLiteral("ابزارهای پژوهش"), QStringLiteral("Research instruments")}},
            {QStringLiteral("analysis"), {QStringLiteral("روش تحلیل"), QStringLiteral("Analysis method")}},
            {QStringLiteral("timeline"), {QStringLiteral("زمان‌بندی"), QStringLiteral("Timeline")}},
            {QStringLiteral("variables"), {QStringLiteral("متغیرها"), QStringLiteral("Variables")}},
            {QStringLiteral("personnel_cost"), {QStringLiteral("هزینه پرسنلی"), QStringLiteral("Personnel costs")}},
            {QStringLiteral("materials_equipment"), {QStringLiteral("مواد و وسایل"), QStringLiteral("Materials and equipment")}},
            {QStringLiteral("miscellaneous_costs"), {QStringLiteral("هزینه‌های متفرقه"), QStringLiteral("Miscellaneous costs")}},
            {QStringLiteral("total_costs"), {QStringLiteral("جمع کل هزینه‌ها"), QStringLiteral("Total costs")}},
            {QStringLiteral("budget"), {QStringLiteral("برآورد کلی هزینه‌ها (سازگاری با پرونده‌های قبلی)"), QStringLiteral("Overall budget (legacy field)")}},
            {QStringLiteral("supervisor_confirmation"), {QStringLiteral("تأیید استاد راهنما"), QStringLiteral("Supervisor confirmation")}},
            {QStringLiteral("consultant_confirmation"), {QStringLiteral("تأیید استاد مشاور"), QStringLiteral("Advisor confirmation")}},
            {QStringLiteral("project_information"), {QStringLiteral("اطلاعات فرم طرح"), QStringLiteral("Project information")}},
            {QStringLiteral("student_commitment"), {QStringLiteral("تعهدنامه دانشجو"), QStringLiteral("Student undertaking")}},
            {QStringLiteral("supervisor_commitment"), {QStringLiteral("تعهدنامه استاد راهنما"), QStringLiteral("Supervisor undertaking")}},
            {QStringLiteral("researcher_commitment"), {QStringLiteral("سایر تعهدات پژوهش"), QStringLiteral("Researcher undertakings")}},
            {QStringLiteral("committee_opinion"), {QStringLiteral("نظر کمیته تخصصی"), QStringLiteral("Specialist committee opinion")}},
            {QStringLiteral("scientific_evaluation"), {QStringLiteral("ارزیابی علمی"), QStringLiteral("Scientific evaluation")}},
            {QStringLiteral("group_council_minutes"), {QStringLiteral("صورت‌جلسه شورای گروه"), QStringLiteral("Department council minutes")}},
            {QStringLiteral("faculty_research_council_opinion"), {QStringLiteral("نظر شورای پژوهشی دانشکده"), QStringLiteral("Faculty research council opinion")}},
            {QStringLiteral("final_approval"), {QStringLiteral("تأیید نهایی"), QStringLiteral("Final approval")}},
        };
        const auto it = labels.constFind(key);
        if (it == labels.cend())
            return key;
        return persian ? it->first : it->second;
    }

    void applyLanguage() {
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        const auto direction = persian ? Qt::RightToLeft : Qt::LeftToRight;
        setLayoutDirection(direction);
        qApp->setLayoutDirection(direction);
        setWindowTitle(persian ? QStringLiteral("یادآور پژوهش") : QStringLiteral("Yadavarifork"));

        headings_[0]->setText(persian ? QStringLiteral("زبان برنامه") : QStringLiteral("Application language"));
        headings_[1]->setText(persian ? QStringLiteral("حالت هوش مصنوعی") : QStringLiteral("AI mode"));
        headings_[2]->setText(persian ? QStringLiteral("نوع فعالیت و عنوان پروژه") : QStringLiteral("Workspace and project title"));
        headings_[3]->setText(persian ? QStringLiteral("بازبینی انتخاب‌ها") : QStringLiteral("Review selections"));
        headings_[4]->setText(persian ? QStringLiteral("ویرایش پروپوزال") : QStringLiteral("Proposal editor"));

        language_->setItemText(0, QStringLiteral("فارسی"));
        language_->setItemText(1, QStringLiteral("English"));
        aiMode_->setItemText(0, persian ? QStringLiteral("کاملاً رایگان، بدون API") : QStringLiteral("Free, no API"));
        aiMode_->setItemText(1, persian ? QStringLiteral("سرویس‌های دارای طرح رایگان") : QStringLiteral("Free-tier AI services"));
        aiMode_->setItemText(2, persian ? QStringLiteral("سرویس‌های پولی") : QStringLiteral("Paid AI services"));
        taskMode_->setItemText(0, persian ? QStringLiteral("تکمیل پروپوزال") : QStringLiteral("Complete proposal"));
        taskMode_->setItemText(1, persian ? QStringLiteral("انجام پژوهش") : QStringLiteral("Conduct research"));
        projectTitle_->setPlaceholderText(persian ? QStringLiteral("عنوان پروژه") : QStringLiteral("Project title"));
        back_->setText(persian ? QStringLiteral("قبلی") : QStringLiteral("Back"));
        saveSection_->setText(persian ? QStringLiteral("ذخیره بخش") : QStringLiteral("Save section"));
        exportPdf_->setText(persian ? QStringLiteral("خروجی PDF") : QStringLiteral("Export PDF"));
        exportDocx_->setText(persian ? QStringLiteral("خروجی Word") : QStringLiteral("Export Word"));
        exportPng_->setText(persian ? QStringLiteral("خروجی تصویر") : QStringLiteral("Export PNG"));
        printDocument_->setText(persian ? QStringLiteral("چاپ") : QStringLiteral("Print"));
        updateAiDescription();
        updateSummary();
        populateSectionList();
        updateNavigation();
    }

    void updateAiDescription() {
        const QString mode = aiMode_->currentData().toString();
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        if (mode == QStringLiteral("no_api")) {
            aiDescription_->setText(persian
                ? QStringLiteral("ابزارهای محلی و قواعد قطعی؛ بدون تولید متن توسط مدل زبانی.")
                : QStringLiteral("Local tools and deterministic rules; no language-model generation."));
        } else if (mode == QStringLiteral("free_tier")) {
            aiDescription_->setText(persian
                ? QStringLiteral("اتصال اختیاری به سرویس‌های دارای طرح رایگان؛ وابسته به سهمیه و دسترسی.")
                : QStringLiteral("Optional free-tier provider access, subject to availability and quotas."));
        } else {
            aiDescription_->setText(persian
                ? QStringLiteral("اتصال اختیاری به ارائه‌دهندگان پولی؛ کلیدها باید پیش از استفاده امن‌سازی شوند.")
                : QStringLiteral("Optional paid-provider access; secure credential storage is required before use."));
        }
    }

    void updateSummary() {
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        if (persian) {
            summary_->setText(QStringLiteral("عنوان: %1\nزبان: %2\nحالت هوش مصنوعی: %3\nفعالیت: %4")
                .arg(projectTitle_->text().trimmed(), language_->currentText(),
                     aiMode_->currentText(), taskMode_->currentText()));
        } else {
            summary_->setText(QStringLiteral("Project: %1\nLanguage: %2\nAI mode: %3\nWorkspace: %4")
                .arg(projectTitle_->text().trimmed(), language_->currentText(),
                     aiMode_->currentText(), taskMode_->currentText()));
        }
    }

    void populateSectionList() {
        if (!sectionList_)
            return;
        QString currentKey;
        if (sectionList_->currentItem())
            currentKey = sectionList_->currentItem()->data(Qt::UserRole).toString();
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        sectionList_->clear();
        const QStringList keys = ProjectStore::proposalSectionKeys();
        for (const QString &key : keys) {
            auto *item = new QListWidgetItem(sectionLabel(key, persian));
            item->setData(Qt::UserRole, key);
            sectionList_->addItem(item);
            if (key == currentKey)
                sectionList_->setCurrentItem(item);
        }
        if (sectionList_->currentRow() < 0 && sectionList_->count() > 0)
            sectionList_->setCurrentRow(0);
    }

    void loadSelectedSection() {
        if (!projectId_ || !sectionList_->currentItem())
            return;
        QString error;
        const auto sections = store_.loadSections(*projectId_, &error);
        if (!error.isEmpty()) {
            editorStatus_->setText(error);
            return;
        }
        const QString key = sectionList_->currentItem()->data(Qt::UserRole).toString();
        sectionEditor_->setPlainText(sections.value(key));
    }

    void saveSelectedSection() {
        if (!projectId_ || !sectionList_->currentItem())
            return;
        const QString key = sectionList_->currentItem()->data(Qt::UserRole).toString();
        QString error;
        if (!store_.saveSection(*projectId_, key, sectionEditor_->toPlainText(), &error)) {
            editorStatus_->setText(error);
            return;
        }
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        editorStatus_->setText(persian ? QStringLiteral("ذخیره شد") : QStringLiteral("Saved"));
    }


    QMap<QString, QString> currentSectionLabels() const {
        QMap<QString, QString> labels;
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        for (const QString &key : ProjectStore::proposalSectionKeys())
            labels.insert(key, sectionLabel(key, persian));
        return labels;
    }

    QMap<QString, QString> currentSections(QString *error = nullptr) {
        if (!projectId_) {
            if (error) *error = QStringLiteral("No active project.");
            return {};
        }
        return store_.loadSections(*projectId_, error);
    }

    void exportPdf() {
        QString error;
        const auto sections = currentSections(&error);
        if (!error.isEmpty()) { editorStatus_->setText(error); return; }
        const QString path = QFileDialog::getSaveFileName(
            this, QStringLiteral("Export PDF"), QStringLiteral("proposal.pdf"),
            QStringLiteral("PDF files (*.pdf)"));
        if (path.isEmpty()) return;
        if (DocumentExport::toPdf(sections, currentSectionLabels(), projectTitle_->text(),
                                  language_->currentData().toString(), path, &error))
            editorStatus_->setText(language_->currentData().toString() == "fa"
                ? QStringLiteral("فایل PDF ذخیره شد: %1").arg(path)
                : QStringLiteral("PDF saved: %1").arg(path));
        else
            editorStatus_->setText(error);
    }

    void exportDocx() {
        QString error;
        const auto sections = currentSections(&error);
        if (!error.isEmpty()) { editorStatus_->setText(error); return; }
        const QString path = QFileDialog::getSaveFileName(
            this, QStringLiteral("Export Word document"), QStringLiteral("proposal.docx"),
            QStringLiteral("Word documents (*.docx)"));
        if (path.isEmpty()) return;
        if (DocumentExport::toDocx(sections, currentSectionLabels(), projectTitle_->text(),
                                   language_->currentData().toString(), path, &error))
            editorStatus_->setText(language_->currentData().toString() == "fa"
                ? QStringLiteral("فایل Word ذخیره شد: %1").arg(path)
                : QStringLiteral("Word file saved: %1").arg(path));
        else
            editorStatus_->setText(error);
    }

    void exportPng() {
        QString error;
        const auto sections = currentSections(&error);
        if (!error.isEmpty()) { editorStatus_->setText(error); return; }
        const QString path = QFileDialog::getSaveFileName(
            this, QStringLiteral("Export PNG pages"), QStringLiteral("proposal_pages"),
            QStringLiteral("New PNG page folder (*)"));
        if (path.isEmpty()) return;
        if (DocumentExport::toPngPages(sections, currentSectionLabels(), projectTitle_->text(),
                                       language_->currentData().toString(), path, &error))
            editorStatus_->setText(language_->currentData().toString() == "fa"
                ? QStringLiteral("تصاویر صفحات ذخیره شدند: %1").arg(path)
                : QStringLiteral("PNG pages saved: %1").arg(path));
        else
            editorStatus_->setText(error);
    }

    void printDocument() {
        QString error;
        const auto sections = currentSections(&error);
        if (!error.isEmpty()) { editorStatus_->setText(error); return; }
        if (!DocumentExport::print(sections, currentSectionLabels(), projectTitle_->text(),
                                   language_->currentData().toString(), this, &error) &&
            error != QStringLiteral("Printing was cancelled."))
            editorStatus_->setText(error);
    }

    void createOrOpenProject() {
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        if (projectId_) {
            if (taskMode_->currentData().toString() == QStringLiteral("proposal")) {
                populateSectionList();
                pages_->setCurrentIndex(4);
            }
            updateNavigation();
            return;
        }
        QString error;
        projectId_ = store_.createProject(
            projectTitle_->text(), language_->currentData().toString(),
            aiMode_->currentData().toString(), taskMode_->currentData().toString(), &error);
        if (!projectId_) {
            summaryStatus_->setText(error.isEmpty()
                ? (persian ? QStringLiteral("ایجاد پروژه ناموفق بود.") : QStringLiteral("Project creation failed."))
                : error);
            return;
        }
        if (taskMode_->currentData().toString() == QStringLiteral("proposal")) {
            populateSectionList();
            pages_->setCurrentIndex(4);
        } else {
            summaryStatus_->setText(persian
                ? QStringLiteral("پروژه پژوهش ذخیره شد؛ اتصال عامل‌های پژوهش هنوز در دست توسعه است.")
                : QStringLiteral("Research project saved; research-agent integration is still under development."));
        }
        updateNavigation();
    }

    void updateNavigation() {
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        const int index = pages_->currentIndex();
        back_->setEnabled(index > 0);
        const bool researchCreated = index == 3 && projectId_ &&
            taskMode_->currentData().toString() == QStringLiteral("research");
        next_->setEnabled(index < 4 && !researchCreated);
        if (index == 2)
            next_->setText(persian ? QStringLiteral("بازبینی") : QStringLiteral("Review"));
        else if (index == 3)
            next_->setText(projectId_
                ? (persian ? QStringLiteral("باز کردن پروپوزال") : QStringLiteral("Open proposal"))
                : (persian ? QStringLiteral("ایجاد پروژه") : QStringLiteral("Create project")));
        else
            next_->setText(persian ? QStringLiteral("بعدی") : QStringLiteral("Next"));
    }

    ProjectStore &store_;
    QStackedWidget *pages_{};
    QComboBox *language_{};
    QComboBox *aiMode_{};
    QComboBox *taskMode_{};
    QLineEdit *projectTitle_{};
    QLabel *aiDescription_{};
    QLabel *taskStatus_{};
    QLabel *summary_{};
    QLabel *summaryStatus_{};
    QListWidget *sectionList_{};
    QTextEdit *sectionEditor_{};
    QLabel *editorStatus_{};
    QPushButton *back_{};
    QPushButton *next_{};
    QPushButton *saveSection_{};
    QPushButton *exportPdf_{};
    QPushButton *exportDocx_{};
    QPushButton *exportPng_{};
    QPushButton *printDocument_{};
    QList<QLabel *> headings_;
    std::optional<qint64> projectId_;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    if (app.arguments().contains(QStringLiteral("--smoke-test"))) {
        QTemporaryDir temporaryData;
        if (!temporaryData.isValid())
            return 3;
        ProjectStore store(temporaryData.filePath(QStringLiteral("smoke.sqlite3")));
        QString error;
        if (!store.initialize(&error))
            return 4;
        WelcomeWindow window(store);
        return window.runSmokeTest() ? 0 : 2;
    }

    const QString dataDirectory = QDir(QCoreApplication::applicationDirPath())
                                      .filePath(QStringLiteral("data"));
    if (!QDir().mkpath(dataDirectory)) {
        QMessageBox::critical(nullptr, QStringLiteral("Yadavarifork"),
                              QStringLiteral("Cannot create the portable data directory."));
        return 5;
    }
    ProjectStore store(QDir(dataDirectory).filePath(QStringLiteral("projects.sqlite3")));
    QString error;
    if (!store.initialize(&error)) {
        QMessageBox::critical(nullptr, QStringLiteral("Yadavarifork"), error);
        return 6;
    }
    WelcomeWindow window(store);
    window.show();
    return app.exec();
}
