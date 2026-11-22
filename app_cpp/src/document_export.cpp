#include "document_export.h"
#include "project_store.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QPageSize>
#include <QPdfWriter>
#include <QPrintDialog>
#include <QPrinter>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QTextDocument>

#include <limits>

namespace DocumentExport {
namespace {
void setError(QString *error, const QString &message) {
    if (error) *error = message;
}

QString xmlEscape(const QString &value) {
    return value.toHtmlEscaped();
}

QString buildHtml(const QMap<QString, QString> &sections,
                  const QMap<QString, QString> &labels,
                  const QString &title, const QString &language) {
    const bool rtl = language == QStringLiteral("fa");
    QString html = QStringLiteral(
        "<!doctype html><html><head><meta charset=\"utf-8\"><style>"
        "body{font-family:Arial,sans-serif;font-size:11pt;line-height:1.55;}"
        "h1{font-size:20pt;text-align:center;margin-bottom:18pt;}"
        "h2{font-size:13pt;margin-top:16pt;border-bottom:1px solid #888;}"
        "p{white-space:pre-wrap;margin:4pt 0;}"
        "</style></head><body dir=\"%1\">").arg(rtl ? QStringLiteral("rtl") : QStringLiteral("ltr"));
    html += QStringLiteral("<h1>%1</h1>").arg(title.toHtmlEscaped());
    for (const QString &key : ProjectStore::proposalSectionKeys()) {
        html += QStringLiteral("<h2>%1</h2>").arg(labels.value(key, key).toHtmlEscaped());
        const QStringList paragraphs = sections.value(key).split(QLatin1Char('\n'), Qt::KeepEmptyParts);
        for (const QString &paragraph : paragraphs)
            html += QStringLiteral("<p>%1</p>").arg(paragraph.toHtmlEscaped());
    }
    html += QStringLiteral("</body></html>");
    return html;
}

bool makeParentDirectory(const QString &path, QString *error) {
    const QString parent = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(parent)) {
        setError(error, QStringLiteral("Cannot create output directory: %1").arg(parent));
        return false;
    }
    return true;
}

quint32 crc32(const QByteArray &data) {
    quint32 crc = 0xFFFFFFFFu;
    for (unsigned char byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
    }
    return ~crc;
}

struct ZipEntry {
    QByteArray name;
    quint32 checksum{};
    quint32 size{};
    quint32 offset{};
};

class StoredZipWriter {
public:
    explicit StoredZipWriter(const QString &path) : file_(path) {}
    bool open() { return file_.open(QIODevice::WriteOnly | QIODevice::Truncate); }

    bool addFile(const QByteArray &name, const QByteArray &contents) {
        if (!file_.isOpen() || name.isEmpty() ||
            contents.size() > static_cast<qsizetype>(std::numeric_limits<quint32>::max()))
            return false;
        ZipEntry entry;
        entry.name = name;
        entry.checksum = crc32(contents);
        entry.size = static_cast<quint32>(contents.size());
        entry.offset = static_cast<quint32>(file_.pos());

        QDataStream stream(&file_);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << quint32(0x04034b50) << quint16(20) << quint16(0) << quint16(0)
               << quint16(0) << quint16(0x0021) << entry.checksum
               << entry.size << entry.size << quint16(entry.name.size()) << quint16(0);
        if (stream.status() != QDataStream::Ok ||
            file_.write(entry.name) != entry.name.size() ||
            file_.write(contents) != contents.size())
            return false;
        entries_.append(entry);
        return true;
    }

    bool finish() {
        if (!file_.isOpen() || entries_.size() > 0xFFFF) return false;
        const quint32 centralOffset = static_cast<quint32>(file_.pos());
        for (const ZipEntry &entry : entries_) {
            QDataStream stream(&file_);
            stream.setByteOrder(QDataStream::LittleEndian);
            stream << quint32(0x02014b50) << quint16(20) << quint16(20)
                   << quint16(0) << quint16(0) << quint16(0) << quint16(0x0021)
                   << entry.checksum << entry.size << entry.size
                   << quint16(entry.name.size()) << quint16(0) << quint16(0)
                   << quint16(0) << quint16(0) << quint32(0) << entry.offset;
            if (stream.status() != QDataStream::Ok ||
                file_.write(entry.name) != entry.name.size())
                return false;
        }
        const quint32 centralSize = static_cast<quint32>(file_.pos()) - centralOffset;
        QDataStream end(&file_);
        end.setByteOrder(QDataStream::LittleEndian);
        end << quint32(0x06054b50) << quint16(0) << quint16(0)
            << quint16(entries_.size()) << quint16(entries_.size())
            << centralSize << centralOffset << quint16(0);
        return end.status() == QDataStream::Ok && file_.flush();
    }

private:
    QFile file_;
    QList<ZipEntry> entries_;
};

QString wordRun(const QString &text, bool rtl, bool bold = false) {
    QString runProperties;
    if (rtl) runProperties += QStringLiteral("<w:rtl/>");
    if (bold) runProperties += QStringLiteral("<w:b/>");
    const QString properties = runProperties.isEmpty()
        ? QString()
        : QStringLiteral("<w:rPr>%1</w:rPr>").arg(runProperties);
    return QStringLiteral("<w:r>%1<w:t xml:space=\"preserve\">%2</w:t></w:r>")
        .arg(properties, xmlEscape(text));
}

QString wordParagraph(const QString &text, bool rtl, bool heading = false) {
    QString paragraphProperties;
    if (heading) paragraphProperties += QStringLiteral("<w:pStyle w:val=\"Heading1\"/>");
    if (rtl) paragraphProperties += QStringLiteral("<w:bidi/>");
    const QString ppr = paragraphProperties.isEmpty()
        ? QString()
        : QStringLiteral("<w:pPr>%1</w:pPr>").arg(paragraphProperties);
    return QStringLiteral("<w:p>%1%2</w:p>").arg(ppr, wordRun(text, rtl, heading));
}

QByteArray buildDocumentXml(const QMap<QString, QString> &sections,
                            const QMap<QString, QString> &labels,
                            const QString &title, const QString &language) {
    const bool rtl = language == QStringLiteral("fa");
    QString body = wordParagraph(title, rtl, true);
    for (const QString &key : ProjectStore::proposalSectionKeys()) {
        body += wordParagraph(labels.value(key, key), rtl, true);
        const QStringList paragraphs = sections.value(key).split(QLatin1Char('\n'), Qt::KeepEmptyParts);
        for (const QString &paragraph : paragraphs)
            body += wordParagraph(paragraph, rtl);
    }
    const QString xml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:body>%1"
        "<w:sectPr><w:pgSz w:w=\"11906\" w:h=\"16838\"/>"
        "<w:pgMar w:top=\"1134\" w:right=\"1134\" w:bottom=\"1134\" "
        "w:left=\"1134\" w:header=\"567\" w:footer=\"567\" w:gutter=\"0\"/>"
        "</w:sectPr></w:body></w:document>").arg(body);
    return xml.toUtf8();
}

bool publishTempFile(const QString &temporaryPath, const QString &outputPath, QString *error) {
    if (QFileInfo::exists(outputPath)) {
        QFile::remove(temporaryPath);
        setError(error, QStringLiteral("Refusing to overwrite existing file: %1").arg(outputPath));
        return false;
    }
    QFile temporary(temporaryPath);
    if (!temporary.rename(outputPath)) {
        const QString reason = temporary.errorString();
        setError(error, QStringLiteral("Cannot publish output file: %1 (%2)")
                     .arg(outputPath, reason));
        QFile::remove(temporaryPath);
        return false;
    }
    return true;
}
}

bool toPdf(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
           const QString &title, const QString &language, const QString &outputPath, QString *error) {
    if (!makeParentDirectory(outputPath, error)) return false;
    if (QFileInfo::exists(outputPath)) {
        setError(error, QStringLiteral("Refusing to overwrite existing PDF: %1").arg(outputPath));
        return false;
    }

    // Use a fresh directory rather than a pre-created temporary file. Some
    // Windows PDF backends cannot reopen/truncate a QTemporaryFile path.
    QTemporaryDir temporaryDirectory(QFileInfo(outputPath).absolutePath() +
                                     QStringLiteral("/.yadavarifork-pdf-XXXXXX"));
    if (!temporaryDirectory.isValid()) {
        setError(error, QStringLiteral("Cannot create temporary PDF workspace."));
        return false;
    }
    const QString temporaryPath = temporaryDirectory.filePath(QStringLiteral("generated.pdf"));
    {
        QPdfWriter writer(temporaryPath);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setResolution(96);
        QTextDocument document;
        document.setHtml(buildHtml(sections, labels, title, language));
        document.print(&writer);
    }
    if (!QFileInfo(temporaryPath).isFile() || QFileInfo(temporaryPath).size() <= 0) {
        setError(error, QStringLiteral("PDF writer did not produce a non-empty output file."));
        return false;
    }
    return publishTempFile(temporaryPath, outputPath, error);
}

bool toDocx(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
            const QString &title, const QString &language, const QString &outputPath, QString *error) {
    if (!makeParentDirectory(outputPath, error)) return false;
    if (QFileInfo::exists(outputPath)) {
        setError(error, QStringLiteral("Refusing to overwrite existing DOCX: %1").arg(outputPath));
        return false;
    }
    QTemporaryDir temporaryDirectory(QFileInfo(outputPath).absolutePath() +
                                     QStringLiteral("/.yadavarifork-docx-XXXXXX"));
    if (!temporaryDirectory.isValid()) {
        setError(error, QStringLiteral("Cannot create temporary DOCX workspace."));
        return false;
    }
    const QString temporaryPath = temporaryDirectory.filePath(QStringLiteral("generated.docx"));

    const QByteArray contentTypes = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
<Default Extension="xml" ContentType="application/xml"/>
<Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
<Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>)";
    const QByteArray rootRels = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>)";
    const QByteArray documentRels = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>)";
    const QByteArray styles = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
<w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/><w:rPr><w:rFonts w:ascii="Arial" w:hAnsi="Arial" w:cs="Arial"/></w:rPr></w:style>
<w:style w:type="paragraph" w:styleId="Heading1"><w:name w:val="heading 1"/><w:basedOn w:val="Normal"/><w:uiPriority w:val="9"/><w:qFormat/><w:rPr><w:b/><w:sz w:val="28"/></w:rPr></w:style>
</w:styles>)";

    bool written = false;
    {
        StoredZipWriter zip(temporaryPath);
        if (zip.open()) {
            written = zip.addFile("[Content_Types].xml", contentTypes) &&
                      zip.addFile("_rels/.rels", rootRels) &&
                      zip.addFile("word/document.xml", buildDocumentXml(sections, labels, title, language)) &&
                      zip.addFile("word/styles.xml", styles) &&
                      zip.addFile("word/_rels/document.xml.rels", documentRels) &&
                      zip.finish();
        }
    }
    if (!written) {
        setError(error, QStringLiteral("Failed to assemble the DOCX package."));
        return false;
    }
    return publishTempFile(temporaryPath, outputPath, error);
}

bool toPngPages(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
                const QString &title, const QString &language, const QString &outputDirectory,
                QString *error) {
    const QFileInfo outputInfo(outputDirectory);
    if (!QDir().mkpath(outputInfo.absolutePath())) {
        setError(error, QStringLiteral("Cannot create PNG parent directory."));
        return false;
    }
    if (!QDir().mkdir(outputDirectory)) {
        setError(error, QStringLiteral("Refusing to overwrite existing PNG directory: %1").arg(outputDirectory));
        return false;
    }
    QTextDocument document;
    document.setHtml(buildHtml(sections, labels, title, language));
    constexpr int pageWidth = 794;
    constexpr int pageHeight = 1123;
    constexpr int scale = 2;
    document.setPageSize(QSizeF(pageWidth, pageHeight));
    const int pageCount = qMax(1, document.pageCount());
    QDir output(outputDirectory);
    QStringList createdFiles;
    for (int page = 0; page < pageCount; ++page) {
        QImage image(pageWidth * scale, pageHeight * scale, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.scale(scale, scale);
        painter.translate(0, -page * pageHeight);
        document.drawContents(&painter, QRectF(0, page * pageHeight, pageWidth, pageHeight));
        painter.end();
        const QString fileName = QStringLiteral("page_%1.png").arg(page + 1, 3, 10, QLatin1Char('0'));
        const QString filePath = output.filePath(fileName);
        if (!image.save(filePath, "PNG")) {
            for (const QString &created : createdFiles) QFile::remove(created);
            QDir().rmdir(outputDirectory);
            setError(error, QStringLiteral("Failed to write PNG page %1.").arg(page + 1));
            return false;
        }
        createdFiles.append(filePath);
    }
    return true;
}

bool print(const QMap<QString, QString> &sections, const QMap<QString, QString> &labels,
           const QString &title, const QString &language, QWidget *parent, QString *error) {
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, parent);
    if (dialog.exec() != QDialog::Accepted) {
        setError(error, QStringLiteral("Printing was cancelled."));
        return false;
    }
    QTextDocument document;
    document.setHtml(buildHtml(sections, labels, title, language));
    document.print(&printer);
    return true;
}
}
