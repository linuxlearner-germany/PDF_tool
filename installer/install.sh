#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_URL="https://github.com/linuxlearner-germany/PDF_tool"
REPO_BRANCH="${REPO_BRANCH:-}"

WORK_ROOT="${HOME}/.cache/pdftool-installer"
SOURCE_DIR="${WORK_ROOT}/source"
BUILD_DIR="${WORK_ROOT}/build-local"
APP_BINARY="${BUILD_DIR}/PDFTool"
LOCAL_LIB_DIR="${SOURCE_DIR}/.localdeps/sysroot/usr/lib/x86_64-linux-gnu"

INSTALL_ROOT="${HOME}/.local/opt/pdftool"
APP_DIR="${INSTALL_ROOT}"
BIN_DIR="${HOME}/.local/bin"
ICON_DIR="${HOME}/.local/share/icons/hicolor/scalable/apps"
APPLICATIONS_DIR="${HOME}/.local/share/applications"

APT_PACKAGES=(
    git
    cmake
    build-essential
    pkg-config
    qt6-base-dev
    libpoppler-qt6-dev
    libjpeg-dev
    zlib1g-dev
    libqpdf-dev
)

if [[ -t 1 ]]; then
    COLOR_RESET=$'\033[0m'
    COLOR_DIM=$'\033[2m'
    COLOR_RED=$'\033[31m'
    COLOR_GREEN=$'\033[32m'
    COLOR_BLUE=$'\033[34m'
    COLOR_YELLOW=$'\033[33m'
else
    COLOR_RESET=""
    COLOR_DIM=""
    COLOR_RED=""
    COLOR_GREEN=""
    COLOR_BLUE=""
    COLOR_YELLOW=""
fi

print_header() {
    printf "\n%s== %s ==%s\n" "${COLOR_BLUE}" "$1" "${COLOR_RESET}"
}

print_info() {
    printf "%s[INFO]%s %s\n" "${COLOR_BLUE}" "${COLOR_RESET}" "$1"
}

print_ok() {
    printf "%s[OK]%s %s\n" "${COLOR_GREEN}" "${COLOR_RESET}" "$1"
}

print_warn() {
    printf "%s[WARN]%s %s\n" "${COLOR_YELLOW}" "${COLOR_RESET}" "$1"
}

print_error() {
    printf "%s[FEHLER]%s %s\n" "${COLOR_RED}" "${COLOR_RESET}" "$1" >&2
}

require_command() {
    local command_name="$1"
    if ! command -v "${command_name}" >/dev/null 2>&1; then
        return 1
    fi
}

check_system_requirements() {
    local missing_items=()

    print_header "Voraussetzungen prüfen"

    for command_name in git cmake pkg-config c++; do
        if require_command "${command_name}"; then
            print_ok "Befehl gefunden: ${command_name}"
        else
            missing_items+=("Befehl fehlt: ${command_name}")
        fi
    done

    if require_command pkg-config; then
        local modules=(
            Qt6Core
            Qt6Gui
            Qt6Widgets
            Qt6PrintSupport
            poppler-qt6
            libjpeg
            zlib
            libqpdf
        )

        local module
        for module in "${modules[@]}"; do
            if pkg-config --exists "${module}"; then
                print_ok "pkg-config Modul gefunden: ${module}"
            else
                missing_items+=("pkg-config Modul fehlt: ${module}")
            fi
        done
    fi

    if ((${#missing_items[@]} > 0)); then
        print_error "Es fehlen Build-Voraussetzungen."
        printf "%s\n" "${missing_items[@]}" >&2
        printf "\n%sVorschlag für Debian/Ubuntu:%s\n" "${COLOR_YELLOW}" "${COLOR_RESET}" >&2
        printf "sudo apt update && sudo apt install -y %s\n" "${APT_PACKAGES[*]}" >&2
        exit 1
    fi

    print_ok "Alle benötigten Build-Voraussetzungen sind vorhanden."
}

clone_repository() {
    print_header "Repository klonen"
    mkdir -p "${WORK_ROOT}"
    rm -rf "${SOURCE_DIR}" "${BUILD_DIR}"

    if [[ -n "${REPO_BRANCH}" ]]; then
        print_info "Klonen von ${REPO_URL} (Branch: ${REPO_BRANCH})"
        git clone --depth 1 --branch "${REPO_BRANCH}" "${REPO_URL}" "${SOURCE_DIR}"
    else
        print_info "Klonen von ${REPO_URL}"
        git clone --depth 1 "${REPO_URL}" "${SOURCE_DIR}"
    fi
}

build_project() {
    print_header "Projekt bauen"
    cmake -S "${SOURCE_DIR}" -B "${BUILD_DIR}"
    cmake --build "${BUILD_DIR}"

    if [[ ! -x "${APP_BINARY}" ]]; then
        print_error "Build fehlgeschlagen: ${APP_BINARY} wurde nicht erzeugt."
        exit 1
    fi

    if [[ ! -f "${LOCAL_LIB_DIR}/libqpdf.so.30" || ! -f "${LOCAL_LIB_DIR}/libpoppler-qt6.so.3" ]]; then
        print_error "Erwartete lokale Laufzeitbibliotheken wurden nicht gefunden."
        print_error "Geprüft wurde: ${LOCAL_LIB_DIR}"
        exit 1
    fi

    print_ok "Build erfolgreich abgeschlossen."
}

install_application() {
    print_header "Lokal installieren"

    mkdir -p "${APP_DIR}/bin" "${APP_DIR}/lib" "${BIN_DIR}" "${ICON_DIR}" "${APPLICATIONS_DIR}"

    install -m 755 "${APP_BINARY}" "${APP_DIR}/bin/PDFTool"
    cp -L "${LOCAL_LIB_DIR}/libqpdf.so.30" "${APP_DIR}/lib/libqpdf.so.30"
    ln -sf "libqpdf.so.30" "${APP_DIR}/lib/libqpdf.so"
    cp -L "${LOCAL_LIB_DIR}/libpoppler-qt6.so.3" "${APP_DIR}/lib/libpoppler-qt6.so.3"
    ln -sf "libpoppler-qt6.so.3" "${APP_DIR}/lib/libpoppler-qt6.so"
    install -m 644 "${SCRIPT_DIR}/assets/pdftool.svg" "${ICON_DIR}/pdftool.svg"

    sed "s|@APP_DIR@|${APP_DIR}|g" \
        "${SCRIPT_DIR}/linux/pdf-tool.in" > "${BIN_DIR}/pdf-tool"
    chmod 755 "${BIN_DIR}/pdf-tool"

    sed \
        -e "s|@EXEC@|${BIN_DIR}/pdf-tool|g" \
        -e "s|@ICON@|pdftool|g" \
        "${SCRIPT_DIR}/linux/pdf-tool.desktop.in" > "${APPLICATIONS_DIR}/pdf-tool.desktop"
    chmod 644 "${APPLICATIONS_DIR}/pdf-tool.desktop"

    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "${APPLICATIONS_DIR}" >/dev/null 2>&1 || true
    fi

    print_ok "PDFTool wurde lokal installiert."
}

print_summary() {
    print_header "Fertig"
    printf "%sStart:%s %s\n" "${COLOR_GREEN}" "${COLOR_RESET}" "${BIN_DIR}/pdf-tool"
    printf "%sQuellen:%s %s\n" "${COLOR_DIM}" "${COLOR_RESET}" "${SOURCE_DIR}"
    printf "%sBuild:%s %s\n" "${COLOR_DIM}" "${COLOR_RESET}" "${BUILD_DIR}"
}

main() {
    check_system_requirements
    clone_repository
    build_project
    install_application
    print_summary
}

main "$@"
