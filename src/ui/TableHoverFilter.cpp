#include "TableHoverFilter.h"

#include "TableHoverDelegate.h"
#include "Theme.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QTableWidget>

namespace {
TableHoverFilter *g_instance = nullptr;
}

void TableHoverFilter::install()
{
    if (g_instance)
        return;
    g_instance = new TableHoverFilter(qApp);
    qApp->installEventFilter(g_instance);
}

TableHoverFilter *TableHoverFilter::instance()
{
    // Testes chamam hoverRow() sem necessariamente ter passado por
    // Theme::aplicar() primeiro; install() é idempotente, então garantir a
    // instância aqui não duplica o filtro caso aplicar() já tenha rodado.
    install();
    return g_instance;
}

int TableHoverFilter::hoverRow(const QTableWidget *table) const
{
    return m_hoverRow.value(table, -1);
}

bool TableHoverFilter::eventFilter(QObject *obj, QEvent *ev)
{
    if (auto *table = qobject_cast<QTableWidget *>(obj)) {
        if (ev->type() == QEvent::Show || ev->type() == QEvent::Polish)
            armar(table);
        return QObject::eventFilter(obj, ev);
    }

    const auto it = m_viewportTable.constFind(obj);
    if (it == m_viewportTable.constEnd() || it.value().isNull())
        return QObject::eventFilter(obj, ev);

    // qobject_cast (dinâmico, via metaobject) em vez do downcast estático
    // implícito de QPointer<QTableWidget> — evento pode chegar aqui com a
    // tabela em meio a ~QTableWidget() (vptr já rebaixado, QPointer ainda não
    // null); qobject_cast devolve nullptr com segurança nesse caso (UB
    // achado por UBSan, ver comentário de m_viewportTable no .h).
    QTableWidget *table = qobject_cast<QTableWidget *>(it.value());
    if (!table)
        return QObject::eventFilter(obj, ev);

    switch (ev->type()) {
    case QEvent::MouseMove: {
        auto *me = static_cast<QMouseEvent *>(ev);
        const QModelIndex idx = table->indexAt(me->pos());
        atualizarHover(table, idx.isValid() ? idx.row() : -1);
        break;
    }
    case QEvent::Leave:
        atualizarHover(table, -1);
        break;
    default:
        break;
    }

    return QObject::eventFilter(obj, ev);
}

void TableHoverFilter::armar(QTableWidget *table)
{
    if (m_hoverRow.contains(table))
        return; // já armada (idempotência do Show/Polish repetido)

    table->setMouseTracking(true);
    QWidget *vp = table->viewport();
    vp->setMouseTracking(true);
    vp->installEventFilter(this);

    m_hoverRow.insert(table, -1);
    m_viewportTable.insert(vp, QPointer<QWidget>(table));

    // Um único delegate compartilhado por tabela; não pisa em delegate
    // customizado que algum widget já tenha instalado (nenhum faz isso hoje,
    // mas o guard é barato e evita surpresa futura).
    if (!qobject_cast<TableHoverDelegate *>(table->itemDelegate()))
        table->setItemDelegate(new TableHoverDelegate(table));

    Theme::aplicarDensidadeTabela(table);

    QObject::connect(table, &QObject::destroyed, this, [this, table, vp]() {
        m_hoverRow.remove(table);
        m_viewportTable.remove(vp);
    });
}

void TableHoverFilter::atualizarHover(QTableWidget *table, int novaLinha)
{
    auto it = m_hoverRow.find(table);
    if (it == m_hoverRow.end())
        return;
    if (it.value() == novaLinha)
        return;

    const int anterior = it.value();
    it.value() = novaLinha;

    repintarLinha(table, anterior);
    repintarLinha(table, novaLinha);
}

void TableHoverFilter::repintarLinha(QTableWidget *table, int linha) const
{
    if (!table || linha < 0 || linha >= table->rowCount())
        return;
    const int y = table->rowViewportPosition(linha);
    const int h = table->rowHeight(linha);
    table->viewport()->update(0, y, table->viewport()->width(), h);
}
