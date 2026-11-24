#pragma once

#include <QMap>
#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

#include <optional>

struct ProjectInfo {
    qint64 id{};
    QString title;
    QString language;
    QString aiMode;
    QString workflow;
    QString createdAt;
    QString updatedAt;
};

struct ResearchSource {
    qint64 id{};
    QString title;
    QString authors;
    QString publicationYear;
    QString doi;
    QString url;
    QString notes;
    QString verificationStatus;
};

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
    QList<ProjectInfo> listProjects(QString *error = nullptr);
    std::optional<ProjectInfo> loadProject(qint64 projectId, QString *error = nullptr);
    bool saveSection(qint64 projectId, const QString &sectionKey,
                     const QString &content, QString *error = nullptr);
    QMap<QString, QString> loadSections(qint64 projectId,
                                        QString *error = nullptr);
    bool saveProjectMetadata(qint64 projectId, const QString &key,
                             const QString &value, QString *error = nullptr);
    QString loadProjectMetadata(qint64 projectId, const QString &key,
                                QString *error = nullptr);
    bool saveResearchSection(qint64 projectId, const QString &sectionKey,
                             const QString &content, QString *error = nullptr);
    QMap<QString, QString> loadResearchSections(qint64 projectId,
                                                 QString *error = nullptr);
    std::optional<qint64> addResearchSource(qint64 projectId, const ResearchSource &source,
                                            QString *error = nullptr);
    QList<ResearchSource> loadResearchSources(qint64 projectId,
                                              QString *error = nullptr);
    bool deleteResearchSource(qint64 projectId, qint64 sourceId,
                              QString *error = nullptr);

    static QStringList proposalSectionKeys();
    static QStringList researchSectionKeys();

private:
    QSqlDatabase database_;
    QString connectionName_;
};
