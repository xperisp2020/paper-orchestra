#include <QApplication>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QStringList>

class WelcomeWindow final : public QMainWindow {
public:
    explicit WelcomeWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setMinimumSize(680, 460);
        setWindowTitle(QStringLiteral("Yadavarifork"));

        pages_ = new QStackedWidget(this);
        setCentralWidget(pages_);

        auto *languagePage = makePage(QStringLiteral("زبان برنامه / Application language"));
        language_ = new QComboBox;
        language_->addItem(QStringLiteral("فارسی"), QStringLiteral("fa"));
        language_->addItem(QStringLiteral("English"), QStringLiteral("en"));
        languagePage->layout()->addWidget(language_);
        pages_->addWidget(languagePage);

        auto *aiPage = makePage(QStringLiteral("حالت هوش مصنوعی / AI mode"));
        aiMode_ = new QComboBox;
        aiMode_->addItem(QStringLiteral("کاملاً رایگان، بدون API"), QStringLiteral("no_api"));
        aiMode_->addItem(QStringLiteral("سرویس‌های دارای طرح رایگان"), QStringLiteral("free_tier"));
        aiMode_->addItem(QStringLiteral("سرویس‌های پولی"), QStringLiteral("paid"));
        aiPage->layout()->addWidget(aiMode_);
        pages_->addWidget(aiPage);

        auto *taskPage = makePage(QStringLiteral("نوع فعالیت / Workspace"));
        taskMode_ = new QComboBox;
        taskMode_->addItem(QStringLiteral("تکمیل پروپوزال"), QStringLiteral("proposal"));
        taskMode_->addItem(QStringLiteral("انجام پژوهش"), QStringLiteral("research"));
        taskPage->layout()->addWidget(taskMode_);
        pages_->addWidget(taskPage);

        auto *summaryPage = makePage(QStringLiteral("انتخاب‌های شما / Your selections"));
        summary_ = new QLabel;
        summary_->setWordWrap(true);
        summaryPage->layout()->addWidget(summary_);
        pages_->addWidget(summaryPage);

        auto *navigation = new QWidget;
        auto *buttons = new QHBoxLayout(navigation);
        back_ = new QPushButton(QStringLiteral("قبلی / Back"));
        next_ = new QPushButton(QStringLiteral("بعدی / Next"));
        buttons->addWidget(back_);
        buttons->addStretch();
        buttons->addWidget(next_);
        static_cast<QVBoxLayout *>(centralWidget()->layout());
        auto *root = new QWidget;
        auto *rootLayout = new QVBoxLayout(root);
        rootLayout->addWidget(pages_, 1);
        rootLayout->addWidget(navigation);
        setCentralWidget(root);

        connect(back_, &QPushButton::clicked, this, [this] {
            if (pages_->currentIndex() > 0) pages_->setCurrentIndex(pages_->currentIndex() - 1);
            updateNavigation();
        });
        connect(next_, &QPushButton::clicked, this, [this] {
            if (pages_->currentIndex() == 2) {
                const auto lang = language_->currentData().toString();
                const auto ai = aiMode_->currentData().toString();
                const auto task = taskMode_->currentData().toString();
                summary_->setText(QStringLiteral("Language: %1\\nAI mode: %2\\nWorkspace: %3")
                    .arg(lang, ai, task));
            }
            if (pages_->currentIndex() < pages_->count() - 1)
                pages_->setCurrentIndex(pages_->currentIndex() + 1);
            updateNavigation();
        });
        updateNavigation();
    }

private:
    QWidget *makePage(const QString &heading) {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        auto *title = new QLabel(heading);
        title->setWordWrap(true);
        title->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: 600;"));
        layout->addWidget(title);
        layout->addStretch();
        return page;
    }

    void updateNavigation() {
        const int index = pages_->currentIndex();
        back_->setEnabled(index > 0);
        next_->setEnabled(index < pages_->count() - 1);
        next_->setText(index == pages_->count() - 2
            ? QStringLiteral("نمایش انتخاب‌ها / Review")
            : QStringLiteral("بعدی / Next"));
    }

    QStackedWidget *pages_{};
    QComboBox *language_{};
    QComboBox *aiMode_{};
    QComboBox *taskMode_{};
    QLabel *summary_{};
    QPushButton *back_{};
    QPushButton *next_{};
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    WelcomeWindow window;
    if (app.arguments().contains(QStringLiteral("--smoke-test"))) return 0;
    window.show();
    return app.exec();
}
