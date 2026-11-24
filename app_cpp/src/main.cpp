#include <algorithm>

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QFile>
#include <QSaveFile>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <iostream>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QSet>
#include <QSignalBlocker>
#include <QStringList>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QRegularExpression>

#include <optional>

#include "project_store.h"
#include "document_export.h"
#include "research_engine_runner.h"

#include <QDateTime>
#include <QEventLoop>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>
#include <QThread>

class WelcomeWindow final : public QMainWindow {
public:
    explicit WelcomeWindow(ProjectStore &store, QWidget *parent = nullptr)
        : QMainWindow(parent), store_(store) {
        setObjectName(QStringLiteral("welcomeWindow"));
        setWindowIcon(QIcon(QStringLiteral(":/brand/shiraztipsy.svg")));
        setMinimumSize(920, 640);
        pages_ = new QStackedWidget;
        pages_->setObjectName(QStringLiteral("wizardPages"));

        auto *languagePage = makePage(QStringLiteral("languagePage"));
        language_ = new QComboBox;
        language_->setObjectName(QStringLiteral("languageSelector"));
        language_->addItem(QStringLiteral("فارسی"), QStringLiteral("fa"));
        language_->addItem(QStringLiteral("English"), QStringLiteral("en"));
        static_cast<QVBoxLayout *>(languagePage->layout())->addWidget(language_);
        auto *savedProjectActions = new QHBoxLayout;
        openExistingProject_ = new QPushButton;
        openExistingProject_->setObjectName(QStringLiteral("openExistingProjectButton"));
        newProject_ = new QPushButton;
        newProject_->setObjectName(QStringLiteral("newProjectButton"));
        savedProjectActions->addWidget(openExistingProject_);
        savedProjectActions->addWidget(newProject_);
        static_cast<QVBoxLayout *>(languagePage->layout())->addLayout(savedProjectActions);
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
        editorNotice_ = new QLabel;
        editorNotice_->setObjectName(QStringLiteral("proposalAdministrativeNotice"));
        editorNotice_->setWordWrap(true);
        static_cast<QVBoxLayout *>(editorPage->layout())->addWidget(editorNotice_);
        auto *exportRow = new QHBoxLayout;
        finalDetailsButton_ = new QPushButton;
        finalDetailsButton_->setObjectName(QStringLiteral("proposalFinalDetailsButton"));
        saveSection_ = new QPushButton;
        saveSection_->setObjectName(QStringLiteral("saveSectionButton"));
        exportPdf_ = new QPushButton;
        exportDocx_ = new QPushButton;
        exportPng_ = new QPushButton;
        printDocument_ = new QPushButton;
        exportRow->addWidget(finalDetailsButton_);
        exportRow->addWidget(saveSection_);
        exportRow->addWidget(exportPdf_);
        exportRow->addWidget(exportDocx_);
        exportRow->addWidget(exportPng_);
        exportRow->addWidget(printDocument_);
        static_cast<QVBoxLayout *>(editorPage->layout())->addLayout(exportRow);
        pages_->addWidget(editorPage);

        auto *researchPage = makePage(QStringLiteral("researchWorkspacePage"));
        auto *researchEditorRow = new QHBoxLayout;
        researchSectionList_ = new QListWidget;
        researchSectionList_->setObjectName(QStringLiteral("researchSectionList"));
        researchEditor_ = new QTextEdit;
        researchEditor_->setObjectName(QStringLiteral("researchSectionEditor"));
        researchEditorRow->addWidget(researchSectionList_, 1);
        researchEditorRow->addWidget(researchEditor_, 3);
        static_cast<QVBoxLayout *>(researchPage->layout())->addLayout(researchEditorRow, 3);

        researchSourcesGroup_ = new QGroupBox;
        auto *sourceLayout = new QVBoxLayout(researchSourcesGroup_);
        researchSourcesList_ = new QListWidget;
        researchSourcesList_->setObjectName(QStringLiteral("researchSourcesList"));
        auto *sourceButtons = new QHBoxLayout;
        addResearchSource_ = new QPushButton;
        addResearchSource_->setObjectName(QStringLiteral("addResearchSourceButton"));
        deleteResearchSource_ = new QPushButton;
        deleteResearchSource_->setObjectName(QStringLiteral("deleteResearchSourceButton"));
        saveResearchSection_ = new QPushButton;
        saveResearchSection_->setObjectName(QStringLiteral("saveResearchSectionButton"));
        importScholarCitations_ = new QPushButton;
        importScholarCitations_->setObjectName(QStringLiteral("importScholarCitationsButton"));
        sourceButtons->addWidget(saveResearchSection_);
        sourceButtons->addWidget(importScholarCitations_);
        sourceButtons->addWidget(addResearchSource_);
        sourceButtons->addWidget(deleteResearchSource_);
        sourceLayout->addWidget(researchSourcesList_);
        sourceLayout->addLayout(sourceButtons);
        static_cast<QVBoxLayout *>(researchPage->layout())->addWidget(researchSourcesGroup_, 2);
        auto *engineButtons = new QHBoxLayout;
        runResearchEngine_ = new QPushButton;
        runResearchEngine_->setObjectName(QStringLiteral("runResearchEngineButton"));
        cancelResearchEngine_ = new QPushButton;
        cancelResearchEngine_->setObjectName(QStringLiteral("cancelResearchEngineButton"));
        cancelResearchEngine_->setEnabled(false);
        researchRunHistory_ = new QPushButton;
        researchRunHistory_->setObjectName(QStringLiteral("researchRunHistoryButton"));
        engineButtons->addWidget(runResearchEngine_);
        engineButtons->addWidget(cancelResearchEngine_);
        engineButtons->addWidget(researchRunHistory_);
        static_cast<QVBoxLayout *>(researchPage->layout())->addLayout(engineButtons);
        researchStatus_ = new QLabel;
        researchStatus_->setWordWrap(true);
        static_cast<QVBoxLayout *>(researchPage->layout())->addWidget(researchStatus_);
        engineRunner_ = new ResearchEngineRunner(this);
        connect(runResearchEngine_, &QPushButton::clicked, this, [this] { configureAndRunResearchEngine(); });
        connect(cancelResearchEngine_, &QPushButton::clicked, this, [this] {
            if (engineRunner_ && engineRunner_->isRunning()) {
                researchRunCancellationRequested_ = true;
                writeResearchRunManifest(QStringLiteral("cancelling"), QStringLiteral("cancel_requested"));
                engineRunner_->cancel();
            }
        });
        connect(researchRunHistory_, &QPushButton::clicked, this, [this] {
            showResearchRunHistory();
        });
        connect(engineRunner_, &ResearchEngineRunner::started, this, [this] {
            writeResearchRunManifest(QStringLiteral("running"), QStringLiteral("process_started"));
            runResearchEngine_->setEnabled(false);
            cancelResearchEngine_->setEnabled(true);
            researchStatus_->setText(language_->currentData().toString() == QStringLiteral("fa")
                ? QStringLiteral("موتور پژوهش در حال اجراست؛ خروجی‌ها به‌صورت زنده نمایش داده می‌شوند.")
                : QStringLiteral("Research engine is running; output is streamed below."));
        });
        connect(engineRunner_, &ResearchEngineRunner::outputLine, this, [this](const QString &line) {
            QString displayLine = line;
            const QJsonDocument eventDocument = QJsonDocument::fromJson(line.toUtf8());
            if (eventDocument.isObject()) {
                const QJsonObject event = eventDocument.object();
                if (event.contains(QStringLiteral("ok")) && event.contains(QStringLiteral("count"))) {
                    if (!event.value(QStringLiteral("ok")).toBool()) {
                        displayLine = event.value(QStringLiteral("message")).toString(
                            language_->currentData().toString() == QStringLiteral("fa")
                                ? QStringLiteral("منابع کافی بازیابی نشدند؛ محتوای ناقص ذخیره نشد.")
                                : QStringLiteral("Not enough sources were retrieved; incomplete content was not saved."));
                    } else {
                        displayLine = language_->currentData().toString() == QStringLiteral("fa")
                            ? QStringLiteral("تعداد منابع بازیابی‌شده: %1").arg(event.value(QStringLiteral("count")).toInt())
                            : QStringLiteral("Retrieved sources: %1").arg(event.value(QStringLiteral("count")).toInt());
                    }
                }
                const QString eventName = event.value(QStringLiteral("event")).toString();
                if (!eventName.isEmpty()) {
                    if (eventName == QStringLiteral("pipeline_started")) {
                        writeResearchRunManifest(QStringLiteral("running"),
                            event.value(QStringLiteral("stage")).toString(QStringLiteral("paper_generation")));
                        displayLine = language_->currentData().toString() == QStringLiteral("fa")
                            ? QStringLiteral("رویداد: آغاز تولید مقاله")
                            : QStringLiteral("Event: paper generation started");
                    } else if (eventName == QStringLiteral("pipeline_completed")) {
                        writeResearchRunManifest(QStringLiteral("completed"),
                            event.value(QStringLiteral("stage")).toString(QStringLiteral("completed")));
                        displayLine = language_->currentData().toString() == QStringLiteral("fa")
                            ? QStringLiteral("رویداد: تولید مقاله پایان یافت")
                            : QStringLiteral("Event: paper generation completed");
                    } else if (eventName == QStringLiteral("pipeline_failed")) {
                        writeResearchRunManifest(QStringLiteral("failed"),
                            event.value(QStringLiteral("stage")).toString(QStringLiteral("pipeline_failed")));
                        displayLine = event.value(QStringLiteral("error")).toString();
                    }
                }
            }
            const QString current = researchStatus_->text();
            const QString combined = current + (current.isEmpty() ? QString() : QStringLiteral("\n")) + displayLine;
            researchStatus_->setText(combined.right(6000));
        });
        connect(engineRunner_, &ResearchEngineRunner::finished, this, [this](int exitCode, bool normalExit) {
            runResearchEngine_->setEnabled(true);
            cancelResearchEngine_->setEnabled(false);
            bool success = normalExit && exitCode == 0;
            if (success && noApiResearchRun_) success = importNoApiResearchResult();
            const bool wasNoApiRun = noApiResearchRun_;
            noApiResearchRun_ = false;
            const QString finalRunStatus = success ? QStringLiteral("completed") :
                (researchRunCancellationRequested_ ? QStringLiteral("cancelled") : QStringLiteral("failed"));
            const bool manifestRecorded = writeResearchRunManifest(
                finalRunStatus, success ? QStringLiteral("completed") : finalRunStatus);
            bool outputRecorded = false;
            if (success && manifestRecorded && currentResearchRunProjectId_ &&
                !currentResearchRunOutput_.isEmpty() && QFileInfo(currentResearchRunOutput_).isDir()) {
                outputRecorded = store_.saveProjectMetadata(
                    *currentResearchRunProjectId_, QStringLiteral("research_engine_last_output"),
                    currentResearchRunOutput_, nullptr);
            }
            currentResearchRunOutput_.clear();
            currentResearchRunProjectId_.reset();
            researchRunCancellationRequested_ = false;
            const QString resultMessage = success
                ? (wasNoApiRun
                    ? (language_->currentData().toString() == QStringLiteral("fa")
                        ? QStringLiteral("جست‌وجوی علمی رایگان تکمیل شد؛ %1 منبع یکتا و بخش‌های پژوهش ذخیره شدند.").arg(noApiImportedSourceCount_)
                        : QStringLiteral("Free scholarly discovery completed; %1 unique sources and research sections were saved.").arg(noApiImportedSourceCount_))
                    : (outputRecorded
                    ? (language_->currentData().toString() == QStringLiteral("fa")
                        ? QStringLiteral("موتور با موفقیت پایان یافت؛ مسیر خروجی در پروژه ثبت شد.")
                        : QStringLiteral("Engine completed successfully; output path saved in the project."))
                    : (language_->currentData().toString() == QStringLiteral("fa")
                        ? QStringLiteral("فرایند با کد صفر پایان یافت، اما پوشه خروجی یا ثبت مسیر آن تأیید نشد.")
                        : QStringLiteral("Process exited with code zero, but output folder or metadata persistence was not verified."))))
                : (language_->currentData().toString() == QStringLiteral("fa")
                    ? QStringLiteral("اجرای موتور موفق نبود؛ کد خروج: %1").arg(exitCode)
                    : QStringLiteral("Engine run failed; exit code: %1").arg(exitCode));
            researchStatus_->setText(researchStatus_->text() + QStringLiteral("\n") + resultMessage);
        });
        connect(engineRunner_, &ResearchEngineRunner::failed, this, [this](const QString &message) {
            runResearchEngine_->setEnabled(true);
            cancelResearchEngine_->setEnabled(false);
            writeResearchRunManifest(QStringLiteral("failed"), QStringLiteral("process_start_failed"));
            noApiResearchRun_ = false;
            currentResearchRunOutput_.clear();
            currentResearchRunProjectId_.reset();
            researchRunCancellationRequested_ = false;
            researchStatus_->setText(researchStatus_->text() + QStringLiteral("\n") + message);
        });
        pages_->addWidget(researchPage);

        auto *navigation = new QWidget;
        auto *buttons = new QHBoxLayout(navigation);
        back_ = new QPushButton;
        back_->setObjectName(QStringLiteral("backButton"));
        about_ = new QPushButton;
        about_->setObjectName(QStringLiteral("aboutInstituteButton"));
        next_ = new QPushButton;
        next_->setObjectName(QStringLiteral("nextButton"));
        buttons->addWidget(back_);
        buttons->addWidget(about_);
        buttons->addStretch();
        buttons->addWidget(next_);

        auto *root = new QWidget;
        auto *rootLayout = new QVBoxLayout(root);
        rootLayout->addWidget(pages_, 1);
        rootLayout->addWidget(navigation);
        setCentralWidget(root);

        connect(about_, &QPushButton::clicked, this, [this] { showAboutInstitute(); });
        connect(back_, &QPushButton::clicked, this, [this] {
            const int index = pages_->currentIndex();
            if (index == 4 && !saveActiveProposalSection())
                return;
            if (index == 5 && !saveActiveResearchSection())
                return;
            if (index >= 4)
                pages_->setCurrentIndex(3);
            else if (index > 0)
                pages_->setCurrentIndex(index - 1);
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
        connect(openExistingProject_, &QPushButton::clicked, this, [this] {
            openExistingProject();
        });
        connect(newProject_, &QPushButton::clicked, this, [this] {
            startNewProject();
        });
        connect(aiMode_, &QComboBox::currentIndexChanged, this, [this](int) {
            updateAiDescription();
        });
        connect(sectionList_, &QListWidget::currentRowChanged, this, [this](int) {
            loadSelectedSection();
        });
        connect(researchSectionList_, &QListWidget::currentRowChanged, this, [this](int) {
            loadSelectedResearchSection();
        });
        connect(sectionEditor_, &QTextEdit::textChanged, this, [this] {
            if (!loadingProposalSection_ && !currentProposalSectionKey_.isEmpty())
                proposalSectionDirty_ = true;
        });
        connect(researchEditor_, &QTextEdit::textChanged, this, [this] {
            if (!loadingResearchSection_ && !currentResearchSectionKey_.isEmpty())
                researchSectionDirty_ = true;
        });
        connect(saveResearchSection_, &QPushButton::clicked, this, [this] {
            saveSelectedResearchSection();
        });
        connect(addResearchSource_, &QPushButton::clicked, this, [this] {
            addResearchSource();
        });
        connect(importScholarCitations_, &QPushButton::clicked, this, [this] {
            importScholarCitations();
        });
        connect(deleteResearchSource_, &QPushButton::clicked, this, [this] {
            deleteResearchSource();
        });
        connect(saveSection_, &QPushButton::clicked, this, [this] {
            saveSelectedSection();
        });
        connect(finalDetailsButton_, &QPushButton::clicked, this, [this] {
            collectFinalDetails(true);
        });
        connect(exportPdf_, &QPushButton::clicked, this, [this] { exportPdf(); });
        connect(exportDocx_, &QPushButton::clicked, this, [this] { exportDocx(); });
        connect(exportPng_, &QPushButton::clicked, this, [this] { exportPng(); });
        connect(printDocument_, &QPushButton::clicked, this, [this] { printDocument(); });

        applyLanguage();
        updateNavigation();
    }

    int runSmokeTest() {
        if (!language_ || !aiMode_ || !taskMode_ || !pages_ || pages_->count() != 6)
            return 20;

        language_->setCurrentIndex(1);
        if (layoutDirection() != Qt::LeftToRight)
            return 21;
        language_->setCurrentIndex(0);
        if (layoutDirection() != Qt::RightToLeft)
            return 22;

        // The visible New Project button must advance from the welcome page.
        newProject_->click();
        if (pages_->currentIndex() != 1 || !projectTitle_->text().isEmpty())
            return 79;
        back_->click();
        if (pages_->currentIndex() != 0)
            return 80;

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
                    return 23;
                back_->click();
                if (pages_->currentIndex() != 2)
                    return 24;
                back_->click();
                if (pages_->currentIndex() != 1)
                    return 25;
            }
        }

        language_->setCurrentIndex(0);
        aiMode_->setCurrentIndex(0);
        taskMode_->setCurrentIndex(0);
        projectTitle_->setText(QStringLiteral("SQLite proposal smoke test"));
        pages_->setCurrentIndex(2);
        next_->click();
        if (pages_->currentIndex() != 3)
            return 26;
        next_->click();
        if (pages_->currentIndex() != 4 || !projectId_)
            return 27;

        int problemRow = -1;
        for (int row = 0; row < sectionList_->count(); ++row) {
            if (sectionList_->item(row)->data(Qt::UserRole).toString() ==
                QStringLiteral("problem_statement")) {
                problemRow = row;
                break;
            }
        }
        if (problemRow < 0)
            return 28;
        for (int row = 0; row < sectionList_->count(); ++row) {
            const QString visibleKey = sectionList_->item(row)->data(Qt::UserRole).toString();
            if (administrativeSectionKeys().contains(visibleKey) ||
                finalDetailKeys().contains(visibleKey))
                return 86;
        }
        sectionList_->setCurrentRow(problemRow);
        sectionEditor_->setPlainText(QStringLiteral("Persisted by GUI smoke test"));
        sectionList_->setCurrentRow(problemRow == 0 ? 1 : 0);
        sectionList_->setCurrentRow(problemRow);
        if (sectionEditor_->toPlainText() != QStringLiteral("Persisted by GUI smoke test"))
            return 29;
        QString storageError;
        const auto saved = store_.loadSections(*projectId_, &storageError);
        if (saved.value(QStringLiteral("problem_statement")) !=
                QStringLiteral("Persisted by GUI smoke test") ||
            saved.value(QStringLiteral("title_fa")) != projectTitle_->text())
            return 29;
        for (const QString &key : finalDetailKeys()) {
            if (!saved.value(key).isEmpty())
                return 60;
        }
        for (const QString &key : administrativeSectionKeys()) {
            if (!saved.value(key).isEmpty())
                return 61;
        }
        bool skipDialogHandled = false;
        QTimer::singleShot(0, [&skipDialogHandled] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            auto *skip = dialog ? dialog->findChild<QPushButton *>(
                QStringLiteral("skipFinalDetailsButton")) : nullptr;
            if (skip) {
                skipDialogHandled = true;
                skip->click();
            }
        });
        finalDetailsButton_->click();
        if (!skipDialogHandled || !finalDetailsReviewed_ ||
            store_.loadProjectMetadata(*projectId_, QStringLiteral("final_details_status"),
                                       &storageError) != QStringLiteral("skipped"))
            return 66;
        const auto stillEmpty = store_.loadSections(*projectId_, &storageError);
        if (!storageError.isEmpty())
            return 67;
        for (const QString &key : finalDetailKeys()) {
            if (!stillEmpty.value(key).isEmpty())
                return 68;
        }
        QMap<QString, QString> smokeFinalDetails;
        for (const QString &key : finalDetailKeys())
            smokeFinalDetails.insert(key, QStringLiteral("Smoke test: ") + key);
        smokeFinalDetails.insert(QStringLiteral("student_information"),
            QStringLiteral("Smoke test: student_information\nSecond line\nمتن فارسی & <بررسی>"));
        smokeFinalDetails.insert(QStringLiteral("supervisor_information"),
            QStringLiteral("Smoke test: supervisor_information\nSecond line\nمتن فارسی & <بررسی استاد>"));
        smokeFinalDetails.insert(QStringLiteral("academic_records"),
            QStringLiteral("Smoke test: academic_records\nAcademic record line"));
        smokeFinalDetails.insert(QStringLiteral("previous_theses"),
            QStringLiteral("Smoke test: previous_theses\nPrevious thesis line"));
        bool saveDialogHandled = false;
        QTimer::singleShot(0, [&saveDialogHandled, &smokeFinalDetails] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            bool fieldsFound = true;
            for (auto it = smokeFinalDetails.cbegin(); it != smokeFinalDetails.cend(); ++it) {
                const QString objectName = QStringLiteral("finalDetail_") + it.key();
                if (auto *line = dialog->findChild<QLineEdit *>(objectName)) {
                    line->setText(it.value());
                } else if (auto *multi = dialog->findChild<QTextEdit *>(objectName)) {
                    multi->setPlainText(it.value());
                } else {
                    fieldsFound = false;
                    break;
                }
            }
            auto *buttons = dialog->findChild<QDialogButtonBox *>();
            if (fieldsFound && buttons && buttons->button(QDialogButtonBox::Save)) {
                saveDialogHandled = true;
                buttons->button(QDialogButtonBox::Save)->click();
            }
        });
        finalDetailsButton_->click();
        if (!saveDialogHandled)
            return 62;
        WelcomeWindow reopenedProposal(store_);
        if (!reopenedProposal.openProjectById(*projectId_) ||
            !reopenedProposal.finalDetailsReviewed_)
            return 65;
        const auto detailsSaved = store_.loadSections(*projectId_, &storageError);
        if (!storageError.isEmpty() ||
            store_.loadProjectMetadata(*projectId_, QStringLiteral("final_details_status"),
                                       &storageError) != QStringLiteral("completed") ||
            !storageError.isEmpty())
            return 63;
        for (const QString &key : finalDetailKeys()) {
            if (detailsSaved.value(key) != smokeFinalDetails.value(key))
                return 63;
        }
        for (const QString &key : administrativeSectionKeys()) {
            if (!detailsSaved.value(key).isEmpty())
                return 64;
        }

        if (!DocumentExport::validateProposalLayout(&storageError)) {
            std::cerr << "Proposal page layout validation failed: " << storageError.toStdString() << std::endl;
            return 40;
        }
        QTemporaryDir exportData;
        if (!exportData.isValid())
            return 30;
        QString exportRoot = exportData.path();
        const QString configuredExportRoot = qEnvironmentVariable("YADAVARIFORK_SMOKE_OUTPUT_DIR");
        if (!configuredExportRoot.isEmpty()) {
            if (QFileInfo::exists(configuredExportRoot) ||
                !QDir().mkpath(configuredExportRoot)) {
                std::cerr << "Smoke export directory is not new or cannot be created: "
                          << configuredExportRoot.toStdString() << std::endl;
                return 41;
            }
            exportRoot = configuredExportRoot;
        }
        const auto labels = currentSectionLabels();
        const QString pdfPath = QDir(exportRoot).filePath(QStringLiteral("proposal.pdf"));
        const QString docxPath = QDir(exportRoot).filePath(QStringLiteral("proposal.docx"));
        const QString pngDirectory = QDir(exportRoot).filePath(QStringLiteral("proposal_png"));
        if (!DocumentExport::toPdf(detailsSaved, labels, projectTitle_->text(), QStringLiteral("fa"),
                                   pdfPath, &storageError)) {
            qCritical().noquote() << "PDF export failed:" << storageError;
            std::cerr << "PDF export failed: " << storageError.toStdString() << std::endl;
            return 31;
        }
        if (!DocumentExport::toDocx(detailsSaved, labels, projectTitle_->text(), QStringLiteral("fa"),
                                    docxPath, &storageError)) {
            qCritical().noquote() << "DOCX export failed:" << storageError;
            std::cerr << "DOCX export failed: " << storageError.toStdString() << std::endl;
            return 32;
        }
        if (!DocumentExport::toPngPages(detailsSaved, labels, projectTitle_->text(), QStringLiteral("fa"),
                                        pngDirectory, &storageError)) {
            qCritical().noquote() << "PNG export failed:" << storageError;
            return 33;
        }
        QFile pdfFile(pdfPath);
        if (!pdfFile.open(QIODevice::ReadOnly) || !pdfFile.read(4).startsWith("%PDF")) {
            qCritical() << "PDF signature validation failed:" << pdfPath;
            return 34;
        }
        QFile docxFile(docxPath);
        if (!docxFile.open(QIODevice::ReadOnly)) {
            qCritical() << "DOCX output cannot be opened:" << docxPath;
            return 35;
        }
        const QByteArray docxBytes = docxFile.readAll();
        if (!docxBytes.startsWith("PK") ||
            !docxBytes.contains("word/document.xml") ||
            !docxBytes.contains(QByteArray::fromHex("504b0506")) ||
            !docxBytes.contains("Smoke test: student_information") ||
            !docxBytes.contains("Smoke test: supervisor_information") ||
            !docxBytes.contains("Second line")) {
            qCritical() << "DOCX ZIP structure validation failed:" << docxBytes.size();
            return 36;
        }
        if (QDir(pngDirectory).entryList({QStringLiteral("*.png")}, QDir::Files).isEmpty()) {
            qCritical() << "PNG page output is empty:" << pngDirectory;
            return 37;
        }

        const QString jpgBase = QDir(exportRoot).filePath(QStringLiteral("proposal_jpg.jpg"));
        if (!DocumentExport::toImagePages(detailsSaved, labels, projectTitle_->text(),
                                          QStringLiteral("fa"), jpgBase, QStringLiteral("jpg"),
                                          &storageError))
            return 81;
        QFile jpgPage(QDir(exportRoot).filePath(QStringLiteral("proposal_jpg_page_001.jpg")));
        if (!jpgPage.open(QIODevice::ReadOnly) ||
            !jpgPage.read(3).startsWith(QByteArray::fromHex("ffd8ff")))
            return 82;

        if (imageFormatFromFilter(QStringLiteral("JPEG images (*.jpg *.jpeg)")) != QStringLiteral("jpg") ||
            imageFormatFromFilter(QStringLiteral("PNG images (*.png)")) != QStringLiteral("png") ||
            imageFormatFromFilter(QStringLiteral("BMP images (*.bmp)")) != QStringLiteral("bmp"))
            return 85;

        const QString bmpBase = QDir(exportRoot).filePath(QStringLiteral("proposal_bmp.bmp"));
        if (!DocumentExport::toImagePages(detailsSaved, labels, projectTitle_->text(),
                                          QStringLiteral("fa"), bmpBase, QStringLiteral("bmp"),
                                          &storageError))
            return 83;
        QFile bmpPage(QDir(exportRoot).filePath(QStringLiteral("proposal_bmp_page_001.bmp")));
        if (!bmpPage.open(QIODevice::ReadOnly) || !bmpPage.read(2).startsWith("BM"))
            return 84;
        if (DocumentExport::toPdf(saved, labels, projectTitle_->text(), QStringLiteral("fa"),
                                  pdfPath, &storageError))
            return 38;

        language_->setCurrentIndex(1);
        pages_->setCurrentIndex(4);
        updateNavigation();
        if (!summary_->text().contains(QStringLiteral("Language: English")))
            return 39;

        WelcomeWindow researchWindow(store_);
        if (researchWindow.pages_->count() != 6)
            return 42;
        researchWindow.projectTitle_->setText(QStringLiteral("Offline research workspace smoke test"));
        researchWindow.taskMode_->setCurrentIndex(1);
        researchWindow.pages_->setCurrentIndex(2);
        researchWindow.next_->click();
        researchWindow.next_->click();
        if (researchWindow.pages_->currentIndex() != 5 || !researchWindow.projectId_ ||
            *researchWindow.projectId_ == *projectId_)
            return 43;
        if (researchWindow.researchSectionList_->count() != 11)
            return 44;

        int questionRow = -1;
        for (int row = 0; row < researchWindow.researchSectionList_->count(); ++row) {
            if (researchWindow.researchSectionList_->item(row)->data(Qt::UserRole).toString() ==
                QStringLiteral("research_question")) {
                questionRow = row;
                break;
            }
        }
        if (questionRow < 0)
            return 45;
        researchWindow.researchSectionList_->setCurrentRow(questionRow);
        researchWindow.researchEditor_->setPlainText(QStringLiteral("A locally saved research question."));
        researchWindow.researchSectionList_->setCurrentRow(questionRow == 0 ? 1 : 0);
        researchWindow.researchSectionList_->setCurrentRow(questionRow);
        if (researchWindow.researchEditor_->toPlainText() !=
            QStringLiteral("A locally saved research question."))
            return 46;
        storageError = QStringLiteral("stale error from previous operation");
        const auto researchSections = store_.loadResearchSections(*researchWindow.projectId_, &storageError);
        if (!storageError.isEmpty() ||
            researchSections.value(QStringLiteral("research_question")) !=
                QStringLiteral("A locally saved research question.") ||
            !store_.loadResearchSections(*projectId_).isEmpty())
            return 46;

        ResearchSource source;
        source.title = QStringLiteral("User-entered source for smoke test");
        source.authors = QStringLiteral("Researcher");
        source.publicationYear = QStringLiteral("2024");
        source.doi = QStringLiteral("10.0000/example-unverified");
        source.url = QStringLiteral("https://example.invalid/source");
        const auto sourceId = store_.addResearchSource(*researchWindow.projectId_, source, &storageError);
        if (!sourceId || !storageError.isEmpty()) {
            std::cerr << "addResearchSource smoke failure: hasId=" << bool(sourceId)
                      << " id=" << sourceId.value_or(-1)
                      << " error=" << storageError.toStdString() << std::endl;
            return 47;
        }
        const auto sources = store_.loadResearchSources(*researchWindow.projectId_, &storageError);
        if (!storageError.isEmpty() || sources.size() != 1 ||
            sources.first().verificationStatus != QStringLiteral("user_provided_unverified"))
            return 48;
        researchWindow.refreshResearchSources();
        if (researchWindow.researchSourcesList_->count() != 1)
            return 49;
        if (!store_.deleteResearchSource(*researchWindow.projectId_, *sourceId, &storageError) ||
            !store_.loadResearchSources(*researchWindow.projectId_).isEmpty())
            return 50;
        researchWindow.back_->click();
        if (researchWindow.pages_->currentIndex() != 3)
            return 51;

        const auto allProjects = store_.listProjects(&storageError);
        if (!storageError.isEmpty() || allProjects.size() != 2)
            return 52;
        WelcomeWindow reopenedResearch(store_);
        if (!reopenedResearch.openProjectById(*researchWindow.projectId_) ||
            reopenedResearch.pages_->currentIndex() != 5 ||
            reopenedResearch.projectTitle_->text() != QStringLiteral("Offline research workspace smoke test") ||
            reopenedResearch.researchSectionList_->count() != 11)
            return 53;
        reopenedResearch.researchEditor_->setPlainText(QStringLiteral("Saved when research window closes."));
        if (!reopenedResearch.close())
            return 56;
        storageError.clear();
        const auto closedResearchSections = store_.loadResearchSections(*researchWindow.projectId_, &storageError);
        if (!storageError.isEmpty() ||
            closedResearchSections.value(QStringLiteral("research_question")) !=
                QStringLiteral("Saved when research window closes."))
            return 57;

        if (!reopenedProposal.openProjectById(*projectId_) ||
            reopenedProposal.pages_->currentIndex() != 4 ||
            reopenedProposal.sectionList_->count() !=
                ProjectStore::proposalSectionKeys().size() - finalDetailKeys().size() -
                    administrativeSectionKeys().size())
            return 54;
        reopenedProposal.sectionEditor_->setPlainText(QStringLiteral("Saved when proposal window closes."));
        if (!reopenedProposal.close())
            return 58;
        storageError.clear();
        const auto closedProposalSections = store_.loadSections(*projectId_, &storageError);
        if (!storageError.isEmpty() ||
            closedProposalSections.value(QStringLiteral("title_fa")) !=
                QStringLiteral("Saved when proposal window closes."))
            return 59;
        reopenedResearch.startNewProject();
        if (reopenedResearch.projectId_ || !reopenedResearch.language_->isEnabled() ||
            !reopenedResearch.taskMode_->isEnabled() || reopenedResearch.pages_->currentIndex() != 1)
            return 55;

        reopenedResearch.currentResearchRunOutput_ =
            exportData.filePath(QStringLiteral("research_runs/project_manifest_smoke"));
        reopenedResearch.currentResearchRunProjectId_ = researchWindow.projectId_;
        if (!reopenedResearch.writeResearchRunManifest(QStringLiteral("starting"),
                                                        QStringLiteral("smoke_test")))
            return 73;
        const QString manifestPath = QDir(reopenedResearch.currentResearchRunOutput_)
                                         .filePath(QStringLiteral("run_manifest.json"));
        QFile manifestFile(manifestPath);
        if (!manifestFile.open(QIODevice::ReadOnly))
            return 74;
        QJsonDocument manifestDocument = QJsonDocument::fromJson(manifestFile.readAll());
        manifestFile.close();
        if (!manifestDocument.isObject() ||
            manifestDocument.object().value(QStringLiteral("status")).toString() != QStringLiteral("starting") ||
            manifestDocument.object().value(QStringLiteral("project_id")).toString() !=
                QString::number(*researchWindow.projectId_))
            return 75;
        if (!reopenedResearch.writeResearchRunManifest(QStringLiteral("completed"),
                                                       QStringLiteral("smoke_finished")))
            return 76;
        if (!manifestFile.open(QIODevice::ReadOnly))
            return 77;
        manifestDocument = QJsonDocument::fromJson(manifestFile.readAll());
        if (!manifestDocument.isObject() ||
            manifestDocument.object().value(QStringLiteral("status")).toString() != QStringLiteral("completed"))
            return 78;
        reopenedResearch.currentResearchRunOutput_.clear();
        reopenedResearch.currentResearchRunProjectId_.reset();

        ResearchEngineRunner bridge;
        QStringList bridgeOutput;
        bool bridgeFinished = false;
        int bridgeExitCode = -1;
        QObject::connect(&bridge, &ResearchEngineRunner::outputLine, &bridge,
                         [&](const QString &line) { bridgeOutput.append(line); });
        QObject::connect(&bridge, &ResearchEngineRunner::finished, &bridge,
                         [&](int code, bool normalExit) {
                             bridgeFinished = normalExit;
                             bridgeExitCode = code;
                         });
        QProcessEnvironment bridgeEnvironment = QProcessEnvironment::systemEnvironment();
        bridgeEnvironment.insert(QStringLiteral("GEMINI_API_KEY"), QStringLiteral("bridge-smoke-secret"));
        if (!bridge.start(QCoreApplication::applicationFilePath(),
                          {QStringLiteral("--bridge-child-test")},
                          QDir::tempPath(), bridgeEnvironment))
            return 69;
        QEventLoop bridgeLoop;
        QObject::connect(&bridge, &ResearchEngineRunner::finished, &bridgeLoop, &QEventLoop::quit);
        QTimer::singleShot(10000, &bridgeLoop, &QEventLoop::quit);
        bridgeLoop.exec();
        const QString bridgeText = bridgeOutput.join(QStringLiteral("\n"));
        if (!bridgeFinished || bridgeExitCode != 0 ||
            !bridgeText.contains(QStringLiteral("bridge-child-ok")) ||
            !bridgeText.contains(QStringLiteral("GEMINI_API_KEY=[REDACTED]")) ||
            bridgeText.contains(QStringLiteral("bridge-smoke-secret")))
            return 70;

        ResearchEngineRunner cancellableBridge;
        bool cancellationFinished = false;
        bool cancellationChildStarted = false;
        QStringList cancellationOutput;
        QObject::connect(&cancellableBridge, &ResearchEngineRunner::outputLine,
                         &cancellableBridge, [&](const QString &line) { cancellationOutput.append(line); });
        QObject::connect(&cancellableBridge, &ResearchEngineRunner::started,
                         &cancellableBridge, [&] {
            cancellationChildStarted = true;
            QTimer::singleShot(100, &cancellableBridge, [&cancellableBridge] {
                cancellableBridge.cancel();
            });
        });
        QObject::connect(&cancellableBridge, &ResearchEngineRunner::finished,
                         &cancellableBridge, [&](int, bool) { cancellationFinished = true; });
        if (!cancellableBridge.start(QCoreApplication::applicationFilePath(),
                                     {QStringLiteral("--bridge-child-sleep-test")},
                                     QDir::tempPath()))
            return 71;
        QEventLoop cancellationLoop;
        QObject::connect(&cancellableBridge, &ResearchEngineRunner::finished,
                         &cancellationLoop, &QEventLoop::quit);
        QTimer::singleShot(8000, &cancellationLoop, &QEventLoop::quit);
        cancellationLoop.exec();
        if (!cancellationChildStarted || !cancellationFinished || cancellableBridge.isRunning() ||
            !cancellationOutput.join(QStringLiteral("\\n")).contains(QStringLiteral("bridge-child-sleeping")))
            return 72;
        return 0;
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        if (engineRunner_ && engineRunner_->isRunning()) {
            researchRunCancellationRequested_ = true;
            writeResearchRunManifest(QStringLiteral("cancelling"), QStringLiteral("cancel_requested"));
            engineRunner_->cancel();
            event->ignore();
            researchStatus_->setText(language_->currentData().toString() == QStringLiteral("fa")
                ? QStringLiteral("ابتدا لغو اجرای موتور تکمیل شود؛ سپس برنامه را ببندید.")
                : QStringLiteral("Wait for engine cancellation to finish before closing the application."));
            return;
        }
        if (!saveActiveProposalSection() || !saveActiveResearchSection()) {
            event->ignore();
            return;
        }
        QMainWindow::closeEvent(event);
    }

private:
    void showAboutInstitute() {
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        QDialog dialog(this);
        dialog.setObjectName(QStringLiteral("aboutInstituteDialog"));
        dialog.setWindowTitle(persian ? QStringLiteral("درباره پژوهشکده سرخوش شیرازی")
                                      : QStringLiteral("About ShirazTipsy Research Institute"));
        dialog.setWindowIcon(QIcon(QStringLiteral(":/brand/shiraztipsy.svg")));
        dialog.setMinimumWidth(540);
        auto *layout = new QVBoxLayout(&dialog);
        auto *brand = new QLabel;
        brand->setAlignment(Qt::AlignCenter);
        brand->setTextFormat(Qt::RichText);
        brand->setText(persian
            ? QStringLiteral("<div style='font-size:23px;font-weight:700;color:#f4c56a'>پژوهشکده سرخوش شیرازی</div><div style='font-size:16px;color:#9bc9ff'>ShirazTipsy Research Institute</div>")
            : QStringLiteral("<div style='font-size:23px;font-weight:700;color:#f4c56a'>ShirazTipsy Research Institute</div><div style='font-size:16px;color:#9bc9ff'>پژوهشکده سرخوش شیرازی</div>"));
        layout->addWidget(brand);
        auto *description = new QLabel(persian
            ? QStringLiteral("پژوهشکده سرخوش شیرازی با هدف پشتیبانی از پژوهشگران و دانشجویان، تسهیل فرایند نگارش و مدیریت پژوهش و بهره‌گیری مسئولانه از فناوری‌های نوین و هوش مصنوعی فعالیت می‌کند. این نرم‌افزار ابزار پژوهشی است و مسئولیت بررسی علمی منابع و محتوای نهایی بر عهده پژوهشگر خواهد بود.")
            : QStringLiteral("ShirazTipsy Research Institute supports researchers and students with research planning, academic writing, source management, and responsible use of modern technologies and AI. This application is a research aid; researchers remain responsible for verifying sources and final scholarly content."));
        description->setWordWrap(true);
        description->setAlignment(Qt::AlignCenter);
        description->setMinimumHeight(100);
        layout->addWidget(description);
        auto *contactTitle = new QLabel(persian ? QStringLiteral("راه‌های ارتباطی") : QStringLiteral("Connect with us"));
        contactTitle->setAlignment(Qt::AlignCenter);
        contactTitle->setStyleSheet(QStringLiteral("font-weight:700;color:#f4c56a;padding-top:8px;"));
        layout->addWidget(contactTitle);
        auto *links = new QLabel(QStringLiteral("<div align='center'><a href='https://t.me/shiraztipsy'>Telegram: @shiraztipsy</a><br/><a href='https://www.instagram.com/shiraztipsy/'>Instagram: @shiraztipsy</a></div>"));
        links->setTextFormat(Qt::RichText);
        links->setTextInteractionFlags(Qt::TextBrowserInteraction);
        links->setOpenExternalLinks(true);
        links->setAlignment(Qt::AlignCenter);
        layout->addWidget(links);
        auto *close = new QPushButton(persian ? QStringLiteral("بستن") : QStringLiteral("Close"));
        close->setObjectName(QStringLiteral("aboutCloseButton"));
        connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);
        layout->addWidget(close);
        dialog.exec();
    }

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
        setWindowTitle(persian ? QStringLiteral("پژوهشکده سرخوش شیرازی") : QStringLiteral("ShirazTipsy Research Institute"));
        about_->setText(persian ? QStringLiteral("درباره ما") : QStringLiteral("About us"));

        headings_[0]->setText(persian ? QStringLiteral("زبان برنامه") : QStringLiteral("Application language"));
        headings_[1]->setText(persian ? QStringLiteral("حالت هوش مصنوعی") : QStringLiteral("AI mode"));
        headings_[2]->setText(persian ? QStringLiteral("نوع فعالیت و عنوان پروژه") : QStringLiteral("Workspace and project title"));
        headings_[3]->setText(persian ? QStringLiteral("بازبینی انتخاب‌ها") : QStringLiteral("Review selections"));
        headings_[4]->setText(persian ? QStringLiteral("ویرایش پروپوزال") : QStringLiteral("Proposal editor"));
        headings_[5]->setText(persian ? QStringLiteral("کارگاه پژوهش محلی") : QStringLiteral("Local research workspace"));
        editorNotice_->setText(persian
            ? QStringLiteral("مشخصات دانشجو و استاد در مرحله نهایی دریافت می‌شود. بخش‌های اداری، تأییدیه‌ها، امضاها و شماره‌های بایگانی عمداً خالی می‌مانند تا بعداً توسط شما تکمیل شوند.")
            : QStringLiteral("Student and supervisor details are collected at the final step. Administrative fields, approvals, signatures and archive numbers are intentionally left blank for you to complete later."));
        finalDetailsButton_->setText(persian
            ? QStringLiteral("تکمیل مشخصات فردی و استاد")
            : QStringLiteral("Complete student and supervisor details"));

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
        exportPng_->setText(persian ? QStringLiteral("خروجی تصویر") : QStringLiteral("Export images"));
        printDocument_->setText(persian ? QStringLiteral("چاپ") : QStringLiteral("Print"));
        openExistingProject_->setText(persian ? QStringLiteral("بازکردن پروژه ذخیره‌شده") : QStringLiteral("Open saved project"));
        newProject_->setText(persian ? QStringLiteral("پروژه جدید") : QStringLiteral("New project"));
        saveResearchSection_->setText(persian ? QStringLiteral("ذخیره بخش") : QStringLiteral("Save section"));
        addResearchSource_->setText(persian ? QStringLiteral("افزودن دستی منبع") : QStringLiteral("Add source manually"));
        importScholarCitations_->setText(persian ? QStringLiteral("ورود گروهی منابع از Google Scholar (Cite)") :
                                                   QStringLiteral("Bulk import Google Scholar citations (Cite)"));
        deleteResearchSource_->setText(persian ? QStringLiteral("حذف منبع") : QStringLiteral("Delete source"));
        runResearchEngine_->setText(persian ? QStringLiteral("اجرای موتور چندعاملی") : QStringLiteral("Run multi-agent engine"));
        cancelResearchEngine_->setText(persian ? QStringLiteral("لغو اجرا") : QStringLiteral("Cancel run"));
        researchRunHistory_->setText(persian ? QStringLiteral("تاریخچه اجراها") : QStringLiteral("Run history"));
        researchSourcesGroup_->setTitle(persian ? QStringLiteral("منابع ثبت‌شده — تأییدنشده") : QStringLiteral("Recorded sources — unverified"));
        updateAiDescription();
        updateSummary();
        populateSectionList();
        populateResearchSectionList();
        if (projectId_ && taskMode_->currentData().toString() == QStringLiteral("research"))
            refreshResearchSources();
        updateNavigation();
    }

    void updateAiDescription() {
        const QString mode = aiMode_->currentData().toString();
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        if (mode == QStringLiteral("no_api")) {
            aiDescription_->setText(persian
                ? QStringLiteral("جست‌وجوی رایگان منابع علمی از OpenAlex، Crossref، Semantic Scholar، Europe PMC، DataCite و arXiv و تکمیل ساختاریافته بخش‌های پژوهش؛ بدون کلید API و بدون مدل زبانی.")
                : QStringLiteral("Free scholarly discovery from OpenAlex, Crossref, Semantic Scholar, Europe PMC, DataCite, and arXiv with structured research drafting; no API key or language model."));
        } else if (mode == QStringLiteral("free_tier")) {
            aiDescription_->setText(persian
                ? QStringLiteral("مسیر OpenRouter با مدل رایگان چندوجهی Nemotron؛ کلید OPENROUTER_API_KEY لازم است. سهمیه فعلی رایگان ۲۰ درخواست در دقیقه و ۵۰ درخواست در روز است و ارائه‌دهنده ثالث ممکن است داده‌ها را ثبت یا برای بهبود مدل استفاده کند.")
                : QStringLiteral("OpenRouter free-tier routing defaults to the multimodal Nemotron model; OPENROUTER_API_KEY is required. Current free limits are 20 requests/minute and 50 requests/day. Third-party providers may log or use submitted content to improve models."));
        } else {
            aiDescription_->setText(persian
                ? QStringLiteral("اتصال اختیاری به Gemini و OpenAI؛ اعتبارنامه‌ها فقط از محیط سیستم خوانده می‌شوند و در برنامه ذخیره نمی‌شوند.")
                : QStringLiteral("Optional Gemini and OpenAI access; credentials are read only from the process environment and are never stored by the application."));
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
        if (!sectionList_ || !saveActiveProposalSection())
            return;
        QString currentKey;
        if (sectionList_->currentItem())
            currentKey = sectionList_->currentItem()->data(Qt::UserRole).toString();
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        sectionList_->clear();
        const QStringList keys = ProjectStore::proposalSectionKeys();
        for (const QString &key : keys) {
            if (finalDetailKeys().contains(key) || administrativeSectionKeys().contains(key))
                continue;
            auto *item = new QListWidgetItem(sectionLabel(key, persian));
            item->setData(Qt::UserRole, key);
            sectionList_->addItem(item);
            if (key == currentKey)
                sectionList_->setCurrentItem(item);
        }
        if (sectionList_->currentRow() < 0 && sectionList_->count() > 0)
            sectionList_->setCurrentRow(0);
    }

    bool saveProposalSection(const QString &key, const QString &content) {
        if (!projectId_ || key.isEmpty())
            return true;
        QString error;
        if (!store_.saveSection(*projectId_, key, content, &error)) {
            editorStatus_->setText(error);
            return false;
        }
        currentProposalSectionKey_ = key;
        proposalSectionDirty_ = false;
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        editorStatus_->setText(persian ? QStringLiteral("ذخیره شد") : QStringLiteral("Saved"));
        return true;
    }

    bool saveActiveProposalSection() {
        if (!proposalSectionDirty_)
            return true;
        return saveProposalSection(currentProposalSectionKey_, sectionEditor_->toPlainText());
    }

    void loadSelectedSection() {
        if (!projectId_ || !sectionList_->currentItem())
            return;
        const QString key = sectionList_->currentItem()->data(Qt::UserRole).toString();
        if (proposalSectionDirty_ && currentProposalSectionKey_ != key &&
            !saveActiveProposalSection()) {
            int previousRow = -1;
            for (int row = 0; row < sectionList_->count(); ++row) {
                if (sectionList_->item(row)->data(Qt::UserRole).toString() == currentProposalSectionKey_) {
                    previousRow = row;
                    break;
                }
            }
            if (previousRow >= 0) {
                const QSignalBlocker blocker(sectionList_);
                sectionList_->setCurrentRow(previousRow);
            }
            return;
        }
        QString error;
        const auto sections = store_.loadSections(*projectId_, &error);
        if (!error.isEmpty()) {
            editorStatus_->setText(error);
            return;
        }
        loadingProposalSection_ = true;
        sectionEditor_->setPlainText(sections.value(key));
        loadingProposalSection_ = false;
        currentProposalSectionKey_ = key;
        proposalSectionDirty_ = false;
    }

    bool saveSelectedSection() {
        if (!projectId_ || !sectionList_->currentItem())
            return false;
        const QString key = sectionList_->currentItem()->data(Qt::UserRole).toString();
        return saveProposalSection(key, sectionEditor_->toPlainText());
    }


    void openExistingProject() {
        QString error;
        const auto projects = store_.listProjects(&error);
        if (!error.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Yadavarifork"), error);
            return;
        }
        if (projects.isEmpty()) {
            const bool persian = language_->currentData().toString() == QStringLiteral("fa");
            QMessageBox::information(this, QStringLiteral("Yadavarifork"),
                persian ? QStringLiteral("هنوز پروژه‌ای ذخیره نشده است.")
                        : QStringLiteral("No saved projects yet."));
            return;
        }
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        QDialog dialog(this);
        dialog.setWindowTitle(persian ? QStringLiteral("انتخاب پروژه") : QStringLiteral("Select project"));
        auto *layout = new QVBoxLayout(&dialog);
        auto *list = new QListWidget(&dialog);
        for (const ProjectInfo &project : projects) {
            const QString workflowLabel = project.workflow == QStringLiteral("proposal")
                ? (persian ? QStringLiteral("پروپوزال") : QStringLiteral("Proposal"))
                : (persian ? QStringLiteral("پژوهش") : QStringLiteral("Research"));
            auto *item = new QListWidgetItem(
                QStringLiteral("%1 — %2 — %3").arg(project.title, workflowLabel, project.updatedAt));
            item->setData(Qt::UserRole, project.id);
            list->addItem(item);
        }
        if (list->count() > 0) list->setCurrentRow(0);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel, &dialog);
        layout->addWidget(list);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        connect(list, &QListWidget::itemDoubleClicked, &dialog, &QDialog::accept);
        if (dialog.exec() != QDialog::Accepted || !list->currentItem())
            return;
        openProjectById(list->currentItem()->data(Qt::UserRole).toLongLong());
    }

    bool openProjectById(qint64 projectId) {
        if (!saveActiveProposalSection() || !saveActiveResearchSection())
            return false;
        currentProposalSectionKey_.clear();
        currentResearchSectionKey_.clear();
        proposalSectionDirty_ = false;
        researchSectionDirty_ = false;
        QString error;
        const auto project = store_.loadProject(projectId, &error);
        if (!project) {
            QMessageBox::warning(this, QStringLiteral("Yadavarifork"), error);
            return false;
        }
        projectId_ = project->id;
        finalDetailsReviewed_ = false;
        projectTitle_->setText(project->title);
        language_->setCurrentIndex(language_->findData(project->language));
        aiMode_->setCurrentIndex(aiMode_->findData(project->aiMode));
        taskMode_->setCurrentIndex(taskMode_->findData(project->workflow));
        language_->setEnabled(false);
        aiMode_->setEnabled(false);
        taskMode_->setEnabled(false);
        projectTitle_->setEnabled(false);
        applyLanguage();
        if (project->workflow == QStringLiteral("proposal")) {
            populateSectionList();
            const auto savedSections = store_.loadSections(*projectId_, &error);
            if (!error.isEmpty()) {
                editorStatus_->setText(error);
                return false;
            }
            const QString reviewStatus = store_.loadProjectMetadata(
                *projectId_, QStringLiteral("final_details_status"), &error);
            if (!error.isEmpty()) {
                editorStatus_->setText(error);
                return false;
            }
            finalDetailsReviewed_ = (reviewStatus == QStringLiteral("skipped") ||
                                     reviewStatus == QStringLiteral("completed"));
            for (const QString &key : finalDetailKeys()) {
                if (!savedSections.value(key).trimmed().isEmpty()) {
                    finalDetailsReviewed_ = true;
                    break;
                }
            }
            pages_->setCurrentIndex(4);
        } else {
            populateResearchSectionList();
            refreshResearchSources();
            pages_->setCurrentIndex(5);
        }
        updateNavigation();
        return true;
    }

    void startNewProject() {
        if (!saveActiveProposalSection() || !saveActiveResearchSection())
            return;
        currentProposalSectionKey_.clear();
        currentResearchSectionKey_.clear();
        proposalSectionDirty_ = false;
        researchSectionDirty_ = false;
        projectId_.reset();
        finalDetailsReviewed_ = false;
        language_->setEnabled(true);
        aiMode_->setEnabled(true);
        taskMode_->setEnabled(true);
        projectTitle_->setEnabled(true);
        projectTitle_->clear();
        summaryStatus_->clear();
        editorStatus_->clear();
        researchStatus_->clear();
        sectionEditor_->clear();
        researchEditor_->clear();
        // Language is already selected on the welcome page; continue the
        // new-project wizard at AI-mode selection instead of appearing inert.
        pages_->setCurrentIndex(1);
        updateNavigation();
    }

    QString researchSectionLabel(const QString &key, bool persian) const {
        static const QMap<QString, QPair<QString, QString>> labels = {
            {QStringLiteral("research_question"), {QStringLiteral("پرسش پژوهش"), QStringLiteral("Research question")}},
            {QStringLiteral("research_background"), {QStringLiteral("پیشینه و زمینه"), QStringLiteral("Background")}},
            {QStringLiteral("research_objectives"), {QStringLiteral("اهداف پژوهش"), QStringLiteral("Research objectives")}},
            {QStringLiteral("research_methodology"), {QStringLiteral("روش‌شناسی"), QStringLiteral("Methodology")}},
            {QStringLiteral("search_strategy"), {QStringLiteral("راهبرد جستجوی منابع"), QStringLiteral("Literature search strategy")}},
            {QStringLiteral("inclusion_criteria"), {QStringLiteral("معیارهای ورود"), QStringLiteral("Inclusion criteria")}},
            {QStringLiteral("exclusion_criteria"), {QStringLiteral("معیارهای خروج"), QStringLiteral("Exclusion criteria")}},
            {QStringLiteral("evidence_synthesis"), {QStringLiteral("سنتز شواهد"), QStringLiteral("Evidence synthesis")}},
            {QStringLiteral("findings"), {QStringLiteral("یافته‌ها"), QStringLiteral("Findings")}},
            {QStringLiteral("limitations"), {QStringLiteral("محدودیت‌ها"), QStringLiteral("Limitations")}},
            {QStringLiteral("future_work"), {QStringLiteral("کارهای آینده"), QStringLiteral("Future work")}}
        };
        const auto it = labels.constFind(key);
        if (it == labels.cend()) return key;
        return persian ? it->first : it->second;
    }

    void populateResearchSectionList() {
        if (!researchSectionList_ || !saveActiveResearchSection()) return;
        QString currentKey;
        if (researchSectionList_->currentItem())
            currentKey = researchSectionList_->currentItem()->data(Qt::UserRole).toString();
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        researchSectionList_->clear();
        for (const QString &key : ProjectStore::researchSectionKeys()) {
            auto *item = new QListWidgetItem(researchSectionLabel(key, persian));
            item->setData(Qt::UserRole, key);
            researchSectionList_->addItem(item);
            if (key == currentKey) researchSectionList_->setCurrentItem(item);
        }
        if (researchSectionList_->currentRow() < 0 && researchSectionList_->count() > 0)
            researchSectionList_->setCurrentRow(0);
    }

    bool saveResearchSection(const QString &key, const QString &content) {
        if (!projectId_ || key.isEmpty())
            return true;
        QString error;
        if (!store_.saveResearchSection(*projectId_, key, content, &error)) {
            researchStatus_->setText(error);
            return false;
        }
        currentResearchSectionKey_ = key;
        researchSectionDirty_ = false;
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        researchStatus_->setText(persian ? QStringLiteral("بخش پژوهش ذخیره شد.") : QStringLiteral("Research section saved."));
        return true;
    }

    bool saveActiveResearchSection() {
        if (!researchSectionDirty_)
            return true;
        return saveResearchSection(currentResearchSectionKey_, researchEditor_->toPlainText());
    }

    void loadSelectedResearchSection() {
        if (!projectId_ || !researchSectionList_->currentItem()) return;
        const QString key = researchSectionList_->currentItem()->data(Qt::UserRole).toString();
        if (researchSectionDirty_ && currentResearchSectionKey_ != key &&
            !saveActiveResearchSection()) {
            int previousRow = -1;
            for (int row = 0; row < researchSectionList_->count(); ++row) {
                if (researchSectionList_->item(row)->data(Qt::UserRole).toString() == currentResearchSectionKey_) {
                    previousRow = row;
                    break;
                }
            }
            if (previousRow >= 0) {
                const QSignalBlocker blocker(researchSectionList_);
                researchSectionList_->setCurrentRow(previousRow);
            }
            return;
        }
        QString error;
        const auto sections = store_.loadResearchSections(*projectId_, &error);
        if (!error.isEmpty()) { researchStatus_->setText(error); return; }
        loadingResearchSection_ = true;
        researchEditor_->setPlainText(sections.value(key));
        loadingResearchSection_ = false;
        currentResearchSectionKey_ = key;
        researchSectionDirty_ = false;
    }

    bool saveSelectedResearchSection() {
        if (!projectId_ || !researchSectionList_->currentItem()) return false;
        const QString key = researchSectionList_->currentItem()->data(Qt::UserRole).toString();
        return saveResearchSection(key, researchEditor_->toPlainText());
    }

    QString researchRunsDirectory() const {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
            .filePath(QStringLiteral("research_runs"));
    }

    bool writeResearchRunManifest(const QString &status, const QString &stage) {
        if (currentResearchRunOutput_.isEmpty())
            return false;
        if (!QDir().mkpath(currentResearchRunOutput_))
            return false;

        const QString manifestPath = QDir(currentResearchRunOutput_)
                                         .filePath(QStringLiteral("run_manifest.json"));
        QJsonObject manifest;
        QFile existing(manifestPath);
        if (existing.exists()) {
            if (!existing.open(QIODevice::ReadOnly))
                return false;
            QJsonParseError parseError;
            const QByteArray existingBytes = existing.readAll();
            existing.close();
            const QJsonDocument document = QJsonDocument::fromJson(existingBytes, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject())
                return false;
            manifest = document.object();
        } else {
            manifest.insert(QStringLiteral("schema_version"), 1);
            manifest.insert(QStringLiteral("run_id"), QFileInfo(currentResearchRunOutput_).fileName());
            manifest.insert(QStringLiteral("created_at"),
                            QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
            if (currentResearchRunProjectId_)
                manifest.insert(QStringLiteral("project_id"), QString::number(*currentResearchRunProjectId_));
        }
        manifest.insert(QStringLiteral("updated_at"),
                        QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
        manifest.insert(QStringLiteral("status"), status);
        manifest.insert(QStringLiteral("stage"), stage);
        if (status != QStringLiteral("failed"))
            manifest.remove(QStringLiteral("error"));

        QSaveFile output(manifestPath);
        if (!output.open(QIODevice::WriteOnly))
            return false;
        const QByteArray data = QJsonDocument(manifest).toJson(QJsonDocument::Indented);
        return output.write(data) == data.size() && output.commit();
    }

    void showResearchRunHistory() {
        if (!projectId_)
            return;
        const QStringList rootPaths{
            researchRunsDirectory(),
            QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("data/research_runs"))
        };
        QDialog dialog(this);
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        dialog.setWindowTitle(persian ? QStringLiteral("تاریخچه اجراهای پژوهش") :
                                        QStringLiteral("Research run history"));
        auto *layout = new QVBoxLayout(&dialog);
        auto *runs = new QListWidget(&dialog);
        const QString projectPrefix = QStringLiteral("project_%1_*").arg(*projectId_);
        const QString noApiPrefix = QStringLiteral("no_api_project_%1_*").arg(*projectId_);
        QFileInfoList folders;
        for (const QString &rootPath : rootPaths) {
            QDir root(rootPath);
            folders.append(root.entryInfoList(
                {projectPrefix, noApiPrefix}, QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time));
        }
        std::sort(folders.begin(), folders.end(), [](const QFileInfo &left, const QFileInfo &right) {
            return left.lastModified() > right.lastModified();
        });
        for (const QFileInfo &folder : folders) {
            QFile manifestFile(QDir(folder.absoluteFilePath()).filePath(QStringLiteral("run_manifest.json")));
            if (!manifestFile.open(QIODevice::ReadOnly))
                continue;
            const QJsonDocument document = QJsonDocument::fromJson(manifestFile.readAll());
            if (!document.isObject())
                continue;
            const QJsonObject manifest = document.object();
            const QString created = manifest.value(QStringLiteral("created_at")).toString();
            const QString status = manifest.value(QStringLiteral("status")).toString();
            const QString stage = manifest.value(QStringLiteral("stage")).toString();
            auto *item = new QListWidgetItem(
                QStringLiteral("%1  |  %2  |  %3").arg(created, status, stage), runs);
            item->setData(Qt::UserRole, folder.absoluteFilePath());
        }
        if (runs->count() == 0)
            runs->addItem(persian ? QStringLiteral("هنوز اجرایی ثبت نشده است.") :
                                    QStringLiteral("No research runs recorded yet."));
        layout->addWidget(runs);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Close, &dialog);
        buttons->button(QDialogButtonBox::Open)->setText(persian ? QStringLiteral("بازکردن پوشه") :
                                                                  QStringLiteral("Open folder"));
        buttons->button(QDialogButtonBox::Close)->setText(persian ? QStringLiteral("بستن") :
                                                                   QStringLiteral("Close"));
        buttons->button(QDialogButtonBox::Open)->setEnabled(runs->currentItem() &&
            runs->currentItem()->data(Qt::UserRole).isValid());
        connect(runs, &QListWidget::currentRowChanged, &dialog, [runs, buttons](int) {
            buttons->button(QDialogButtonBox::Open)->setEnabled(
                runs->currentItem() && runs->currentItem()->data(Qt::UserRole).isValid());
        });
        connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
            if (runs->currentItem()) {
                const QString path = runs->currentItem()->data(Qt::UserRole).toString();
                if (!path.isEmpty())
                    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            }
        });
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        layout->addWidget(buttons);
        dialog.resize(760, 420);
        dialog.exec();
    }

    bool importNoApiResearchResult() {
        if (!projectId_ || currentResearchRunOutput_.isEmpty()) return false;
        QFile file(QDir(currentResearchRunOutput_).filePath(QStringLiteral("result.json")));
        if (!file.open(QIODevice::ReadOnly)) return false;
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) return false;
        const QJsonObject root = document.object();
        if (!root.value(QStringLiteral("ok")).toBool() || root.value(QStringLiteral("count")).toInt() < 10) {
            researchStatus_->setText(root.value(QStringLiteral("message")).toString(
                QStringLiteral("کمتر از ۱۰ منبع علمی یکتا بازیابی شد؛ منبع ساختگی اضافه نشد.")));
            return false;
        }
        const QJsonObject sections = root.value(QStringLiteral("sections")).toObject();
        for (const QString &key : ProjectStore::researchSectionKeys()) {
            const QString value = sections.value(key).toString();
            if (value.trimmed().isEmpty()) continue;
            QString error;
            if (!store_.saveResearchSection(*projectId_, key, value, &error)) {
                researchStatus_->setText(error);
                return false;
            }
        }
        QSet<QString> existing;
        QString error;
        const auto previous = store_.loadResearchSources(*projectId_, &error);
        if (!error.isEmpty()) return false;
        for (const ResearchSource &source : previous) {
            if (!source.doi.trimmed().isEmpty()) existing.insert(source.doi.trimmed().toLower());
            existing.insert(source.title.trimmed().toLower());
        }
        noApiImportedSourceCount_ = 0;
        for (const QJsonValue &value : root.value(QStringLiteral("records")).toArray()) {
            if (!value.isObject()) continue;
            const QJsonObject record = value.toObject();
            ResearchSource source;
            source.title = record.value(QStringLiteral("title")).toString();
            source.authors = record.value(QStringLiteral("authors")).toString();
            source.publicationYear = record.value(QStringLiteral("year")).toString();
            source.doi = record.value(QStringLiteral("doi")).toString();
            source.url = record.value(QStringLiteral("url")).toString();
            source.notes = QStringLiteral("Provider: %1; venue: %2; type: %3; open access: %4")
                .arg(record.value(QStringLiteral("provider")).toString(),
                     record.value(QStringLiteral("venue")).toString(),
                     record.value(QStringLiteral("type")).toString(),
                     record.value(QStringLiteral("is_oa")).toString());
            const QString doiKey = source.doi.trimmed().toLower();
            const QString titleKey = source.title.trimmed().toLower();
            if (titleKey.isEmpty() || (!doiKey.isEmpty() && existing.contains(doiKey)) ||
                existing.contains(titleKey)) continue;
            if (!store_.addResearchSource(*projectId_, source, &error)) {
                researchStatus_->setText(error);
                return false;
            }
            if (!doiKey.isEmpty()) existing.insert(doiKey);
            existing.insert(titleKey);
            ++noApiImportedSourceCount_;
        }
        populateResearchSectionList();
        refreshResearchSources();
        return true;
    }

    void startNoApiScholarlyResearch() {
        if (!projectId_ || !saveActiveResearchSection() || (engineRunner_ && engineRunner_->isRunning()))
            return;
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        QString python = qEnvironmentVariable("YADAVARIFORK_PYTHON").trimmed();
        if (!python.isEmpty() && !QFileInfo(python).isFile())
            python.clear();
        const QString appDirectory = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
        const QStringList bundledRuntimeCandidates{
            QDir(appDirectory).filePath(QStringLiteral("runtime/Scripts/python.exe")),
            QDir(appDirectory).filePath(QStringLiteral("data/research-runtime/Scripts/python.exe")),
            QDir(appDirectory).filePath(QStringLiteral(".venv/Scripts/python.exe")),
            QDir(appDirectory).filePath(QStringLiteral("venv/Scripts/python.exe"))
        };
#else
        const QStringList bundledRuntimeCandidates{
            QDir(appDirectory).filePath(QStringLiteral("runtime/bin/python3")),
            QDir(appDirectory).filePath(QStringLiteral("data/research-runtime/bin/python3")),
            QDir(appDirectory).filePath(QStringLiteral(".venv/bin/python3")),
            QDir(appDirectory).filePath(QStringLiteral("venv/bin/python3"))
        };
#endif
        if (python.isEmpty()) {
            for (const QString &candidate : bundledRuntimeCandidates) {
                if (QFileInfo(candidate).isFile()) { python = candidate; break; }
            }
        }
        if (python.isEmpty()) python = QStandardPaths::findExecutable(QStringLiteral("python"));
        if (python.isEmpty()) python = QStandardPaths::findExecutable(QStringLiteral("python3"));
#ifdef Q_OS_WIN
        if (python.isEmpty()) python = QStandardPaths::findExecutable(QStringLiteral("py"));
#endif
        const QString script = QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("scripts/no_api_scholarly_research.py"));
        if (python.isEmpty() || !QFileInfo(script).isFile()) {
            researchStatus_->setText(persian
                ? QStringLiteral("برای جست‌وجوی علمی رایگان، Python 3 و فایل scripts/no_api_scholarly_research.py لازم است.")
                : QStringLiteral("Free scholarly discovery requires Python 3 and scripts/no_api_scholarly_research.py."));
            return;
        }
        const QString root = researchRunsDirectory();
        if (QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).isEmpty() ||
            !QDir().mkpath(root)) {
            researchStatus_->setText(persian
                ? QStringLiteral("پوشهٔ دادهٔ قابل‌نوشتن برای اجرای پژوهش در دسترس نیست.")
                : QStringLiteral("A writable application-data directory is unavailable for research runs."));
            return;
        }
        const QString output = QDir(root).filePath(QStringLiteral("no_api_project_%1_%2")
            .arg(*projectId_).arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"))));
        if (QFileInfo::exists(output) || !QDir().mkpath(output)) return;
        currentResearchRunOutput_ = output;
        currentResearchRunProjectId_ = projectId_;
        noApiResearchRun_ = true;
        noApiImportedSourceCount_ = 0;
        researchRunCancellationRequested_ = false;
        if (!writeResearchRunManifest(QStringLiteral("starting"), QStringLiteral("keyless_scholarly_discovery"))) {
            noApiResearchRun_ = false;
            currentResearchRunOutput_.clear();
            currentResearchRunProjectId_.reset();
            return;
        }
        researchStatus_->setText(persian
            ? QStringLiteral("جست‌وجوی رایگان در OpenAlex، Crossref، Semantic Scholar، Europe PMC، DataCite و arXiv آغاز شد؛ هدف ۵۰ منبع یکتا است و کلید API لازم نیست.")
            : QStringLiteral("Free OpenAlex, Crossref, Semantic Scholar, Europe PMC, DataCite, and arXiv discovery started; target is 50 unique records, no API key required."));
        const QStringList arguments{
            script, QStringLiteral("--query"), projectTitle_->text().trimmed(),
            QStringLiteral("--language"), language_->currentData().toString(),
            QStringLiteral("--target"), QStringLiteral("50"),
            QStringLiteral("--output"), QDir(output).filePath(QStringLiteral("result.json"))
        };
        const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        if (!engineRunner_->start(python, arguments, QFileInfo(script).absolutePath(), environment)) {
            writeResearchRunManifest(QStringLiteral("failed"), QStringLiteral("process_start_failed"));
            noApiResearchRun_ = false;
            currentResearchRunOutput_.clear();
            currentResearchRunProjectId_.reset();
            researchStatus_->setText(persian ? QStringLiteral("راه‌اندازی جست‌وجوی علمی ممکن نشد؛ Python را بررسی کنید.") :
                                               QStringLiteral("Could not start scholarly discovery; verify Python."));
        }
    }

    void configureAndRunResearchEngine() {
        if (!projectId_ || taskMode_->currentData().toString() != QStringLiteral("research"))
            return;
        if (engineRunner_ && engineRunner_->isRunning())
            return;
        if (!saveActiveResearchSection())
            return;

        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        const QString selectedAiMode = aiMode_->currentData().toString();
        const bool freeTierMode = selectedAiMode == QStringLiteral("free_tier");
        if (selectedAiMode == QStringLiteral("no_api")) {
            startNoApiScholarlyResearch();
            return;
        }
        QDialog dialog(this);
        dialog.setWindowTitle(persian ? QStringLiteral("تنظیم اجرای موتور پژوهش") :
                                        QStringLiteral("Configure research engine"));
        auto *form = new QFormLayout(&dialog);
        auto addPathRow = [&](const QString &label, QLineEdit *&field, bool directory,
                              const QString &filter, const QString &initial) {
            auto *row = new QWidget(&dialog);
            auto *layout = new QHBoxLayout(row);
            layout->setContentsMargins(0, 0, 0, 0);
            field = new QLineEdit(initial, row);
            auto *browse = new QPushButton(persian ? QStringLiteral("انتخاب") : QStringLiteral("Browse"), row);
            layout->addWidget(field, 1);
            layout->addWidget(browse);
            connect(browse, &QPushButton::clicked, &dialog, [&, field, directory, filter] {
                const QString selected = directory
                    ? QFileDialog::getExistingDirectory(&dialog,
                        persian ? QStringLiteral("انتخاب پوشه") : QStringLiteral("Select folder"),
                        field->text())
                    : QFileDialog::getOpenFileName(&dialog,
                        persian ? QStringLiteral("انتخاب فایل") : QStringLiteral("Select file"),
                        field->text(), filter);
                if (!selected.isEmpty()) field->setText(QDir::cleanPath(selected));
            });
            form->addRow(label, row);
        };

        QString pythonDefault = QStandardPaths::findExecutable(QStringLiteral("python"));
        if (pythonDefault.isEmpty())
            pythonDefault = QStandardPaths::findExecutable(QStringLiteral("python3"));
        QString scriptDefault = QDir(QCoreApplication::applicationDirPath())
                                    .filePath(QStringLiteral("../paper_writing_cli.py"));
        if (!QFileInfo::exists(scriptDefault))
            scriptDefault = QDir::current().filePath(QStringLiteral("paper_writing_cli.py"));
        QString templateDefault = QDir(QCoreApplication::applicationDirPath())
                                      .filePath(QStringLiteral("templates"));
        if (!QFileInfo::exists(templateDefault))
            templateDefault = QDir::current().filePath(QStringLiteral("templates"));

        QLineEdit *pythonField = nullptr;
        QLineEdit *scriptField = nullptr;
        QLineEdit *materialsField = nullptr;
        QLineEdit *templateField = nullptr;
        addPathRow(persian ? QStringLiteral("مفسر Python") : QStringLiteral("Python executable"),
                   pythonField, false, QStringLiteral("Programs (*.exe);;All files (*)"), pythonDefault);
        addPathRow(persian ? QStringLiteral("اسکریپت CLI") : QStringLiteral("CLI script"),
                   scriptField, false, QStringLiteral("Python files (*.py);;All files (*)"), scriptDefault);
        addPathRow(persian ? QStringLiteral("پوشه داده‌های خام") : QStringLiteral("Raw materials folder"),
                   materialsField, true, QString(), QString());
        addPathRow(persian ? QStringLiteral("قالب LaTeX") : QStringLiteral("LaTeX template folder"),
                   templateField, true, QString(), templateDefault);

        auto *writerModelField = new QLineEdit(
            freeTierMode ? QStringLiteral("nvidia/nemotron-3.5-super-vl:free")
                         : QStringLiteral("gemini-3.1-pro-preview"), &dialog);
        auto *reflectionModelField = new QLineEdit(&dialog);
        reflectionModelField->setPlaceholderText(persian
            ? QStringLiteral("خالی = همان مدل نویسنده")
            : QStringLiteral("Blank = use writer model"));
        form->addRow(persian ? QStringLiteral("مدل نویسنده و مرور منابع") :
                               QStringLiteral("Writer and literature model"), writerModelField);
        form->addRow(persian ? QStringLiteral("مدل بازبینی و اصلاح") :
                               QStringLiteral("Reflection and refinement model"), reflectionModelField);
        auto *resumeCheck = new QCheckBox(persian
            ? QStringLiteral("ادامه آخرین اجرای ناموفق با اعتبارسنجی checkpointها")
            : QStringLiteral("Resume the previous run after checkpoint validation"), &dialog);
        const QString savedResearchOutput = store_.loadProjectMetadata(
            *projectId_, QStringLiteral("research_engine_last_output"));
        resumeCheck->setEnabled(
            QFileInfo(savedResearchOutput).isDir() &&
            !QFileInfo(QDir(savedResearchOutput).filePath(QStringLiteral("final_paper.pdf"))).isFile());
        resumeCheck->setToolTip(persian
            ? QStringLiteral("فقط مراحل دارای ورودی یکسان و خروجی‌های سالم بازیابی می‌شوند.")
            : QStringLiteral("Only stages with matching inputs and intact artifacts are reused."));
        form->addRow(resumeCheck);
        auto *crossrefCheck = new QCheckBox(persian
            ? QStringLiteral("اعتبارسنجی اختیاری DOI با Crossref")
            : QStringLiteral("Optionally verify DOIs with Crossref"), &dialog);
        crossrefCheck->setToolTip(persian
            ? QStringLiteral("برای DOIهای عمومی درخواست شبکه می‌فرستد و ممکن است زمان اجرا را افزایش دهد؛ اگر Crossref در دسترس نباشد یا DOI را پیدا نکند، منبع و DOI حذف یا تغییر نمی‌شوند.")
            : QStringLiteral("Sends public DOI identifiers to Crossref and may add latency; unavailable or not-found results never remove or change a source DOI."));
        form->addRow(crossrefCheck);
        QCheckBox *freeTierConsent = nullptr;
        if (freeTierMode) {
            freeTierConsent = new QCheckBox(persian
                ? QStringLiteral("می‌پذیرم داده‌ها به ارائه‌دهنده ثالث مدل رایگان ارسال شوند؛ اطلاعات محرمانه ارسال نمی‌کنم.")
                : QStringLiteral("I understand prompts go to third-party free-model providers; I will not submit confidential data."), &dialog);
            form->addRow(freeTierConsent);
        }
        auto *notice = new QLabel(persian
            ? QStringLiteral("کد موتور پژوهش همراه برنامه است؛ مفسر Python و بسته‌های وابسته همراه برنامه نیستند. برای ساخت محیط جداگانه، scripts/setup_research_runtime.ps1 را اجرا و سپس Scripts\\python.exe ساخته‌شده را انتخاب کنید. ابزارهای pdflatex و bibtex لازم‌اند. کلیدهای Gemini/OpenAI/OpenRouter فقط از محیط سیستم خوانده می‌شوند و ذخیره نمی‌شوند. سرویس‌های رایگان ممکن است داده‌های ارسالی را طبق سیاست ارائه‌دهنده ثبت یا برای بهبود مدل استفاده کنند؛ داده محرمانه ارسال نکنید.")
            : QStringLiteral("Python and engine dependencies are not bundled. Run scripts/setup_research_runtime.ps1 and select its Scripts\\python.exe. pdflatex and bibtex are required. Gemini/OpenAI/OpenRouter credentials are read only from the process environment and are never stored. Free-tier providers may log or use submitted content to improve services; do not send confidential data."));
        notice->setWordWrap(true);
        form->addRow(notice);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        buttons->button(QDialogButtonBox::Ok)->setText(persian ? QStringLiteral("شروع") : QStringLiteral("Start"));
        buttons->button(QDialogButtonBox::Cancel)->setText(persian ? QStringLiteral("انصراف") : QStringLiteral("Cancel"));
        form->addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() != QDialog::Accepted)
            return;

        const QString python = QFileInfo(pythonField->text().trimmed()).absoluteFilePath();
        const QString script = QFileInfo(scriptField->text().trimmed()).absoluteFilePath();
        const QString materials = QFileInfo(materialsField->text().trimmed()).absoluteFilePath();
        const QString templateDir = QFileInfo(templateField->text().trimmed()).absoluteFilePath();
        auto fail = [&](const QString &message) {
            researchStatus_->setText(message);
            QMessageBox::warning(this, QStringLiteral("Yadavarifork"), message);
        };
        if (freeTierMode && (!freeTierConsent || !freeTierConsent->isChecked())) {
            fail(persian
                ? QStringLiteral("برای استفاده از سرویس رایگان، ابتدا هشدار حریم خصوصی را تأیید کنید.")
                : QStringLiteral("Acknowledge the free-tier privacy notice before using a free provider."));
            return;
        }
        if (!QFileInfo(python).isFile() || !QFileInfo(script).isFile() ||
            !QFileInfo(materials).isDir() || !QFileInfo(templateDir).isDir()) {
            fail(persian ? QStringLiteral("یکی از مسیرهای انتخاب‌شده معتبر نیست.") :
                            QStringLiteral("One or more selected paths are invalid."));
            return;
        }
        if (!QFileInfo(QDir(materials).filePath(QStringLiteral("idea_sparse.md"))).isFile() ||
            !QFileInfo(QDir(materials).filePath(QStringLiteral("experimental_log.md"))).isFile()) {
            fail(persian
                ? QStringLiteral("پوشه داده خام باید شامل idea_sparse.md و experimental_log.md باشد.")
                : QStringLiteral("The raw materials folder must contain idea_sparse.md and experimental_log.md."));
            return;
        }
        if (!QFileInfo(QDir(templateDir).filePath(QStringLiteral("template.tex"))).isFile() ||
            !QFileInfo(QDir(templateDir).filePath(QStringLiteral("guidelines.md"))).isFile()) {
            fail(persian
                ? QStringLiteral("قالب انتخابی باید شامل template.tex و guidelines.md باشد.")
                : QStringLiteral("The selected template must contain template.tex and guidelines.md."));
            return;
        }
        const QString writerModel = writerModelField->text().trimmed();
        const QString reflectionModel = reflectionModelField->text().trimmed().isEmpty()
            ? writerModel : reflectionModelField->text().trimmed();
        const auto isOpenRouterModel = [](const QString &model) {
            return model.trimmed().contains(QLatin1Char('/'));
        };
        const auto isOpenAiModel = [&](const QString &model) {
            const QString normalized = model.trimmed().toLower();
            return normalized.startsWith(QStringLiteral("gpt-")) ||
                   normalized.startsWith(QStringLiteral("o1")) ||
                   normalized.startsWith(QStringLiteral("o3")) ||
                   normalized.startsWith(QStringLiteral("o4")) ||
                   isOpenRouterModel(normalized);
        };
        const auto isGeminiModel = [](const QString &model) {
            return model.trimmed().toLower().startsWith(QStringLiteral("gemini-"));
        };
        const auto isFreeOpenRouterModel = [](const QString &model) {
            const QString normalized = model.trimmed().toLower();
            return normalized == QStringLiteral("openrouter/free") ||
                   normalized.endsWith(QStringLiteral(":free"));
        };
        if (writerModel.isEmpty() || reflectionModel.isEmpty() ||
            (!isOpenAiModel(writerModel) && !isGeminiModel(writerModel)) ||
            (!isOpenAiModel(reflectionModel) && !isGeminiModel(reflectionModel))) {
            fail(persian
                ? QStringLiteral("مدل نویسنده و بازبینی باید از خانواده Gemini، OpenAI یا شناسه OpenRouter باشند.")
                : QStringLiteral("Writer and reflection models must be Gemini, OpenAI, or OpenRouter model IDs."));
            return;
        }
        if (freeTierMode &&
            (!isFreeOpenRouterModel(writerModel) || !isFreeOpenRouterModel(reflectionModel))) {
            fail(persian
                ? QStringLiteral("در حالت طرح رایگان فقط openrouter/free یا مدل‌های OpenRouter با پسوند :free مجازند.")
                : QStringLiteral("Free-tier mode accepts only openrouter/free or OpenRouter model IDs ending in :free."));
            return;
        }

        const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        const QStringList geminiKeyNames{
            QStringLiteral("GEMINI_API_KEY"), QStringLiteral("GOOGLE_API_KEY"),
            QStringLiteral("GOOGLE_GENAI_API_KEY")
        };
        bool hasGeminiKey = false;
        for (const QString &key : geminiKeyNames)
            hasGeminiKey = hasGeminiKey || !environment.value(key).trimmed().isEmpty();
        const bool hasVertexConfiguration =
            !environment.value(QStringLiteral("VERTEX_AI_PROJECT")).trimmed().isEmpty() &&
            !environment.value(QStringLiteral("VERTEX_AI_LOCATION")).trimmed().isEmpty();
        const bool hasOpenAiKey =
            !environment.value(QStringLiteral("OPENAI_API_KEY")).trimmed().isEmpty();
        const bool hasOpenRouterKey =
            !environment.value(QStringLiteral("OPENROUTER_API_KEY")).trimmed().isEmpty();
        const bool needsGemini = !isOpenAiModel(writerModel) || !isOpenAiModel(reflectionModel);
        const bool needsOpenRouter =
            isOpenRouterModel(writerModel) || isOpenRouterModel(reflectionModel);
        const bool needsOpenAi =
            (isOpenAiModel(writerModel) && !isOpenRouterModel(writerModel)) ||
            (isOpenAiModel(reflectionModel) && !isOpenRouterModel(reflectionModel));
        if ((needsGemini && !hasGeminiKey && !hasVertexConfiguration) ||
            (needsOpenAi && !hasOpenAiKey) ||
            (needsOpenRouter && !hasOpenRouterKey)) {
            QString missing;
            if (needsGemini && !hasGeminiKey && !hasVertexConfiguration)
                missing += QStringLiteral("Gemini/Vertex AI");
            if (needsOpenAi && !hasOpenAiKey) {
                if (!missing.isEmpty()) missing += QStringLiteral(" and ");
                missing += QStringLiteral("OpenAI");
            }
            if (needsOpenRouter && !hasOpenRouterKey) {
                if (!missing.isEmpty()) missing += QStringLiteral(" and ");
                missing += QStringLiteral("OpenRouter");
            }
            fail(persian
                ? QStringLiteral("اعتبارنامه %1 در محیط سیستم پیدا نشد. متغیرهای محیطی ارائه‌دهنده را تنظیم و برنامه را دوباره اجرا کنید.").arg(missing)
                : QStringLiteral("Missing %1 credentials in the process environment. Set the provider environment variables and restart the application.").arg(missing));
            return;
        }

        const QString dataRoot = researchRunsDirectory();
        if (!QDir().mkpath(dataRoot)) {
            fail(persian ? QStringLiteral("پوشه خروجی قابل ایجاد نیست.") :
                            QStringLiteral("Cannot create research run directory."));
            return;
        }
        const bool resumeRequested = resumeCheck->isChecked();
        const QString output = resumeRequested
            ? savedResearchOutput
            : QDir(dataRoot).filePath(
                QStringLiteral("project_%1_%2").arg(*projectId_)
                    .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"))));
        if (resumeRequested && !QFileInfo(output).isDir()) {
            fail(persian ? QStringLiteral("پوشه اجرای قبلی برای ادامه در دسترس نیست.") :
                            QStringLiteral("The previous run directory is not available for resume."));
            return;
        }
        if (!resumeRequested && QFileInfo::exists(output)) {
            fail(persian ? QStringLiteral("مسیر خروجی از قبل وجود دارد؛ اجرای جدید متوقف شد.") :
                            QStringLiteral("Output path already exists; refusing to overwrite it."));
            return;
        }
        QStringList arguments{
            script,
            QStringLiteral("--raw_materials_dir"), materials,
            QStringLiteral("--output_dir"), output,
            QStringLiteral("--latex_template_dir"), templateDir,
            QStringLiteral("--writer_model_name"), writerModel,
            QStringLiteral("--reflection_model_name"), reflectionModel,
            QStringLiteral("--events_jsonl")
        };
        // The Python engine also verifies a published PDF against the completed refinement checkpoint.
        if (resumeRequested)
            arguments.append(QStringLiteral("--resume"));
        if (crossrefCheck->isChecked())
            arguments.append(QStringLiteral("--verify_dois_with_crossref"));
        currentResearchRunOutput_ = output;
        currentResearchRunProjectId_ = projectId_;
        researchRunCancellationRequested_ = false;
        QString metadataError;
        if (!store_.saveProjectMetadata(*projectId_,
                QStringLiteral("research_engine_last_output"), output, &metadataError)) {
            currentResearchRunOutput_.clear();
            currentResearchRunProjectId_.reset();
            fail(metadataError.isEmpty()
                ? (persian ? QStringLiteral("ثبت مسیر اجرای پژوهش ناموفق بود.")
                           : QStringLiteral("Could not save the research run path."))
                : metadataError);
            return;
        }
        if (!QDir().mkpath(output) ||
            !writeResearchRunManifest(QStringLiteral("starting"), QStringLiteral("process_start"))) {
            currentResearchRunOutput_.clear();
            currentResearchRunProjectId_.reset();
            fail(persian ? QStringLiteral("ثبت manifest اجرای پژوهش ممکن نشد.") :
                            QStringLiteral("Could not create the research run manifest."));
            return;
        }
        researchStatus_->clear();
        if (!engineRunner_->start(python, arguments, QFileInfo(script).absolutePath(), environment)) {
            writeResearchRunManifest(QStringLiteral("failed"), QStringLiteral("process_start_failed"));
            currentResearchRunOutput_.clear();
            currentResearchRunProjectId_.reset();
            runResearchEngine_->setEnabled(true);
            cancelResearchEngine_->setEnabled(false);
            fail(persian ? QStringLiteral("راه‌اندازی فرایند موتور ناموفق بود؛ مسیر Python و دسترسی‌ها را بررسی کنید.") :
                            QStringLiteral("Could not start the engine process; check Python path and permissions."));
        }
    }

    void refreshResearchSources() {
        if (!researchSourcesList_) return;
        researchSourcesList_->clear();
        if (!projectId_) return;
        QString error;
        const auto sources = store_.loadResearchSources(*projectId_, &error);
        if (!error.isEmpty()) { researchStatus_->setText(error); return; }
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        for (const ResearchSource &source : sources) {
            QString details = source.title;
            if (!source.authors.isEmpty()) details += QStringLiteral(" — ") + source.authors;
            if (!source.publicationYear.isEmpty()) details += QStringLiteral(" (%1)").arg(source.publicationYear);
            details += persian ? QStringLiteral(" [تأییدنشده]") : QStringLiteral(" [unverified]");
            auto *item = new QListWidgetItem(details);
            item->setData(Qt::UserRole, source.id);
            researchSourcesList_->addItem(item);
        }
    }

    void addResearchSource() {
        if (!projectId_) return;
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        QDialog dialog(this);
        dialog.setWindowTitle(persian ? QStringLiteral("ثبت منبع پژوهشی") : QStringLiteral("Record research source"));
        auto *form = new QFormLayout(&dialog);
        auto *title = new QLineEdit(&dialog);
        auto *authors = new QLineEdit(&dialog);
        auto *year = new QLineEdit(&dialog);
        auto *doi = new QLineEdit(&dialog);
        auto *url = new QLineEdit(&dialog);
        auto *notes = new QTextEdit(&dialog);
        form->addRow(persian ? QStringLiteral("عنوان *") : QStringLiteral("Title *"), title);
        form->addRow(persian ? QStringLiteral("نویسندگان") : QStringLiteral("Authors"), authors);
        form->addRow(persian ? QStringLiteral("سال") : QStringLiteral("Year"), year);
        form->addRow(QStringLiteral("DOI"), doi);
        form->addRow(QStringLiteral("URL"), url);
        form->addRow(persian ? QStringLiteral("یادداشت") : QStringLiteral("Notes"), notes);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
        form->addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() != QDialog::Accepted) return;
        ResearchSource source;
        source.title = title->text();
        source.authors = authors->text();
        source.publicationYear = year->text();
        source.doi = doi->text();
        source.url = url->text();
        source.notes = notes->toPlainText();
        QString error;
        if (!store_.addResearchSource(*projectId_, source, &error)) {
            QMessageBox::warning(this, QStringLiteral("Yadavarifork"), error);
            return;
        }
        refreshResearchSources();
        researchStatus_->setText(persian
            ? QStringLiteral("منبع ثبت شد؛ صحت DOI و نشانی باید جداگانه بررسی شود.")
            : QStringLiteral("Source recorded; DOI and URL still require independent verification."));
    }


    void importScholarCitations() {
        if (!projectId_) return;
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        QDialog dialog(this);
        dialog.setWindowTitle(persian ? QStringLiteral("ورود گروهی استنادهای Google Scholar") :
                                        QStringLiteral("Bulk import Google Scholar citations"));
        dialog.resize(900, 700);
        auto *layout = new QVBoxLayout(&dialog);
        auto *guide = new QLabel(&dialog);
        guide->setWordWrap(true);
        guide->setText(persian
            ? QStringLiteral("راهنمای مرحله‌به‌مرحله:\n"
                "۱) در مرورگر وارد Google Scholar شوید و موضوع پژوهش را جست‌وجو کنید.\n"
                "۲) برای هر مقاله واجد شرایط، روی گزینهٔ Cite / نقل‌قول زیر همان نتیجه بزنید.\n"
                "۳) در پنجرهٔ بازشده، BibTeX را انتخاب کنید تا فایل .bib همان مقاله دانلود شود؛ برای ۲۰ یا ۵۰ مقاله، همین کار را برای مقاله‌های انتخابی تکرار کنید.\n"
                "۴) با دکمهٔ افزودن فایل، فایل‌های .bib، .ris، .enw یا .csv را انتخاب کنید (در هر نوبت یک فرمت)؛ یا متن چند رکورد هم‌نوع را پشت سر هم در کادر بچسبانید.\n"
                "۵) دکمهٔ «بررسی و ورود» را بزنید. می‌توانید ۲۰، ۵۰ یا تعداد بیشتری رکورد را در یک نوبت وارد کنید. موارد تکراری و رکوردهای بدون عنوان معتبر کنار گذاشته می‌شوند.\n\n"
                "نکته: Google Scholar API عمومی برای ورود انبوه خودکار ارائه نمی‌کند؛ این روش بر خروجی‌ای تکیه دارد که خودتان از رابط Scholar می‌گیرید. منابع پس از ورود همچنان «تأییدنشده» هستند و DOI/فراداده باید بازبینی شود.")
            : QStringLiteral("Step-by-step guide:\\n"
                "1) Open Google Scholar in your browser and search for your research topic.\\n"
                "2) For each relevant result, click Cite below the article.\\n"
                "3) Choose BibTeX to download that article's .bib file; repeat for each of the 20 or 50 selected articles.\\n"
                "4) Use Add citation files to load .bib, .ris, .enw, or .csv files (one format per batch), or paste multiple records of the same format below.\\n"
                "5) Select Review and import. You can import 20, 50, or more records in one batch. Duplicates and records without a valid title are skipped.\\n\\n"
                "Google Scholar does not provide a public bulk API. This workflow imports exports you obtain manually; imported records remain unverified until their DOI and metadata are checked."));
        layout->addWidget(guide);
        auto *openScholar = new QPushButton(persian ? QStringLiteral("بازکردن Google Scholar") :
                                                      QStringLiteral("Open Google Scholar"), &dialog);
        layout->addWidget(openScholar);
        connect(openScholar, &QPushButton::clicked, &dialog, [] {
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://scholar.google.com/")));
        });
        auto *input = new QTextEdit(&dialog);
        input->setObjectName(QStringLiteral("scholarCitationBatchInput"));
        input->setPlaceholderText(persian
            ? QStringLiteral("@article{...} یا TY  - JOUR ... ER  - را برای چند مقاله اینجا بچسبانید؛ هر رکورد BibTeX/RIS جداگانه.")
            : QStringLiteral("Paste multiple @article{...} BibTeX entries or TY  - ... ER  - RIS records here."));
        layout->addWidget(input, 1);
        auto *loadFiles = new QPushButton(persian ? QStringLiteral("افزودن فایل‌های خروجی Cite") :
                                                    QStringLiteral("Add citation files"), &dialog);
        layout->addWidget(loadFiles);
        connect(loadFiles, &QPushButton::clicked, &dialog, [input, &dialog, persian] {
            const QStringList files = QFileDialog::getOpenFileNames(&dialog,
                persian ? QStringLiteral("انتخاب فایل‌های استناد") : QStringLiteral("Select citation export files"),
                QString(), QStringLiteral("Citation files (*.bib *.ris *.enw *.txt *.csv);;All files (*.*)"));
            QString selectedFormat;
            for (const QString &path : files) {
                const QString currentFormat = QFileInfo(path).suffix().toLower();
                if (!selectedFormat.isEmpty() && !currentFormat.isEmpty() && currentFormat != selectedFormat) {
                    QMessageBox::information(&dialog, QStringLiteral("Yadavarifork"),
                        persian ? QStringLiteral("در هر نوبت فایل‌های یک فرمت را انتخاب کنید؛ برای فرمت دیگر یک بار دیگر ورود را انجام دهید.") :
                                  QStringLiteral("Select files of one export format per batch; import another format in a separate batch."));
                    return;
                }
                if (!currentFormat.isEmpty()) selectedFormat = currentFormat;
            }
            QString combined = input->toPlainText();
            for (const QString &path : files) {
                QFile file(path);
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
                const QString contents = QString::fromUtf8(file.readAll()).trimmed();
                if (contents.isEmpty()) continue;
                if (!combined.isEmpty()) combined += QStringLiteral("\n\n");
                combined += contents;
            }
            input->setPlainText(combined);
        });
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
        buttons->button(QDialogButtonBox::Save)->setText(persian ? QStringLiteral("بررسی و ورود منابع") :
                                                               QStringLiteral("Review and import"));
        buttons->button(QDialogButtonBox::Cancel)->setText(persian ? QStringLiteral("انصراف") :
                                                            QStringLiteral("Cancel"));
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        if (dialog.exec() != QDialog::Accepted) return;

        const QString raw = input->toPlainText().trimmed();
        if (raw.isEmpty()) return;
        QList<ResearchSource> parsed;
        auto field = [](const QString &entry, const QStringList &keys) {
            for (const QString &key : keys) {
                QRegularExpression re(QStringLiteral("(?is)(?:^|[,\\n])\\s*%1\\s*=\\s*[\\{\\\"]([^\\}\\\"]+)[\\}\\\"]")
                                      .arg(QRegularExpression::escape(key)));
                const auto match = re.match(entry);
                if (match.hasMatch()) return match.captured(1).trimmed();
            }
            return QString();
        };
        auto addParsed = [&](const QString &title, const QString &authors, const QString &year,
                             const QString &doi, const QString &url, const QString &venue) {
            ResearchSource source;
            source.title = title.trimmed();
            source.authors = authors.trimmed();
            source.publicationYear = year.trimmed();
            source.doi = doi.trimmed();
            source.url = url.trimmed();
            source.notes = QStringLiteral("Imported from user-provided Google Scholar Cite export; venue: %1; unverified").arg(venue.trimmed());
            if (!source.title.isEmpty()) parsed.push_back(source);
        };

        QRegularExpression bibStart(QStringLiteral("@[A-Za-z]+\\s*[\\{\\(]"));
        auto bibMatches = bibStart.globalMatch(raw);
        QList<int> starts;
        while (bibMatches.hasNext()) starts.push_back(bibMatches.next().capturedStart());
        if (!starts.isEmpty()) {
            for (int i = 0; i < starts.size(); ++i) {
                const QString entry = raw.mid(starts[i], (i + 1 < starts.size() ? starts[i + 1] : raw.size()) - starts[i]);
                addParsed(field(entry, {QStringLiteral("title")}), field(entry, {QStringLiteral("author")}),
                          field(entry, {QStringLiteral("year"), QStringLiteral("date")}),
                          field(entry, {QStringLiteral("doi")}), field(entry, {QStringLiteral("url"), QStringLiteral("howpublished")}),
                          field(entry, {QStringLiteral("journal"), QStringLiteral("booktitle"), QStringLiteral("publisher")}));
            }
        } else if (raw.contains(QRegularExpression(QStringLiteral("(?im)^TY  -")))) {
            const QStringList entries = raw.split(QRegularExpression(QStringLiteral("(?im)^ER  -.*$")), Qt::SkipEmptyParts);
            for (const QString &entry : entries) {
                auto ris = [&](const QString &tag) {
                    QRegularExpression re(QStringLiteral("(?im)^%1  -\\s*(.+)$").arg(QRegularExpression::escape(tag)));
                    QStringList values;
                    auto matches = re.globalMatch(entry);
                    while (matches.hasNext()) values << matches.next().captured(1).trimmed();
                    return values.join(QStringLiteral("; "));
                };
                addParsed(ris(QStringLiteral("TI")).isEmpty() ? ris(QStringLiteral("T1")) : ris(QStringLiteral("TI")),
                          ris(QStringLiteral("AU")).isEmpty() ? ris(QStringLiteral("A1")) : ris(QStringLiteral("AU")),
                          ris(QStringLiteral("PY")).isEmpty() ? ris(QStringLiteral("Y1")) : ris(QStringLiteral("PY")),
                          ris(QStringLiteral("DO")), ris(QStringLiteral("UR")), ris(QStringLiteral("JO")).isEmpty() ? ris(QStringLiteral("JF")) : ris(QStringLiteral("JO")));
            }
        } else if (raw.section(QLatin1Char('\n'), 0, 0).contains(QStringLiteral("title"), Qt::CaseInsensitive)) {
            auto csvCells = [](const QString &line) {
                QStringList cells;
                QString cell;
                bool quoted = false;
                for (int i = 0; i < line.size(); ++i) {
                    const QChar ch = line[i];
                    if (ch == QLatin1Char('"')) {
                        if (quoted && i + 1 < line.size() && line[i + 1] == QLatin1Char('"')) {
                            cell += QLatin1Char('"');
                            ++i;
                        } else quoted = !quoted;
                    } else if (ch == QLatin1Char(',') && !quoted) {
                        cells << cell.trimmed();
                        cell.clear();
                    } else cell += ch;
                }
                cells << cell.trimmed();
                return cells;
            };
            const QStringList lines = raw.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
            if (!lines.isEmpty()) {
                QStringList headers = csvCells(lines.first());
                for (QString &header : headers) header = header.toLower().remove(QRegularExpression(QStringLiteral("[^a-z]")));
                auto column = [&](const QStringList &names) {
                    for (int i = 0; i < headers.size(); ++i)
                        if (names.contains(headers[i])) return i;
                    return -1;
                };
                const int titleCol = column({QStringLiteral("title"), QStringLiteral("articletitle")});
                const int authorCol = column({QStringLiteral("authors"), QStringLiteral("author")});
                const int yearCol = column({QStringLiteral("year"), QStringLiteral("publicationyear")});
                const int doiCol = column({QStringLiteral("doi")});
                const int urlCol = column({QStringLiteral("url"), QStringLiteral("link")});
                const int venueCol = column({QStringLiteral("publication"), QStringLiteral("journal"), QStringLiteral("venue")});
                for (int row = 1; row < lines.size(); ++row) {
                    const QStringList cells = csvCells(lines[row]);
                    auto valueAt = [&](int index) { return index >= 0 && index < cells.size() ? cells[index] : QString(); };
                    addParsed(valueAt(titleCol), valueAt(authorCol), valueAt(yearCol),
                              valueAt(doiCol), valueAt(urlCol), valueAt(venueCol));
                }
            }
        } else if (raw.contains(QRegularExpression(QStringLiteral("(?im)^%0\\\\s+")))) {
            const QStringList entries = raw.split(QRegularExpression(QStringLiteral("(?im)^%0\\s+")), Qt::SkipEmptyParts);
            for (const QString &entry : entries) {
                auto endnote = [&](const QString &tag) {
                    QRegularExpression re(QStringLiteral("(?im)^%1\\s+(.+)$").arg(QRegularExpression::escape(tag)));
                    const auto match = re.match(entry);
                    return match.hasMatch() ? match.captured(1).trimmed() : QString();
                };
                addParsed(endnote(QStringLiteral("%T")), endnote(QStringLiteral("%A")),
                          endnote(QStringLiteral("%D")), endnote(QStringLiteral("%R")),
                          endnote(QStringLiteral("%U")), endnote(QStringLiteral("%J")));
            }
        } else {
            const QStringList lines = raw.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
            for (const QString &line : lines) {
                const QString doi = QRegularExpression(QStringLiteral("10\\.\\d{4,9}/[^\\s,;]+"), QRegularExpression::CaseInsensitiveOption)
                                        .match(line).captured(0);
                const QString url = QRegularExpression(QStringLiteral("https?://[^\\s]+"), QRegularExpression::CaseInsensitiveOption)
                                        .match(line).captured(0);
                addParsed(line, QString(), QString(), doi, url, QString());
            }
        }

        QString error;
        const auto previous = store_.loadResearchSources(*projectId_, &error);
        if (!error.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Yadavarifork"), error);
            return;
        }
        QSet<QString> known;
        for (const auto &source : previous) {
            if (!source.doi.trimmed().isEmpty()) known.insert(source.doi.trimmed().toLower());
            if (!source.title.trimmed().isEmpty()) known.insert(source.title.simplified().toLower());
        }
        int imported = 0, duplicates = 0, invalid = 0;
        for (ResearchSource &source : parsed) {
            const QString doiKey = source.doi.trimmed().toLower();
            const QString titleKey = source.title.simplified().toLower();
            if (titleKey.isEmpty()) { ++invalid; continue; }
            if ((!doiKey.isEmpty() && known.contains(doiKey)) || known.contains(titleKey)) {
                ++duplicates;
                continue;
            }
            if (!doiKey.isEmpty()) known.insert(doiKey);
            known.insert(titleKey);
            if (store_.addResearchSource(*projectId_, source, &error)) ++imported;
            else { ++invalid; error.clear(); }
        }
        refreshResearchSources();
        researchStatus_->setText(persian
            ? QStringLiteral("ورود گروهی تمام شد: %1 منبع افزوده شد؛ %2 تکراری و %3 نامعتبر/بدون عنوان کنار گذاشته شد. همه منابع واردشده تا راستی‌آزمایی دستی تأییدنشده‌اند.")
                .arg(imported).arg(duplicates).arg(invalid + (parsed.size() - imported - duplicates - invalid))
            : QStringLiteral("Bulk import finished: %1 sources added; %2 duplicates and %3 invalid/incomplete records skipped. Imported sources remain unverified.")
                .arg(imported).arg(duplicates).arg(invalid + (parsed.size() - imported - duplicates - invalid)));
    }

    void deleteResearchSource() {
        if (!projectId_ || !researchSourcesList_->currentItem()) return;
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        const auto answer = QMessageBox::question(this, QStringLiteral("Yadavarifork"),
            persian ? QStringLiteral("این منبع حذف شود؟") : QStringLiteral("Delete this source?"));
        if (answer != QMessageBox::Yes) return;
        const qint64 sourceId = researchSourcesList_->currentItem()->data(Qt::UserRole).toLongLong();
        QString error;
        if (!store_.deleteResearchSource(*projectId_, sourceId, &error)) {
            researchStatus_->setText(error);
            return;
        }
        refreshResearchSources();
    }

    QStringList finalDetailKeys() const {
        return {
            QStringLiteral("degree"), QStringLiteral("field"), QStringLiteral("specialization"),
            QStringLiteral("supervisor"), QStringLiteral("consultant"),
            QStringLiteral("student_name"), QStringLiteral("student_id"),
            QStringLiteral("student_information"), QStringLiteral("supervisor_information"),
            QStringLiteral("academic_records"), QStringLiteral("previous_theses")
        };
    }

    QStringList administrativeSectionKeys() const {
        return {
            QStringLiteral("archive_number"), QStringLiteral("approval_date"),
            QStringLiteral("supervisor_confirmation"), QStringLiteral("consultant_confirmation"),
            QStringLiteral("project_information"), QStringLiteral("student_commitment"),
            QStringLiteral("supervisor_commitment"), QStringLiteral("researcher_commitment"),
            QStringLiteral("committee_opinion"), QStringLiteral("scientific_evaluation"),
            QStringLiteral("group_council_minutes"),
            QStringLiteral("faculty_research_council_opinion"), QStringLiteral("final_approval")
        };
    }

    bool saveFinalDetails(const QMap<QString, QString> &values) {
        if (!projectId_)
            return false;
        QString error;
        for (const QString &key : finalDetailKeys()) {
            if (!store_.saveSection(*projectId_, key, values.value(key), &error)) {
                editorStatus_->setText(error);
                return false;
            }
        }
        if (!store_.saveProjectMetadata(*projectId_, QStringLiteral("final_details_status"),
                                         QStringLiteral("completed"), &error)) {
            editorStatus_->setText(error);
            return false;
        }
        finalDetailsReviewed_ = true;
        editorStatus_->setText(language_->currentData().toString() == QStringLiteral("fa")
            ? QStringLiteral("مشخصات فردی و استاد ذخیره شد.")
            : QStringLiteral("Student and supervisor details saved."));
        return true;
    }

    bool collectFinalDetails(bool forceDialog) {
        if (!projectId_)
            return false;
        if (finalDetailsReviewed_ && !forceDialog)
            return true;
        if (!saveActiveProposalSection())
            return false;
        QString error;
        const auto current = store_.loadSections(*projectId_, &error);
        if (!error.isEmpty()) {
            editorStatus_->setText(error);
            return false;
        }
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        QDialog dialog(this);
        dialog.setWindowTitle(persian
            ? QStringLiteral("مشخصات پایانی دانشجو و استاد")
            : QStringLiteral("Final student and supervisor details"));
        auto *form = new QFormLayout(&dialog);
        auto *intro = new QLabel(persian
            ? QStringLiteral("این اطلاعات در پایان تکمیل پروپوزال دریافت می‌شود. مواردی را که اکنون نمی‌دانید خالی بگذارید؛ می‌توانید بعداً دوباره ویرایش کنید.")
            : QStringLiteral("These details are collected at the end of proposal preparation. Leave unknown items empty; you can edit them later."));
        intro->setWordWrap(true);
        form->addRow(intro);
        QMap<QString, QLineEdit *> lineInputs;
        QMap<QString, QTextEdit *> multilineInputs;
        for (const QString &key : finalDetailKeys()) {
            if (key == QStringLiteral("student_information") ||
                key == QStringLiteral("supervisor_information") ||
                key == QStringLiteral("academic_records") ||
                key == QStringLiteral("previous_theses")) {
                auto *input = new QTextEdit(&dialog);
                input->setObjectName(QStringLiteral("finalDetail_") + key);
                input->setPlainText(current.value(key));
                input->setMinimumHeight(88);
                multilineInputs.insert(key, input);
                form->addRow(sectionLabel(key, persian), input);
            } else {
                auto *input = new QLineEdit(&dialog);
                input->setObjectName(QStringLiteral("finalDetail_") + key);
                input->setText(current.value(key));
                lineInputs.insert(key, input);
                form->addRow(sectionLabel(key, persian), input);
            }
        }
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
        auto *skipButton = new QPushButton(persian
            ? QStringLiteral("فعلاً رد شو؛ بعداً تکمیل می‌کنم")
            : QStringLiteral("Skip for now; complete later"), &dialog);
        skipButton->setObjectName(QStringLiteral("skipFinalDetailsButton"));
        form->addRow(buttons);
        form->addRow(skipButton);
        bool skipped = false;
        connect(skipButton, &QPushButton::clicked, &dialog, [&dialog, &skipped] {
            skipped = true;
            dialog.accept();
        });
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() != QDialog::Accepted)
            return false;
        if (skipped) {
            if (!store_.saveProjectMetadata(*projectId_, QStringLiteral("final_details_status"),
                                            QStringLiteral("skipped"), &error)) {
                editorStatus_->setText(error);
                return false;
            }
            finalDetailsReviewed_ = true;
            editorStatus_->setText(persian
                ? QStringLiteral("مشخصات فردی و استاد فعلاً خالی ماند؛ هر زمان خواستید از دکمه تکمیل مشخصات اقدام کنید.")
                : QStringLiteral("Student and supervisor details were left blank; use the details button whenever you are ready."));
            return true;
        }
        QMap<QString, QString> values;
        for (auto it = lineInputs.cbegin(); it != lineInputs.cend(); ++it)
            values.insert(it.key(), it.value()->text().trimmed());
        for (auto it = multilineInputs.cbegin(); it != multilineInputs.cend(); ++it)
            values.insert(it.key(), it.value()->toPlainText().trimmed());
        return saveFinalDetails(values);
    }

    QMap<QString, QString> currentSectionLabels() const {
        QMap<QString, QString> labels;
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        for (const QString &key : ProjectStore::proposalSectionKeys())
            labels.insert(key, sectionLabel(key, persian));
        return labels;
    }

    QMap<QString, QString> currentSections(QString *error = nullptr) {
        if (!saveActiveProposalSection()) {
            if (error) *error = editorStatus_->text();
            return {};
        }
        if (!projectId_) {
            if (error) *error = QStringLiteral("No active project.");
            return {};
        }
        return store_.loadSections(*projectId_, error);
    }

    void exportPdf() {
        if (!collectFinalDetails(false))
            return;
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
        if (!collectFinalDetails(false))
            return;
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

    static QString imageFormatFromFilter(const QString &selectedFilter) {
        if (selectedFilter.contains(QStringLiteral("PNG"), Qt::CaseInsensitive))
            return QStringLiteral("png");
        if (selectedFilter.contains(QStringLiteral("BMP"), Qt::CaseInsensitive))
            return QStringLiteral("bmp");
        return QStringLiteral("jpg");
    }

    void exportPng() {
        if (!collectFinalDetails(false))
            return;
        QString error;
        const auto sections = currentSections(&error);
        if (!error.isEmpty()) { editorStatus_->setText(error); return; }

        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        QString selectedFilter = persian
            ? QStringLiteral("تصویر JPEG (*.jpg *.jpeg)")
            : QStringLiteral("JPEG images (*.jpg *.jpeg)");
        const QString filters = persian
            ? QStringLiteral("تصویر JPEG (*.jpg *.jpeg);;تصویر PNG (*.png);;تصویر BMP (*.bmp)")
            : QStringLiteral("JPEG images (*.jpg *.jpeg);;PNG images (*.png);;BMP images (*.bmp)");
        QString path = QFileDialog::getSaveFileName(
            this,
            persian ? QStringLiteral("ذخیره صفحات به‌صورت تصویر") : QStringLiteral("Export proposal pages as images"),
            QStringLiteral("proposal.jpg"), filters, &selectedFilter);
        if (path.isEmpty())
            return;

        const QString format = imageFormatFromFilter(selectedFilter);
        const QString extension = format;

        QFileInfo selectedInfo(path);
        QString baseName = selectedInfo.completeBaseName().trimmed();
        if (baseName.isEmpty())
            baseName = QStringLiteral("proposal");
        path = QDir(selectedInfo.absolutePath()).filePath(baseName + QLatin1Char('.') + extension);

        if (DocumentExport::toImagePages(sections, currentSectionLabels(), projectTitle_->text(),
                                         language_->currentData().toString(), path, format, &error)) {
            editorStatus_->setText(persian
                ? QStringLiteral("تصاویر صفحات با قالب %1 ذخیره شدند: %2")
                      .arg(format.toUpper(), QFileInfo(path).absolutePath())
                : QStringLiteral("%1 page images saved in: %2")
                      .arg(format.toUpper(), QFileInfo(path).absolutePath()));
        } else {
            editorStatus_->setText(error);
        }
    }

    void printDocument() {
        if (!collectFinalDetails(false))
            return;
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
            } else {
                populateResearchSectionList();
                refreshResearchSources();
                pages_->setCurrentIndex(5);
            }
            updateNavigation();
            return;
        }
        QString error;
        projectId_ = store_.createProject(
            projectTitle_->text(), language_->currentData().toString(),
            aiMode_->currentData().toString(), taskMode_->currentData().toString(), &error);
        finalDetailsReviewed_ = false;
        if (!projectId_) {
            summaryStatus_->setText(error.isEmpty()
                ? (persian ? QStringLiteral("ایجاد پروژه ناموفق بود.") : QStringLiteral("Project creation failed."))
                : error);
            return;
        }
        language_->setEnabled(false);
        aiMode_->setEnabled(false);
        taskMode_->setEnabled(false);
        projectTitle_->setEnabled(false);
        if (taskMode_->currentData().toString() == QStringLiteral("proposal")) {
            const QString titleKey = language_->currentData().toString() == QStringLiteral("fa")
                ? QStringLiteral("title_fa") : QStringLiteral("title_en");
            if (!store_.saveSection(*projectId_, titleKey, projectTitle_->text().trimmed(), &error)) {
                summaryStatus_->setText(error);
                return;
            }
            populateSectionList();
            pages_->setCurrentIndex(4);
        } else {
            populateResearchSectionList();
            refreshResearchSources();
            pages_->setCurrentIndex(5);
            summaryStatus_->setText(persian
                ? QStringLiteral("کارگاه پژوهش محلی آماده است. یادداشت‌ها و منابع دستی در SQLite ذخیره می‌شوند؛ منابع تا بررسی مستقل تأییدنشده‌اند.")
                : QStringLiteral("Local research workspace is ready. Notes and manually entered sources are stored in SQLite; sources remain unverified until independently checked."));
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
    QLabel *editorNotice_{};
    QPushButton *back_{};
    QPushButton *about_{};
    QPushButton *next_{};
    QPushButton *saveSection_{};
    QPushButton *finalDetailsButton_{};
    QPushButton *exportPdf_{};
    QPushButton *exportDocx_{};
    QPushButton *exportPng_{};
    QPushButton *printDocument_{};
    QListWidget *researchSectionList_{};
    QTextEdit *researchEditor_{};
    QListWidget *researchSourcesList_{};
    QGroupBox *researchSourcesGroup_{};
    QPushButton *saveResearchSection_{};
    QPushButton *addResearchSource_{};
    QPushButton *importScholarCitations_{};
    QPushButton *deleteResearchSource_{};
    QLabel *researchStatus_{};
    QPushButton *runResearchEngine_{};
    QPushButton *cancelResearchEngine_{};
    ResearchEngineRunner *engineRunner_{};
    QPushButton *openExistingProject_{};
    QPushButton *newProject_{};
    QList<QLabel *> headings_;
    std::optional<qint64> projectId_;
    QString currentProposalSectionKey_;
    QString currentResearchSectionKey_;
    QString currentResearchRunOutput_;
    std::optional<qint64> currentResearchRunProjectId_;
    QPushButton *researchRunHistory_{};
    bool researchRunCancellationRequested_{false};
    bool noApiResearchRun_{false};
    int noApiImportedSourceCount_{0};
    bool proposalSectionDirty_{false};
    bool researchSectionDirty_{false};
    bool finalDetailsReviewed_{false};
    bool loadingProposalSection_{false};
    bool loadingResearchSection_{false};
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    // Keep the legacy application-data namespace so existing projects remain discoverable.
    QCoreApplication::setApplicationName(QStringLiteral("yadavarifork"));
    app.setWindowIcon(QIcon(QStringLiteral(":/brand/shiraztipsy.svg")));
    app.setStyleSheet(QStringLiteral(
        "QWidget { background-color:#091a35; color:#eaf2ff; font-size:13px; }"
        "QMainWindow, QDialog { background-color:#07152d; }"
        "QLabel { background:transparent; }"
        "QPushButton { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #244b7e,stop:1 #122b50); color:#f5f7ff; border:1px solid #527eb2; border-radius:9px; padding:8px 14px; }"
        "QPushButton:hover { border:1px solid #f3c66b; background-color:#315b8d; }"
        "QPushButton:pressed { background-color:#102443; }"
        "QPushButton:disabled { color:#71839c; border-color:#263b58; }"
        "QLineEdit, QTextEdit, QListWidget, QComboBox { background-color:#102746; color:#edf4ff; border:1px solid #385d88; border-radius:7px; padding:7px; selection-background-color:#b8873c; }"
        "QComboBox QAbstractItemView { background:#102746; color:#edf4ff; selection-background-color:#315b8d; }"
        "QGroupBox { border:1px solid #45688f; border-radius:10px; margin-top:12px; padding:10px; font-weight:600; }"
        "QGroupBox::title { subcontrol-origin:margin; left:12px; padding:0 6px; color:#f2c56b; }"
        "QScrollBar:vertical { background:#0a1b34; width:12px; margin:0; }"
        "QScrollBar::handle:vertical { background:#42658c; border-radius:5px; min-height:24px; }"
        "QCheckBox { spacing:8px; }"
        "QLabel#proposalAdministrativeNotice { color:#f1c875; }"
        "QPushButton#aboutInstituteButton, QPushButton#aboutCloseButton { border:1px solid #c99a4b; color:#ffe0a0; }"
        "QMessageBox { background:#091a35; }"
    ));
    if (app.arguments().contains(QStringLiteral("--bridge-child-test"))) {
        std::cout << "bridge-child-ok\n";
        std::cout << "GEMINI_API_KEY=" << qEnvironmentVariable("GEMINI_API_KEY").toStdString() << '\n';
        return 0;
    }
    if (app.arguments().contains(QStringLiteral("--bridge-child-sleep-test"))) {
        std::cout << "bridge-child-sleeping\\n" << std::flush;
        QThread::msleep(30000);
        return 0;
    }
    if (app.arguments().contains(QStringLiteral("--smoke-test"))) {
        QTemporaryDir temporaryData;
        if (!temporaryData.isValid())
            return 3;
        ProjectStore store(temporaryData.filePath(QStringLiteral("smoke.sqlite3")));
        QString error;
        if (!store.initialize(&error))
            return 4;
        WelcomeWindow window(store);
        return window.runSmokeTest();
    }

    QString dataDirectory = qEnvironmentVariable("YADAVARIFORK_DATA_DIR").trimmed();
    if (dataDirectory.isEmpty()) {
        dataDirectory = QDir(QCoreApplication::applicationDirPath())
                            .filePath(QStringLiteral("data"));
    }
    if (!QDir().mkpath(dataDirectory) || !QFileInfo(dataDirectory).isWritable()) {
        dataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (dataDirectory.isEmpty() || !QDir().mkpath(dataDirectory) ||
            !QFileInfo(dataDirectory).isWritable()) {
            QMessageBox::critical(nullptr, QStringLiteral("ShirazTipsy Research Institute"),
                                  QStringLiteral("Cannot create a writable application data directory."));
            return 5;
        }
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
