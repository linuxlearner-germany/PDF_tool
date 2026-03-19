# PDFTool

Desktop-PDF-Viewer und PDF-Bearbeitungstool auf Basis von Qt 6, CMake, Poppler und qpdf.

PDFTool ist ein pragmatisches Linux-Desktop-Projekt mit Fokus auf produktive PDF-Arbeit:
- PDFs oeffnen und lesen
- Seiten, Outline und Suche nutzen
- markieren, kommentieren und redigieren
- Text sichtbar ersetzen
- OCR auf Seiten oder Bereichen ausfuehren
- Unterschriften als Bild einfuegen
- neue bearbeitete PDFs exportieren

## Status

Das Projekt ist deutlich ueber ein MVP hinaus und laeuft aktuell als fortgeschrittener Desktop-Viewer/Editor mit Overlay-basierter Bearbeitung und Exportpfaden fuer dauerhaft veraenderte PDFs.

Wichtig:
- direkte semantische Bearbeitung des internen PDF-Textstroms ist nicht implementiert
- Textaenderungen, Signaturen, Formularwerte und Redactions werden ueber Overlays verwaltet
- mit `Bearbeitetes PDF exportieren...` werden diese Aenderungen dauerhaft in ein neues PDF eingebrannt

## Features

### Dokument und Navigation

- PDF oeffnen
- Rendern ueber Poppler
- Zoom
- Scrollen und Seitenwechsel per Mausrad
- `Ctrl + Mausrad` fuer Zoom
- Seitenleiste mit Thumbnails
- aktive Seite in der Seitenleiste hervorgehoben
- Outline / Bookmarks
- Metadaten-Dialog
- Drucken

### Auswahl, Suche und Bearbeitung

- Rechteckauswahl
- Textselektion innerhalb einer Auswahl
- `Ctrl+C` fuer Copy
- dokumentweite Suche mit Highlighting
- Highlight-Annotationen
- Rechteck-Annotationen
- Textnotizen
- freie Textfelder
- sichtbares Text-Ersetzen innerhalb einer Auswahl
- Farbaenderung und Loeschen fuer ausgewaehlte Annotationen

### Formulare

- Textfelder aus PDFs auslesen
- Checkboxen aus PDFs auslesen
- Formularwerte im Viewer bearbeiten
- Formularzustand per Sidecar-Datei sichern

### Redaction

- Auswahlbereiche schwaerzen
- schwarze Redaction-Overlays im Viewer
- Export als dauerhaft geschwaerztes PDF

### OCR

- OCR fuer aktuelle Seite
- OCR fuer Auswahlbereich
- OCR-Ergebnisdialog mit Copy-Funktion
- lokale Ausfuehrung ueber `tesseract`

### Unterschriften

- Unterschrift als Bilddatei einfuegen
- Platzierung auf Auswahl oder an Klickposition
- Persistenz ueber Sidecar-Datei
- Export in bearbeitetes PDF

### PDF-Operationen

- PDFs zusammenfuehren
- PDFs aufteilen
- passwortgeschuetzte PDFs oeffnen
- verschluesseltes PDF exportieren

### UI

- ueberarbeitete Desktop-Oberflaeche
- umschaltbarer Light/Dark Mode
- Theme-Auswahl wird gespeichert
- Statusleiste mit Dokument-, Modus-, Seiten-, Zoom- und Suchstatus

## Architektur

Die bestehende Architektur ist bewusst getrennt gehalten:

- `src/ui`
  UI-Komponenten wie `MainWindow`, `PdfView`, Thumbnail-Liste
- `src/document`
  Controller und Dokumentmodelle fuer Annotationen, Formulare, Suche, Redactions
- `src/rendering`
  Render-Backend ueber Poppler
- `src/operations`
  qpdf-basierte Operationen wie Merge, Split, Verschluesselung
- `src/services`
  Zusatzdienste wie OCR

Wichtige Kernklassen:
- `PdfDocumentController`
- `PopplerAdapter`
- `PdfView`
- `AnnotationModel`
- `FormFieldModel`
- `RedactionModel`
- `OcrService`

## Sidecar-Dateien

PDFTool speichert nicht alle Aenderungen direkt ins Original-PDF. Stattdessen wird zusaetzlich eine Sidecar-Datei neben dem PDF abgelegt:

```text
dein_dokument.pdf.annotations.json
```

Darueber werden aktuell unter anderem gespeichert:
- Annotationen
- freie Textfelder
- Signaturen
- Formularwerte
- Redactions

## Voraussetzungen

### Pflichtabhaengigkeiten

Beispiel fuer Debian/Ubuntu:

```bash
sudo apt install \
  cmake \
  ninja-build \
  g++ \
  qt6-base-dev \
  libpoppler-qt6-dev \
  libqpdf-dev \
  libjpeg62-turbo-dev \
  extra-cmake-modules \
  zlib1g-dev
```

### Optionale Abhaengigkeiten

Fuer OCR:

```bash
sudo apt install tesseract-ocr tesseract-ocr-deu tesseract-ocr-eng
```

## Build

### Build gegen Systempakete

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/PDFTool
```

### Build gegen lokale Dependencies

Das Projekt kann auch mit der vorhandenen `.localdeps`-Struktur gebaut werden:

```bash
cmake -S . -B build-local -G Ninja
cmake --build build-local
./build-local/PDFTool
```

## Bedienung

### Wichtige Shortcuts

- `Ctrl+O` PDF oeffnen
- `Ctrl+P` drucken
- `Ctrl+F` Suche fokussieren
- `Ctrl+C` Text kopieren
- `Ctrl+Mausrad` zoomen
- `PageUp` vorherige Seite
- `PageDown` naechste Seite
- `Delete` ausgewaehltes Overlay loeschen
- `F3` naechster Treffer
- `Shift+F3` vorheriger Treffer

### Typischer Workflow

1. PDF oeffnen
2. Bereich auswaehlen
3. je nach Bedarf:
   - Text hervorheben
   - Text ersetzen
   - Redaction markieren
   - OCR auf Auswahl ausfuehren
   - Signatur in Auswahl platzieren
4. `Bearbeitetes PDF exportieren...` oder `Geschwaerztes PDF exportieren...`

## Exportmodi

### Bearbeitetes PDF exportieren

Erzeugt ein neues PDF mit eingebrannten:
- Text-Overlays
- Signaturen
- Formularwerten
- Annotationen
- Redactions

### Geschwaerztes PDF exportieren

Erzeugt ein neues PDF, bei dem Redactions dauerhaft schwarz ausgegeben werden.

## Bekannte Grenzen

- keine echte semantische PDF-Textbearbeitung mit Reflow
- keine kryptografischen PDF-Signaturen
- OCR erzeugt aktuell keinen echten durchsuchbaren PDF-Textlayer
- Formularwerte werden aktuell nicht ins Original-PDF zurueckgeschrieben
- Annotationen werden nicht als native PDF-Annotationen gespeichert, sondern per Sidecar verwaltet

## Roadmap

Sinnvolle naechste Schritte:

- echte PDF-Annotation-Writes
- OCR-Textlayer im Export
- digitale Signaturen / Zertifikate
- Drag/Resize fuer Textfelder und Signaturen
- Undo/Redo
- Performance-Optimierung fuer grosse PDFs

## Entwicklung

Das Projekt wurde schrittweise erweitert, ohne die Grundarchitektur neu zu schreiben. Ziel ist ein sauberer Ausbau statt eines Voll-Rewrites.

Wenn du am Projekt weiterarbeiten willst, sind diese Dateien die wichtigsten Einstiegsstellen:

- `src/ui/MainWindow.cpp`
- `src/ui/PdfView.cpp`
- `src/document/PdfDocumentController.cpp`
- `src/rendering/PopplerAdapter.cpp`
- `src/operations/QPdfOperations.cpp`

## Haftungsausschluss

Die Nutzung dieser Software erfolgt auf eigene Verantwortung.

Ich uebernehme keine Haftung fuer:
- Datenverlust
- beschaedigte oder unbrauchbar gewordene PDF-Dateien
- fehlerhafte OCR-Ergebnisse
- fehlerhafte Exportergebnisse
- Sicherheitsluecken, Sicherheitsfehler oder sonstige Schwachstellen
- rechtliche oder technische Folgen durch den Einsatz der Software

Insbesondere bei Bearbeitung, Redaction, OCR, Verschluesselung, Signaturen und Exporten sollte die Software nicht ohne eigene Pruefung in produktiven oder rechtlich sensiblen Workflows eingesetzt werden.
