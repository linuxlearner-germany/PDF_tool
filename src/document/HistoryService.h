#pragma once

#include <QJsonObject>
#include <QVector>

class HistoryService
{
public:
    void reset();
    bool canUndo() const;
    bool canRedo() const;
    void recordSnapshot(const QJsonObject &snapshot);
    bool undo(QJsonObject &state);
    bool redo(QJsonObject &state);

private:
    QVector<QJsonObject> m_snapshots;
    int m_index {-1};
};
