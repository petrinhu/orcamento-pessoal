// PeuBud - teste do skin compartilhado das tabelas (#167 T1, §11 manual de estilo)
//
// Cobre:
//   1. QSS: sem zebra poluída em QTableWidget (alternate-background-color ==
//      fundo, não o antigo tom de "linha alternada"); sem grade vertical
//      (gridline transparente); divisória só horizontal fina no item;
//      cabeçalho com borda-bottom 2px accent + maiúsculas, sem borda vertical.
//   2. Densidade: default Confortável (48px); toggle Compacta (34px) via
//      Theme::setDensidadeTabelasCompacta()/aplicarDensidadeTabela(s)().
//   3. AppSettings::tabelaDensidadeCompacta(): persistência do toggle
//      (round-trip + restauração do valor original — não polui config real).
//   4. TableHoverFilter: rastreia a linha certa sob o mouse (MouseMove/Leave
//      sintéticos no viewport) — lógica pura, headless (offscreen). O hover
//      em si é interativo e foi validado também no display real (ver report).
//   5. Render offscreen (smoke) light+dark de uma tabela populada, salvos em
//      /tmp/tabela_t1_{light,dark}.png para aprovação visual.
//
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest>
#include <QApplication>
#include <QFont>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPixmap>
#include <QSettings>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTemporaryDir>

#include "core/settings/AppSettings.h"
#include "ui/TableHoverDelegate.h"
#include "ui/TableHoverFilter.h"
#include "ui/Theme.h"

namespace {

// Cria uma tabela populada (5 linhas de dado + 1 linha TOTAL), imitando o
// padrão de GastosWidget/RecorrenciaWidget — usada nos testes de render e de
// rastreio de hover. setAlternatingRowColors(true) simula os ~24 widgets que
// ainda chamam isso: o QSS deve neutralizar visualmente o efeito (#167 T1).
QTableWidget *criarTabelaDemo(QWidget *parent = nullptr)
{
    auto *t = new QTableWidget(6, 3, parent);
    t->setHorizontalHeaderLabels({QStringLiteral("Descrição"), QStringLiteral("Categoria"), QStringLiteral("Valor")});
    t->verticalHeader()->setVisible(false);
    t->setAlternatingRowColors(true);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);

    static const char *descricoes[] = {"Aluguel", "Internet", "Supermercado", "Streaming", "Combustível"};
    static const char *categorias[] = {"Moradia", "Utilidades", "Alimentação", "Lazer", "Transporte"};
    static const char *valores[]    = {"R$ 1.800,00", "R$ 120,00", "R$ 650,00", "R$ 55,90", "R$ 300,00"};
    for (int r = 0; r < 5; ++r) {
        t->setItem(r, 0, new QTableWidgetItem(QString::fromUtf8(descricoes[r])));
        t->setItem(r, 1, new QTableWidgetItem(QString::fromUtf8(categorias[r])));
        t->setItem(r, 2, new QTableWidgetItem(QString::fromUtf8(valores[r])));
    }
    auto *totalLabel = new QTableWidgetItem(QStringLiteral("TOTAL"));
    auto *totalValor = new QTableWidgetItem(QStringLiteral("R$ 2.925,90"));
    QFont fb = totalLabel->font();
    fb.setBold(true);
    totalLabel->setFont(fb);
    totalValor->setFont(fb);
    t->setItem(5, 0, totalLabel);
    t->setItem(5, 2, totalValor);

    t->resize(420, 320);
    return t;
}

// Simula um MouseMove no viewport da tabela, na posição vertical central da
// linha `row`. Passa pelo TableHoverFilter (instalado em qApp) exatamente
// como um movimento real do mouse passaria.
void moverMousePara(QTableWidget *t, int row)
{
    QWidget *vp = t->viewport();
    const int y = t->rowViewportPosition(row) + t->rowHeight(row) / 2;
    const QPoint pos(10, y);
    QMouseEvent me(QEvent::MouseMove, QPointF(pos), QPointF(vp->mapToGlobal(pos)),
                   Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(vp, &me);
}

} // namespace

class TestTableSkin : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup(); // roda após CADA teste — restaura densidade em memória

    // ── QSS: skin compartilhado (§11 manual de estilo) ─────────────────────
    void qss_semZebraPoluida();
    void qss_semGridVertical();
    void qss_itemTemDivisoriaHorizontal();
    void qss_headerBordaBottomAccent2px();
    void qss_headerMaiusculas();
    void qss_headerSemBordaVertical();

    // ── Densidade ───────────────────────────────────────────────────────────
    void densidade_defaultConfortavel();
    void densidade_toggleAlteraAltura();
    void densidade_aplicarDensidadeTabela_setaDefaultSectionSize();
    void densidade_aplicarDensidadeTabelas_afetaTodasAsTabelasVivas();

    // ── AppSettings: persistência do toggle ─────────────────────────────────
    void appSettings_tabelaDensidadeCompacta_roundTrip();

    // ── TableHoverFilter: lógica de rastreio de linha (headless) ───────────
    void hoverFilter_semHoverInicial();
    void hoverFilter_marcaLinhaSobMouse();
    void hoverFilter_atualizaAoMoverDeLinha();
    void hoverFilter_leaveLimpaHover();
    void hoverFilter_armaDensidadeEDelegateNoShow();

    // ── Render offscreen (smoke) ────────────────────────────────────────────
    void renderLight_naoNulo();
    void renderDark_naoNulo();

private:
    QTemporaryDir m_settingsDir;
};

void TestTableSkin::initTestCase()
{
    // #198 (achado durante a varredura de A11Y-01/03): appSettings_tabela-
    // DensidadeCompacta_roundTrip() escreve em QSettings("petrinhu","PeuBud")
    // real — o save/restore do valor original mitiga mas não isola (residual
    // se o teste falhar entre o write e o restore). Fix: mesmo padrão de
    // QSettings::setPath (Ini+Native) usado em test_locale_moeda (#197).
    QVERIFY(m_settingsDir.isValid());
    QSettings::setPath(QSettings::IniFormat,    QSettings::UserScope, m_settingsDir.path());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());

    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

void TestTableSkin::cleanup()
{
    Theme::setDensidadeTabelasCompacta(false);
}

// ── QSS ──────────────────────────────────────────────────────────────────────

void TestTableSkin::qss_semZebraPoluida()
{
    const QString qss = Theme::qssParaTeste();
    const int pos = qss.indexOf(QStringLiteral("QTableWidget {"));
    QVERIFY2(pos >= 0, "Regra QTableWidget ausente no QSS");
    const int abre = qss.indexOf('{', pos);
    const int fecha = qss.indexOf('}', abre);
    QVERIFY(abre >= 0 && fecha > abre);
    const QString bloco = qss.mid(abre, fecha - abre);

    // alternate-background-color deve ser IGUAL ao background-color (zebra
    // neutralizada) — nunca o antigo tom de "linha alternada" (Surface2).
    QVERIFY2(bloco.contains(QStringLiteral("alternate-background-color: %1").arg(Theme::surface())),
             "alternate-background-color não usa o mesmo tom do fundo — zebra ainda poluída (#167 T1)");
    QVERIFY2(!bloco.contains(QStringLiteral("#F7FAFD")),
             "QTableWidget ainda referencia o antigo tom de zebra (Surface2 #F7FAFD)");
}

void TestTableSkin::qss_semGridVertical()
{
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(qss.contains(QStringLiteral("gridline-color: transparent;")),
             "gridline-color não está transparente — grade vertical ainda visível (#167 T1)");
}

void TestTableSkin::qss_itemTemDivisoriaHorizontal()
{
    const QString qss = Theme::qssParaTeste();
    const int pos = qss.indexOf(QStringLiteral("QTableWidget::item {"));
    QVERIFY2(pos >= 0, "Regra QTableWidget::item ausente no QSS");
    const int abre = qss.indexOf('{', pos);
    const int fecha = qss.indexOf('}', abre);
    QVERIFY(abre >= 0 && fecha > abre);
    const QString bloco = qss.mid(abre, fecha - abre);

    QVERIFY2(bloco.contains(QStringLiteral("border-bottom: 1px solid %1").arg(Theme::borda())),
             "QTableWidget::item sem divisória horizontal fina (border-bottom, §11)");
}

void TestTableSkin::qss_headerBordaBottomAccent2px()
{
    const QString qss = Theme::qssParaTeste();
    const int pos = qss.indexOf(QStringLiteral("QHeaderView::section {"));
    QVERIFY2(pos >= 0, "Regra QHeaderView::section ausente no QSS");
    const int abre = qss.indexOf('{', pos);
    const int fecha = qss.indexOf('}', abre);
    QVERIFY(abre >= 0 && fecha > abre);
    const QString bloco = qss.mid(abre, fecha - abre);

    QVERIFY2(bloco.contains(QStringLiteral("border-bottom: 2px solid %1").arg(Theme::acento())),
             "Cabeçalho sem borda-bottom 2px accent (§11)");
}

void TestTableSkin::qss_headerMaiusculas()
{
    const QString qss = Theme::qssParaTeste();
    const int pos = qss.indexOf(QStringLiteral("QHeaderView::section {"));
    QVERIFY(pos >= 0);
    const int abre = qss.indexOf('{', pos);
    const int fecha = qss.indexOf('}', abre);
    const QString bloco = qss.mid(abre, fecha - abre);

    QVERIFY2(bloco.contains(QStringLiteral("text-transform: uppercase")),
             "Cabeçalho sem maiúsculas pequenas (§11: 12px SemiBold uppercase)");
}

void TestTableSkin::qss_headerSemBordaVertical()
{
    const QString qss = Theme::qssParaTeste();
    const int pos = qss.indexOf(QStringLiteral("QHeaderView::section {"));
    QVERIFY(pos >= 0);
    const int abre = qss.indexOf('{', pos);
    const int fecha = qss.indexOf('}', abre);
    const QString bloco = qss.mid(abre, fecha - abre);

    QVERIFY2(!bloco.contains(QStringLiteral("border-right")),
             "Cabeçalho ainda com borda vertical entre colunas — só divisórias horizontais (§11)");
}

// ── Densidade ──────────────────────────────────────────────────────────────

void TestTableSkin::densidade_defaultConfortavel()
{
    QCOMPARE(Theme::alturaLinhaTabela(), Theme::kAlturaLinhaConfortavel);
    QCOMPARE(Theme::kAlturaLinhaConfortavel, 48);
}

void TestTableSkin::densidade_toggleAlteraAltura()
{
    Theme::setDensidadeTabelasCompacta(true);
    QCOMPARE(Theme::alturaLinhaTabela(), Theme::kAlturaLinhaCompacta);
    QCOMPARE(Theme::kAlturaLinhaCompacta, 34);

    Theme::setDensidadeTabelasCompacta(false);
    QCOMPARE(Theme::alturaLinhaTabela(), Theme::kAlturaLinhaConfortavel);
}

void TestTableSkin::densidade_aplicarDensidadeTabela_setaDefaultSectionSize()
{
    QTableWidget t(3, 2);

    Theme::setDensidadeTabelasCompacta(false);
    Theme::aplicarDensidadeTabela(&t);
    QCOMPARE(t.verticalHeader()->defaultSectionSize(), Theme::kAlturaLinhaConfortavel);

    Theme::setDensidadeTabelasCompacta(true);
    Theme::aplicarDensidadeTabela(&t);
    QCOMPARE(t.verticalHeader()->defaultSectionSize(), Theme::kAlturaLinhaCompacta);
}

void TestTableSkin::densidade_aplicarDensidadeTabelas_afetaTodasAsTabelasVivas()
{
    QTableWidget t1(2, 2);
    QTableWidget t2(2, 2);

    Theme::setDensidadeTabelasCompacta(false);
    Theme::aplicarDensidadeTabelas();
    QCOMPARE(t1.verticalHeader()->defaultSectionSize(), Theme::kAlturaLinhaConfortavel);
    QCOMPARE(t2.verticalHeader()->defaultSectionSize(), Theme::kAlturaLinhaConfortavel);

    // Toggle "ao vivo": reaplica sem precisar reiniciar/recriar as tabelas.
    Theme::setDensidadeTabelasCompacta(true);
    Theme::aplicarDensidadeTabelas();
    QCOMPARE(t1.verticalHeader()->defaultSectionSize(), Theme::kAlturaLinhaCompacta);
    QCOMPARE(t2.verticalHeader()->defaultSectionSize(), Theme::kAlturaLinhaCompacta);
}

// ── AppSettings ────────────────────────────────────────────────────────────

void TestTableSkin::appSettings_tabelaDensidadeCompacta_roundTrip()
{
    const bool original = AppSettings::instance().tabelaDensidadeCompacta();

    AppSettings::instance().setTabelaDensidadeCompacta(true);
    QVERIFY(AppSettings::instance().tabelaDensidadeCompacta());

    AppSettings::instance().setTabelaDensidadeCompacta(false);
    QVERIFY(!AppSettings::instance().tabelaDensidadeCompacta());

    AppSettings::instance().setTabelaDensidadeCompacta(original); // não polui config real
}

// ── TableHoverFilter ───────────────────────────────────────────────────────

void TestTableSkin::hoverFilter_semHoverInicial()
{
    TableHoverFilter::install();
    auto *t = criarTabelaDemo();
    t->show();
    QApplication::processEvents();

    QCOMPARE(TableHoverFilter::instance()->hoverRow(t), -1);
    delete t;
}

void TestTableSkin::hoverFilter_marcaLinhaSobMouse()
{
    TableHoverFilter::install();
    auto *t = criarTabelaDemo();
    t->show();
    QApplication::processEvents();

    moverMousePara(t, 2);
    QCOMPARE(TableHoverFilter::instance()->hoverRow(t), 2);

    delete t;
}

void TestTableSkin::hoverFilter_atualizaAoMoverDeLinha()
{
    TableHoverFilter::install();
    auto *t = criarTabelaDemo();
    t->show();
    QApplication::processEvents();

    moverMousePara(t, 1);
    QCOMPARE(TableHoverFilter::instance()->hoverRow(t), 1);

    moverMousePara(t, 4);
    QCOMPARE(TableHoverFilter::instance()->hoverRow(t), 4);

    delete t;
}

void TestTableSkin::hoverFilter_leaveLimpaHover()
{
    TableHoverFilter::install();
    auto *t = criarTabelaDemo();
    t->show();
    QApplication::processEvents();

    moverMousePara(t, 0);
    QCOMPARE(TableHoverFilter::instance()->hoverRow(t), 0);

    QEvent leaveEv(QEvent::Leave);
    QApplication::sendEvent(t->viewport(), &leaveEv);
    QCOMPARE(TableHoverFilter::instance()->hoverRow(t), -1);

    delete t;
}

void TestTableSkin::hoverFilter_armaDensidadeEDelegateNoShow()
{
    TableHoverFilter::install();
    Theme::setDensidadeTabelasCompacta(true);

    auto *t = criarTabelaDemo();
    t->show();
    QApplication::processEvents();

    QCOMPARE(t->verticalHeader()->defaultSectionSize(), Theme::kAlturaLinhaCompacta);
    QVERIFY2(qobject_cast<TableHoverDelegate *>(t->itemDelegate()) != nullptr,
             "TableHoverFilter não instalou o TableHoverDelegate no Show/Polish");

    delete t;
}

// ── Render offscreen (smoke) ──────────────────────────────────────────────────

void TestTableSkin::renderLight_naoNulo()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();

    auto *t = criarTabelaDemo();
    t->show();
    QApplication::processEvents();
    const QPixmap pm = t->grab();
    QVERIFY2(!pm.isNull(), "Render light da tabela nulo — crash ou widget invisível");
    pm.save(QStringLiteral("/tmp/tabela_t1_light.png"));
    delete t;
}

void TestTableSkin::renderDark_naoNulo()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    Theme::aplicar();

    auto *t = criarTabelaDemo();
    t->show();
    QApplication::processEvents();
    const QPixmap pm = t->grab();
    QVERIFY2(!pm.isNull(), "Render dark da tabela nulo — crash ou widget invisível");
    pm.save(QStringLiteral("/tmp/tabela_t1_dark.png"));
    delete t;

    // Restaura light para não poluir outros testes
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

QTEST_MAIN(TestTableSkin)
#include "test_table_skin.moc"
