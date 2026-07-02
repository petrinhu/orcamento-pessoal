#include "TableHoverDelegate.h"

#include "TableHoverFilter.h"
#include "Theme.h"

#include <QPainter>
#include <QTableWidget>

void TableHoverDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    const bool selecionada         = option.state & QStyle::State_Selected;
    const bool temBackgroundProprio = index.data(Qt::BackgroundRole).isValid();

    if (!selecionada && !temBackgroundProprio) {
        // parent() é a QTableWidget dona: TableHoverFilter::armar() sempre
        // constrói o delegate com `new TableHoverDelegate(table)`.
        if (auto *table = qobject_cast<QTableWidget *>(parent())) {
            if (TableHoverFilter::instance()->hoverRow(table) == index.row())
                painter->fillRect(option.rect, QColor(Theme::hoverBg()));
        }
    }

    QStyledItemDelegate::paint(painter, option, index);
}
