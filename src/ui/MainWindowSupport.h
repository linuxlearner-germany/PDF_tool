#pragma once

#include <QColor>
#include <QIcon>
#include <QString>
#include <QStringList>

#include "document/PdfDocumentTypes.h"

class QPushButton;
class QWidget;

namespace MainWindowSupport
{
struct PasswordDialogResult
{
    QString ownerPassword;
    QString userPassword;
    bool accepted {false};
};

struct TextAnnotationDialogResult
{
    QString text;
    PdfTextStyle style;
    QColor backgroundColor {255, 255, 255, 230};
    bool accepted {false};
};

struct TextStyleDialogResult
{
    PdfTextStyle style;
    bool accepted {false};
};

QIcon loadToolbarIcon(const QString &name);
QStringList supportedPdfFontFamilies();
QString normalizedPdfFontFamily(const QString &fontFamily);
void updateColorButton(QPushButton *button, const QColor &color);
TextAnnotationDialogResult promptForTextAnnotation(
    QWidget *parent,
    const QString &title,
    const QString &label,
    const QString &initialText = QString(),
    const PdfTextStyle &initialStyle = PdfTextStyle(),
    const QColor &initialBackground = QColor(255, 255, 255, 230));
TextStyleDialogResult promptForTextStyle(
    QWidget *parent,
    const QString &title,
    const PdfTextStyle &initialStyle = PdfTextStyle());
PasswordDialogResult promptForPasswords(
    QWidget *parent,
    const QString &title,
    const QString &description,
    bool askOwnerPassword,
    bool askUserPassword,
    const QString &ownerLabel,
    const QString &userLabel);
bool cryptographicSigningAvailable();
}
