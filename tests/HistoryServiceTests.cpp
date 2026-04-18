#include <QtTest>

#include "document/HistoryService.h"

class HistoryServiceTests : public QObject
{
    Q_OBJECT

private slots:
    void undoRedoInvariantsHoldAcrossBranchingHistory();
};

void HistoryServiceTests::undoRedoInvariantsHoldAcrossBranchingHistory()
{
    HistoryService history;
    QJsonObject state;

    history.recordSnapshot(QJsonObject {{"value", 1}});
    history.recordSnapshot(QJsonObject {{"value", 2}});
    history.recordSnapshot(QJsonObject {{"value", 3}});

    QVERIFY(history.canUndo());
    QVERIFY(!history.canRedo());

    QVERIFY(history.undo(state));
    QCOMPARE(state.value(QStringLiteral("value")).toInt(), 2);
    QVERIFY(history.canUndo());
    QVERIFY(history.canRedo());

    QVERIFY(history.undo(state));
    QCOMPARE(state.value(QStringLiteral("value")).toInt(), 1);
    QVERIFY(!history.canUndo());
    QVERIFY(history.canRedo());

    history.recordSnapshot(QJsonObject {{"value", 7}});
    QVERIFY(history.canUndo());
    QVERIFY(!history.canRedo());

    QVERIFY(history.undo(state));
    QCOMPARE(state.value(QStringLiteral("value")).toInt(), 1);
    QVERIFY(history.redo(state));
    QCOMPARE(state.value(QStringLiteral("value")).toInt(), 7);
}

QTEST_MAIN(HistoryServiceTests)

#include "HistoryServiceTests.moc"
