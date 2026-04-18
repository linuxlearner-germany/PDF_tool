#include "document/SidecarService.h"

#include "document/AnnotationModel.h"
#include "document/AtomicFileTransaction.h"
#include "document/DocumentStateStore.h"
#include "document/FormFieldModel.h"
#include "document/RedactionModel.h"

SidecarService::SidecarService(int version)
    : m_version(version)
{
}

QString SidecarService::sidecarPathForDocument(const QString &documentPath) const
{
    return DocumentStateStore::sidecarPathForDocument(documentPath);
}

QJsonObject SidecarService::currentState(
    const AnnotationModel &annotationModel,
    const FormFieldModel &formFieldModel,
    const RedactionModel &redactionModel) const
{
    return DocumentStateStore::buildState(annotationModel, formFieldModel, redactionModel, m_version);
}

bool SidecarService::load(
    const QString &documentPath,
    AnnotationModel &annotationModel,
    FormFieldModel &formFieldModel,
    RedactionModel &redactionModel,
    QString *errorMessage) const
{
    QJsonObject root;
    if (!DocumentStateStore::loadFromFile(sidecarPathForDocument(documentPath), root, errorMessage)) {
        return false;
    }

    if (root.isEmpty()) {
        return true;
    }

    return DocumentStateStore::applyState(root, annotationModel, formFieldModel, redactionModel, errorMessage);
}

bool SidecarService::save(
    const QString &documentPath,
    const AnnotationModel &annotationModel,
    const FormFieldModel &formFieldModel,
    const RedactionModel &redactionModel,
    QString *errorMessage) const
{
    return DocumentStateStore::saveToFile(
        sidecarPathForDocument(documentPath),
        currentState(annotationModel, formFieldModel, redactionModel),
        errorMessage);
}

bool SidecarService::remove(const QString &documentPath, QString *errorMessage) const
{
    return AtomicFileTransaction::removeFileIfExists(sidecarPathForDocument(documentPath), errorMessage);
}
