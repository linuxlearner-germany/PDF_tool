#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build-local"
APP_BINARY="${BUILD_DIR}/PDFTool"
LOCAL_LIB_DIR="${PROJECT_ROOT}/.localdeps/sysroot/usr/lib/x86_64-linux-gnu"

VERSION="$(grep -F 'project(PDFTool VERSION ' "${PROJECT_ROOT}/CMakeLists.txt" | head -n 1 | awk '{print $3}')"
ARCH="$(dpkg --print-architecture)"
PACKAGE_NAME="pdf-tool"
STAGE_DIR="${SCRIPT_DIR}/dist/${PACKAGE_NAME}_${VERSION}_${ARCH}"
OUTPUT_DEB="${SCRIPT_DIR}/dist/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"

mkdir -p "${SCRIPT_DIR}/dist"
rm -rf "${STAGE_DIR}" "${OUTPUT_DEB}"

if [[ ! -x "${APP_BINARY}" ]]; then
    echo "Build fehlt, versuche build-local zu bauen..."
    cmake --build "${BUILD_DIR}"
fi

if [[ ! -x "${APP_BINARY}" ]]; then
    echo "Fehler: ${APP_BINARY} wurde nicht gefunden." >&2
    exit 1
fi

mkdir -p \
    "${STAGE_DIR}/DEBIAN" \
    "${STAGE_DIR}/opt/pdftool/bin" \
    "${STAGE_DIR}/opt/pdftool/lib" \
    "${STAGE_DIR}/usr/bin" \
    "${STAGE_DIR}/usr/share/applications" \
    "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps" \
    "${STAGE_DIR}/usr/share/doc/${PACKAGE_NAME}"

install -m 755 "${APP_BINARY}" "${STAGE_DIR}/opt/pdftool/bin/PDFTool"
cp -L "${LOCAL_LIB_DIR}/libqpdf.so.30" "${STAGE_DIR}/opt/pdftool/lib/libqpdf.so.30"
ln -sf "libqpdf.so.30" "${STAGE_DIR}/opt/pdftool/lib/libqpdf.so"
cp -L "${LOCAL_LIB_DIR}/libpoppler-qt6.so.3" "${STAGE_DIR}/opt/pdftool/lib/libpoppler-qt6.so.3"
ln -sf "libpoppler-qt6.so.3" "${STAGE_DIR}/opt/pdftool/lib/libpoppler-qt6.so"
install -m 644 "${SCRIPT_DIR}/assets/pdftool.svg" "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/pdftool.svg"
install -m 644 "${PROJECT_ROOT}/README.md" "${STAGE_DIR}/usr/share/doc/${PACKAGE_NAME}/README.md"

sed "s|@APP_DIR@|/opt/pdftool|g" \
    "${SCRIPT_DIR}/linux/pdf-tool.in" > "${STAGE_DIR}/usr/bin/pdf-tool"
chmod 755 "${STAGE_DIR}/usr/bin/pdf-tool"

sed \
    -e 's|@EXEC@|/usr/bin/pdf-tool|g' \
    -e 's|@ICON@|pdftool|g' \
    "${SCRIPT_DIR}/linux/pdf-tool.desktop.in" > "${STAGE_DIR}/usr/share/applications/pdf-tool.desktop"
chmod 644 "${STAGE_DIR}/usr/share/applications/pdf-tool.desktop"

cat > "${STAGE_DIR}/DEBIAN/control" <<EOF
Package: ${PACKAGE_NAME}
Version: ${VERSION}
Section: utils
Priority: optional
Architecture: ${ARCH}
Maintainer: Paul <unknown@example.com>
Depends: libc6, libstdc++6, libqt6core6, libqt6gui6, libqt6widgets6, libqt6printsupport6, qt6-qpa-plugins, libjpeg62-turbo, zlib1g
Description: Desktop PDF-Werkzeug auf Basis von Qt 6
 PDFTool bietet Anzeigen, Suchen, Annotationen, OCR, Signaturen,
 Formularbearbeitung und PDF-Export in einer Desktop-Anwendung.
EOF

cat > "${STAGE_DIR}/DEBIAN/postinst" <<'EOF'
#!/usr/bin/env bash
set -e
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database /usr/share/applications >/dev/null 2>&1 || true
fi
exit 0
EOF
chmod 755 "${STAGE_DIR}/DEBIAN/postinst"

fakeroot dpkg-deb --build "${STAGE_DIR}" "${OUTPUT_DEB}" >/dev/null
echo "Debian-Paket erstellt: ${OUTPUT_DEB}"
