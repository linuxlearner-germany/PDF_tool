# roadmap.md

## Priority A

1. `PdfDocumentController` fachlich weiter verkleinern
   - Selection-/Overlay-Orchestrierung separieren
   - Search-Orchestrierung separieren

2. OCR weiter haerten
   - optional echte Cancellation
   - Session-Guard bei Dokumentwechsel

3. Dateiersetzung noch robuster machen
   - atomische Replace-Pfade fuer mehr Workflows
   - explizite Recovery/Restore-Berichte

## Priority B

4. Capability-System weiter zentralisieren
   - qpdf-abhaengige UI-Action-Aktivierung konsolidieren
   - Build-/Runtime-Capabilities klar trennen

5. Tests erweitern
   - Export-Fallback bis auf Dateiebene
   - OCR-Parallelitaet und Fehlerpfade
   - Rollback-/Recovery-Pfade

## Priority C

6. MainWindow weiter zerlegen
   - File-Workflows
   - Export-Workflows
   - Dialog-Workflows

7. Packaging / Startfaehigkeit
   - Qt6/CMake-Prefix sauber dokumentieren
   - reproduzierbarer lokaler Start
