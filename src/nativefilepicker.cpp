#include "nativefilepicker.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {
QString openFolder() {
    const QString videos = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    return QDir(videos).exists() ? videos : QDir::homePath();
}

QString videoFilter() {
    return QStringLiteral(
        "Video files (*.avi *.m4v *.mkv *.mov *.mp4 *.mpeg *.mpg *.webm)");
}

QString openFilters() {
    return videoFilter() + QStringLiteral(";;All files (*)");
}
}

NativeFilePicker::NativeFilePicker(QObject *parent) : FilePicker(parent) {}

QStringList NativeFilePicker::exportNameFilters(const QList<int> &scaleHeights) {
    if (scaleHeights.isEmpty())
        return {QStringLiteral("MP4 video (*.mp4)")};

    QStringList filters{QStringLiteral("MP4 video, Original (*.mp4)")};
    for (const int height : scaleHeights)
        filters.append(QStringLiteral("MP4 video, %1p (*.mp4)").arg(height));
    return filters;
}

int NativeFilePicker::scaleHeightForFilter(const QString &filter) {
    static const QRegularExpression re(QStringLiteral(",\\s*(\\d+)p\\s*\\("));
    const QRegularExpressionMatch match = re.match(filter);
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

void NativeFilePicker::openVideo() {
    const QString path = QFileDialog::getOpenFileName(
        nullptr, QStringLiteral("Open Video File"), openFolder(), openFilters());
    if (path.isEmpty())
        return;
    emit openSelected(QUrl::fromLocalFile(path));
}

void NativeFilePicker::exportVideo(const QUrl &suggestedUrl, double start, double end,
                                   const QList<int> &scaleHeights) {
    const QStringList filters = exportNameFilters(scaleHeights);
    QString selectedFilter = filters.first();

    const QString path = QFileDialog::getSaveFileName(
        nullptr, QStringLiteral("Save Video File"), suggestedUrl.toLocalFile(),
        filters.join(QStringLiteral(";;")), &selectedFilter);
    if (path.isEmpty())
        return;

    // The extension is Backend's business: it rewrites whatever comes back to
    // .mp4 and refuses the rewrite if that would land on an existing file.
    emit exportSelected(QUrl::fromLocalFile(path), start, end,
                        scaleHeightForFilter(selectedFilter));
}
