#pragma once
#include <QFileInfo>
#include <QStringList>

namespace DownloadPolicy {
// Deliberately an allowlist: new and ambiguous types require opening the folder.
inline bool allowsExternalOpen(const QString &name) {
    const auto base = QFileInfo(name).fileName();
    if (base.endsWith(' ') || base.endsWith('.') || base.contains(':')) return false;
    static const QStringList documents{
        "pdf", "txt", "csv", "png", "jpg", "jpeg", "gif", "webp", "bmp",
        "docx", "xlsx", "pptx", "odt", "ods", "odp", "mp3", "wav", "ogg", "mp4", "webm"
    };
    return documents.contains(QFileInfo(base).suffix().toLower());
}
}
