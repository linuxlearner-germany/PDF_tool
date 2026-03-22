# 🧩 PDFTool

![PDFTool Screenshot](images/Screenshot_with_dokument.png)

**Local-first PDF editor for Linux – built for viewing, annotating, redacting and exporting PDF documents with OCR support.**

---

## 🚀 Overview

PDFTool is a Linux desktop application for working with PDF files in a practical, local-first workflow.

The goal of this project is to provide a PDF tool that is not limited to simple viewing, but also supports editing-related workflows such as annotations, OCR, redaction and export. The application is designed for users who want to keep full control over their files and process documents locally instead of relying on cloud-based services.

This project is especially focused on Linux desktop environments and is being developed as a real-world software project using modern C++ tooling and a CMake-based workflow.

---

## 🎯 Project Goals

PDFTool is being built with the following goals in mind:

- Provide a usable PDF workflow directly on Linux
- Support annotation and document interaction
- Enable OCR-based text extraction
- Offer redaction and export functionality
- Keep all processing local on the user’s machine
- Create a maintainable and extendable C++/Qt codebase
- Learn and apply practical desktop software engineering concepts

---

## ✨ Main Capabilities

The current direction of the project includes support for:

- 📄 Opening and navigating PDF files
- 🔎 Searching within PDF documents
- ✏️ Annotation and editing-related workflows
- 📝 Text-related interaction with documents
- 🔒 Redaction / black-out workflows
- 🔍 OCR support for extracting text from pages or selections
- 📤 Exporting processed documents
- 🖥️ Linux desktop integration
- 📦 Debian package distribution via `.deb`

---

## 📸 Screenshot

### 🧩 Empty workspace

This screenshot shows the clean default UI before a document is loaded.

![Without Document](images/Screenshot_without_dokument.png)

---

## 🧠 Key Features

### PDF Viewing & Navigation
- Load and display PDF documents
- Navigate through document pages
- Work in a desktop-oriented interface
- Focus on usability for Linux users

### Annotation & Editing Workflow
- Tools for interacting with PDF content
- Editing-oriented actions through the GUI
- Visual workflow for document processing

### OCR Support
- OCR functionality via **Tesseract**
- Intended for extracting text from PDF content
- Useful for scanned or image-based documents

### Redaction
- Black-out / redaction workflow for hiding sensitive information
- Export-oriented processing after redaction

### Local-First Processing
- No cloud dependency
- Files stay on the user’s system
- Better privacy and full local control

### Linux Packaging
- Project includes a Debian package build/output
- Makes installation easier on Debian-based systems

---

## 🏗️ Architecture

PDFTool is designed around a desktop application architecture with a clear separation between UI, PDF rendering and document-processing components.

### Core Components

- **Qt 6** → User interface and desktop application framework  
- **Poppler** → PDF rendering and document handling  
- **Tesseract** → OCR engine for text extraction  
- **qpdf** → Additional PDF processing support  

### High-Level Workflow

User → Qt UI → PDF Processing Layer → Export / Output

### Internal Design Idea

The application follows a practical layered approach:

1. **User Interface Layer**  
   Handles windows, controls, actions, menus and interaction.

2. **Document Layer**  
   Responsible for loading, displaying and navigating PDF files.

3. **Processing Layer**  
   Handles OCR, redaction, editing-related features and export workflows.

4. **Output Layer**  
   Produces saved/exported results after processing.

This structure helps keep the project modular and easier to extend in the future.

---

## 🛠️ Development Environment

This project is primarily developed in a Linux environment and uses professional C++ development tooling.

### Main Development Setup

- **IDE:** CLion (JetBrains)
- **Build system:** CMake
- **Platform:** Linux / Debian
- **Language:** C++
- **Framework:** Qt 6

### Why CLion

PDFTool is developed with **CLion**, which is used as the primary IDE for the project.

CLion is especially useful here because it provides:

- Integrated **CMake** support
- Good C++ code navigation
- Debugging tools
- Refactoring support
- Static analysis support
- Efficient management of a multi-file C++ project

Using CLion also reflects that this project is developed with a more professional desktop/software-engineering workflow rather than as a minimal experiment.

---

## ⚙️ Requirements

To build or run PDFTool from source, the following components are relevant:

- Linux (recommended: Debian-based distribution)
- Qt 6
- Poppler
- CMake
- C++ compiler
- Optional:
  - Tesseract
  - qpdf

Depending on your system, package names may vary.

---

## 📦 Installation (.deb)

A Debian package is included/provided for easier installation on Debian-based Linux systems.

### Install via `.deb`

```bash
sudo dpkg -i dist/pdf-tool_0.1.0_amd64.deb
sudo apt-get install -f
