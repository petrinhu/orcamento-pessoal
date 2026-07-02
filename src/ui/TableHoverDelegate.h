#pragma once

#include <QStyledItemDelegate>

// Delegate compartilhado (#167 T1, §11 manual de estilo) instalado pelo
// TableHoverFilter em toda QTableWidget da aplicação.
//
// Pinta o fundo da CÉLULA com o token @@HOV@@ (Theme::hoverBg()) quando a
// linha da célula é a linha em hover (rastreada pelo TableHoverFilter) e
// deixa o QStyledItemDelegate::paint desenhar o conteúdo por cima. Aplicado
// a cada célula visível da linha (uma chamada de paint() por célula), o
// efeito visual resultante é hover de LINHA INTEIRA — um QSS `::item:hover`
// só pintaria a célula sob o cursor, insuficiente.
//
// Não pinta por cima de:
//   - célula selecionada (a cor de seleção — accent — tem prioridade e não
//     pode ser "quebrada" pelo hover, §11 do manual);
//   - célula com background explícito via QTableWidgetItem::setBackground
//     (preserva estilização própria de linhas de aviso/alerta, ex.: a linha
//     "vence em breve" do LembretesWidget).
class TableHoverDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};
