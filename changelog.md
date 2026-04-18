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
- `PdfDocumentControllerState.cpp` enthaelt Persistenz-, Historien- und Render-Helfer.

### Tests

- Neu hinzugefuegt:
  - `HistoryServiceTests`
  - `OcrServiceControllerTests`
  - `ExportServiceTests`
  - `RedactionModelTests`
  - `SearchModelTests`
  - `DocumentStateStoreTests`
  - `PdfDocumentControllerTests`
