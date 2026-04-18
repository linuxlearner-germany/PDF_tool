#pragma once

#include <QJsonObject>
#include <QString>

class AnnotationModel;
class FormFieldModel;
class RedactionModel;

class SidecarService
{
public:
    explicit SidecarService(int version = 3);

    QString sidecarPathForDocument(const QString &documentPath) const;
    QJsonObject currentState(
        const AnnotationModel &annotationModel,
        const FormFieldModel &formFieldModel,
        const RedactionModel &redactionModel) const;
    bool load(
        const QString &documentPath,
        AnnotationModel &annotationModel,
        FormFieldModel &formFieldModel,
        RedactionModel &redactionModel,
        QString *errorMessage = nullptr) const;
    bool save(
        const QString &documentPath,
        const AnnotationModel &annotationModel,
        const FormFieldModel &formFieldModel,
        const RedactionModel &redactionModel,
        QString *errorMessage = nullptr) const;
    bool remove(const QString &documentPath, QString *errorMessage = nullptr) const;

private:
    int m_version {3};
};
