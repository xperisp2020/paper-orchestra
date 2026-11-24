#pragma once

#include <QMap>
#include <QString>
#include <QWidget>

namespace DocumentExport {

bool validateProposalLayout(QString *error = nullptr);
bool toPdf(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
           const QString &title, const QString &language, const QString &outputPath,
           QString *error = nullptr);
bool toDocx(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
            const QString &title, const QString &language, const QString &outputPath,
            QString *error = nullptr);
bool toImagePages(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
                  const QString &title, const QString &language, const QString &outputBasePath,
                  const QString &imageFormat, QString *error = nullptr);
bool toPngPages(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
                const QString &title, const QString &language, const QString &outputDirectory,
                QString *error = nullptr);
bool print(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
           const QString &title, const QString &language, QWidget *parent,
           QString *error = nullptr);
}
