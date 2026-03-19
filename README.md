# PDF Tool MVP

Qt-6-Desktop-Anwendung zum Laden und Rendern der ersten PDF-Seite mit klar getrennter Architektur.

## Abhängigkeiten

Beispiel für Debian/Ubuntu:

```bash
sudo apt install cmake ninja-build g++ qt6-base-dev libpoppler-qt6-dev libqpdf-dev extra-cmake-modules
```

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/PDFTool
```
