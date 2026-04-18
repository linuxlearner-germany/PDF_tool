#include "ui/MainWindowSupport.h"

#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

#include <poppler-qt6.h>

namespace MainWindowSupport
{
QIcon loadToolbarIcon(const QString &name)
{
    return QIcon(QStringLiteral(":/icons/%1.svg").arg(name));
}

QStringList supportedPdfFontFamilies()
{
    return {
        QStringLiteral("Helvetica"),
        QStringLiteral("Times New Roman"),
        QStringLiteral("Courier"),
    };
}

QString normalizedPdfFontFamily(const QString &fontFamily)
{
    const QString family = fontFamily.trimmed();
    if (family.contains(QStringLiteral("courier"), Qt::CaseInsensitive)) {
        return QStringLiteral("Courier");
    }
    if (family.contains(QStringLiteral("times"), Qt::CaseInsensitive)) {
        return QStringLiteral("Times New Roman");
    }
    return QStringLiteral("Helvetica");
}

void updateColorButton(QPushButton *button, const QColor &color)
{
    const QString label = QStringLiteral("%1 (%2)").arg(color.name(QColor::HexRgb), color.alpha());
    button->setText(label);
    button->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: rgba(%1, %2, %3, %4); color: %5; border: 1px solid #666; padding: 4px 8px; }")
                              .arg(color.red())
                              .arg(color.green())
                              .arg(color.blue())
                              .arg(color.alpha())
                              .arg(color.lightnessF() < 0.45 ? QStringLiteral("#ffffff") : QStringLiteral("#000000")));
}

TextAnnotationDialogResult promptForTextAnnotation(
    QWidget *parent,
    const QString &title,
    const QString &label,
    const QString &initialText,
    const PdfTextStyle &initialStyle,
    const QColor &initialBackground)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(520, 420);

    auto *layout = new QVBoxLayout(&dialog);
    auto *formLayout = new QFormLayout();
    layout->addLayout(formLayout);

    auto *textEdit = new QTextEdit(&dialog);
    textEdit->setPlainText(initialText);
    formLayout->addRow(label, textEdit);

    auto *fontFamilyBox = new QComboBox(&dialog);
    fontFamilyBox->addItems(supportedPdfFontFamilies());
    const QString initialFamily = normalizedPdfFontFamily(initialStyle.fontFamily);
    const int initialFamilyIndex = fontFamilyBox->findText(initialFamily);
    fontFamilyBox->setCurrentIndex(initialFamilyIndex >= 0 ? initialFamilyIndex : 0);
    formLayout->addRow(QStringLiteral("Schriftart:"), fontFamilyBox);

    auto *fontSizeSpin = new QSpinBox(&dialog);
    fontSizeSpin->setRange(6, 96);
    fontSizeSpin->setValue(qRound(initialStyle.fontSize > 0.0 ? initialStyle.fontSize : 12.0));
    formLayout->addRow(QStringLiteral("Schriftgroesse:"), fontSizeSpin);

    QColor textColor = initialStyle.textColor.isValid() ? initialStyle.textColor : QColor(Qt::black);
    QColor backgroundColor = initialBackground.isValid() ? initialBackground : QColor(255, 255, 255, 230);

    auto *textColorButton = new QPushButton(&dialog);
    updateColorButton(textColorButton, textColor);
    QObject::connect(textColorButton, &QPushButton::clicked, &dialog, [&dialog, textColorButton, &textColor]() {
        const QColor selected = QColorDialog::getColor(textColor, &dialog, QStringLiteral("Schriftfarbe waehlen"), QColorDialog::ShowAlphaChannel);
        if (!selected.isValid()) {
            return;
        }
        textColor = selected;
        updateColorButton(textColorButton, textColor);
    });
    formLayout->addRow(QStringLiteral("Schriftfarbe:"), textColorButton);

    auto *backgroundColorButton = new QPushButton(&dialog);
    updateColorButton(backgroundColorButton, backgroundColor);
    QObject::connect(backgroundColorButton, &QPushButton::clicked, &dialog, [&dialog, backgroundColorButton, &backgroundColor]() {
        const QColor selected = QColorDialog::getColor(
            backgroundColor,
            &dialog,
            QStringLiteral("Hintergrundfarbe waehlen"),
            QColorDialog::ShowAlphaChannel);
        if (!selected.isValid()) {
            return;
        }
        backgroundColor = selected;
        updateColorButton(backgroundColorButton, backgroundColor);
    });
    formLayout->addRow(QStringLiteral("Hintergrund:"), backgroundColorButton);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    TextAnnotationDialogResult result;
    result.accepted = (dialog.exec() == QDialog::Accepted);
    if (!result.accepted) {
        return result;
    }

    result.text = textEdit->toPlainText().trimmed();
    result.style.textColor = textColor;
    result.style.fontFamily = normalizedPdfFontFamily(fontFamilyBox->currentText());
    result.style.fontSize = fontSizeSpin->value();
    result.backgroundColor = backgroundColor;
    return result;
}

TextStyleDialogResult promptForTextStyle(
    QWidget *parent,
    const QString &title,
    const PdfTextStyle &initialStyle)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);

    auto *layout = new QVBoxLayout(&dialog);
    auto *formLayout = new QFormLayout();
    layout->addLayout(formLayout);

    auto *fontFamilyBox = new QComboBox(&dialog);
    fontFamilyBox->addItems(supportedPdfFontFamilies());
    const QString initialFamily = normalizedPdfFontFamily(initialStyle.fontFamily);
    const int initialFamilyIndex = fontFamilyBox->findText(initialFamily);
    fontFamilyBox->setCurrentIndex(initialFamilyIndex >= 0 ? initialFamilyIndex : 0);
    formLayout->addRow(QStringLiteral("Schriftart:"), fontFamilyBox);

    auto *fontSizeSpin = new QSpinBox(&dialog);
    fontSizeSpin->setRange(6, 96);
    fontSizeSpin->setValue(qRound(initialStyle.fontSize > 0.0 ? initialStyle.fontSize : 12.0));
    formLayout->addRow(QStringLiteral("Schriftgroesse:"), fontSizeSpin);

    QColor textColor = initialStyle.textColor.isValid() ? initialStyle.textColor : QColor(Qt::black);
    auto *textColorButton = new QPushButton(&dialog);
    updateColorButton(textColorButton, textColor);
    QObject::connect(textColorButton, &QPushButton::clicked, &dialog, [&dialog, textColorButton, &textColor]() {
        const QColor selected = QColorDialog::getColor(textColor, &dialog, QStringLiteral("Schriftfarbe waehlen"), QColorDialog::ShowAlphaChannel);
        if (!selected.isValid()) {
            return;
        }
        textColor = selected;
        updateColorButton(textColorButton, textColor);
    });
    formLayout->addRow(QStringLiteral("Schriftfarbe:"), textColorButton);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    TextStyleDialogResult result;
    result.accepted = (dialog.exec() == QDialog::Accepted);
    if (!result.accepted) {
        return result;
    }

    result.style.fontFamily = normalizedPdfFontFamily(fontFamilyBox->currentText());
    result.style.fontSize = fontSizeSpin->value();
    result.style.textColor = textColor;
    return result;
}

PasswordDialogResult promptForPasswords(
    QWidget *parent,
    const QString &title,
    const QString &description,
    bool askOwnerPassword,
    bool askUserPassword,
    const QString &ownerLabel,
    const QString &userLabel)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(description, &dialog));

    auto *formLayout = new QFormLayout();
    layout->addLayout(formLayout);

    QLineEdit *ownerEdit = nullptr;
    QLineEdit *userEdit = nullptr;

    if (askOwnerPassword) {
        ownerEdit = new QLineEdit(&dialog);
        ownerEdit->setEchoMode(QLineEdit::Password);
        formLayout->addRow(ownerLabel, ownerEdit);
    }

    if (askUserPassword) {
        userEdit = new QLineEdit(&dialog);
        userEdit->setEchoMode(QLineEdit::Password);
        formLayout->addRow(userLabel, userEdit);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    PasswordDialogResult result;
    result.accepted = (dialog.exec() == QDialog::Accepted);
    if (result.accepted) {
        if (ownerEdit) {
            result.ownerPassword = ownerEdit->text();
        }
        if (userEdit) {
            result.userPassword = userEdit->text();
        }
    }
    return result;
}

bool cryptographicSigningAvailable()
{
    return !Poppler::availableCryptoSignBackends().isEmpty();
}
}
