# mind.md

## Index

- [changelog.md](/home/paul/Dokumente/GitHub/PDF_tool/changelog.md)
- [architecture.md](/home/paul/Dokumente/GitHub/PDF_tool/architecture.md)
- [roadmap.md](/home/paul/Dokumente/GitHub/PDF_tool/roadmap.md)

## Verifikation

- `cmake --build cmake-build-debug`
  - fehlgeschlagen, weil im Build-Tree kein nutzbares `Makefile` vorhanden war
- `cmake -S . -B cmake-build-debug`
  - fehlgeschlagen, weil auf dem System kein `Qt6Config.cmake` gefunden wurde
  - konkrete Meldung: `Qt6_DIR:PATH=Qt6_DIR-NOTFOUND`
- vorhandene Paket-Binary gestartet:
  - `./installer/dist/pdf-tool_0.1.0_amd64/opt/pdftool/bin/PDFTool`
  - Prozess lief lokal an und erzeugte keinen sofortigen Fehleroutput
  - danach wieder beendet, damit kein Hintergrundprozess offen bleibt
- Dadurch konnten Build und Tests in dieser Umgebung weiterhin nicht bis zum Ende ausgefuehrt werden, aber ein vorhandenes Artefakt liess sich lokal starten.
