#pragma once

#include <QRectF>
#include <QString>
#include <QtGlobal>
#include <QVector>

struct PdfTextSelectionFragment
{
    QRectF pageRect;
    QString text;
    bool hasTrailingSpace {false};
};

struct PdfTextSelection
{
    QVector<PdfTextSelectionFragment> fragments;

    bool isEmpty() const
    {
        return fragments.isEmpty();
    }

    QString toPlainText() const
    {
        QString result;
        double previousCenterY = 0.0;
        double previousHeight = 0.0;
        bool previousHadTrailingSpace = false;
        bool hasPreviousFragment = false;

        for (const PdfTextSelectionFragment &fragment : fragments) {
            if (fragment.text.isEmpty()) {
                continue;
            }

            if (hasPreviousFragment) {
                const double lineThreshold = qMax(2.0, previousHeight * 0.7);
                const double lineDelta = qAbs(fragment.pageRect.center().y() - previousCenterY);

                if (lineDelta > lineThreshold) {
                    if (!result.endsWith(QLatin1Char('\n'))) {
                        result.append(QLatin1Char('\n'));
                    }
                } else if (previousHadTrailingSpace
                           && !result.endsWith(QLatin1Char(' '))
                           && !result.endsWith(QLatin1Char('\n'))) {
                    result.append(QLatin1Char(' '));
                }
            }

            result.append(fragment.text);

            previousCenterY = fragment.pageRect.center().y();
            previousHeight = fragment.pageRect.height();
            previousHadTrailingSpace = fragment.hasTrailingSpace;
            hasPreviousFragment = true;
        }

        return result.trimmed();
    }
};
