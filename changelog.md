# changelog.md

## Aktueller Stand

### Capability-Transparenz

- `QPdfOperations` liefert jetzt `CapabilityState`.
- `OcrService` liefert jetzt `CapabilityState`.
- `PdfDocumentController` exponiert zentrale Capability-Abfragen fuer Export und OCR.

### OCR

- OCR laeuft asynchron ueber `OcrServiceController`.
- Jeder OCR-Request bekommt eine `requestId`.
- `activeRequestId` wird verfolgt.
- Veraltete Ergebnisse werden ignoriert.

### Dateioperationen

- Sidecar-Schreiben nutzt `QSaveFile`.
- Datei-Ersetzungen laufen ueber `AtomicFileTransaction`.
- PDF-Save-Pfade verwenden Temp-Datei plus kontrollierten Replace statt direktem Ueberschreiben.

### Controller-Split

- `PdfDocumentController` delegiert jetzt an:
  - `OcrServiceController`
  - `ExportService`
  - `HistoryService`
  - `SidecarService`

### UI-/Controller-Aufteilung

- `MainWindowSupport.cpp` enthaelt Dialog- und Helperlogik.
- `MainWindowState.cpp` enthaelt Theme-, Status- und Kontextmenu-Logik.
- `MainWindowFileWorkflows.cpp` enthaelt Datei-, Export-, Druck- und OCR-Workflows.
- `MainWindowDialogWorkflows.cpp` enthaelt dialoggetriebene UI-Workflows.
- `PdfDocumentControllerState.cpp` enthaelt Persistenz-, Historien- und Render-Helfer.
- `PdfDocumentControllerSelection.cpp` kapselt Selection-/Overlay-Orchestrierung.
- `PdfDocumentControllerSearch.cpp` kapselt Search-Orchestrierung.

### OCR-Haertung

- Dokumentwechsel invalidiert jetzt aktive OCR-Sessions.
- Veraltete Ergebnisse werden nach `openDocument()` und `closeDocument()` ignoriert.

### Tests

- Neu hinzugefuegt:
  - `HistoryServiceTests`
  - `OcrServiceControllerTests`
  - `ExportServiceTests`
  - `RedactionModelTests`
  - `SearchModelTests`
  - `DocumentStateStoreTests`
  - `PdfDocumentControllerTests`
- `OcrServiceControllerTests` prueft jetzt auch Session-Invalidierung bei pending OCR.
