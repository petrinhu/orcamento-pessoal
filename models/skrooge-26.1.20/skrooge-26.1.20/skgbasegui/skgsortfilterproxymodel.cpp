/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a proxy model with better filter mechanism.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgsortfilterproxymodel.h"

#include <qregularexpression.h>

#include "skgobjectmodelbase.h"
#include "skgservices.h"
#include "skgtraces.h"

/**
 * This private class of SKGObjectBase
 */
class SKGSortFilterProxyModelPrivate
{
public:
    /**
     * the previous sort column
     */
    int m_previous;

    /**
     * the current sort column
     */
    int m_current;

    /**
     * to know if we are in a sub sort
     */
    bool m_insubsort;
};

SKGSortFilterProxyModel::SKGSortFilterProxyModel(QObject *iParent)
    : QSortFilterProxyModel(iParent)
    , d(new SKGSortFilterProxyModelPrivate)
{
    _SKGTRACEINFUNC(10)

    setSortCaseSensitivity(Qt::CaseInsensitive);
    setSortLocaleAware(true);
    setFilterKeyColumn(0);

    d->m_previous = -1;
    d->m_current = -1;
    d->m_insubsort = false;
}

SKGSortFilterProxyModel::~SKGSortFilterProxyModel()
{
    _SKGTRACEINFUNC(10)
    delete d;
}

int SKGSortFilterProxyModel::getPreviousSortColumn() const
{
    return d->m_previous;
}

void SKGSortFilterProxyModel::setPreviousSortColumn(int iCol)
{
    if (d->m_previous != iCol) {
        d->m_previous = iCol;
        Q_EMIT previousSortColumnModified();
    }
}

bool SKGSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    // Get source
    auto *model = qobject_cast<SKGObjectModelBase *>(this->sourceModel());
    if ((model != nullptr) && (d != nullptr)) {
        // Set previous column
        if (!d->m_insubsort) {
            int newCol = left.column();
            if (newCol != d->m_current && d->m_current != -1) {
                d->m_previous = d->m_current;
            }

            d->m_current = newCol;
        }

        // Special sort order
        if (d->m_current != -1) {
            auto columname = model->getAttribute(d->m_current);
            if (columname.startsWith(QStringLiteral("f_BALANCE"))) {
                auto d_date_index = model->getIndexAttribute(QStringLiteral("d_date"));
                if (d_date_index != -1) {
                    d->m_current = d_date_index;
                    d->m_previous = -1;
                    SKGTRACEL(5) << "SKGSortFilterProxyModel::lessThan-Special sort order on " << columname << Qt::endl;
                }
            }
        }

        // Comparison
        QVariant leftData = model->data(model->index(left.row(), d->m_current, left.parent()), Qt::UserRole);
        QVariant rightData = model->data(model->index(right.row(), d->m_current, right.parent()), Qt::UserRole);
        SKGObjectBase *leftObj = model->getObjectPointer(left);
        if (leftData == rightData && (leftObj != nullptr)) {
            if (leftObj->getTable().isEmpty() && d->m_current != 0 && !d->m_insubsort) {
                // Groups
                d->m_insubsort = true;
                bool test = SKGSortFilterProxyModel::lessThan(model->index(left.row(), 0), model->index(right.row(), 0));
                d->m_insubsort = false;
                return test;
            }
            // Compare on previous column
            if (!d->m_insubsort && d->m_previous != -1 && !d->m_insubsort) {
                d->m_insubsort = true;
                auto vl = model->index(left.row(), d->m_previous, left.parent());
                auto vr = model->index(right.row(), d->m_previous, right.parent());
                bool test = QSortFilterProxyModel::lessThan(vl, vr);
                d->m_insubsort = false;
                return test;
            }

            // Compare on ID for stability
            SKGObjectBase *rightObj = model->getObjectPointer(right);
            return ((rightObj != nullptr) && leftObj->getID() < rightObj->getID());
        }
        // For better performances, and avoid too many calls to QAbstractItemModel::data
        // return QSortFilterProxyModel::lessThan(left, right);
        return lessThan(leftData, rightData);
    }
    return false;
}

bool SKGSortFilterProxyModel::lessThan(const QVariant &iLeftData, const QVariant &iRightData) const
{
    switch (iLeftData.userType()) {
    case QMetaType::UnknownType:
        return (iRightData.userType() != QMetaType::UnknownType);
    case QMetaType::Int:
        return iLeftData.toInt() < iRightData.toInt();
    case QMetaType::UInt:
        return iLeftData.toUInt() < iRightData.toUInt();
    case QMetaType::LongLong:
        return iLeftData.toLongLong() < iRightData.toLongLong();
    case QMetaType::ULongLong:
        return iLeftData.toULongLong() < iRightData.toULongLong();
    case QMetaType::Float:
        return iLeftData.toFloat() < iRightData.toFloat();
    case QMetaType::Double:
        return iLeftData.toDouble() < iRightData.toDouble();
    case QMetaType::QChar:
        return iLeftData.toChar() < iRightData.toChar();
    case QMetaType::QDate:
        return iLeftData.toDate() < iRightData.toDate();
    case QMetaType::QTime:
        return iLeftData.toTime() < iRightData.toTime();
    case QMetaType::QDateTime:
        return iLeftData.toDateTime() < iRightData.toDateTime();
    case QMetaType::QString:
    default:
        if (this->isSortLocaleAware()) {
            return iLeftData.toString().localeAwareCompare(iRightData.toString()) < 0;
        } else {
            return iLeftData.toString().compare(iRightData.toString(), this->sortCaseSensitivity()) < 0;
        }
    }
}

bool SKGSortFilterProxyModel::moreThan(const QVariant &iLeftData, const QVariant &iRightData) const
{
    switch (iLeftData.userType()) {
    case QMetaType::UnknownType:
        return (iRightData.userType() != QMetaType::UnknownType);
    case QMetaType::Int:
        return iLeftData.toInt() > iRightData.toInt();
    case QMetaType::UInt:
        return iLeftData.toUInt() > iRightData.toUInt();
    case QMetaType::LongLong:
        return iLeftData.toLongLong() > iRightData.toLongLong();
    case QMetaType::ULongLong:
        return iLeftData.toULongLong() > iRightData.toULongLong();
    case QMetaType::Float:
        return iLeftData.toFloat() > iRightData.toFloat();
    case QMetaType::Double:
        return iLeftData.toDouble() > iRightData.toDouble();
    case QMetaType::Char:
        return iLeftData.toChar() > iRightData.toChar();
    case QMetaType::QDate:
        return iLeftData.toDate() > iRightData.toDate();
    case QMetaType::QTime:
        return iLeftData.toTime() > iRightData.toTime();
    case QMetaType::QDateTime:
        return iLeftData.toDateTime() > iRightData.toDateTime();
    case QMetaType::QString:
    default:
        if (this->isSortLocaleAware()) {
            return iLeftData.toString().localeAwareCompare(iRightData.toString()) > 0;
        } else {
            return iLeftData.toString().compare(iRightData.toString(), this->sortCaseSensitivity()) > 0;
        }
    }
}

bool SKGSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    _SKGTRACEINFUNC(10)

    // Initialisation
    const auto &filter = property("filter").toString();
    bool output = filter.isEmpty();
    if (!output) {
        // Build list of criterias
        SKGServices::SKGSearchCriteriaList criterias = SKGServices::stringToSearchCriterias(filter);

        // Check if at least one group validate the line
        int nbList = criterias.count();
        output = false;
        for (int i = 0; i < nbList; ++i) {
            QChar mode = criterias.at(i).mode;

            bool validateAllWords = filterAcceptsRowWords(source_row, source_parent, criterias.at(i).words);
            if (mode == '+') {
                output |= validateAllWords;
            } else if (mode == '-' && validateAllWords) {
                output = false;
            }
        }

        if (!output) {
            QAbstractItemModel *model = this->sourceModel();
            if (model != nullptr) {
                QModelIndex index0 = model->index(source_row, 0, source_parent);

                int nb = model->rowCount(index0);
                for (int i = 0; !output && i < nb; ++i) {
                    output = filterAcceptsRow(i, index0);
                }
            }
        }
    }
    return output;
}

bool SKGSortFilterProxyModel::filterAcceptsRowWords(int source_row, const QModelIndex &source_parent, const QStringList &iWords) const
{
    _SKGTRACEINFUNC(10)

    // Initialisation
    bool output = true;

    // Get source
    QAbstractItemModel *model = this->sourceModel();
    if (model != nullptr) {
        int nbwords = iWords.count();
        for (int w = 0; output && w < nbwords; ++w) {
            QString word = iWords.at(w).toLower();
            QString att;
            QString op(':');
            bool modeStartWith = true;

            int pos = word.indexOf(QStringLiteral(":"));
            int pos2 = word.indexOf(QStringLiteral("<="));
            int pos3 = word.indexOf(QStringLiteral(">="));
            int pos4 = word.indexOf(QStringLiteral("="));
            int pos5 = word.indexOf(QStringLiteral("<"));
            int pos6 = word.indexOf(QStringLiteral(">"));
            int pos7 = word.indexOf(QStringLiteral("#"));
            int opLength = 1;
            if (pos2 != -1 && (pos2 < pos || pos == -1)) {
                pos = pos2;
                opLength = 2;
            }
            if (pos3 != -1 && (pos3 < pos || pos == -1)) {
                pos = pos3;
                opLength = 2;
            }
            if (pos4 != -1 && (pos4 < pos || pos == -1)) {
                pos = pos4;
            }
            if (pos5 != -1 && (pos5 < pos || pos == -1)) {
                pos = pos5;
            }
            if (pos6 != -1 && (pos6 < pos || pos == -1)) {
                pos = pos6;
            }
            if (pos7 != -1 && (pos7 < pos || pos == -1)) {
                pos = pos7;
            }

            if (pos != -1) {
                att = word.left(pos);
                if (att.endsWith(QStringLiteral("."))) {
                    modeStartWith = false;
                    att = att.left(att.size() - 1);
                }
                op = word.mid(pos, opLength);
                word = word.right(word.size() - pos - op.size());
            }

            // Is this word validated by at least one column ?
            output = false;
            int nbcol = model->columnCount();
            for (int i = 0; !output && i < nbcol; ++i) {
                QModelIndex index0 = model->index(source_row, i, source_parent);
                if (index0.isValid()) {
                    // Check if the header validates the attribute
                    if (att.isEmpty() || (modeStartWith && model->headerData(i, Qt::Horizontal).toString().startsWith(att, Qt::CaseInsensitive))
                        || (!modeStartWith && model->headerData(i, Qt::Horizontal).toString().compare(att, Qt::CaseInsensitive) == 0)) {
                        // Check if the value validate the attribute
                        if (op == QStringLiteral(":")) {
                            output = model->data(index0).toString().contains(word, Qt::CaseInsensitive);
                            if (!output) {
                                output = model->data(index0, Qt::UserRole).toString().contains(word, Qt::CaseInsensitive);
                            }
                        } else if (op == QStringLiteral("<")) {
                            QVariant var = model->data(index0, Qt::UserRole);
                            output = lessThan(var, QVariant(word));
                        } else if (op == QStringLiteral(">")) {
                            QVariant var = model->data(index0, Qt::UserRole);
                            output = moreThan(var, QVariant(word));
                        } else if (op == QStringLiteral("<=")) {
                            QVariant var = model->data(index0, Qt::UserRole);
                            output = (var == QVariant(word)) || lessThan(var, QVariant(word));
                        } else if (op == QStringLiteral(">=")) {
                            QVariant var = model->data(index0, Qt::UserRole);
                            output = (var == QVariant(word)) || moreThan(var, QVariant(word));
                        } else if (op == QStringLiteral("=")) {
                            QVariant var = model->data(index0, Qt::UserRole);
                            output = (var == QVariant(word));
                        } else if (op == QStringLiteral("#")) {
                            QVariant var = model->data(index0, Qt::UserRole);
                            QRegularExpression pattern(QRegularExpression::anchoredPattern(word), QRegularExpression::CaseInsensitiveOption);
                            output = pattern.match(var.toString()).hasMatch();
                        }
                    }
                }
            }
        }
    }
    return output;
}
