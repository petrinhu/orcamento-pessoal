/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGObjectModelBase.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgobjectmodelbase.h"

#include <kcolorscheme.h>
#include <kformat.h>
#include <klocalizedstring.h>

#include <qapplication.h>
#include <qcolor.h>
#include <qdir.h>
#include <qicon.h>
#include <qmimedata.h>

#include "skgdocument.h"
#include "skgmainpanel.h"
#include "skgnodeobject.h"
#include "skgpropertyobject.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"
#include <utility>

SKGObjectModelBase::SKGObjectModelBase(SKGDocument *iDocument,
                                       const QString &iTable,
                                       QString iWhereClause,
                                       QWidget *iParent,
                                       QString iParentAttribute,
                                       bool iResetOnCreation)
    : QAbstractItemModel(iParent)
    , m_isResetRealyNeeded(iResetOnCreation)
    , m_cache(new QMap<QString, QVariant>())
    , m_document(iDocument)
    , m_whereClause(std::move(iWhereClause))
    , m_parentAttribute(std::move(iParentAttribute))
    , m_doctransactionTable(false)
    , m_nodeTable(false)
    , m_parametersTable(false)
    , m_refreshBlocked(false)
{
    SKGTRACEINFUNC(1)

    setTable(iTable);

    connect(m_document, &SKGDocument::tableModified, this, &SKGObjectModelBase::dataModified);
    if (SKGMainPanel::getMainPanel() != nullptr) {
        connect(SKGMainPanel::getMainPanel(), &SKGMainPanel::currentPageChanged, this, &SKGObjectModelBase::pageChanged, Qt::QueuedConnection);
    }
}

SKGObjectModelBase::~SKGObjectModelBase()
{
    SKGTRACEINFUNC(1)
    clear();
    m_document = nullptr;
    delete m_cache;
    m_cache = nullptr;
}

void SKGObjectModelBase::clear()
{
    SKGTRACEINFUNC(1)
    QHashIterator<int, SKGObjectBase *> i(m_objectsHashTable);
    while (i.hasNext()) {
        i.next();
        SKGObjectBase *val = i.value();
        delete val;
        val = nullptr;
    }

    m_listObjects.clear();
    m_parentChildRelations.clear();
    m_childParentRelations.clear();
    m_objectsHashTable.clear();
    m_objectsHashTableRows.clear();
}

SKGDocument::SKGModelTemplateList SKGObjectModelBase::getSchemas() const
{
    return m_listSchema;
}

void SKGObjectModelBase::setSupportedAttributes(const QStringList &iListAttribute)
{
    SKGTRACEINFUNC(1)
    m_listSupported.clear();
    m_listVisibility.clear();
    m_listSize.clear();

    QStringList l = iListAttribute;
    if (!m_listSchema.isEmpty()) {
        l += SKGServices::splitCSVLine(m_listSchema.at(0).schema);
    }

    QStringList attributesAvailablesTmp;
    if (!m_listSchema.isEmpty()) {
        attributesAvailablesTmp = SKGServices::splitCSVLine(m_listSchema.at(0).schema);
    }
    int nb = attributesAvailablesTmp.count();
    QStringList attributesAvailables;
    attributesAvailables.reserve(nb);
    for (int i = 0; i < nb; ++i) {
        attributesAvailables.push_back(attributesAvailablesTmp.at(i).split('|').at(0));
    }

    nb = l.count();
    for (int i = 0; i < nb; ++i) {
        QStringList values = l.at(i).split('|');
        int nbValues = values.count();
        const QString &att = values.at(0);

        if (nbValues > 0 && !m_listSupported.contains(att) && attributesAvailables.contains(att)) {
            m_listSupported.push_back(att);
            bool visible = true;
            if (nbValues > 1) {
                visible = (i == 0 || values.at(1) == QStringLiteral("Y")); // First column is always visible to support grouping
            }
            m_listVisibility.push_back(visible);
            if (nbValues > 2) {
                m_listSize.push_back(SKGServices::stringToInt(values.at(2)));
            } else {
                m_listSize.push_back(-1);
            }
        }
    }

    m_isResetRealyNeeded = true;
}

bool SKGObjectModelBase::setFilter(const QString &iWhereClause)
{
    if (iWhereClause != m_whereClause) {
        m_isResetRealyNeeded = true;
    }
    m_whereClause = iWhereClause;
    return m_isResetRealyNeeded;
}

void SKGObjectModelBase::setTable(const QString &iTable)
{
    if (iTable != m_table) {
        if (!m_table.isEmpty()) {
            m_isResetRealyNeeded = true;
        }
        m_table = iTable;
        m_realTable = SKGServices::getRealTable(m_table);
        if (m_document != nullptr) {
            m_listSchema = m_document->getDisplaySchemas(m_realTable);
        }
    }
}

QString SKGObjectModelBase::getTable() const
{
    return m_table;
}

void SKGObjectModelBase::setGroupBy(const QString &iAttribute)
{
    if (iAttribute != m_groupby) {
        m_isResetRealyNeeded = true;
        m_groupby = iAttribute;
    }
}

QString SKGObjectModelBase::getGroupBy() const
{
    return m_groupby;
}

QString SKGObjectModelBase::getParentChildAttribute() const
{
    return m_parentAttribute;
}

QString SKGObjectModelBase::getRealTable() const
{
    return m_realTable;
}

QString SKGObjectModelBase::getWhereClause() const
{
    return m_whereClause;
}

SKGDocument *SKGObjectModelBase::getDocument() const
{
    return m_document;
}

void SKGObjectModelBase::buidCache()
{
    SKGTRACEINFUNC(1)
    m_doctransactionTable = (getRealTable() == QStringLiteral("doctransaction"));
    m_nodeTable = (getRealTable() == QStringLiteral("node"));
    m_parametersTable = (getRealTable() == QStringLiteral("parameters"));

    // Get std colors
    KColorScheme scheme(QPalette::Normal);
    m_fontNegativeColor = QVariant::fromValue(scheme.foreground(KColorScheme::NegativeText).color());
}

bool SKGObjectModelBase::blockRefresh(bool iBlocked)
{
    bool previous = m_refreshBlocked;
    m_refreshBlocked = iBlocked;
    return previous;
}

bool SKGObjectModelBase::isRefreshBlocked()
{
    return m_refreshBlocked;
}

void SKGObjectModelBase::refresh()
{
    if (!m_isResetRealyNeeded || isRefreshBlocked()) {
        return;
    }
    SKGTRACEIN(1, "SKGObjectModelBase::refresh-" % getTable())
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    beginResetModel();
    Q_EMIT beforeReset();
    m_cache->clear();
    {
        _SKGTRACEINFUNC(10)
        clear();
        m_listAttibutes.clear();
        m_listAttributeTypes.clear();
        /*beginRemoveRows(QModelIndex(), 0, rowCount(QModelIndex())-1);
        endRemoveRows();*/
    }

    {
        _SKGTRACEINFUNC(10)
        QStringList listAttibutesTmp;
        if (m_document != nullptr && m_document->getAttributesList(m_table, listAttibutesTmp).isSucceeded()) {
            m_isResetRealyNeeded = false;
            if (!listAttibutesTmp.isEmpty()) {
                // Filter attributes
                int nb = m_listSupported.count();
                if (nb == 0) {
                    setSupportedAttributes(QStringList());
                    nb = m_listSupported.count();
                }
                for (int i = 0; i < nb; ++i) {
                    QString att = m_listSupported.at(i);
                    if (listAttibutesTmp.contains(att) || att.startsWith(QStringLiteral("p_"))) {
                        m_listAttibutes.push_back(att);
                        if (att.startsWith(QStringLiteral("t_")) || att.startsWith(QStringLiteral("p_")) || att == QStringLiteral("d_DATEWEEK")
                            || att == QStringLiteral("d_DATEMONTH") || att == QStringLiteral("d_DATEQUARTER") || att == QStringLiteral("d_DATESEMESTER")
                            || att == QStringLiteral("d_DATEYEAR")) {
                            m_listAttributeTypes.push_back(SKGServices::TEXT);
                        } else if (att.startsWith(QStringLiteral("f_"))) {
                            m_listAttributeTypes.push_back(SKGServices::FLOAT);
                        } else if (att.startsWith(QStringLiteral("i_"))) {
                            m_listAttributeTypes.push_back(SKGServices::INTEGER);
                        } else if (att.startsWith(QStringLiteral("d_"))) {
                            m_listAttributeTypes.push_back(SKGServices::DATE);
                        } else {
                            m_listAttributeTypes.push_back(SKGServices::OTHER);
                        }
                    }
                }

                // Remove double
                nb = m_listAttibutes.count();
                for (int i = nb - 1; i >= 0; --i) {
                    QString att = m_listAttibutes.at(i);
                    if (att.contains(QStringLiteral("_REAL"))) {
                        att.replace(QStringLiteral("_REAL"), QStringLiteral("_"));
                        int p = m_listAttibutes.indexOf(att);
                        if (p == -1) {
                            att = att.toLower();
                            p = m_listAttibutes.indexOf(att);
                        }
                        if (p != -1) {
                            m_listAttibutes.removeAt(p);
                            m_listAttributeTypes.removeAt(p);
                            if (p < i) {
                                --i;
                            }
                        }
                    }
                }
            }

            // Get objects
            QString wc = m_whereClause;
            if (!m_groupby.isEmpty()) {
                if (wc.isEmpty()) {
                    wc = QStringLiteral("1=1");
                }
                if (m_groupby.startsWith(QStringLiteral("p_"))) {
                    wc += " ORDER BY (SELECT t_value FROM parameters WHERE t_uuid_parent=" % getTable() % ".id||'-" % getRealTable() % "' AND t_name='"
                        % m_groupby % "')";
                } else {
                    wc += " ORDER BY " % m_groupby;
                }
            }
            if (m_document != nullptr) {
                m_document->getObjects(m_table, wc, m_listObjects);
            }

            // Initialize object to treat
            QString currentGroup;
            int currentGoupId = 0;
            bool groupByDate =
                (m_groupby.startsWith(QStringLiteral("d_")) && m_groupby != QStringLiteral("d_DATEWEEK") && m_groupby != QStringLiteral("d_DATEMONTH")
                 && m_groupby != QStringLiteral("d_DATEQUARTER") && m_groupby != QStringLiteral("d_DATESEMESTER") && m_groupby != QStringLiteral("d_DATEYEAR"));

            bool groupByNum = (m_groupby.startsWith(QStringLiteral("f_")) || m_groupby.startsWith(QStringLiteral("i_")));
            int nb = m_listObjects.count();
            SKGTRACEL(1) << nb << " objects found" << Qt::endl;
            for (int t = 0; t < nb; ++t) {
                auto c = new SKGObjectBase(m_listObjects.at(t));
                int id = (c != nullptr ? c->getID() : 0);

                int idparent = 0;
                if (m_groupby.isEmpty()) {
                    // Grouping by tree
                    if (!m_parentAttribute.isEmpty()) {
                        int idp = SKGServices::stringToInt(c->getAttribute(m_parentAttribute));
                        if (idp > 0) {
                            idparent = idp;
                        }
                    }
                } else {
                    // Grouping on specified attribute
                    QString att = getAttributeForGrouping(*c, m_groupby);
                    if (groupByDate) {
                        QDate date = SKGServices::stringToDate(att);
                        if (date == QDate::currentDate()) {
                            att = i18nc("A group name for grouping by date attribute", "Today");
                        } else if (date > QDate::currentDate()) {
                            // In future
                            if (date < QDate::currentDate().addDays(7)) {
                                att = i18nc("A group name for grouping by date attribute", "Next 7 days");
                            } else if (date < QDate::currentDate().addDays(15)) {
                                att = i18nc("A group name for grouping by date attribute", "Next 15 days");
                            } else if (date < QDate::currentDate().addMonths(1)) {
                                att = i18nc("A group name for grouping by date attribute", "Next month");
                            } else if (date < QDate::currentDate().addMonths(3)) {
                                att = i18nc("A group name for grouping by date attribute", "Next 3 months");
                            } else if (date < QDate::currentDate().addMonths(6)) {
                                att = i18nc("A group name for grouping by date attribute", "Next 6 months");
                            } else if (date < QDate::currentDate().addYears(1)) {
                                att = i18nc("A group name for grouping by date attribute", "Next year");
                            } else if (date < QDate::currentDate().addYears(3)) {
                                att = i18nc("A group name for grouping by date attribute", "Next 3 years");
                            } else {
                                att = i18nc("A group name for grouping by date attribute", "Far away in the future");
                            }
                        } else {
                            // In the past
                            if (date > QDate::currentDate().addDays(-7)) {
                                att = i18nc("A group name for grouping by date attribute", "Last 7 days");
                            } else if (date > QDate::currentDate().addDays(-15)) {
                                att = i18nc("A group name for grouping by date attribute", "Last 15 days");
                            } else if (date > QDate::currentDate().addMonths(-1)) {
                                att = i18nc("A group name for grouping by date attribute", "Last month");
                            } else if (date > QDate::currentDate().addMonths(-3)) {
                                att = i18nc("A group name for grouping by date attribute", "Last 3 months");
                            } else if (date > QDate::currentDate().addMonths(-6)) {
                                att = i18nc("A group name for grouping by date attribute", "Last 6 months");
                            } else if (date > QDate::currentDate().addYears(-1)) {
                                att = i18nc("A group name for grouping by date attribute", "Last year");
                            } else if (date > QDate::currentDate().addYears(-3)) {
                                att = i18nc("A group name for grouping by date attribute", "Last 3 years");
                            } else {
                                att = i18nc("A group name for grouping by date attribute", "Far away in the past");
                            }
                        }
                    } else if (groupByNum) {
                        if (att != QChar(8734)) {
                            double d = SKGServices::stringToDouble(att);
                            if (d > 10000.0) {
                                att = i18nc("A group name for grouping by numerical attribute", "> 10000");
                            } else if (d > 1000.0) {
                                att = i18nc("A group name for grouping by numerical attribute", "> 1000");
                            } else if (d > 100.0) {
                                att = i18nc("A group name for grouping by numerical attribute", "> 100");
                            } else if (d > 10.0) {
                                att = i18nc("A group name for grouping by numerical attribute", "> 10");
                            } else if (d > 0.0) {
                                att = i18nc("A group name for grouping by numerical attribute", "> 0");
                            } else if (d < -10000.0) {
                                att = i18nc("A group name for grouping by numerical attribute", "< -10000");
                            } else if (d < -1000.0) {
                                att = i18nc("A group name for grouping by numerical attribute", "< -1000");
                            } else if (d < -100.0) {
                                att = i18nc("A group name for grouping by numerical attribute", "< -100");
                            } else if (d < -10.0) {
                                att = i18nc("A group name for grouping by numerical attribute", "< -10");
                            } else if (d < 0.0) {
                                att = i18nc("A group name for grouping by numerical attribute", "< 0");
                            } else {
                                att = i18nc("A group name for grouping by numerical attribute", "= 0");
                            }
                        }
                    }
                    if (att != currentGroup || currentGoupId == 0) {
                        // Addition of a new group
                        currentGroup = std::move(att);
                        currentGoupId--;
                        auto group = new SKGObjectBase(m_document, QString(), currentGoupId);
                        group->setAttribute(QStringLiteral("t_title"), currentGroup);

                        m_childParentRelations.insert(currentGoupId, 0);

                        QList<int> childrensids = m_parentChildRelations.value(0);
                        childrensids.push_back(currentGoupId);

                        m_parentChildRelations.insert(0, childrensids);
                        m_objectsHashTableRows.insert(currentGoupId, childrensids.count() - 1);
                        m_objectsHashTable.insert(currentGoupId, group);
                    }

                    idparent = currentGoupId;
                }

                m_childParentRelations.insert(id, idparent);

                QList<int> childrensids = m_parentChildRelations.value(idparent);
                childrensids.push_back(id);

                m_parentChildRelations.insert(idparent, childrensids);
                m_objectsHashTableRows.insert(id, childrensids.count() - 1);
                m_objectsHashTable.insert(id, c);
            }
        }

        // Build cache
        buidCache();
    }
    endResetModel();
    Q_EMIT afterReset();

    QApplication::restoreOverrideCursor();
}

bool SKGObjectModelBase::hasChildren(const QModelIndex &iParent) const
{
    if (iParent.column() > 0) {
        return false;
    }
    _SKGTRACEINFUNC(10)

    if (iParent.isValid() && m_parentAttribute.isEmpty() && m_groupby.isEmpty()) {
        return false;
    }
    return QAbstractItemModel::hasChildren(iParent);
}

int SKGObjectModelBase::rowCount(const QModelIndex &iParent) const
{
    if (iParent.column() > 0) {
        return 0;
    }
    _SKGTRACEINFUNC(10)

    int idParent = 0;
    if (iParent.isValid()) {
        idParent = iParent.internalId();
    }

    return m_parentChildRelations.value(idParent).count();
}

int SKGObjectModelBase::columnCount(const QModelIndex &iParent) const
{
    Q_UNUSED(iParent)
    return m_listAttibutes.count();
}

QModelIndex SKGObjectModelBase::index(int row, int column, const QModelIndex &iParent) const
{
    if (!hasIndex(row, column, iParent)) {
        return {};
    }
    _SKGTRACEINFUNC(10)

    int idParent = 0;
    if (iParent.isValid()) {
        idParent = iParent.internalId();
    }

    int idChild = m_parentChildRelations.value(idParent).at(row);

    // SKGTRACE << table << "-" << idParent << "(" << row << ") ==> " << idChild << Qt::endl;
    return (idChild != 0 ? createIndex(row, column, idChild) : QModelIndex());
}

QModelIndex SKGObjectModelBase::parent(const QModelIndex &iIndex) const
{
    if (!iIndex.isValid()) {
        return {};
    }
    _SKGTRACEINFUNC(10)

    int idChild = 0;
    if (iIndex.isValid()) {
        idChild = iIndex.internalId();
    }

    int idParent = m_childParentRelations.value(idChild);
    int row = m_objectsHashTableRows.value(idParent);
    // SKGTRACE << table << "-" << idChild << "(" << row << ") <== " << idParent << Qt::endl;
    return idParent != 0 ? createIndex(row, 0, idParent) : QModelIndex();
}

int SKGObjectModelBase::getIndexAttribute(const QString &iAttributeName) const
{
    int output = m_listAttibutes.indexOf(iAttributeName);
    if (output == -1) {
        SKGTRACE << "[" << iAttributeName << "] not found in [" << getRealTable() << "]" << Qt::endl;
    }
    return output;
}

QString SKGObjectModelBase::getAttribute(int iIndex) const
{
    return m_listAttibutes.value(iIndex);
}

SKGServices::AttributeType SKGObjectModelBase::getAttributeType(int iIndex) const
{
    return m_listAttributeTypes.value(iIndex);
}

QVariant SKGObjectModelBase::headerData(int iSection, Qt::Orientation iOrientation, int iRole) const
{
    _SKGTRACEINFUNC(10)

    if (iOrientation == Qt::Horizontal) {
        if (iRole == Qt::DisplayRole) {
            QString att;
            if (iSection >= 0 && iSection < m_listAttibutes.count()) {
                att = m_listAttibutes.at(iSection);
            } else {
                att = SKGServices::intToString(iSection);
            }

            return getDocument()->getDisplay(getTable() % u'.' % att).remove(getTable() % u'.');
        }
        if (iRole == Qt::UserRole) {
            QString att;
            if (iSection >= 0 && iSection < m_listAttibutes.count()) {
                att = m_listAttibutes.at(iSection);
            } else {
                att = SKGServices::intToString(iSection);
            }

            int indexAtt = m_listSupported.indexOf(att);

            att = getDocument()->getDisplay(getTable() % u'.' % att).remove(getTable() % u'.');

            if (indexAtt >= 0 && indexAtt < m_listVisibility.count()) {
                bool visible = m_listVisibility.at(indexAtt);
                att += QStringLiteral("|") % (visible ? QStringLiteral("Y") : QStringLiteral("N"));
                if (indexAtt >= 0 && indexAtt < m_listSize.count()) {
                    att += u'|' % SKGServices::intToString(m_listSize.at(indexAtt));
                }
            }
            return att;
        }
        if (iRole == Qt::DecorationRole) {
            QString att;
            if (iSection >= 0 && iSection < m_listAttibutes.count()) {
                att = m_listAttibutes.at(iSection);
            } else {
                att = SKGServices::intToString(iSection);
            }

            return getDocument()->getIcon(getTable() % u'.' % att);
        }
    }
    return QVariant();
}

SKGObjectBase SKGObjectModelBase::getObject(const QModelIndex &iIndex) const
{
    SKGObjectBase *obj = getObjectPointer(iIndex);
    SKGObjectBase output;
    if (obj != nullptr) {
        output = *obj;
    }
    return output;
}

SKGObjectBase *SKGObjectModelBase::getObjectPointer(const QModelIndex &iIndex) const
{
    _SKGTRACEINFUNC(10)
    return m_objectsHashTable.value(iIndex.internalId());
}

QVariant SKGObjectModelBase::data(const QModelIndex &iIndex, int iRole) const
{
    if (!iIndex.isValid()) {
        return QVariant();
    }
    _SKGTRACEINFUNC(10)
    // Build cache id
    QString idcache = getObjectPointer(iIndex)->getUniqueID() % "-" % SKGServices::intToString(iIndex.row()) % "-" % SKGServices::intToString(iIndex.column())
        % "-" % SKGServices::intToString(iRole);

    // Check cache
    if (!m_cache->contains(idcache)) {
        // Compute value
        m_cache->insert(idcache, computeData(iIndex, iRole));
    }

    return m_cache->value(idcache);
}

QVariant SKGObjectModelBase::computeData(const QModelIndex &iIndex, int iRole) const
{
    if (!iIndex.isValid()) {
        return QVariant();
    }
    _SKGTRACEINFUNC(10)

    switch (iRole) {
    case Qt::BackgroundRole: {
        SKGObjectBase *obj = getObjectPointer(iIndex);
        if (obj->getTable().isEmpty()) {
            // This is a group
            return QApplication::palette().brush(QPalette::Button);
        }
        break;
    }
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::UserRole: {
        SKGObjectBase *obj = getObjectPointer(iIndex);
        QString att = m_listAttibutes.at(iIndex.column());
        if (obj->getTable().isEmpty()) {
            // This is a group
            if (iIndex.column() == 0) {
                if (iRole == Qt::UserRole) {
                    return -obj->getID(); // For sorting
                }
                int nb = rowCount(iIndex);

                // Is it possible to compute sums of f_CURRENTAMOUNT ?
                int posAmount = m_listAttibutes.indexOf(QStringLiteral("f_CURRENTAMOUNT"));
                if (posAmount == -1) {
                    posAmount = m_listAttibutes.indexOf(QStringLiteral("f_REALCURRENTAMOUNT"));
                }
                if (posAmount != -1 && nb > 0) {
                    // Compute sums
                    double sum = 0.0;
                    double average = 0.0;
                    double min = std::numeric_limits<double>::max();
                    double max = -std::numeric_limits<double>::max();
                    for (int i = 0; i < nb; ++i) {
                        double amount = data(SKGObjectModelBase::index(i, posAmount, iIndex), Qt::UserRole).toDouble();
                        sum += amount;
                        max = qMax(max, amount);
                        min = qMin(min, amount);
                    }
                    average = sum / nb;

                    return i18nc("How to display a grouping title. Here \"title (count) Sum= [min , average , max]\"",
                                 "%1: %2 (%3) Sum=%4 [%5 , %6 , %7]",
                                 getDocument()->getDisplay(m_groupby),
                                 obj->getAttribute(QStringLiteral("t_title")),
                                 nb,
                                 formatMoney(sum),
                                 formatMoney(min),
                                 formatMoney(average),
                                 formatMoney(max));
                }
                return i18nc("How to display a grouping title. Here \"title (count)\"",
                             "%1: %2 (%3)",
                             getDocument()->getDisplay(m_groupby),
                             obj->getAttribute(QStringLiteral("t_title")),
                             nb);
            }
            return "";
        }
        QString val;
        if (att.startsWith(QStringLiteral("p_"))) {
            // This is a property
            val = obj->getProperty(att.right(att.size() - 2));
        } else {
            // This is a real attribute
            val = obj->getAttribute(att);

            switch (getAttributeType(iIndex.column())) {
            case SKGServices::FLOAT: {
                double dval = SKGServices::stringToDouble(val);
                return dval;
            }
            case SKGServices::INTEGER: {
                return SKGServices::stringToInt(val);
            }
            case SKGServices::DATE: {
                QDate dval = SKGServices::stringToDate(val);
                if (iRole == Qt::DisplayRole) {
                    return SKGMainPanel::dateToString(dval);
                }
                if (iRole == Qt::UserRole) {
                    return dval;
                }
            }
            default: {
            }
            }

            if (m_doctransactionTable && att == QStringLiteral("t_savestep")) {
                return "";
            }
        }

        // return val+"("+SKGServices::intToString(rowCount(index))+")";
        return val;
    }

    case Qt::FontRole: {
        SKGObjectBase *obj = getObjectPointer(iIndex);
        // Text color
        if (obj->getTable().isEmpty()) {
            // This is a group
            if (iIndex.column() == 0) {
                QFont f;
                f.setBold(true);

                return QVariant::fromValue(f);
            }
            return "";
        }

        int propertyId = data(iIndex, 101).toInt();
        if (propertyId != 0) {
            SKGPropertyObject p(m_document, propertyId);
            if (!p.getUrl().scheme().isEmpty()) {
                QFont f;
                f.setUnderline(true);

                return QVariant::fromValue(f);
            }
        }
        break;
    }
    case Qt::ForegroundRole: {
        // Text color
        if (getAttributeType(iIndex.column()) == SKGServices::FLOAT) {
            QVariant value_displayed = SKGObjectModelBase::data(iIndex, Qt::UserRole);
            bool ok = false;
            double value_double = value_displayed.toDouble(&ok);
            if (ok && value_double < 0) {
                return m_fontNegativeColor;
            }
        }

        int propertyId = data(iIndex, 101).toInt();
        if (propertyId != 0) {
            SKGPropertyObject p(m_document, propertyId);
            if (!p.getUrl().scheme().isEmpty()) {
                KColorScheme scheme(QPalette::Normal);
                return QVariant::fromValue(scheme.foreground(KColorScheme::ActiveText).color());
            }
        }
        break;
    }
    case Qt::TextAlignmentRole: {
        // Text alignment
        SKGServices::AttributeType attType = getAttributeType(iIndex.column());
        return static_cast<int>(Qt::AlignVCenter | (attType == SKGServices::FLOAT || attType == SKGServices::INTEGER ? Qt::AlignRight : Qt::AlignLeft));
    }
    case Qt::DecorationRole: {
        // Decoration
        SKGObjectBase *obj = getObjectPointer(iIndex);
        QString att = m_listAttibutes.at(iIndex.column());
        if (obj->getTable().isEmpty()) {
            // This is a group
            if (iIndex.column() == 0) {
                return QVariant::fromValue(SKGServices::fromTheme(QStringLiteral("arrow-right")));
            };
            return "";
        }
        if (iIndex.column() == 0 && m_nodeTable) {
            SKGNodeObject node(*obj);
            return QVariant::fromValue(node.getIcon());
        }
        if (iIndex.column() == 0 && m_doctransactionTable) {
            return QVariant::fromValue(SKGServices::fromTheme(
                obj->getAttribute(QStringLiteral("t_mode")) == QStringLiteral("U") ? QStringLiteral("edit-undo") : QStringLiteral("edit-redo")));
        }
        if (m_doctransactionTable && att == QStringLiteral("t_savestep")) {
            if (obj->getAttribute(QStringLiteral("t_savestep")) == QStringLiteral("Y")) {
                return QVariant::fromValue(SKGServices::fromTheme(QStringLiteral("document-save")));
            }
        }

        break;
    }
    case Qt::ToolTipRole: {
        // Tooltip
        QString toolTipString;
        SKGObjectBase *obj = getObjectPointer(iIndex);

        if (obj != nullptr && m_document != nullptr) {
            if (m_doctransactionTable) {
                SKGDocument::SKGMessageList msg;
                m_document->getMessages(obj->getID(), msg);
                int nbMessages = msg.count();
                if (nbMessages > 0) {
                    for (int i = 0; i < nbMessages; ++i) {
                        if (i != 0) {
                            toolTipString += '\n';
                        }
                        toolTipString += msg.at(i).Text;
                    }
                }
            } else if (getAttributeType(iIndex.column()) == SKGServices::DATE) {
                QString att = m_listAttibutes.at(iIndex.column());
                QString val = obj->getAttribute(att);

                QDate dval = SKGServices::stringToDate(val);
                QString fancyDate = QLocale().toString(dval, QLocale::LongFormat);
                QString shortDate = QLocale().toString(dval, QLocale::ShortFormat);

                if (shortDate != fancyDate) {
                    toolTipString = std::move(shortDate);
                }
            }

            // Add properties
            QStringList props = obj->getProperties();
            if (!props.isEmpty() && !toolTipString.isEmpty()) {
                toolTipString += '\n';
            }
            for (const auto &prop : std::as_const(props)) {
                if (!toolTipString.isEmpty()) {
                    toolTipString += '\n';
                }
                toolTipString += i18nc("To display a property and its value", "%1=%2", prop, obj->getProperty(prop));
            }

            // Add UUID
            IFSKGTRACEL(1)
            {
                toolTipString += '\n' % obj->getUniqueID();
            }
        }
        return toolTipString;
    }
    case 99: {
        SKGObjectBase *obj = getObjectPointer(iIndex);
        if (obj != nullptr) {
            return SKGServices::stringToDouble(obj->getAttribute(QStringLiteral("f_sortorder")));
        }
        break;
    }
    case 101: {
        SKGObjectBase *obj = getObjectPointer(iIndex);
        // Is it a URL ?
        QString att = m_listAttibutes.at(iIndex.column());
        if (att.startsWith(QStringLiteral("p_"))) {
            SKGPropertyObject p(obj->getPropertyObject(att.right(att.size() - 2)));
            if (!p.getUrl().scheme().isEmpty()) {
                return QVariant::fromValue(p.getID());
            }
        } else if (m_parametersTable && att == QStringLiteral("t_value")) {
            SKGPropertyObject p(*obj);
            if (!p.getUrl().scheme().isEmpty()) {
                return QVariant::fromValue(p.getID());
            }
        }
        return 0;
    }
    default: {
    }
    }

    return QVariant();
}

Qt::ItemFlags SKGObjectModelBase::flags(const QModelIndex &iIndex) const
{
    _SKGTRACEINFUNC(10)

    Qt::ItemFlags f = QAbstractItemModel::flags(iIndex) | Qt::ItemIsDropEnabled;

    if (iIndex.isValid()) {
        f |= Qt::ItemIsUserCheckable;
    }

    if (m_nodeTable && iIndex.isValid()) {
        f |= Qt::ItemIsEditable | Qt::ItemIsDragEnabled;
    }

    if (iIndex.isValid()) {
        QString att = m_listAttibutes.at(iIndex.column());
        if (att.toLower() == att || !getDocument()->getRealAttribute(att).isEmpty()) {
            f |= Qt::ItemIsEditable;
        }
        SKGObjectBase *obj = getObjectPointer(iIndex);
        if (obj->getTable().isEmpty()) {
            f = Qt::ItemIsEnabled;
        }
    }

    return f;
}

bool SKGObjectModelBase::setData(const QModelIndex &iIndex, const QVariant &iValue, int iRole)
{
    if (!iIndex.isValid()) {
        return false;
    }

    if (iRole == Qt::EditRole) {
        SKGError err;
        if (m_nodeTable) {
            SKGNodeObject obj(getObject(iIndex));
            QString name = iValue.toString();
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Bookmark update '%1'", name), err)
            IFOKDO(err, err = obj.setName(name))
            IFOKDO(err, obj.save())
        } else {
            SKGObjectBase obj(getObject(iIndex));

            SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Update object"), err)
            SKGObjectBase obj2(obj.getDocument(), obj.getRealTable(), obj.getID()); // To be sure this is not a complex object
            QString att = m_listAttibutes.at(iIndex.column());
            IFOKDO(err,
                   obj2.setAttribute(att,
                                     att.startsWith(QStringLiteral("d_")) && iValue.canConvert<QDateTime>() ? SKGServices::dateToSqlString(iValue.toDateTime())
                                                                                                            : iValue.toString()))
            IFOKDO(err, obj2.save())
        }

        SKGMainPanel::displayErrorMessage(err);
        return !err;
    }
    return QAbstractItemModel::setData(iIndex, iValue, iRole);
}

Qt::DropActions SKGObjectModelBase::supportedDragActions() const
{
    return (m_nodeTable ? Qt::MoveAction : Qt::IgnoreAction);
}

Qt::DropActions SKGObjectModelBase::supportedDropActions() const
{
    return Qt::MoveAction | Qt::LinkAction | Qt::CopyAction;
}

QStringList SKGObjectModelBase::mimeTypes() const
{
    QStringList types;
    types << "application/skg." % getRealTable() % ".ids";
    types << QStringLiteral("application/data");
    types << QStringLiteral("text/uri-list");
    return types;
}

QMimeData *SKGObjectModelBase::mimeData(const QModelIndexList &iIndexes) const
{
    auto md = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    QString t = getTable();
    for (const auto &idx : std::as_const(iIndexes)) {
        if (idx.isValid() && idx.column() == 0) {
            SKGObjectBase obj = getObject(idx);
            t = obj.getRealTable();
            stream << t;
            stream << obj.getID();
        }
    }

    md->setData("application/skg." % t % ".ids", encodedData);
    return md;
}

bool SKGObjectModelBase::dropMimeData(const QMimeData *iData, Qt::DropAction iAction, int iRow, int iColumn, const QModelIndex &iParent)
{
    Q_UNUSED(iRow)
    if (iAction == Qt::IgnoreAction) {
        return true;
    }
    if ((iData == nullptr) || !(iData->hasFormat(QStringLiteral("application/skg.node.ids")) || iData->hasUrls())) {
        return false; // TODO(Stephane MANKOWSKI): accept all
    }
    if (iColumn > 0) {
        return false;
    }

    SKGError err;
    // Drop files
    if (iData->hasUrls() && iParent.isValid() && getRealTable() != QStringLiteral("node")) {
        QList<QUrl> urls = iData->urls();
        int nb = urls.count();
        {
            SKGObjectBase obj(getObject(iParent));

            SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Property creation"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                QString name = i18n("File");
                int idx = 1;
                while (!err && !obj.getProperty(name).isEmpty()) {
                    idx++;
                    name = i18n("File") % " (" % SKGServices::intToString(idx) % u')';
                }
                QString f = urls.at(i).toLocalFile();
                err = obj.setProperty(name, f, iAction == Qt::LinkAction ? QString() : f);
                if (!err && iAction == Qt::MoveAction) {
                    QFile(f).remove();
                }
                IFOKDO(err, getDocument()->stepForward(i + 1))
            }
        }
    } else if (iData->hasFormat(QStringLiteral("application/skg.node.ids"))) {
        // Drop nodes
        QByteArray encodedData = iData->data(QStringLiteral("application/skg.node.ids"));
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        QModelIndex parentIndex = iParent;
        SKGNodeObject parentNode;
        if (parentIndex.isValid()) {
            parentNode = getObject(parentIndex);
            if (!parentNode.isFolder()) {
                // The parent is not a directory
                parentNode.getParentNode(parentNode);
                parentIndex = parentIndex.parent();
            }
        }
        {
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Move bookmark"), err)

            double min = 0;
            double max = 0;
            if (iRow >= 1) {
                QModelIndex previousIndex = SKGObjectModelBase::index(iRow - 1, 0, parentIndex);
                SKGNodeObject previousObject(getObject(previousIndex));
                min = previousObject.getOrder();
            }

            if (iRow >= rowCount(parentIndex)) {
                max = min + 1;
            } else {
                QModelIndex nextIndex = SKGObjectModelBase::index(iRow, 0, parentIndex);
                SKGNodeObject nextObject(getObject(nextIndex));
                max = nextObject.getOrder();
            }
            if (max <= min) {
                max = min + 1;
            }

            while (!stream.atEnd() && !err) {
                int o_id;
                QString o_table;
                stream >> o_table;
                stream >> o_id;

                // Set parent
                SKGNodeObject child(getDocument(), o_id);
                err = child.load();
                QString oldName = child.getDisplayName();
                IFOK(err)
                {
                    if (parentIndex.isValid()) {
                        err = child.setParentNode(parentNode);
                    } else {
                        err = child.removeParentNode();
                    }
                }

                // Set order
                IFOKDO(err, child.setOrder((min + max) / 2.0))

                // Save
                IFOKDO(err, child.save())

                // Send message
                IFOKDO(
                    err,
                    getDocument()->sendMessage(i18nc("An information to the user", "The bookmark '%1' has been moved to '%2'", oldName, child.getDisplayName()),
                                               SKGDocument::Hidden))
            }
        }
    }
    SKGMainPanel::displayErrorMessage(err);
    return !err;
}

void SKGObjectModelBase::pageChanged()
{
    if (m_isResetRealyNeeded) {
        dataModified(QString(), 0);
    }
}

void SKGObjectModelBase::dataModified(const QString &iTableName, int iIdTransaction)
{
    if (getTable() == iTableName || iTableName.isEmpty()) {
        SKGTRACEINFUNC(1)
        SKGTRACEL(1) << "getTable=" << getRealTable() << Qt::endl;
        SKGTRACEL(1) << "Parameters=" << iTableName << " , " << iIdTransaction << Qt::endl;
        SKGTabPage *page = SKGTabPage::parentTabPage(qobject_cast<QWidget *>(this->QObject::parent()));
        SKGTabPage *cpage = SKGMainPanel::getMainPanel() != nullptr ? SKGMainPanel::getMainPanel()->currentPage() : nullptr;
        if (page != nullptr && page != cpage) {
            m_isResetRealyNeeded = true;
            return;
        }

        // Full refresh
        m_isResetRealyNeeded = true;

        // Refresh model
        refresh();
    }
}

QString SKGObjectModelBase::getAttributeForGrouping(const SKGObjectBase &iObject, const QString &iAttribute) const
{
    if (iAttribute.startsWith(QStringLiteral("p_"))) {
        // This is a property
        return iObject.getProperty(iAttribute.right(iAttribute.size() - 2));
    }
    // This is a real attribute
    return iObject.getAttribute(iAttribute);
}

QString SKGObjectModelBase::formatMoney(double iValue) const
{
    return SKGServices::doubleToString(iValue);
}
