#pragma once

#include <QJsonObject>
#include <QString>

class AnnotationModel;
class FormFieldModel;
class RedactionModel;

class DocumentStateStore
{
public:
    static QString sidecarPathForDocument(const QString &documentPath);
    static QJsonObject buildState(
        const AnnotationModel &annotationModel,
        const FormFieldModel &formFieldModel,
        const RedactionModel &redactionModel,
        int version);
    static bool applyState(
        const QJsonObject &root,
        AnnotationModel &annotationModel,
        FormFieldModel &formFieldModel,
        RedactionModel &redactionModel,
        QString *errorMessage = nullptr);
    static bool loadFromFile(const QString &sidecarPath, QJsonObject &root, QString *errorMessage = nullptr);
    static bool saveToFile(const QString &sidecarPath, const QJsonObject &root, QString *errorMessage = nullptr);
};
