#include "ffmpeg.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>

namespace ffmpeg {

namespace {
// Upper bound on a synchronous probe so a hung/unresponsive file (e.g. a stalled
// network mount) can't freeze the caller — load() runs probe() on the UI thread.
constexpr int kProbeTimeoutMs = 15000;
// Poll granularity while waiting on a thumbnail child, so cancellation is prompt.
constexpr int kThumbPollMs = 50;
}

QString toolPath(const QString &tool) {
    const QString onPath = QStandardPaths::findExecutable(tool);
    if (!onPath.isEmpty())
        return onPath;

#ifdef Q_OS_MACOS
    // A .app launched from Finder or the Dock inherits launchd's PATH, which is
    // /usr/bin:/bin:/usr/sbin:/sbin and nothing else — so an ffmpeg installed by
    // Homebrew or MacPorts is invisible even though it works fine in a terminal.
    // Fall back to where those installers actually put it before giving up.
    static const QStringList extraDirs{
        QStringLiteral("/opt/homebrew/bin"),  // Homebrew, Apple silicon
        QStringLiteral("/usr/local/bin"),     // Homebrew, Intel
        QStringLiteral("/opt/local/bin"),     // MacPorts
    };
    return QStandardPaths::findExecutable(tool, extraDirs);
#else
    return QString();
#endif
}

VideoInfo probe(const QString &path) {
    VideoInfo info;
    info.path = path;

    const QString ffprobe = toolPath("ffprobe");
    if (ffprobe.isEmpty()) {
        info.error = "`ffprobe` was not found on your PATH. Install ffmpeg.";
        return info;
    }

    QProcess proc;
    proc.start(ffprobe, {
        "-v", "error",
        "-print_format", "json",
        "-show_format",
        "-show_streams",
        "-select_streams", "v:0",
        path,
    });
    if (!proc.waitForFinished(kProbeTimeoutMs)) {
        proc.kill();
        proc.waitForFinished(-1);
        info.error = "ffprobe timed out reading this file.";
        return info;
    }

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        info.error = QString::fromUtf8(proc.readAllStandardError()).trimmed();
        if (info.error.isEmpty())
            info.error = "ffprobe failed.";
        return info;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(proc.readAllStandardOutput());
    const QJsonObject root = doc.object();
    const QJsonArray streams = root.value("streams").toArray();
    if (streams.isEmpty()) {
        info.error = "No video stream found in this file.";
        return info;
    }

    const QJsonObject stream = streams.first().toObject();

    info.width = stream.value("width").toInt();
    info.height = stream.value("height").toInt();

    // Duration can live on the stream or on the container.
    QString durationStr = stream.value("duration").toString();
    if (durationStr.isEmpty())
        durationStr = root.value("format").toObject().value("duration").toString();
    if (durationStr.isEmpty()) {
        info.error = "Could not determine the video duration.";
        return info;
    }

    info.duration = durationStr.toDouble();

    info.ok = info.duration > 0.0;
    if (!info.ok)
        info.error = "Video has a zero or invalid duration.";
    return info;
}

QImage thumbnail(const QString &path, double time, int height,
                 const std::atomic<bool> *cancel) {
    const QString ffmpeg = toolPath("ffmpeg");
    if (ffmpeg.isEmpty())
        return {};

    QProcess proc;
    proc.start(ffmpeg, {
        "-loglevel", "error",
        "-ss", QString::number(qMax(time, 0.0), 'f', 3),
        "-i", path,
        "-frames:v", "1",
        "-vf", QString("scale=-1:%1").arg(height),
        "-f", "image2pipe",
        "-vcodec", "mjpeg",
        "pipe:1",
    });

    // Poll instead of waitForFinished(-1) so a cancel request can kill the child
    // promptly — otherwise the std::future destructor in ThumbWorker would block
    // the UI thread until ffmpeg finishes on its own.
    while (!proc.waitForFinished(kThumbPollMs)) {
        if (proc.state() == QProcess::NotRunning)
            break;  // failed to start, or exited between polls
        if (cancel && cancel->load(std::memory_order_relaxed)) {
            proc.kill();
            proc.waitForFinished(-1);
            return {};
        }
    }

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
        return {};

    const QByteArray data = proc.readAllStandardOutput();
    QImage img;
    img.loadFromData(data, "JPEG");
    return img;
}

QStringList trimArgs(const QString &src, const QString &dst, double start, double end,
                     int scaleHeight) {
    // A clip with no length is not a clip. Refusing here rather than clamping to
    // -t 0 means a caller that skipped Backend's own check gets no command at
    // all, instead of a valid MP4 containing nothing.
    if (end - start <= 0.0)
        return {};

    // Machine-readable progress on stdout (errors stay on stderr), so the UI
    // can show how far along the encode is.
    QStringList args = {"-y", "-loglevel", "error", "-progress", "pipe:1"};
    // -ss before -i seeks fast; -t gives the output duration. +faststart puts
    // the moov atom up front so shared clips start playing before they finish
    // downloading.
    args << "-ss" << QString::number(start, 'f', 3)
         << "-i" << src
         << "-t" << QString::number(end - start, 'f', 3);
    // "Original" is a length cut and nothing else, so it copies the streams
    // through instead of re-encoding them. That is lossless, near-instant, and
    // it means libx264 never opens — which retires the whole family of
    // even-dimension failures for the path most exports take.
    //
    // The price, measured rather than assumed, is that a copy can only start on
    // a keyframe. ffmpeg hides that behind an mp4 edit list, so the presented
    // clip is right, but the file physically carries everything back to the
    // previous keyframe. On a 60s 1080p source cut 7.0 -> 12.0:
    //
    //   GOP        presented   physical   lead-in   size
    //   1s          5.066667   5.133334     0.067   3.7 MB
    //   2s (phone)  5.066667   6.133334     1.067   4.5 MB
    //   x264 dflt   5.066667  12.133334     7.067   8.7 MB
    //
    // and the duration lands within about two frames rather than exactly, where
    // a re-encode gives 5.000000 every time.
    if (scaleHeight <= 0) {
        args << "-c" << "copy" << "-movflags" << "+faststart" << dst;
        return args;
    }

    // Cap the shorter side, judged on the decoded (rotation-applied) frame, so
    // portrait and landscape both keep their aspect ratio.
    //
    // The target is min(asked, shorter side) rather than the asked height, so
    // this can never upscale no matter what the caller passes.
    // Backend::exportHeights only offers heights below the source's shorter
    // side, but that governs what the dialog shows, not what exportClip
    // accepts — asking exportClip for 1080p on a 720p source used to blow it up
    // to 1080. Expressing the limit inside the filter keeps it true for every
    // caller, and needs no source dimensions passed down here to be right.
    //
    // Both sides are computed rather than left to ffmpeg's -2 "round to even".
    // -2 rounds up as readily as down, which on a source with an odd side is an
    // upscale for a request that should have changed nothing, and it only fixes
    // the side it is applied to. Leaving an odd side alone is worse than a
    // pixel: a 1280x719 frame asked for 720p kept its odd height, and whether
    // that survives depends on the pixel format it lands in —
    //
    //   odd frame, 4:4:4 out  -> exit 0, 1280x719
    //   odd frame, 4:2:0 out  -> exit 187, "height not divisible by 2", no file
    //
    // and 4:2:0 is what nearly all real footage encodes to. Flooring both sides
    // to even removes the condition rather than betting on it. Floored to at
    // least 2 as well, because on a tiny source the rounding reaches zero.
    const QString shortSide = QStringLiteral("min(iw,ih)");
    const QString target = QStringLiteral("min(%1,%2)").arg(scaleHeight).arg(shortSide);
    const auto sideExpr = [&](const QString &side) {
        return QStringLiteral("max(2,2*floor(%3*%1/%2/2))").arg(target, shortSide, side);
    };
    args << "-vf"
         << QStringLiteral("scale='%1':'%2'")
                .arg(sideExpr(QStringLiteral("iw")), sideExpr(QStringLiteral("ih")));
    args << "-c:v" << "libx264" << "-preset" << "veryfast"
         << "-crf" << "18" << "-c:a" << "aac"
         << "-movflags" << "+faststart"
         << dst;
    return args;
}

}  // namespace ffmpeg
