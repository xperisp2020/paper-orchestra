#pragma once

#include <QMap>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

#include <optional>

class ProjectStore final {
public:
    explicit ProjectStore(const QString &databasePath);
    ~ProjectStore();

    ProjectStore(const ProjectStore &) = delete;
    ProjectStore &operator=(const ProjectStore &) = delete;

    bool initialize(QString *error = nullptr);
    std::optional<qint64> createProject(const QString &title,
                                        const QString &language,
                                        const QString &aiMode,
                                        const QString &workflow,
                                        QString *error = nullptr);
    bool saveSection(qint64 projectId, const QString &sectionKey,
                     const QString &content, QString *error = nullptr);
    QMap<QString, QString> loadSections(qint64 projectId,
                                        QString *error = nullptr);

    static QStringList proposalSectionKeys();

private:
    QSqlDatabase database_;
    QString connectionName_;
};
