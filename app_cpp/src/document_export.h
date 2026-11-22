#pragma once

#include <QMap>
#include <QString>
#include <QWidget>

namespace DocumentExport {
bool toPdf(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
           const QString &title, const QString &language, const QString &outputPath,
           QString *error = nullptr);
bool toDocx(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
            const QString &title, const QString &language, const QString &outputPath,
            QString *error = nullptr);
bool toPngPages(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
                const QString &title, const QString &language, const QString &outputDirectory,
                QString *error = nullptr);
bool print(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
           const QString &title, const QString &language, QWidget *parent,
           QString *error = nullptr);
}
