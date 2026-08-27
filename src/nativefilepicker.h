#pragma once

#include <QStringList>

#include "filepicker.h"

// A FilePicker backed by Qt's own native file dialogs — NSOpenPanel/NSSavePanel
// on macOS. Used wherever there is no xdg-desktop-portal for PortalFilePicker
// to talk to, which on a Mac is always.
//
// The portal picker gets a real "Quality" combo in the save dialog. A native
// macOS save panel has no such slot, but it does render one popup: the file
// format list built from the dialog's name filters. So the export quality rides
// in there instead — same choice, same single dialog, no extra prompt.
class NativeFilePicker : public FilePicker {
    Q_OBJECT

public:
    explicit NativeFilePicker(QObject *parent = nullptr);

    void openVideo() override;
    void exportVideo(const QUrl &suggestedUrl, double start, double end,
                     const QList<int> &scaleHeights) override;

    // The name filters offered for a set of downscale heights. Always at least
    // one entry, always with "Original" first, and every entry matches *.mp4 —
    // the format is fixed, only the scale differs.
    static QStringList exportNameFilters(const QList<int> &scaleHeights);

    // The scale height a chosen name filter asks for; 0 for "Original" and for
    // anything unrecognised, which is the same no-downscale answer the portal
    // picker gives when no choice comes back.
    static int scaleHeightForFilter(const QString &filter);
};
