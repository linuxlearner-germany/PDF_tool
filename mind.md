# mind.md

## Ziel

Kritische Risiken aus der Review direkt im Code reduzieren und jede relevante Aenderung nachvollziehbar festhalten.

## Umgesetzte Arbeit

### 1. Capability-Transparenz fuer qpdf und OCR

- `QPdfOperations` hat jetzt `availabilityError()`, damit das UI den Grund fuer deaktivierte native PDF-Operationen sauber anzeigen kann.
- `OcrService` hat jetzt `availabilityError()`, damit fehlendes `tesseract` nicht nur implizit ueber deaktivierte Buttons sichtbar ist.
- `PdfDocumentController` exponiert jetzt:
  - `isOcrBusy()`
  - `ocrAvailabilityError()`
  - `supportsNativePdfEditExport()`
  - `nativePdfEditExportError()`
- `MainWindow` aktualisiert Tooltips und eine neue Statusleisten-Anzeige mit dem aktuellen Runtime-Status fuer `qpdf` und OCR.

### 2. OCR nicht mehr blockierend

- OCR-Laeufe werden im `PdfDocumentController` jetzt ueber `QtConcurrent` in einen Hintergrundjob verschoben.
- Das UI bekommt weiterhin `busyStateChanged`, `ocrFinished` und `errorOccurred`, aber ohne das bisherige blockierende `waitForFinished(...)` im UI-Fluss.
- Doppelstarts werden verhindert: Wenn bereits ein OCR-Job aktiv ist, wird ein klarer Fehler emittiert.

### 3. Seitenoperationen mit Backup/Rollback

- `MainWindow::applyPageOrderChange(...)` ersetzt die Originaldatei nicht mehr ueber `remove(...)` vor dem eigentlichen Uebernehmen der neuen Datei.
- Stattdessen:
  - Original wird nach `*.pageops.bak.pdf` verschoben
  - neue Datei wird an den Zielpfad uebernommen
  - bei Fehlern in Sidecar-Remap oder Reopen wird ein Rollback versucht
- Damit ist der vorherige Data-Loss-Pfad bei fehlgeschlagenem Rename oder spaeterem Reopen deutlich reduziert.

### 4. Exportpfade expliziter gemacht

- Beim Export bearbeiteter PDFs wird jetzt ein Statushinweis emittiert, wenn native PDF-Aenderungen nicht verfuegbar sind und auf ein gerastertes PDF zurueckgefallen wird.
- Das UI zeigt fuer Export- und qpdf-abhaengige Aktionen jetzt klarere Hinweise an.

### 5. Build-Anpassung

- `Qt6::Concurrent` wurde in `CMakeLists.txt` aufgenommen, damit die asynchrone OCR-Ausfuehrung sauber gebaut werden kann.

## Betroffene Dateien

- `src/services/OcrService.h`
- `src/services/OcrService.cpp`
- `src/operations/QPdfOperations.h`
- `src/operations/QPdfOperations.cpp`
- `src/document/PdfDocumentController.h`
- `src/document/PdfDocumentController.cpp`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `CMakeLists.txt`

## Offene Punkte / naechste sinnvolle Schritte

- `PdfDocumentController` ist weiterhin zu breit und sollte in `History`, `Export`, `Sidecar` und `OCR` Services zerlegt werden.
- Es fehlen weiterhin Tests fuer:
  - Controller-Logik
  - Undo/Redo
  - Export-Fallbacks
  - Fehler- und Rollback-Pfade
- Sidecar-Laden/Speichern behandelt Fehler noch nicht durchgaengig transaktional.
- OCR-Resultate koennen derzeit noch nach Dokumentwechsel eintreffen; wenn das stoert, sollte ein Request-Token oder Session-Guard eingebaut werden.

## Verifikation

- `cmake --build cmake-build-debug`
  - fehlgeschlagen, weil im Build-Tree kein nutzbares `Makefile` vorhanden war
- `cmake -S . -B cmake-build-debug`
  - fehlgeschlagen, weil auf dem System kein `Qt6Config.cmake` gefunden wurde
  - konkrete Meldung: `Qt6_DIR:PATH=Qt6_DIR-NOTFOUND`
- Dadurch konnten Build und Tests in dieser Umgebung nicht bis zum Ende ausgefuehrt werden.
