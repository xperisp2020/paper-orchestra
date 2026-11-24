#pragma once

#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>

class ResearchEngineRunner final : public QObject {
    Q_OBJECT
public:
    explicit ResearchEngineRunner(QObject *parent = nullptr);
    bool start(const QString &program, const QStringList &arguments,
               const QString &workingDirectory,
               const QProcessEnvironment &environment = QProcessEnvironment::systemEnvironment());
    void cancel();
    bool isRunning() const;

signals:
    void started();
    void outputLine(const QString &line);
    void finished(int exitCode, bool normalExit);
    void failed(const QString &message);

private:
    void consumeOutput(bool flush = false);
    QString redact(QString value) const;

    QProcess process_;
    QByteArray pendingOutput_;
    QStringList secrets_;
};
