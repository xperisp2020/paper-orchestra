#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QFile>
#include <QFileDialog>
#include <iostream>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QSignalBlocker>
#include <QStringList>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <optional>

#include "project_store.h"
#include "document_export.h"
#include "research_engine_runner.h"

#include <QDateTime>
#include <QEventLoop>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>

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
        sourceButtons->addWidget(saveResearchSection_);
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
        engineButtons->addWidget(runResearchEngine_);
        engineButtons->addWidget(cancelResearchEngine_);
        static_cast<QVBoxLayout *>(researchPage->layout())->addLayout(engineButtons);
        researchStatus_ = new QLabel;
        researchStatus_->setWordWrap(true);
        static_cast<QVBoxLayout *>(researchPage->layout())->addWidget(researchStatus_);
        engineRunner_ = new ResearchEngineRunner(this);
        connect(runResearchEngine_, &QPushButton::clicked, this, [this] { configureAndRunResearchEngine(); });
        connect(cancelResearchEngine_, &QPushButton::clicked, this, [this] {
            if (engineRunner_) engineRunner_->cancel();
        });
        connect(engineRunner_, &ResearchEngineRunner::started, this, [this] {
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
                const QString eventName = event.value(QStringLiteral("event")).toString();
                if (!eventName.isEmpty()) {
                    if (eventName == QStringLiteral("pipeline_started"))
                        displayLine = language_->currentData().toString() == QStringLiteral("fa")
                            ? QStringLiteral("رویداد: آغاز تولید مقاله")
                            : QStringLiteral("Event: paper generation started");
                    else if (eventName == QStringLiteral("pipeline_completed"))
                        displayLine = language_->currentData().toString() == QStringLiteral("fa")
                            ? QStringLiteral("رویداد: تولید مقاله پایان یافت")
                            : QStringLiteral("Event: paper generation completed");
                    else if (eventName == QStringLiteral("pipeline_failed"))
                        displayLine = event.value(QStringLiteral("error")).toString();
                }
            }
            const QString current = researchStatus_->text();
            const QString combined = current + (current.isEmpty() ? QString() : QStringLiteral("\n")) + displayLine;
            researchStatus_->setText(combined.right(6000));
        });
        connect(engineRunner_, &ResearchEngineRunner::finished, this, [this](int exitCode, bool normalExit) {
            runResearchEngine_->setEnabled(true);
            cancelResearchEngine_->setEnabled(false);
            const bool success = normalExit && exitCode == 0;
            bool outputRecorded = false;
            if (success && !currentResearchRunOutput_.isEmpty() &&
                QFileInfo(currentResearchRunOutput_).isDir()) {
                outputRecorded = store_.saveProjectMetadata(
                    *projectId_, QStringLiteral("research_engine_last_output"),
                    currentResearchRunOutput_, nullptr);
            }
            currentResearchRunOutput_.clear();
            const QString resultMessage = success
                ? (outputRecorded
                    ? (language_->currentData().toString() == QStringLiteral("fa")
                        ? QStringLiteral("موتور با موفقیت پایان یافت؛ مسیر خروجی در پروژه ثبت شد.")
                        : QStringLiteral("Engine completed successfully; output path saved in the project."))
                    : (language_->currentData().toString() == QStringLiteral("fa")
                        ? QStringLiteral("فرایند با کد صفر پایان یافت، اما پوشه خروجی یا ثبت مسیر آن تأیید نشد.")
                        : QStringLiteral("Process exited with code zero, but output folder or metadata persistence was not verified.")))
                : (language_->currentData().toString() == QStringLiteral("fa")
                    ? QStringLiteral("اجرای موتور موفق نبود؛ کد خروج: %1").arg(exitCode)
                    : QStringLiteral("Engine run failed; exit code: %1").arg(exitCode));
            researchStatus_->setText(researchStatus_->text() + QStringLiteral("\n") + resultMessage);
        });
        connect(engineRunner_, &ResearchEngineRunner::failed, this, [this](const QString &message) {
            runResearchEngine_->setEnabled(true);
            cancelResearchEngine_->setEnabled(false);
            currentResearchRunOutput_.clear();
            researchStatus_->setText(researchStatus_->text() + QStringLiteral("\n") + message);
        });
        pages_->addWidget(researchPage);

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
        sectionList_->setCurrentRow(problemRow);
        sectionEditor_->setPlainText(QStringLiteral("Persisted by GUI smoke test"));
        sectionList_->setCurrentRow(problemRow == 0 ? 1 : 0);
        sectionList_->setCurrentRow(problemRow);
        if (sectionEditor_->toPlainText() != QStringLiteral("Persisted by GUI smoke test"))
            return 29;
        QString storageError;
        const auto saved = store_.loadSections(*projectId_, &storageError);
        if (saved.value(QStringLiteral("problem_statement")) !=
            QStringLiteral("Persisted by GUI smoke test"))
            return 29;
        for (const QString &key : finalDetailKeys()) {
            if (!saved.value(key).isEmpty())
                return 60;
        }
        for (const QString &key : administrativeSectionKeys()) {
            if (!saved.value(key).isEmpty())
                return 61;
        }
        if (!store_.saveProjectMetadata(*projectId_, QStringLiteral("final_details_status"),
                                        QStringLiteral("skipped"), &storageError))
            return 66;
        WelcomeWindow reopenedSkippedProposal(store_);
        if (!reopenedSkippedProposal.openProjectById(*projectId_) ||
            !reopenedSkippedProposal.finalDetailsReviewed_)
            return 67;
        const auto stillEmpty = store_.loadSections(*projectId_, &storageError);
        for (const QString &key : finalDetailKeys()) {
            if (!stillEmpty.value(key).isEmpty())
                return 68;
        }
        QMap<QString, QString> smokeFinalDetails;
        for (const QString &key : finalDetailKeys())
            smokeFinalDetails.insert(key, QStringLiteral("Smoke test: ") + key);
        smokeFinalDetails.insert(QStringLiteral("student_information"),
            QStringLiteral("Smoke test: student_information\nSecond line"));
        smokeFinalDetails.insert(QStringLiteral("supervisor_information"),
            QStringLiteral("Smoke test: supervisor_information\nSecond line"));
        if (!saveFinalDetails(smokeFinalDetails))
            return 62;
        WelcomeWindow reopenedProposal(store_);
        if (!reopenedProposal.openProjectById(*projectId_) ||
            !reopenedProposal.finalDetailsReviewed_)
            return 65;
        const auto detailsSaved = store_.loadSections(*projectId_, &storageError);
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
                ProjectStore::proposalSectionKeys().size() - finalDetailKeys().size())
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
            !reopenedResearch.taskMode_->isEnabled() || reopenedResearch.pages_->currentIndex() != 0)
            return 55;
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
        return 0;
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        if (engineRunner_ && engineRunner_->isRunning()) {
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
        exportPng_->setText(persian ? QStringLiteral("خروجی تصویر") : QStringLiteral("Export PNG"));
        printDocument_->setText(persian ? QStringLiteral("چاپ") : QStringLiteral("Print"));
        openExistingProject_->setText(persian ? QStringLiteral("بازکردن پروژه ذخیره‌شده") : QStringLiteral("Open saved project"));
        newProject_->setText(persian ? QStringLiteral("پروژه جدید") : QStringLiteral("New project"));
        saveResearchSection_->setText(persian ? QStringLiteral("ذخیره بخش") : QStringLiteral("Save section"));
        addResearchSource_->setText(persian ? QStringLiteral("افزودن منبع") : QStringLiteral("Add source"));
        deleteResearchSource_->setText(persian ? QStringLiteral("حذف منبع") : QStringLiteral("Delete source"));
        runResearchEngine_->setText(persian ? QStringLiteral("اجرای موتور چندعاملی") : QStringLiteral("Run multi-agent engine"));
        cancelResearchEngine_->setText(persian ? QStringLiteral("لغو اجرا") : QStringLiteral("Cancel run"));
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
        if (!sectionList_ || !saveActiveProposalSection())
            return;
        QString currentKey;
        if (sectionList_->currentItem())
            currentKey = sectionList_->currentItem()->data(Qt::UserRole).toString();
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        sectionList_->clear();
        const QStringList keys = ProjectStore::proposalSectionKeys();
        for (const QString &key : keys) {
            if (finalDetailKeys().contains(key))
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
        pages_->setCurrentIndex(0);
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

    void configureAndRunResearchEngine() {
        if (!projectId_ || taskMode_->currentData().toString() != QStringLiteral("research"))
            return;
        if (engineRunner_ && engineRunner_->isRunning())
            return;
        if (!saveActiveResearchSection())
            return;

        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        const QString selectedAiMode = aiMode_->currentData().toString();
        if (selectedAiMode == QStringLiteral("no_api")) {
            researchStatus_->setText(persian
                ? QStringLiteral("حالت بدون API فقط کارگاه محلی است و موتور مولد را اجرا نمی‌کند.")
                : QStringLiteral("No-API mode is a local workspace and cannot run the generative engine."));
            return;
        }
        if (selectedAiMode == QStringLiteral("free_tier")) {
            researchStatus_->setText(persian
                ? QStringLiteral("مسیریابی طرح رایگان هنوز در CLI موجود نیست؛ برای جلوگیری از استفاده ناخواسته از مدل پولی، اجرا مسدود شد.")
                : QStringLiteral("Free-tier routing is not implemented in the CLI. Execution is blocked to avoid unintentionally using a paid model."));
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
        auto *notice = new QLabel(persian
            ? QStringLiteral("این نسخه Python و وابستگی‌های موتور را همراه خود بسته‌بندی نکرده است. کلید Gemini باید از متغیر محیطی سیستم فراهم باشد؛ کلید هرگز در آرگومان، پایگاه داده یا گزارش ذخیره نمی‌شود.")
            : QStringLiteral("Python and engine dependencies are not bundled in this build. A Gemini key must be supplied through the process environment; credentials are never stored in arguments, the database, or logs."));
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
        const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        const QStringList keyNames{QStringLiteral("GEMINI_API_KEY"), QStringLiteral("GOOGLE_API_KEY"),
                                   QStringLiteral("GOOGLE_GENAI_API_KEY")};
        bool hasProviderKey = false;
        for (const QString &key : keyNames)
            hasProviderKey = hasProviderKey || (!environment.value(key).trimmed().isEmpty());
        if (!hasProviderKey) {
            fail(persian
                ? QStringLiteral("کلید Gemini در محیط سیستم پیدا نشد. GEMINI_API_KEY یا GOOGLE_API_KEY را در محیط سیستم تنظیم و برنامه را دوباره اجرا کنید.")
                : QStringLiteral("No Gemini key found in the process environment. Set GEMINI_API_KEY or GOOGLE_API_KEY and restart the application."));
            return;
        }

        const QString dataRoot = QDir(QCoreApplication::applicationDirPath())
                                     .filePath(QStringLiteral("data/research_runs"));
        if (!QDir().mkpath(dataRoot)) {
            fail(persian ? QStringLiteral("پوشه خروجی قابل ایجاد نیست.") :
                            QStringLiteral("Cannot create research run directory."));
            return;
        }
        const QString output = QDir(dataRoot).filePath(
            QStringLiteral("project_%1_%2").arg(*projectId_)
                .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"))));
        if (QFileInfo::exists(output)) {
            fail(persian ? QStringLiteral("مسیر خروجی از قبل وجود دارد؛ اجرای جدید متوقف شد.") :
                            QStringLiteral("Output path already exists; refusing to overwrite it."));
            return;
        }
        const QStringList arguments{
            script,
            QStringLiteral("--raw_materials_dir"), materials,
            QStringLiteral("--output_dir"), output,
            QStringLiteral("--latex_template_dir"), templateDir,
            QStringLiteral("--events_jsonl")
        };
        currentResearchRunOutput_ = output;
        researchStatus_->clear();
        if (!engineRunner_->start(python, arguments, QFileInfo(script).absolutePath(), environment)) {
            currentResearchRunOutput_.clear();
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
            QStringLiteral("student_information"), QStringLiteral("supervisor_information")
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
                key == QStringLiteral("supervisor_information")) {
                auto *input = new QTextEdit(&dialog);
                input->setPlainText(current.value(key));
                input->setMinimumHeight(88);
                multilineInputs.insert(key, input);
                form->addRow(sectionLabel(key, persian), input);
            } else {
                auto *input = new QLineEdit(&dialog);
                input->setText(current.value(key));
                lineInputs.insert(key, input);
                form->addRow(sectionLabel(key, persian), input);
            }
        }
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
        auto *skipButton = new QPushButton(persian
            ? QStringLiteral("فعلاً رد شو؛ بعداً تکمیل می‌کنم")
            : QStringLiteral("Skip for now; complete later"), &dialog);
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

    void exportPng() {
        if (!collectFinalDetails(false))
            return;
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
    bool proposalSectionDirty_{false};
    bool researchSectionDirty_{false};
    bool finalDetailsReviewed_{false};
    bool loadingProposalSection_{false};
    bool loadingResearchSection_{false};
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    if (app.arguments().contains(QStringLiteral("--bridge-child-test"))) {
        std::cout << "bridge-child-ok\n";
        std::cout << "GEMINI_API_KEY=" << qEnvironmentVariable("GEMINI_API_KEY").toStdString() << '\n';
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
