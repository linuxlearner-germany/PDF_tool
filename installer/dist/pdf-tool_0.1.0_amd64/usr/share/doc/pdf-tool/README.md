# 🧩 PDFTool

![PDFTool Screenshot](images/Screenshot_with_dokument.png)

<p align="center">
  <b>Local-first PDF editor for Linux – OCR, Annotation, Redaction</b>
</p>

<p align="center">
  <img src="https://skillicons.dev/icons?i=cpp,qt,cmake,linux,bash,git" />
</p>

---

## 🚀 Overview

PDFTool is a Linux desktop application designed for working with PDF files in a **local-first workflow**.

Unlike simple viewers, it focuses on real document workflows such as annotation, OCR, redaction and export — all processed locally without cloud dependency.

This project is built as a **practical C++/Qt desktop application**, using modern tooling and a structured architecture.

---

## 🎯 Project Goals

- Provide a complete PDF workflow on Linux  
- Enable annotation and document interaction  
- Support OCR-based text extraction  
- Offer redaction and export functionality  
- Keep all processing local (privacy-first)  
- Build a maintainable and scalable C++/Qt codebase  
- Apply real-world desktop software engineering practices  

---

## ⚙️ Tech Stack

<p>
  <img src="https://skillicons.dev/icons?i=cpp,qt,cmake,linux,bash,git" />
</p>

- **C++** – Core application logic  
- **Qt 6** – GUI framework  
- **CMake** – Build system  
- **Linux (Debian)** – Target platform  
- **Poppler** – PDF rendering  
- **Tesseract** – OCR engine  
- **qpdf** – PDF processing  

---

## ✨ Capabilities

- 📄 Open and navigate PDF documents  
- 🔎 Search within documents  
- ✏️ Annotation and editing workflows  
- 🔍 OCR (via Tesseract)  
- 🔒 Redaction / black-out functionality  
- 📤 Export processed documents  
- 🖥️ Native Linux desktop integration  
- 📦 Debian package distribution (.deb)  

---

## 📸 Screenshots

### 📄 With document loaded

![With Document](images/Screenshot_with_dokument.png)

---

### 🧩 Empty workspace

![Without Document](images/Screenshot_without_dokument.png)

---

## 🧠 Key Features

### 📖 PDF Viewing & Navigation
- Load and display PDF documents  
- Navigate pages efficiently  
- Desktop-focused UI workflow  

### ✏️ Annotation & Editing
- Interactive document tools  
- GUI-based editing workflows  
- Visual document interaction  

### 🔍 OCR Support
- Powered by **Tesseract**  
- Extract text from scanned PDFs  
- Page and selection-based OCR  

### 🔒 Redaction
- Hide sensitive data  
- Black-out areas in documents  
- Export redacted versions  

### 🔐 Local-First Design
- No cloud dependency  
- Full control over data  
- Privacy-focused workflow  

---

## 🏗️ Architecture

### Core Components

- **Qt 6** → UI Layer  
- **Poppler** → PDF rendering  
- **Tesseract** → OCR  
- **qpdf** → PDF processing  

---

### Workflow

---

### Layers

1. **UI Layer** – user interaction  
2. **Document Layer** – PDF handling  
3. **Processing Layer** – OCR, editing, redaction  
4. **Output Layer** – export  

---

## 🛠️ Development Environment

- **IDE:** CLion (JetBrains)  
- **Build System:** CMake  
- **Platform:** Linux (Debian)  
- **Language:** C++  
- **Framework:** Qt 6  

CLion is used for:

- CMake integration  
- Debugging  
- Refactoring  
- Managing larger C++ projects  

---

## ⚙️ Requirements

- Linux (Debian recommended)  
- Qt 6  
- Poppler  
- CMake  
- C++ compiler  

Optional:

- Tesseract  
- qpdf  

---

## 📦 Installation (.deb)

```bash
sudo dpkg -i dist/pdf-tool_0.1.0_amd64.deb
sudo apt-get install -f
