#include "research_engine_runner.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTimer>

ResearchEngineRunner::ResearchEngineRunner(QObject *parent)
    : QObject(parent) {
    process_.setProcessChannelMode(QProcess::MergedChannels);
    connect(&process_, &QProcess::started, this, &ResearchEngineRunner::started);
    connect(&process_, &QProcess::readyReadStandardOutput, this, [this] {
        consumeOutput();
    });
    connect(&process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        consumeOutput(true);
        emit finished(exitCode, status == QProcess::NormalExit);
    });
    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            emit failed(QStringLiteral("Research engine process could not be started: %1")
                            .arg(redact(process_.errorString())));
    });
}

bool ResearchEngineRunner::start(const QString &program, const QStringList &arguments,
                                 const QString &workingDirectory,
                                 const QProcessEnvironment &environment) {
    if (isRunning() || program.trimmed().isEmpty() || workingDirectory.trimmed().isEmpty())
        return false;
    const QFileInfo executableInfo(program);
    const QFileInfo workingInfo(workingDirectory);
    if (!executableInfo.exists() || !executableInfo.isFile() ||
        !workingInfo.exists() || !workingInfo.isDir())
        return false;
    for (const QString &argument : arguments) {
        if (argument.contains(QChar::Null))
            return false;
    }

    pendingOutput_.clear();
    secrets_.clear();
    const QStringList secretNames{
        QStringLiteral("GEMINI_API_KEY"), QStringLiteral("GOOGLE_API_KEY"),
        QStringLiteral("GOOGLE_GENAI_API_KEY"), QStringLiteral("OPENAI_API_KEY"),
        QStringLiteral("ANTHROPIC_API_KEY"), QStringLiteral("COHERE_API_KEY"),
        QStringLiteral("MISTRAL_API_KEY"), QStringLiteral("SEMANTIC_SCHOLAR_API_KEY")
    };
    for (const QString &name : secretNames) {
        const QString value = environment.value(name);
        if (!value.isEmpty())
            secrets_.append(value);
    }

    process_.setWorkingDirectory(QDir(workingDirectory).absolutePath());
    process_.setProcessEnvironment(environment);
    process_.start(QFileInfo(program).absoluteFilePath(), arguments, QIODevice::ReadOnly);
    return true;
}

void ResearchEngineRunner::cancel() {
    if (!isRunning())
        return;
    process_.terminate();
    QTimer::singleShot(5000, this, [this] {
        if (isRunning())
            process_.kill();
    });
}

bool ResearchEngineRunner::isRunning() const {
    return process_.state() != QProcess::NotRunning;
}

QString ResearchEngineRunner::redact(QString value) const {
    for (const QString &secret : secrets_) {
        if (!secret.isEmpty())
            value.replace(secret, QStringLiteral("[REDACTED]"));
    }
    static const QRegularExpression bearer(
        QStringLiteral("(?i)(authorization\\s*[:=]\\s*bearer\\s+)[^\\s,;\\\"']+"));
    value.replace(bearer, QStringLiteral("\\1[REDACTED]"));
    static const QRegularExpression apiKey(
        QStringLiteral("(?i)((?:x-api-key|api[_-]?key|access[_-]?token|client[_-]?secret)\\s*[:=]\\s*)[^\\s,;\\\"']+"));
    value.replace(apiKey, QStringLiteral("\\1[REDACTED]"));
    return value;
}

void ResearchEngineRunner::consumeOutput(bool flush) {
    pendingOutput_.append(process_.readAllStandardOutput());
    int newline = -1;
    while ((newline = pendingOutput_.indexOf('\n')) >= 0) {
        QByteArray line = pendingOutput_.left(newline);
        pendingOutput_.remove(0, newline + 1);
        if (line.endsWith('\r'))
            line.chop(1);
        emit outputLine(redact(QString::fromUtf8(line)));
    }
    constexpr qsizetype maxPending = 1024 * 1024;
    while (pendingOutput_.size() > maxPending) {
        const QByteArray chunk = pendingOutput_.left(65536);
        pendingOutput_.remove(0, 65536);
        emit outputLine(redact(QString::fromUtf8(chunk)));
    }
    if (flush && !pendingOutput_.isEmpty()) {
        emit outputLine(redact(QString::fromUtf8(pendingOutput_)));
        pendingOutput_.clear();
    }
}
