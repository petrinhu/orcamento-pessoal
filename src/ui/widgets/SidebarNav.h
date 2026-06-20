#pragma once

#include <QMap>
#include <QPainter>
#include <QPaintEvent>
#include <QString>
#include <QWidget>

class QLabel;
class QPushButton;
class QVBoxLayout;

// SidebarNav — barra lateral de navegacao agrupada (#165, redesign Fase 2 Task 1).
//
// Substitui o QTabWidget do topo: agrupa itens de navegacao por categorias
// (grupos), emite chave estavel ao clicar e mantem o estado de item ativo.
//
// Layout (de cima para baixo):
//   [itens + grupos na ordem de insercao]
//   <addStretch>
//   [footer items]
//   [avatar inicial + nome do usuario]
//
// QSS hooks:
//   QPushButton objectName="navItem"         -- item navegavel
//   QPushButton property "ativo"=true/false  -- item ativo (pill colorido, task futura)
//   QLabel      objectName="grupo"           -- rotulo de grupo (nao clicavel)
//
// Fix #165 (paintEvent): o fundo navy e pintado via QPainter no paintEvent,
// contornando a instabilidade de QSS background em QWidget customizado sob
// Fusion/Wayland com stylesheet global ativo. QPainter e deterministico e
// independe de WA_StyledBackground, QPalette::Window ou propagacao de QSS.
class SidebarNav : public QWidget {
    Q_OBJECT
public:
    explicit SidebarNav(QWidget *parent = nullptr);

    // Adiciona rotulo de secao nao-clicavel (ex.: "PRINCIPAL", "RELATORIOS").
    void addGrupo(const QString &titulo);

    // Adiciona item navegavel. emoji vira QIcon via Theme::emojiIcon.
    // Nunca colocar o emoji no texto do botao.
    void addItem(const QString &chave, const QString &emoji, const QString &titulo);

    // Adiciona item de rodape (configuracoes, ajuda etc.).
    void addFooterItem(const QString &chave, const QString &emoji, const QString &titulo);

    // Exibe avatar (inicial em circulo) + nome abaixo dos footer items.
    void setUsuario(const QString &nome);

    // Destaca visualmente o item indicado; desmarca os demais.
    // Usa setProperty("ativo", bool) + style()->polish() para o QSS reagir.
    void setAtivo(const QString &chave);

    // Acessor minimo para testes: devolve o botao associado a chave ou nullptr.
    // Cobre tanto itens normais quanto footer items.
    QPushButton *itemParaTeste(const QString &chave) const;

    // Acessor minimo para testes: texto exibido no circulo de avatar.
    // Retorna a inicial maiuscula do nome atual, ou "?" se vazio.
    QString avatarTexto() const;

signals:
    void navegou(const QString &chave);

protected:
    // Pinta o fundo navy via QPainter (deterministico, independe de QSS/palette/Wayland).
    // Contorna a instabilidade de QWidget custom background sob Fusion + stylesheet global.
    void paintEvent(QPaintEvent *event) override;

private:
    // indice == -1 (default): addWidget; >= 0: insertWidget na posicao dada.
    void registrarItem(const QString &chave, const QString &emoji,
                       const QString &titulo, QVBoxLayout *layout,
                       int indice = -1);

    QVBoxLayout *m_mainLayout   = nullptr; // layout de itens + grupos
    QVBoxLayout *m_footerLayout = nullptr; // layout de footer items + usuario

    QMap<QString, QPushButton *> m_itens; // chave -> botao (todos: normais + footer)

    QLabel *m_avatarLabel = nullptr;
    QLabel *m_nomeLabel   = nullptr;
};
