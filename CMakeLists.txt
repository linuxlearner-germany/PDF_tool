cmake_minimum_required(VERSION 3.21)

project(PDFTool VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(PDF_TOOL_LOCAL_PKGCONFIG_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/.localdeps/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig")

if(EXISTS "${PDF_TOOL_LOCAL_PKGCONFIG_DIR}")
    if(DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
        set(ENV{PKG_CONFIG_PATH} "${PDF_TOOL_LOCAL_PKGCONFIG_DIR}:$ENV{PKG_CONFIG_PATH}")
    else()
        set(ENV{PKG_CONFIG_PATH} "${PDF_TOOL_LOCAL_PKGCONFIG_DIR}")
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_QT6 REQUIRED IMPORTED_TARGET poppler-qt6)
pkg_check_modules(JPEG_LIB REQUIRED IMPORTED_TARGET libjpeg)
pkg_check_modules(QPDF IMPORTED_TARGET libqpdf)
pkg_check_modules(ZLIB_PC REQUIRED IMPORTED_TARGET zlib)

qt_standard_project_setup()

set(PDF_TOOL_SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/PageThumbnailListModel.cpp
    src/ui/PdfView.cpp
    src/document/AnnotationModel.cpp
    src/document/FormFieldModel.cpp
    src/document/PdfDocumentController.cpp
    src/document/PageThumbnailProvider.cpp
    src/document/RedactionModel.cpp
    src/services/OcrService.cpp
    src/rendering/PopplerAdapter.cpp
    src/operations/QPdfOperations.cpp
    resources/icons.qrc
)

set(PDF_TOOL_HEADERS
    src/ui/MainWindow.h
    src/ui/PageThumbnailListModel.h
    src/ui/PdfView.h
    src/document/AnnotationModel.h
    src/document/FormFieldModel.h
    src/document/PageThumbnailProvider.h
    src/document/PdfDocumentController.h
    src/document/PdfDocumentTypes.h
    src/document/PdfTextSelection.h
    src/document/RedactionModel.h
    src/document/SearchModel.h
    src/document/SelectionModel.h
    src/services/OcrService.h
    src/rendering/PdfRenderEngine.h
    src/rendering/PopplerAdapter.h
    src/operations/QPdfOperations.h
)

qt_add_executable(PDFTool
    ${PDF_TOOL_SOURCES}
    ${PDF_TOOL_HEADERS}
)

target_include_directories(PDFTool
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_directories(PDFTool
    PRIVATE
        ${POPPLER_QT6_LIBRARY_DIRS}
)

target_link_libraries(PDFTool
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::PrintSupport
        Qt6::Widgets
        PkgConfig::POPPLER_QT6
)

if(QPDF_FOUND)
    target_include_directories(PDFTool PRIVATE ${QPDF_INCLUDE_DIRS})
    target_link_directories(PDFTool PRIVATE ${QPDF_LIBRARY_DIRS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.localdeps/sysroot/usr/lib/x86_64-linux-gnu/libqpdf.so")
        target_link_libraries(PDFTool PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/.localdeps/sysroot/usr/lib/x86_64-linux-gnu/libqpdf.so"
            PkgConfig::JPEG_LIB
            PkgConfig::ZLIB_PC)
    else()
        target_link_libraries(PDFTool PRIVATE PkgConfig::QPDF PkgConfig::JPEG_LIB PkgConfig::ZLIB_PC)
    endif()
    target_compile_definitions(PDFTool PRIVATE PDF_TOOL_HAS_QPDF=1)
else()
    message(WARNING "libqpdf wurde nicht gefunden. Operations-Layer bleibt im MVP deaktiviert.")
endif()

target_compile_options(PDFTool
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)
