#pragma once
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

namespace DownloadPolicy {
// Windows reserves these device names with ANY extension (CON.txt, nul.pdf, COM1.png): opening such
// a file externally can address a device instead of the downloaded data. The engine keeps the name.
inline bool isReservedDeviceName(const QString &name) {
    const QString stem = QFileInfo(name).fileName().section(QLatin1Char('.'), 0, 0).trimmed().toLower();
    static const QRegularExpression ports(QStringLiteral("^(com|lpt)[1-9\u00b9\u00b2\u00b3]$"));
    static const QStringList devices{"con", "prn", "aux", "nul", "conin$", "conout$"};
    return devices.contains(stem) || ports.match(stem).hasMatch();
}

// Deliberately an allowlist: new and ambiguous types require opening the folder.
inline bool allowsExternalOpen(const QString &name) {
    const auto base = QFileInfo(name).fileName();
    if (base.endsWith(' ') || base.endsWith('.') || base.contains(':') || isReservedDeviceName(base)) return false;
    static const QStringList documents{
        "pdf", "txt", "csv", "png", "jpg", "jpeg", "gif", "webp", "bmp",
        "docx", "xlsx", "pptx", "odt", "ods", "odp", "mp3", "wav", "ogg", "mp4", "webm"
    };
    return documents.contains(QFileInfo(base).suffix().toLower());
}
}
