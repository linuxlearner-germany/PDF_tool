# Installer

Enthalten:

- `install.sh`
  Klont das GitHub-Repo und installiert lokal nach `~/.local`
- `update.sh`
  Aktualisiert eine bestehende lokale Installation in `~/.local`
- `build_deb.sh`
  Baut aus dem aktuellen lokalen Checkout ein Debian-Paket in `installer/dist/`
- `linux/pdf-tool.desktop.in`
  Desktop-Datei-Vorlage
- `linux/pdf-tool.in`
  Start-Wrapper mit `LD_LIBRARY_PATH`
- `assets/pdftool.svg`
  App-Icon

Schnellstart:

```bash
./installer/install.sh
./installer/update.sh
./installer/build_deb.sh
```

`install.sh` prüft vor dem Klonen und Bauen automatisch:

- `git`
- `cmake`
- `pkg-config`
- `c++`
- Qt 6 Module
- `poppler-qt6`
- `libjpeg`
- `zlib`
- `libqpdf`

Wenn etwas fehlt, gibt das Skript direkt einen passenden `apt install`-Vorschlag für Debian/Ubuntu aus.
Auf Debian/Ubuntu kann es die fehlenden Pakete auf Wunsch auch direkt selbst installieren.

GitHub-Quelle für `install.sh`:

```text
https://github.com/linuxlearner-germany/PDF_tool
```

Optionaler Branch:

```bash
REPO_BRANCH=main ./installer/install.sh
REPO_BRANCH=main ./installer/update.sh
```

Automatische Paketinstallation ohne Rückfrage:

```bash
AUTO_INSTALL_DEPS=1 ./installer/install.sh
AUTO_INSTALL_DEPS=1 ./installer/update.sh
```

Vorhandene lokale Installation aktualisieren:

```bash
./installer/update.sh
```
