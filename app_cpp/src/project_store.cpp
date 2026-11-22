#include "project_store.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace {
void setError(QString *target, const QString &message) {
    if (target)
        *target = message;
}
}

ProjectStore::ProjectStore(const QString &databasePath)
    : connectionName_(QStringLiteral("yadavarifork_") +
                      QUuid::createUuid().toString(QUuid::WithoutBraces)) {
    database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    database_.setDatabaseName(databasePath);
    database_.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
}

ProjectStore::~ProjectStore() {
    if (database_.isValid()) {
        database_.close();
        database_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName_);
    }
}

QStringList ProjectStore::proposalSectionKeys() {
    return {
        QStringLiteral("title_fa"), QStringLiteral("title_en"),
        QStringLiteral("field"), QStringLiteral("main_question"),
        QStringLiteral("problem_statement"), QStringLiteral("necessity"),
        QStringLiteral("literature_review"), QStringLiteral("innovation"),
        QStringLiteral("keywords"), QStringLiteral("objectives"),
        QStringLiteral("beneficiaries"), QStringLiteral("questions"),
        QStringLiteral("hypotheses"), QStringLiteral("definitions"),
        QStringLiteral("references"), QStringLiteral("methodology"),
        QStringLiteral("data_collection"), QStringLiteral("analysis"),
        QStringLiteral("variables"), QStringLiteral("timeline"),
        QStringLiteral("budget")
    };
}

bool ProjectStore::initialize(QString *error) {
    const QString parent = QFileInfo(database_.databaseName()).absolutePath();
    if (!QDir().mkpath(parent)) {
        setError(error, QStringLiteral("Cannot create the project data directory: %1").arg(parent));
        return false;
    }
    if (!database_.open()) {
        setError(error, database_.lastError().text());
        return false;
    }

    QSqlQuery query(database_);
    if (!query.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        setError(error, query.lastError().text());
        return false;
    }
    const QStringList statements = {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS projects ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "title TEXT NOT NULL,"
            "language TEXT NOT NULL CHECK(language IN ('fa','en')),"
            "ai_mode TEXT NOT NULL CHECK(ai_mode IN ('no_api','free_tier','paid')),"
            "workflow TEXT NOT NULL CHECK(workflow IN ('proposal','research')),"
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS proposal_sections ("
            "project_id INTEGER NOT NULL REFERENCES projects(id) ON DELETE CASCADE,"
            "section_key TEXT NOT NULL,"
            "content TEXT NOT NULL DEFAULT '',"
            "PRIMARY KEY(project_id, section_key))")
    };
    for (const QString &statement : statements) {
        if (!query.exec(statement)) {
            setError(error, query.lastError().text());
            return false;
        }
    }
    return true;
}

std::optional<qint64> ProjectStore::createProject(const QString &title,
                                                   const QString &language,
                                                   const QString &aiMode,
                                                   const QString &workflow,
                                                   QString *error) {
    const QString cleanTitle = title.trimmed();
    if (cleanTitle.isEmpty()) {
        setError(error, QStringLiteral("Project title cannot be empty."));
        return std::nullopt;
    }
    if ((language != QStringLiteral("fa") && language != QStringLiteral("en")) ||
        (aiMode != QStringLiteral("no_api") && aiMode != QStringLiteral("free_tier") &&
         aiMode != QStringLiteral("paid")) ||
        (workflow != QStringLiteral("proposal") && workflow != QStringLiteral("research"))) {
        setError(error, QStringLiteral("Invalid language, AI mode, or workflow selection."));
        return std::nullopt;
    }
    if (!database_.isOpen() && !initialize(error))
        return std::nullopt;
    if (!database_.transaction()) {
        setError(error, database_.lastError().text());
        return std::nullopt;
    }

    QSqlQuery insert(database_);
    insert.prepare(QStringLiteral(
        "INSERT INTO projects(title, language, ai_mode, workflow) VALUES (?, ?, ?, ?)"));
    insert.addBindValue(cleanTitle);
    insert.addBindValue(language);
    insert.addBindValue(aiMode);
    insert.addBindValue(workflow);
    if (!insert.exec()) {
        setError(error, insert.lastError().text());
        database_.rollback();
        return std::nullopt;
    }
    const qint64 projectId = insert.lastInsertId().toLongLong();

    if (workflow == QStringLiteral("proposal")) {
        QSqlQuery sectionInsert(database_);
        sectionInsert.prepare(QStringLiteral(
            "INSERT INTO proposal_sections(project_id, section_key, content) VALUES (?, ?, '')"));
        for (const QString &key : proposalSectionKeys()) {
            sectionInsert.bindValue(0, projectId);
            sectionInsert.bindValue(1, key);
            if (!sectionInsert.exec()) {
                setError(error, sectionInsert.lastError().text());
                database_.rollback();
                return std::nullopt;
            }
        }
    }
    if (!database_.commit()) {
        setError(error, database_.lastError().text());
        database_.rollback();
        return std::nullopt;
    }
    return projectId;
}

bool ProjectStore::saveSection(qint64 projectId, const QString &sectionKey,
                               const QString &content, QString *error) {
    if (!proposalSectionKeys().contains(sectionKey)) {
        setError(error, QStringLiteral("Unknown proposal section: %1").arg(sectionKey));
        return false;
    }
    if (!database_.isOpen() && !initialize(error))
        return false;

    QSqlQuery query(database_);
    query.prepare(QStringLiteral(
        "INSERT INTO proposal_sections(project_id, section_key, content) VALUES (?, ?, ?) "
        "ON CONFLICT(project_id, section_key) DO UPDATE SET content=excluded.content"));
    query.addBindValue(projectId);
    query.addBindValue(sectionKey);
    query.addBindValue(content);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    QSqlQuery update(database_);
    update.prepare(QStringLiteral("UPDATE projects SET updated_at=CURRENT_TIMESTAMP WHERE id=?"));
    update.addBindValue(projectId);
    if (!update.exec() || update.numRowsAffected() == 0) {
        setError(error, update.lastError().text().isEmpty()
                            ? QStringLiteral("Project does not exist.")
                            : update.lastError().text());
        return false;
    }
    return true;
}

QMap<QString, QString> ProjectStore::loadSections(qint64 projectId, QString *error) {
    QMap<QString, QString> sections;
    if (!database_.isOpen() && !initialize(error))
        return sections;

    QSqlQuery query(database_);
    query.prepare(QStringLiteral(
        "SELECT section_key, content FROM proposal_sections WHERE project_id=?"));
    query.addBindValue(projectId);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return sections;
    }
    while (query.next())
        sections.insert(query.value(0).toString(), query.value(1).toString());
    return sections;
}
