#pragma once

#include <QImage>
#include <QString>
#include <QStringList>

#include <atomic>

// Thin wrappers around the ffmpeg/ffprobe command-line tools.
namespace ffmpeg {

struct VideoInfo {
    QString path;
    double duration = 0.0;  // seconds
    int width = 0;
    int height = 0;
    bool ok = false;
    QString error;
    QString videoCodec;  // empty if the file has none
    QString audioCodec;
};

// Which streams of a probed file may be copied into an MP4 rather than
// re-encoded. The test is not "will ffmpeg mux it" - it will mux plenty that a
// Mac then refuses to play - but "does AVFoundation open the result".
// Measured with an AVURLAsset on the copied file:
//
//   h264 + aac    PLAYABLE, both tracks present        -> copy both
//   mpeg4 + aac   PLAYABLE, both tracks present        -> copy both
//   mjpeg + aac   PLAYABLE, both tracks present        -> copy both
//   h264 + mp3    PLAYABLE, but the AUDIO TRACK IS NOT THERE -> copy video only
//   vp9 + opus    NOT-PLAYABLE                          -> copy neither
//
// So it is per stream, not per file. A codec joins this list when a test for it
// is green, never because it ought to work: HEVC and ProRes are unmeasured and
// deliberately absent.
struct CopyPlan {
    bool video = false;
    bool audio = false;
};
CopyPlan copyPlanFor(const VideoInfo &info);

// Probe a file for a usable video stream and duration (runs ffprobe).
VideoInfo probe(const QString &path);

// Grab a single frame at `time` seconds, scaled to `height` px.
// Returns a null QImage on failure. If `cancel` is set and flips to true while
// the ffmpeg child is running, the child is killed and a null QImage returned.
QImage thumbnail(const QString &path, double time, int height = 90,
                 const std::atomic<bool> *cancel = nullptr);

// Build the ffmpeg argument list that writes [start, end] of src to dst.
// Cuts are frame-accurate and re-encoded with libx264/aac. A non-zero
// scaleHeight downscales so the shorter side becomes scaleHeight (1080p of a
// portrait video is 1080 wide), always preserving the aspect ratio.
//
// Downscale only: a scaleHeight above the source's shorter side leaves the frame
// alone rather than blowing it up, and a clip with no length gives back an empty
// list. Both limits live here rather than in the callers, so there is no route
// to an upscaled or empty export.
QStringList trimArgs(const QString &src, const QString &dst, double start, double end,
                     int scaleHeight = 0, CopyPlan copy = {});

// Locate a tool on PATH; returns empty string if missing.
QString toolPath(const QString &tool);

}  // namespace ffmpeg
