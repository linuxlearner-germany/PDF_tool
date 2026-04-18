#include "document/DocumentStateStore.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

#include "document/AnnotationModel.h"
#include "document/FormFieldModel.h"
#include "document/RedactionModel.h"

namespace
{
constexpr auto kVersionKey = "version";
constexpr auto kAnnotationsKey = "annotations";
constexpr auto kFormFieldsKey = "formFields";
constexpr auto kRedactionsKey = "redactions";
}

QString DocumentStateStore::sidecarPathForDocument(const QString &documentPath)
{
    return documentPath.isEmpty()
        ? QString()
        : QStringLiteral("%1.annotations.json").arg(documentPath);
}

QJsonObject DocumentStateStore::buildState(
    const AnnotationModel &annotationModel,
    const FormFieldModel &formFieldModel,
    const RedactionModel &redactionModel,
    int version)
{
    QJsonObject root;
    root.insert(QStringLiteral(kVersionKey), version);
    root.insert(QStringLiteral(kAnnotationsKey), annotationModel.toJson());
    root.insert(QStringLiteral(kFormFieldsKey), formFieldModel.toJson());
    root.insert(QStringLiteral(kRedactionsKey), redactionModel.toJson());
    return root;
}

bool DocumentStateStore::applyState(
    const QJsonObject &root,
    AnnotationModel &annotationModel,
    FormFieldModel &formFieldModel,
    RedactionModel &redactionModel,
    QString *errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    if (root.isEmpty()) {
        annotationModel.clear();
        formFieldModel.fromJson(QJsonArray());
        redactionModel.clear();
        return true;
    }

    if (root.contains(QStringLiteral(kVersionKey))) {
        if (!annotationModel.fromJson(root.value(QStringLiteral(kAnnotationsKey)).toArray())) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Annotationen konnten nicht aus dem Sidecar geladen werden.");
            }
            return false;
        }
        if (!formFieldModel.fromJson(root.value(QStringLiteral(kFormFieldsKey)).toArray())) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Formularfelder konnten nicht aus dem Sidecar geladen werden.");
            }
            return false;
        }
        if (!redactionModel.fromJson(root.value(QStringLiteral(kRedactionsKey)).toArray())) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Schwaerzungen konnten nicht aus dem Sidecar geladen werden.");
            }
            return false;
        }
        return true;
    }

    if (root.contains(QStringLiteral(kAnnotationsKey))) {
        annotationModel.clear();
        formFieldModel.fromJson(QJsonArray());
        redactionModel.clear();
        if (!annotationModel.fromJson(root.value(QStringLiteral(kAnnotationsKey)).toArray())) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Annotationen konnten nicht aus dem Legacy-Sidecar geladen werden.");
            }
            return false;
        }
        return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("Sidecar-Datei ist gueltig formatiert, enthaelt aber keinen unterstuetzten Zustand.");
    }
    return false;
}

bool DocumentStateStore::loadFromFile(const QString &sidecarPath, QJsonObject &root, QString *errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    root = {};
    if (sidecarPath.isEmpty()) {
        return true;
    }

    QFile file(sidecarPath);
    if (!file.exists()) {
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sidecar-Datei konnte nicht geladen werden.");
        }
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sidecar-Datei ist ungueltig.");
        }
        return false;
    }

    root = document.object();
    return true;
}

bool DocumentStateStore::saveToFile(const QString &sidecarPath, const QJsonObject &root, QString *errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    if (sidecarPath.isEmpty()) {
        return true;
    }

    QSaveFile file(sidecarPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sidecar-Datei konnte nicht geschrieben werden.");
        }
        return false;
    }

    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sidecar-Datei konnte nicht vollstaendig geschrieben werden.");
        }
        return false;
    }

    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sidecar-Datei konnte nicht atomar gespeichert werden.");
        }
        return false;
    }

    return true;
}
