# Omacut

A dead-simple video **length** trimmer. Open a video, drag the two handles to pick a start and end, preview the clip, and export. On Omarchy, the interface follows your theme's accent color.

Built using **Qt Quick (QML)** UI with the Material style — the same Qt stack Quickshell builds on — and **ffmpeg** for the cut. The C++ side compiles to a single executable; the QML is embedded in it via Qt resources.

<img width="3227" height="3227" alt="screenshot-2026-06-23_15-20-40" src="https://github.com/user-attachments/assets/c76047c8-618f-4c1c-91f9-e7024c4f953b" />

## Hotkeys

- *Space*: Start/stop video playback.
- *Left/Right*: Move the playhead by 1 second.
- *Shift+Left/Right*: Move the playhead by 5 seconds.
- *Alt+Left/Right*: Move the playhead by 0.2 seconds.
- *Ctrl+Space*: Move the start of the trim to the playhead.
- *Alt+Space*: Move the end of the trim to the playhead.
- *Z*: Zoom into the trimmed selection for fine tuning (Z again zooms back out).
- *Ctrl+O*: Open a new file to trim.
- *Ctrl+S*: Export the current trim.
- *Q*: Quit (asks first if the trim hasn't been exported).
- *?*: Show the hotkeys in the app.

On macOS `Ctrl` is `⌘`, and because `⌘Space` belongs to Spotlight the trim edges
are also on *I* (start) and *O* (end). Press *?* in the app for the list that
applies to the machine you are on.

## Install

Install via the Omarchy Package Repository via the `omacut` package. It's installed by default in new installations of Omarchy (from Quattro forward).

On macOS, see [macOS](#macos) below — this fork builds and runs there too.

## Requirements

- On Linux: `xdg-desktop-portal` and a portal backend for the file picker.
  On macOS the system's own open/save panels are used instead.
- `ffmpeg` and `ffprobe` (used at runtime)

Exports are always written as MP4 files, regardless of the input video's container. The export dialog offers Original/1080p/720p quality — never upscaling, and always preserving the aspect ratio.

**Original copies the streams instead of re-encoding them.** The cut is lossless and near-instant, and the frame is untouched — but the codec is whatever the source had, so a VP9 WebM comes out as VP9 in an MP4 container rather than H.264. A copy can also only begin on a keyframe: the clip you get is the one you asked for, because the timing is carried in an MP4 edit list, but the file physically holds everything back to the previous keyframe. On long-GOP sources that is several seconds of hidden lead-in and a noticeably larger file, and a player that ignores edit lists will show it. Pick 1080p or 720p to re-encode instead — those are frame-exact H.264/AAC.

## Build

Uses Qt's own build tool, `qmake6` (no cmake needed):

```bash
./bin/build
```

This produces a single `omacut` binary in `build/` (on macOS, a `build/omacut.app`
bundle — see below).

Requirements:

- A C++17 compiler and Qt6: `qt6-base`, `qt6-declarative` (Qt Quick + Controls),
  `qt6-multimedia`

## macOS

```bash
brew install qt ffmpeg
./bin/build     # -> build/omacut.app
./bin/bundle    # copies Qt into the bundle, so it runs without Homebrew
cp -R build/omacut.app /Applications/
```

`./bin/build` alone gives you an app that runs on *this* machine — it still loads
Qt out of Homebrew. `./bin/bundle` runs `macdeployqt`, drops the plugins it
collected but could not complete, checks the result and re-signs, after which the
`.app` is self-contained and can be copied to another Mac. `ffmpeg` stays an
external dependency either way; it is not bundled.

`macdeployqt` on a Homebrew Qt reports dozens of `Cannot resolve rpath` errors
and still exits 0, so its exit code is not evidence that the bundle works.
`./bin/verify-bundle build/omacut.app` is what decides: it walks every Mach-O
inside and fails if anything still points outside the bundle. `bin/bundle` will
not sign a bundle that does not pass it, and you can run it on its own.

Differences from the Linux build, all of them forced by the platform:

- **File dialogs** are the system open/save panels, not `xdg-desktop-portal`.
  The export quality (Original/1080p/720p) rides in the save panel's file-format
  popup, because a Cocoa save panel has no room for a separate combo.
- **Hotkeys**: Qt maps `Ctrl` to ⌘, so open and export are ⌘O and ⌘S as a Mac
  user expects. `Ctrl+Space` would become ⌘Space, which is Spotlight, so the
  trim points also answer to **I** (start) and **O** (end) here.
- **ffmpeg lookup** falls back to `/opt/homebrew/bin`, `/usr/local/bin` and
  `/opt/local/bin` when it is not on `PATH` — an app launched from Finder
  inherits launchd's `PATH`, which contains none of them.
- Video files can be opened with Finder's "Open With", or by dropping them on
  the app icon.

## Test

```bash
./bin/test
```

## Package

Build and install the local Arch package:

```bash
./bin/install
```

This runs `./bin/build`, then `makepkg -fsi` from `pkgbuild/` so same-version local packages are rebuilt and reinstalled. Extra arguments are passed through to `makepkg`, for example `./bin/install --clean`. The package installs the binary, desktop entry, app icon, and MIT license. Local package outputs such as `pkgbuild/pkg/`, `pkgbuild/src/`, and `*.pkg.tar.*` are ignored.

## License

MIT. See `LICENSE`.
