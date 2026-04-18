# architecture.md

## Zielbild

`PdfDocumentController` soll nur noch Orchestrator sein. Fachlogik fuer OCR, Export, History und Sidecar liegt in eigenen Diensten.

## Aktuelle Struktur

### Orchestrierung

- `PdfDocumentController`
  - haelt UI-relevanten Dokumentzustand
  - verbindet Services mit Qt-Signalen
  - steuert Seitennavigation und Dokumentlebenszyklus
  - delegiert Search- und Selection-Flows in eigene Services

### Services

- `SelectionService`
  - Textselektion und Bereichsauswahl
  - Overlay-Selektion
  - Redaction aus Selektion
  - Form-Field-Style-Lookups fuer UI-nahe Selection-Flows

- `SearchService`
  - Query-Zustand
  - Trefferlisten
  - Navigation ueber Suchtreffer

- `OcrServiceController`
  - asynchrone OCR-Ausfuehrung
  - `requestId` / `activeRequestId`
  - stale-result Schutz
  - Session-Invalidierung bei Dokumentwechsel

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

- `MainWindow` ist weiter entlastet; Action-/Menu-/Toolbar-Wiring liegt jetzt in `MainWindowActions.cpp`.
- `PdfDocumentController` ist schlanker, kennt aber weiterhin Annotation- und Overlay-Zustand direkt.
- Export- und OCR-Service sind ausgelagert, aber noch nicht maximal granular.
