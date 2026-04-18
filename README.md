# PDFTool

PDFTool is a local-first PDF desktop application for Linux. The current focus is document viewing, annotation, OCR, redaction and export workflows without cloud dependencies.

![PDFTool Screenshot](images/Screenshot_with_dokument.png)

## Overview

- Qt 6 desktop UI
- Poppler-based PDF rendering
- Tesseract-based OCR at runtime
- qpdf-backed native PDF operations when available
- Sidecar-backed document state persistence

## Current Architecture

- `MainWindow` owns the desktop shell.
- `MainWindowActions.cpp` isolates action, menu and toolbar wiring.
- `PdfDocumentController` is the document orchestrator for UI-facing state.
- `SelectionService` and `SearchService` own selection/search state outside the controller.
- `ExportService`, `HistoryService`, `SidecarService` and `OcrServiceController` handle specialized workflows.
- `AtomicFileTransaction` and `QSaveFile` protect sensitive write paths.

Internal target architecture is documented in [architecture.md](architecture.md).

## Dependencies

Required:

- Qt 6 (`Core`, `Gui`, `Widgets`, `PrintSupport`, `Concurrent`, `Test`)
- Poppler Qt6
- libjpeg
- zlib
- CMake 3.21+
- C++17 compiler

Optional:

- `qpdf` for native PDF operation paths
- `tesseract` binary for OCR

## Local Build

The project expects a working Qt 6 CMake package on the machine. If `cmake -S . -B cmake-build-debug` fails with `Qt6Config.cmake` not found, set a Qt prefix explicitly.

Recommended local flow:

```bash
export QT6_PREFIX=/path/to/Qt/6.x/gcc_64
./scripts/configure.sh
./scripts/build.sh
```

For systems with distro Qt packages, `Qt6_DIR` or `CMAKE_PREFIX_PATH` must point at the directory containing `Qt6Config.cmake`.

The repository also checks `.localdeps/sysroot/.../pkgconfig` automatically when present.

## Local Start

Built binary:

```bash
./cmake-build-debug/PDFTool
```

Existing packaged artifact in this repository:

```bash
./installer/dist/pdf-tool_0.1.0_amd64/opt/pdftool/bin/PDFTool
```

That packaged binary was started locally in this workspace on 2026-04-18 and produced no immediate stderr output before being terminated again.

## Documentation

- External overview: `README.md`
- Internal target picture: [architecture.md](architecture.md)
- Technical change log: [changelog.md](changelog.md)
- Next steps: [roadmap.md](roadmap.md)
- Verification notes: [mind.md](mind.md)
