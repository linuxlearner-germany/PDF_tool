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

### 6. Persistenz aus dem Controller gezogen

- Mit `DocumentStateStore` gibt es jetzt einen eigenen Baustein fuer:
  - Sidecar-Pfadberechnung
  - JSON-Schema fuer Dokumentzustand
  - Laden/Schreiben des Sidecars
  - Legacy-Kompatibilitaet fuer das fruehere reine Annotation-Schema
- `PdfDocumentController` delegiert die Sidecar-Persistenz jetzt an diesen Store statt IO und Schema-Logik selbst zu tragen.

### 7. MainWindow und Controller physisch aufgeteilt

- `MainWindow.cpp` wurde verkleinert, indem Hilfsdialoge/Styling in `MainWindowSupport.*` ausgelagert wurden.
- Theme-, Status-, Backup-/Rollback- und Kontextmenu-Logik liegen jetzt in `MainWindowState.cpp`.
- `PdfDocumentController` hat mit `PdfDocumentControllerState.cpp` eine eigene Datei fuer Persistenz-, Historien- und Render-/Overlay-Helfer.
- Ergebnis:
  - `src/ui/MainWindow.cpp` ist von 2438 auf 1724 Zeilen gesunken
  - `src/document/PdfDocumentController.cpp` ist von 1829 auf 1585 Zeilen gesunken

### 8. Testbasis erweitert

- Neue Tests:
  - `RedactionModelTests`
  - `SearchModelTests`
  - `DocumentStateStoreTests`
  - `PdfDocumentControllerTests`
- Damit ist jetzt nicht mehr nur Modell-Detailverhalten abgesichert, sondern auch:
  - Sidecar-Roundtrip
  - Legacy-Schema-Laden
  - Search-Navigation
  - Undo/Redo fuer einen echten Controller-Workflow mit Fake-Render-Engine

## Betroffene Dateien

- `src/services/OcrService.h`
- `src/services/OcrService.cpp`
- `src/operations/QPdfOperations.h`
- `src/operations/QPdfOperations.cpp`
- `src/document/PdfDocumentController.h`
- `src/document/PdfDocumentController.cpp`
- `src/document/PdfDocumentControllerState.cpp`
- `src/document/DocumentStateStore.h`
- `src/document/DocumentStateStore.cpp`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `src/ui/MainWindowState.cpp`
- `src/ui/MainWindowSupport.h`
- `src/ui/MainWindowSupport.cpp`
- `CMakeLists.txt`
- `tests/RedactionModelTests.cpp`
- `tests/SearchModelTests.cpp`
- `tests/DocumentStateStoreTests.cpp`
- `tests/PdfDocumentControllerTests.cpp`

## Offene Punkte / naechste sinnvolle Schritte

- `PdfDocumentController` ist trotz Aufteilung auf Datei-Ebene weiterhin fachlich breit und sollte als naechstes in `History`, `Export` und `OCR` Services zerlegt werden.
- `MainWindow` ist kleiner, aber immer noch stark orchestrierend; als naechster Schritt sollten File-/Export-/Dialog-Workflows weiter aus der Klasse herausgezogen werden.
- Es fehlen weiterhin gezielte Tests fuer:
  - Export-Fallbacks
  - Fehler- und Rollback-Pfade
  - OCR-Abbruch bzw. gleichzeitige OCR-Anfragen
- Sidecar-Laden/Speichern ist jetzt gekapselt, aber noch nicht transaktional mit atomischem Write/Replace umgesetzt.
- OCR-Resultate koennen derzeit noch nach Dokumentwechsel eintreffen; wenn das stoert, sollte ein Request-Token oder Session-Guard eingebaut werden.

## Verifikation

- `cmake --build cmake-build-debug`
  - fehlgeschlagen, weil im Build-Tree kein nutzbares `Makefile` vorhanden war
- `cmake -S . -B cmake-build-debug`
  - fehlgeschlagen, weil auf dem System kein `Qt6Config.cmake` gefunden wurde
  - konkrete Meldung: `Qt6_DIR:PATH=Qt6_DIR-NOTFOUND`
- Dadurch konnten Build und Tests in dieser Umgebung nicht bis zum Ende ausgefuehrt werden.
