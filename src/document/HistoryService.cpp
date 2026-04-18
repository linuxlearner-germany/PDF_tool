#include "document/HistoryService.h"

void HistoryService::reset()
{
    m_snapshots.clear();
    m_index = -1;
}

bool HistoryService::canUndo() const
{
    return m_index > 0 && m_index < m_snapshots.size();
}

bool HistoryService::canRedo() const
{
    return m_index >= 0 && m_index < (m_snapshots.size() - 1);
}

void HistoryService::recordSnapshot(const QJsonObject &snapshot)
{
    if (m_index >= 0 && m_index < m_snapshots.size() && m_snapshots.at(m_index) == snapshot) {
        return;
    }

    while (m_snapshots.size() > m_index + 1) {
        m_snapshots.removeLast();
    }

    m_snapshots.append(snapshot);
    m_index = m_snapshots.size() - 1;
}

bool HistoryService::undo(QJsonObject &state)
{
    if (!canUndo()) {
        return false;
    }

    --m_index;
    state = m_snapshots.at(m_index);
    return true;
}

bool HistoryService::redo(QJsonObject &state)
{
    if (!canRedo()) {
        return false;
    }

    ++m_index;
    state = m_snapshots.at(m_index);
    return true;
}
