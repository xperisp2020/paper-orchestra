#include <QApplication>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

class WelcomeWindow final : public QMainWindow {
public:
    explicit WelcomeWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setObjectName(QStringLiteral("welcomeWindow"));
        setMinimumSize(680, 460);
        pages_ = new QStackedWidget;
        pages_->setObjectName(QStringLiteral("wizardPages"));

        auto *languagePage = makePage(QStringLiteral("languagePage"));
        language_ = new QComboBox;
        language_->setObjectName(QStringLiteral("languageSelector"));
        language_->addItem(QStringLiteral("فارسی"), QStringLiteral("fa"));
        language_->addItem(QStringLiteral("English"), QStringLiteral("en"));
        static_cast<QVBoxLayout *>(languagePage->layout())->addWidget(language_);
        pages_->addWidget(languagePage);

        auto *aiPage = makePage(QStringLiteral("aiPage"));
        aiMode_ = new QComboBox;
        aiMode_->setObjectName(QStringLiteral("aiModeSelector"));
        aiMode_->addItem(QStringLiteral("کاملاً رایگان، بدون API"), QStringLiteral("no_api"));
        aiMode_->addItem(QStringLiteral("سرویس‌های دارای طرح رایگان"), QStringLiteral("free_tier"));
        aiMode_->addItem(QStringLiteral("سرویس‌های پولی"), QStringLiteral("paid"));
        static_cast<QVBoxLayout *>(aiPage->layout())->addWidget(aiMode_);
        pages_->addWidget(aiPage);

        auto *taskPage = makePage(QStringLiteral("taskPage"));
        taskMode_ = new QComboBox;
        taskMode_->setObjectName(QStringLiteral("taskModeSelector"));
        taskMode_->addItem(QStringLiteral("تکمیل پروپوزال"), QStringLiteral("proposal"));
        taskMode_->addItem(QStringLiteral("انجام پژوهش"), QStringLiteral("research"));
        static_cast<QVBoxLayout *>(taskPage->layout())->addWidget(taskMode_);
        pages_->addWidget(taskPage);

        auto *summaryPage = makePage(QStringLiteral("summaryPage"));
        summary_ = new QLabel;
        summary_->setObjectName(QStringLiteral("selectionSummary"));
        summary_->setWordWrap(true);
        static_cast<QVBoxLayout *>(summaryPage->layout())->addWidget(summary_);
        pages_->addWidget(summaryPage);

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
            if (pages_->currentIndex() == 2)
                updateSummary();
            if (pages_->currentIndex() < pages_->count() - 1)
                pages_->setCurrentIndex(pages_->currentIndex() + 1);
            updateNavigation();
        });
        connect(language_, &QComboBox::currentIndexChanged, this, [this](int) {
            applyLanguage();
        });

        applyLanguage();
        updateNavigation();
    }

    bool runSmokeTest() {
        if (!language_ || !aiMode_ || !taskMode_ || !pages_ || pages_->count() != 4)
            return false;

        language_->setCurrentIndex(1);
        if (layoutDirection() != Qt::LeftToRight)
            return false;
        language_->setCurrentIndex(0);
        if (layoutDirection() != Qt::RightToLeft)
            return false;

        aiMode_->setCurrentIndex(1);
        taskMode_->setCurrentIndex(1);
        next_->click();
        next_->click();
        next_->click();
        if (pages_->currentIndex() != 3 || !summary_->text().contains(QStringLiteral("انجام پژوهش")))
            return false;
        back_->click();
        return pages_->currentIndex() == 2;
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
        layout->addStretch();
        headings_.append(heading);
        return page;
    }

    void applyLanguage() {
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        const auto direction = persian ? Qt::RightToLeft : Qt::LeftToRight;
        setLayoutDirection(direction);
        qApp->setLayoutDirection(direction);
        setWindowTitle(persian ? QStringLiteral("یادآور پژوهش") : QStringLiteral("Yadavarifork"));

        headings_[0]->setText(persian ? QStringLiteral("زبان برنامه") : QStringLiteral("Application language"));
        headings_[1]->setText(persian ? QStringLiteral("حالت هوش مصنوعی") : QStringLiteral("AI mode"));
        headings_[2]->setText(persian ? QStringLiteral("نوع فعالیت") : QStringLiteral("Workspace"));
        headings_[3]->setText(persian ? QStringLiteral("بازبینی انتخاب‌ها") : QStringLiteral("Review selections"));

        language_->setItemText(0, QStringLiteral("فارسی"));
        language_->setItemText(1, QStringLiteral("English"));
        aiMode_->setItemText(0, persian ? QStringLiteral("کاملاً رایگان، بدون API") : QStringLiteral("Free, no API"));
        aiMode_->setItemText(1, persian ? QStringLiteral("سرویس‌های دارای طرح رایگان") : QStringLiteral("Free-tier AI services"));
        aiMode_->setItemText(2, persian ? QStringLiteral("سرویس‌های پولی") : QStringLiteral("Paid AI services"));
        taskMode_->setItemText(0, persian ? QStringLiteral("تکمیل پروپوزال") : QStringLiteral("Complete proposal"));
        taskMode_->setItemText(1, persian ? QStringLiteral("انجام پژوهش") : QStringLiteral("Conduct research"));
        back_->setText(persian ? QStringLiteral("قبلی") : QStringLiteral("Back"));
        updateNavigation();
        if (pages_->currentIndex() == 3)
            updateSummary();
    }

    void updateSummary() {
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        const QString language = language_->currentText();
        const QString ai = aiMode_->currentText();
        const QString task = taskMode_->currentText();
        if (persian) {
            summary_->setText(QStringLiteral("زبان: %1\nحالت هوش مصنوعی: %2\nفعالیت: %3")
                .arg(language, ai, task));
        } else {
            summary_->setText(QStringLiteral("Language: %1\nAI mode: %2\nWorkspace: %3")
                .arg(language, ai, task));
        }
    }

    void updateNavigation() {
        const bool persian = language_->currentData().toString() == QStringLiteral("fa");
        const int index = pages_->currentIndex();
        back_->setEnabled(index > 0);
        next_->setEnabled(index < pages_->count() - 1);
        next_->setText(index == pages_->count() - 2
            ? (persian ? QStringLiteral("بازبینی") : QStringLiteral("Review"))
            : (persian ? QStringLiteral("بعدی") : QStringLiteral("Next")));
    }

    QStackedWidget *pages_{};
    QComboBox *language_{};
    QComboBox *aiMode_{};
    QComboBox *taskMode_{};
    QLabel *summary_{};
    QPushButton *back_{};
    QPushButton *next_{};
    QList<QLabel *> headings_;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    WelcomeWindow window;
    if (app.arguments().contains(QStringLiteral("--smoke-test")))
        return window.runSmokeTest() ? 0 : 2;
    window.show();
    return app.exec();
}
