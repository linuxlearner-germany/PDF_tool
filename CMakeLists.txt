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

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport Concurrent Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_QT6 REQUIRED IMPORTED_TARGET poppler-qt6)
pkg_check_modules(JPEG_LIB REQUIRED IMPORTED_TARGET libjpeg)
pkg_check_modules(QPDF IMPORTED_TARGET libqpdf)
pkg_check_modules(ZLIB_PC REQUIRED IMPORTED_TARGET zlib)

qt_standard_project_setup()

set(PDF_TOOL_SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/MainWindowActions.cpp
    src/ui/MainWindowDialogWorkflows.cpp
    src/ui/MainWindowFileWorkflows.cpp
    src/ui/MainWindowState.cpp
    src/ui/MainWindowSupport.cpp
    src/ui/PageThumbnailListModel.cpp
    src/ui/PdfView.cpp
    src/document/AnnotationModel.cpp
    src/document/AtomicFileTransaction.cpp
    src/document/ExportService.cpp
    src/document/FormFieldModel.cpp
    src/document/HistoryService.cpp
    src/document/OcrServiceController.cpp
    src/document/DocumentStateStore.cpp
    src/document/PdfDocumentController.cpp
    src/document/PdfDocumentControllerSearch.cpp
    src/document/PdfDocumentControllerSelection.cpp
    src/document/PdfDocumentControllerState.cpp
    src/document/PageThumbnailProvider.cpp
    src/document/RedactionModel.cpp
    src/document/SearchService.cpp
    src/document/SelectionService.cpp
    src/document/SidecarService.cpp
    src/services/OcrService.cpp
    src/rendering/PopplerAdapter.cpp
    src/operations/QPdfOperations.cpp
    resources/icons.qrc
)

set(PDF_TOOL_HEADERS
    src/ui/MainWindow.h
    src/ui/MainWindowSupport.h
    src/ui/PageThumbnailListModel.h
    src/ui/PdfView.h
    src/document/AnnotationModel.h
    src/document/AtomicFileTransaction.h
    src/document/ExportService.h
    src/document/FormFieldModel.h
    src/document/HistoryService.h
    src/document/OcrServiceController.h
    src/document/DocumentStateStore.h
    src/document/PageThumbnailProvider.h
    src/document/PdfDocumentController.h
    src/document/PdfDocumentTypes.h
    src/document/PdfTextSelection.h
    src/document/RedactionModel.h
    src/document/SearchModel.h
    src/document/SearchService.h
    src/document/SelectionModel.h
    src/document/SelectionService.h
    src/document/SidecarService.h
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
        Qt6::Concurrent
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

enable_testing()

qt_add_executable(AnnotationModelTests
    tests/AnnotationModelTests.cpp
    src/document/AnnotationModel.cpp
)

target_include_directories(AnnotationModelTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(AnnotationModelTests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)

qt_add_executable(FormFieldModelTests
    tests/FormFieldModelTests.cpp
    src/document/FormFieldModel.cpp
)

target_include_directories(FormFieldModelTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(FormFieldModelTests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)

add_test(NAME AnnotationModelTests COMMAND AnnotationModelTests)
add_test(NAME FormFieldModelTests COMMAND FormFieldModelTests)

qt_add_executable(RedactionModelTests
    tests/RedactionModelTests.cpp
    src/document/RedactionModel.cpp
)

target_include_directories(RedactionModelTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(RedactionModelTests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)

qt_add_executable(SearchModelTests
    tests/SearchModelTests.cpp
)

target_include_directories(SearchModelTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(SearchModelTests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)

qt_add_executable(DocumentStateStoreTests
    tests/DocumentStateStoreTests.cpp
    src/document/AnnotationModel.cpp
    src/document/AtomicFileTransaction.cpp
    src/document/FormFieldModel.cpp
    src/document/RedactionModel.cpp
    src/document/DocumentStateStore.cpp
    src/document/SidecarService.cpp
)

target_include_directories(DocumentStateStoreTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(DocumentStateStoreTests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)

qt_add_executable(PdfDocumentControllerTests
    tests/PdfDocumentControllerTests.cpp
    src/document/AnnotationModel.cpp
    src/document/AtomicFileTransaction.cpp
    src/document/ExportService.cpp
    src/document/FormFieldModel.cpp
    src/document/HistoryService.cpp
    src/document/OcrServiceController.cpp
    src/document/RedactionModel.cpp
    src/document/DocumentStateStore.cpp
    src/document/PdfDocumentController.cpp
    src/document/PdfDocumentControllerSearch.cpp
    src/document/PdfDocumentControllerSelection.cpp
    src/document/PdfDocumentControllerState.cpp
    src/document/PageThumbnailProvider.cpp
    src/document/SearchService.cpp
    src/document/SelectionService.cpp
    src/document/SidecarService.cpp
    src/services/OcrService.cpp
    src/operations/QPdfOperations.cpp
)

target_include_directories(PdfDocumentControllerTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PdfDocumentControllerTests
    PRIVATE
        Qt6::Concurrent
        Qt6::Core
        Qt6::Gui
        Qt6::PrintSupport
        Qt6::Test
)

qt_add_executable(HistoryServiceTests
    tests/HistoryServiceTests.cpp
    src/document/HistoryService.cpp
)

target_include_directories(HistoryServiceTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(HistoryServiceTests
    PRIVATE
        Qt6::Core
        Qt6::Test
)

qt_add_executable(OcrServiceControllerTests
    tests/OcrServiceControllerTests.cpp
    src/document/OcrServiceController.cpp
    src/services/OcrService.cpp
)

target_include_directories(OcrServiceControllerTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(OcrServiceControllerTests
    PRIVATE
        Qt6::Concurrent
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)

qt_add_executable(ExportServiceTests
    tests/ExportServiceTests.cpp
    src/document/ExportService.cpp
    src/document/AtomicFileTransaction.cpp
    src/services/OcrService.cpp
    src/operations/QPdfOperations.cpp
)

qt_add_executable(SearchServiceTests
    tests/SearchServiceTests.cpp
    src/document/SearchService.cpp
)

target_include_directories(SearchServiceTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(SearchServiceTests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)

qt_add_executable(SelectionServiceTests
    tests/SelectionServiceTests.cpp
    src/document/RedactionModel.cpp
    src/document/SelectionService.cpp
)

target_include_directories(SelectionServiceTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(SelectionServiceTests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)

target_include_directories(ExportServiceTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ExportServiceTests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::PrintSupport
        Qt6::Test
)

add_test(NAME RedactionModelTests COMMAND RedactionModelTests)
add_test(NAME SearchModelTests COMMAND SearchModelTests)
add_test(NAME DocumentStateStoreTests COMMAND DocumentStateStoreTests)
add_test(NAME PdfDocumentControllerTests COMMAND PdfDocumentControllerTests)
add_test(NAME HistoryServiceTests COMMAND HistoryServiceTests)
add_test(NAME OcrServiceControllerTests COMMAND OcrServiceControllerTests)
add_test(NAME ExportServiceTests COMMAND ExportServiceTests)
add_test(NAME SearchServiceTests COMMAND SearchServiceTests)
add_test(NAME SelectionServiceTests COMMAND SelectionServiceTests)
