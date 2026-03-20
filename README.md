# PDFTool

Linux-Desktop-App zum Ansehen, Kommentieren, Redigieren und Exportieren von PDFs.
Das Projekt basiert auf `Qt 6`, `Poppler` und optional `qpdf` fuer weitergehende PDF-Operationen.

## Ueberblick

PDFTool richtet sich an lokale Desktop-Workflows, bei denen PDFs nicht nur gelesen, sondern direkt bearbeitet werden sollen:

- PDFs oeffnen, lesen und durchsuchen
- Seiten und Bookmarks schnell navigieren
- Bereiche markieren, kommentieren und schwaerzen
- sichtbare Textersetzungen als Overlay anlegen
- OCR auf Seiten oder Auswahlbereichen ausfuehren
- Signaturen als Bild einfuegen
- bearbeitete oder redigierte PDFs als neues Dokument exportieren

Wichtig:

- Das Original-PDF wird nicht "live" semantisch umgeschrieben.
- Viele Aenderungen beginnen intern als Overlays bzw. temporaere Sidecar-Daten.
- Mit Export oder Speichern werden diese Aenderungen dauerhaft in PDF-Dateien uebernommen.

## Feature-Set

### Viewer und Navigation

- PDF-Rendering ueber Poppler
- Zoom, Scrollen und Seitenwechsel per Mausrad
- `Ctrl + Mausrad` fuer Zoom
- Seitenleiste mit Thumbnails
- Outline / Bookmarks
- Metadaten-Dialog
- Drucken

### Bearbeitung und Annotationen

- Rechteckauswahl und Textselektion
- `Ctrl+C` fuer Kopieren
- Dokumentweite Suche mit Hervorhebung
- Highlight-Annotationen
- Rechteck-Annotationen
- Textnotizen und freie Textfelder
- sichtbares Text-Ersetzen in Auswahlbereichen
- Farben aendern und ausgewaehlte Annotationen loeschen
- Export schreibt Highlights, Rechtecke, Notizen, Freitext und Signaturbilder wenn moeglich als native PDF-Annotationen

### Formulare

- PDF-Textfelder auslesen
- Checkboxen auslesen
- Formularwerte im Viewer bearbeiten
- Formularzustand per Sidecar-Datei sichern

### Redaction

- Bereiche zur Schwaerzung markieren
- schwarze Redaction-Overlays im Viewer
- Export als dauerhaft geschwaerztes PDF
- nativer PDF-Export mit schwarzen Vektor-Overlays statt reinem Raster-Fallback
- destruktiver Redaction-Export ersetzt redigierte Seiten durch neu aufgebaute Seitenabbilder

### OCR

- OCR fuer die aktuelle Seite
- OCR fuer einen Auswahlbereich
- Ergebnisdialog mit Copy-Funktion
- lokale Ausfuehrung ueber `tesseract`
- Export erzeugt zusaetzlich einen durchsuchbaren OCR-Textlayer, wenn `tesseract` verfuegbar ist

### Weitere PDF-Operationen

- PDFs zusammenfuehren
- PDFs aufteilen
- passwortgeschuetzte PDFs oeffnen
- verschluesselte PDFs exportieren
- kryptografisch signierte PDFs ueber das Poppler-Signatur-Backend exportieren

## Status

Das Projekt ist deutlich ueber ein MVP hinaus und funktioniert bereits als produktiver Desktop-Viewer mit Bearbeitungs- und Export-Workflow.

Aktuell nicht enthalten:

- echte semantische PDF-Textbearbeitung mit Reflow
- selektive objektgenaue Redaction einzelner PDF-Inhalte ohne Seiten-Flattening

## Schnellstart

### Voraussetzungen

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

Optional fuer OCR:

```bash
sudo apt install tesseract-ocr tesseract-ocr-deu tesseract-ocr-eng
```

### Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/PDFTool
```

### Build mit lokalen Dependencies

Wenn die vorhandene `.localdeps`-Struktur genutzt werden soll:

```bash
cmake -S . -B build-local -G Ninja
cmake --build build-local
./build-local/PDFTool
```

## Installation

### Lokale Installation per Installer

Im Ordner `installer/` liegt ein Installer, der das Projekt aus GitHub klonen, bauen und lokal nach `~/.local` installieren kann.

Empfohlen:

```bash
cd installer
bash install.sh
```

Update einer bestehenden lokalen Installation:

```bash
cd installer
bash update.sh
```

Wichtig:

- das Skript nicht mit `sudo` starten
- sonst landet die Installation unter `/root/.local/...`

Ohne Rueckfrage fehlende Pakete installieren:

```bash
cd installer
AUTO_INSTALL_DEPS=1 bash install.sh
```

Dasselbe fuer Updates:

```bash
cd installer
AUTO_INSTALL_DEPS=1 bash update.sh
```

Bestimmten Branch installieren:

```bash
cd installer
REPO_BRANCH=main bash install.sh
```

### Debian-Paket bauen

```bash
cd installer
./build_deb.sh
```

Das Paket wird anschliessend unter `installer/dist/` erzeugt, derzeit typischerweise als:

```text
installer/dist/pdf-tool_0.1.0_amd64.deb
```

Installation:

```bash
sudo dpkg -i installer/dist/pdf-tool_0.1.0_amd64.deb
```

## Starten

Nach lokaler Installation:

```bash
~/.local/bin/pdf-tool
```

Falls `~/.local/bin` im `PATH` liegt, reicht auch:

```bash
pdf-tool
```

Direkt aus dem Build:

```bash
./build/PDFTool
```

## Speichern

`Strg+S` schreibt den aktuellen Bearbeitungsstand direkt in das geoeffnete PDF zurueck.
Dabei wird die Datei neu erzeugt und anschliessend erneut geladen.
Wenn Redactions enthalten sind, wird dabei ein destruktiver Exportpfad verwendet.

## Sidecar-Dateien

Waehrend der Bearbeitung nutzt PDFTool weiterhin Sidecar-Dateien neben dem Dokument, zum Beispiel:

```text
dein_dokument.pdf.annotations.json
```

Darin werden unter anderem der aktuelle Arbeitsstand und noch nicht ins PDF geschriebene Aenderungen abgelegt:

- Annotationen
- freie Textfelder
- Signaturen
- Formularwerte
- Redactions

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
3. Aktion ausfuehren, zum Beispiel Highlight, Textersetzung, Redaction, OCR oder Signatur
4. `Bearbeitetes PDF exportieren...` oder `Geschwaerztes PDF exportieren...`

## Export

### Bearbeitetes PDF exportieren

Exportiert ein neues PDF mit eingebrannten:

- Text-Overlays
- Signaturen
- Formularwerten
- Annotationen
- Redactions

### Geschwaerztes PDF exportieren

Erzeugt ein neues PDF, in dem markierte Redactions dauerhaft schwarz ausgegeben werden.

## Architektur

Die Codebasis ist bewusst in klar getrennte Bereiche aufgeteilt:

- `src/ui` fuer Fenster, View und Navigation
- `src/document` fuer Controller, Modelle und Bearbeitungszustand
- `src/rendering` fuer das Render-Backend auf Basis von Poppler
- `src/operations` fuer qpdf-basierte PDF-Operationen
- `src/services` fuer Zusatzdienste wie OCR

Zentrale Einstiegspunkte:

- `src/ui/MainWindow.cpp`
- `src/ui/PdfView.cpp`
- `src/document/PdfDocumentController.cpp`
- `src/rendering/PopplerAdapter.cpp`
- `src/operations/QPdfOperations.cpp`

## Projektstruktur

```text
.
+-- CMakeLists.txt
+-- README.md
+-- installer/
+-- resources/
`-- src/
    +-- document/
    +-- operations/
    +-- rendering/
    +-- services/
    `-- ui/
```

## Roadmap

Sinnvolle naechste Schritte:

- native PDF-Annotationen schreiben
- OCR-Textlayer im Export erzeugen
- digitale Signaturen / Zertifikate
- Drag/Resize fuer Textfelder und Signaturen
- Undo/Redo
- Performance fuer grosse PDFs verbessern

## Entwicklung

Der Fokus liegt auf einem pragmatischen Desktop-Workflow statt auf einem kompletten PDF-Editor-Rewrite.
Falls du am Projekt weiterarbeiten willst, sind die Kernpfade in `src/ui`, `src/document`, `src/rendering`, `src/operations` und `src/services` die relevanten Einstiegspunkte.

## Haftungsausschluss

Die Nutzung erfolgt auf eigene Verantwortung. Vor allem bei Redaction, OCR, Export, Verschluesselung und produktiven Dokument-Workflows sollten Ergebnisse immer manuell geprueft werden.
