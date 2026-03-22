# 🧩 PDFTool

<p align="center">
  <img src="images/Screenshot_with_dokument.png" width="1000"/>
</p>

Linux desktop application for viewing, annotating, redacting and exporting PDF files.

Built with **Qt 6**, **Poppler**, and optional **Tesseract / qpdf**.

---

## 🚀 Overview

PDFTool is a local-first PDF editor designed for Linux desktop workflows.

It focuses on:

- 📄 Reading and navigating PDFs  
- ✏️ Annotation and editing  
- 🔒 Redaction (data masking)  
- 🔍 OCR (text extraction)  
- 📤 Exporting modified documents  

👉 All processing happens locally – no cloud dependency.

---

## 📸 Screenshots

### 📄 PDF loaded

Interface showing document navigation, editing tools and OCR features.

<p align="center">
  <img src="images/Screenshot_with_dokument.png" width="900"/>
</p>

---

### 🧩 Empty workspace

Clean workspace before loading a document.

<p align="center">
  <img src="images/Screenshot_without_dokument.png" width="900"/>
</p>

---

## 🧠 Features

### 📖 Viewer & Navigation
- Fast PDF rendering via Poppler  
- Zoom & scroll support  
- Page thumbnails & bookmarks  
- Search with highlighting  

---

### ✏️ Editing & Annotation
- Text selection & copy  
- Highlight annotations  
- Rectangle annotations  
- Free text notes  
- Visual text replacement  

---

### 🔒 Redaction
- Mark areas for redaction  
- Black overlay masking  
- Export as permanently redacted PDF  

---

### 🔍 OCR (Optional)
- Page-level OCR  
- Selection-based OCR  
- Powered by Tesseract  
- Export searchable PDFs  

---

### 📦 Additional Features
- Merge PDFs  
- Split PDFs  
- Handle encrypted PDFs  
- Export modified documents  

---

## 🏗️ Architecture

### Core Components

- **Qt 6** → UI Layer  
- **Poppler** → PDF rendering  
- **Tesseract** → OCR engine  
- **qpdf** → PDF processing  

---

### Workflow

User → Qt UI → Processing Layer → Export Pipeline  

- UI handles interaction  
- Poppler renders PDF  
- Editing is handled via overlays  
- Export writes changes into a new PDF  

---

## ⚙️ Requirements

- Linux (Debian recommended)  
- Qt 6  
- Poppler  
- (Optional) Tesseract  
- (Optional) qpdf  

---

## 🚀 Build

```bash
git clone https://github.com/linuxlearner-germany/PDF_tool
cd PDF_tool

mkdir build
cd build

cmake ..
make
