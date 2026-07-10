#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>

class QTableWidget;
class QWidget;

// App-wide event filter (#167 T1, §11 manual de estilo — "skin compartilhado
// das tabelas") que dá a TODA QTableWidget da aplicação:
//
//   1. Hover de LINHA INTEIRA: mouse tracking no viewport + TableHoverDelegate
//      compartilhado (um QSS `::item:hover` só pintaria a célula sob o
//      cursor — insuficiente para linha inteira).
//   2. Densidade de linha (Confortável/Compacta) no momento em que a tabela
//      aparece — Theme::aplicarDensidadeTabela() na mesma varredura que arma
//      o hover, sem precisar de código por-widget.
//
// Instalado uma vez em qApp via install() — mesmo padrão de ButtonShadowFilter
// e FocusVisibleFilter (Theme::aplicar() chama install() no boot). Idempotente.
//
//   - Show/Polish em uma QTableWidget ainda não armada -> armar(): liga mouse
//     tracking, instala TableHoverDelegate (se ainda não houver um) e aplica
//     a densidade de linha atual.
//   - MouseMove no viewport de uma tabela armada -> recalcula a linha sob o
//     cursor (indexAt) e, se mudou, repinta só as linhas afetadas (antiga +
//     nova) — não a tabela inteira.
//   - Leave no viewport -> limpa o hover (linha = -1).
class TableHoverFilter : public QObject {
    Q_OBJECT
public:
    static void install();
    static TableHoverFilter *instance();

    // Linha (índice do modelo) atualmente sob o mouse na tabela, ou -1 se
    // nenhuma linha está em hover (mouse fora do viewport ou tabela ainda não
    // armada). Usado pelo TableHoverDelegate para decidir o que pintar.
    int hoverRow(const QTableWidget *table) const;

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    using QObject::QObject;

    void armar(QTableWidget *table);
    void atualizarHover(QTableWidget *table, int novaLinha);
    void repintarLinha(QTableWidget *table, int linha) const;

    // tabela -> linha em hover (-1 = nenhuma). A presença de uma chave indica
    // que a tabela já foi armada (idempotência de armar()).
    QHash<const QTableWidget *, int> m_hoverRow;

    // viewport -> tabela dona (mapeamento reverso: MouseMove/Leave chegam com
    // obj == viewport, não a QTableWidget).
    //
    // Valor é QPointer<QWidget> (não QPointer<QTableWidget>) de propósito: a
    // conversão de QPointer<T> para T* é um downcast ESTÁTICO de QObject*.
    // Se um evento chegar no viewport durante o meio de ~QTableWidget() (vptr
    // já rebaixado, mas o QPointer ainda não é null), esse downcast estático
    // é UB (achado por UBSan). Guardando QWidget* (upcast, sempre seguro) e
    // usando qobject_cast<QTableWidget*> no ponto de uso — dinâmico, baseado
    // no metaobject, retorna nullptr com segurança nesse cenário — elimina o
    // UB sem mascarar o sanitizer.
    QHash<QObject *, QPointer<QWidget>> m_viewportTable;
};
