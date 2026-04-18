# architecture.md

## Zielbild

`PdfDocumentController` soll nur noch Orchestrator sein. Fachlogik fuer OCR, Export, History und Sidecar liegt in eigenen Diensten.

## Aktuelle Struktur

### Orchestrierung

- `PdfDocumentController`
  - haelt UI-relevanten Dokumentzustand
  - verbindet Services mit Qt-Signalen
  - steuert Seitennavigation, Auswahl und Overlay-Emission

### Services

- `OcrServiceController`
  - asynchrone OCR-Ausfuehrung
  - `requestId` / `activeRequestId`
  - stale-result Schutz

- `ExportService`
  - Print
  - bearbeiteter PDF-Export
  - Redaction-Export
  - Speichern via temp-file + Replace

- `HistoryService`
  - Snapshot-Speicherung
  - Undo/Redo-Invarianten

- `SidecarService`
  - Sidecar-Pfad
  - State-Roundtrip
  - Legacy-Schema-Laden

- `AtomicFileTransaction`
  - kontrollierte Dateiersetzung mit Backup/Restore-Pfad

## Capability-System

- Zentrale Form:

```cpp
struct CapabilityState {
    bool available;
    QString error;
};
```

- Verwendet in:
  - `OcrService`
  - `QPdfOperations`
  - `PdfDocumentController`
  - `MainWindow`

## Noch nicht vollendet

- `MainWindow` orchestriert weiterhin viele Datei- und Workflow-Pfade.
- `PdfDocumentController` kennt noch zu viel von Annotation-, Selection- und Search-Zustand.
- Export- und OCR-Service sind ausgelagert, aber noch nicht maximal granular.
