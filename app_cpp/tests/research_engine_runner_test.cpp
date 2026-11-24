#include "research_engine_runner.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTextStream>
#include <QThread>
#include <QTimer>

namespace {
int fail(const QString &message) {
    QTextStream(stderr) << "ResearchEngineRunner test failed: " << message << Qt::endl;
    return 1;
}

int runChild(const QString &mode) {
    if (mode == QStringLiteral("--child-emit")) {
        QTextStream out(stdout);
        out << "environment=" << qEnvironmentVariable("OPENAI_API_KEY") << Qt::endl;
        out << "bearer=Authorization: Bearer runner-bearer-secret" << Qt::endl;
        out << "header=x-api-key: runner-header-secret" << Qt::endl;
        return 0;
    }
    if (mode == QStringLiteral("--child-sleep")) {
        for (;;) QThread::msleep(100);
    }
    return 2;
}
}  // namespace

int main(int argc, char *argv[]) {
    QCoreApplication application(argc, argv);
    const QStringList arguments = application.arguments();
    if (arguments.size() >= 2 && arguments.at(1).startsWith(QStringLiteral("--child-")))
        return runChild(arguments.at(1));

    QTemporaryDir workingDirectory;
    if (!workingDirectory.isValid())
        return fail(QStringLiteral("could not create a temporary working directory"));

    const QString executable = QCoreApplication::applicationFilePath();
    if (executable.isEmpty())
        return fail(QStringLiteral("test executable path is empty"));

    // Verify that child output is streamed and all supported credential forms
    // are redacted before crossing the runner's outputLine signal.
    {
        ResearchEngineRunner runner;
        QStringList output;
        bool finished = false;
        bool normalExit = false;
        bool startFailed = false;
        int exitCode = -1;
        bool timedOut = false;
        QEventLoop loop;
        QTimer watchdog;
        watchdog.setSingleShot(true);

        QObject::connect(&runner, &ResearchEngineRunner::outputLine,
                         [&output](const QString &line) { output.append(line); });
        QObject::connect(&runner, &ResearchEngineRunner::finished,
                         [&loop, &finished, &normalExit, &exitCode](int code, bool normal) {
            finished = true;
            normalExit = normal;
            exitCode = code;
            loop.quit();
        });
        QObject::connect(&runner, &ResearchEngineRunner::failed,
                         [&loop, &startFailed](const QString &) {
            startFailed = true;
            loop.quit();
        });
        QObject::connect(&watchdog, &QTimer::timeout, [&loop, &timedOut] {
            timedOut = true;
            loop.quit();
        });

        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert(QStringLiteral("OPENAI_API_KEY"),
                           QStringLiteral("runner-test-openai-secret"));
        if (!runner.start(executable, {QStringLiteral("--child-emit")},
                          workingDirectory.path(), environment)) {
            return fail(QStringLiteral("runner rejected a valid child process"));
        }
        if (runner.start(executable, {QStringLiteral("--child-sleep")},
                         workingDirectory.path(), environment)) {
            return fail(QStringLiteral("runner accepted a second process while busy"));
        }

        watchdog.start(5000);
        if (!finished && !startFailed)
            loop.exec();
        watchdog.stop();

        if (timedOut)
            return fail(QStringLiteral("output/redaction child process timed out"));
        if (startFailed)
            return fail(QStringLiteral("output/redaction child process could not start"));
        if (!finished || !normalExit || exitCode != 0)
            return fail(QStringLiteral("output/redaction child process did not exit successfully"));

        const QString joined = output.join(QLatin1Char('\n'));
        for (const QString &secret : {
                 QStringLiteral("runner-test-openai-secret"),
                 QStringLiteral("runner-bearer-secret"),
                 QStringLiteral("runner-header-secret")}) {
            if (joined.contains(secret))
                return fail(QStringLiteral("a test credential escaped output redaction"));
        }
        if (!joined.contains(QStringLiteral("environment=[REDACTED]")) ||
            !joined.contains(QStringLiteral("Authorization: Bearer [REDACTED]")) ||
            !joined.contains(QStringLiteral("x-api-key: [REDACTED]"))) {
            return fail(QStringLiteral("expected redacted output markers were not observed"));
        }
    }

    // Verify cancellation against a real child process. On Windows terminate()
    // may require the runner's delayed kill fallback; the test allows both paths.
    {
        ResearchEngineRunner runner;
        bool childStarted = false;
        bool finished = false;
        bool startFailed = false;
        bool timedOut = false;
        QEventLoop loop;
        QTimer watchdog;
        watchdog.setSingleShot(true);

        QObject::connect(&runner, &ResearchEngineRunner::started, [&] {
            childStarted = true;
            QTimer::singleShot(100, &runner, [&runner] { runner.cancel(); });
        });
        QObject::connect(&runner, &ResearchEngineRunner::finished,
                         [&loop, &finished](int, bool) {
            finished = true;
            loop.quit();
        });
        QObject::connect(&runner, &ResearchEngineRunner::failed,
                         [&loop, &startFailed](const QString &) {
            startFailed = true;
            loop.quit();
        });
        QObject::connect(&watchdog, &QTimer::timeout, [&loop, &timedOut] {
            timedOut = true;
            loop.quit();
        });

        if (!runner.start(executable, {QStringLiteral("--child-sleep")},
                          workingDirectory.path())) {
            return fail(QStringLiteral("runner rejected the cancellable child process"));
        }
        watchdog.start(10000);
        if (!finished && !startFailed)
            loop.exec();
        watchdog.stop();

        if (timedOut)
            return fail(QStringLiteral("cancel did not stop the child within ten seconds"));
        if (startFailed || !childStarted || !finished || runner.isRunning())
            return fail(QStringLiteral("cancel lifecycle did not finish cleanly"));
    }

    QTextStream(stdout) << "ResearchEngineRunner cancellation and secret-redaction tests passed."
                        << Qt::endl;
    return 0;
}
