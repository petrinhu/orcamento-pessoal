// Render offscreen do SidebarNav (#165 contraste fix).
// Exercita as DUAS hipoteses documentadas no task-fase2-t6:
//   H1: fundo navy via QSS + QPalette combinados (caminho normal).
//   H2: fundo navy via QPalette sozinha (gradiente QSS falha por qualquer razao).
// Salva PNGs em /tmp para inspecao visual manual:
//   /tmp/sidebar_fix_h1.png  — QSS + palette (render normal)
//   /tmp/sidebar_fix_h2.png  — palette apenas (sem gradiente QSS)
// Assertions: verifica que o pixel central da sidebar e dark (navy),
// nao claro (fundo app default #EEF3F9).

#include <QtTest>
#include <QApplication>
#include <QPixmap>
#include <QColor>

#include "ui/widgets/SidebarNav.h"
#include "ui/Theme.h"
#include "ui/NavRegistry.h"

class TestSidebarRender : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void h1_fundoNavyComQss();
    void h2_fundoNavySemGradiente();
};

// ── helper ──────────────────────────────────────────────────────────────────

static SidebarNav *criarNavCompleto()
{
    auto *nav = new SidebarNav;
    nav->resize(230, 600);

    // Popula com os 13 itens + grupos + footer (espelha NavRegistry::todos()).
    nav->addGrupo("Principal");
    nav->addItem("tab_gastos",     "💳", "Gastos");
    nav->addItem("tab_entradas",   "💵", "Entradas");
    nav->addItem("tab_resumo",     "📊", "Resumo");
    nav->addGrupo("Investimentos");
    nav->addItem("tab_investimentos", "📈", "Investimentos");
    nav->addItem("tab_metas",      "🎯", "Metas");
    nav->addGrupo("Ferramentas");
    nav->addItem("tab_calculadora","🧮", "Calculadora");
    nav->addItem("tab_lembretes",  "🔔", "Lembretes");
    nav->addItem("tab_orcamento",  "📋", "Orçamento");
    nav->addGrupo("Relatórios");
    nav->addItem("tab_relatorios", "📑", "Relatórios");
    nav->addItem("tab_fechamento", "🗓️", "Fechamento");
    nav->addItem("tab_exportar",   "📤", "Exportar");
    nav->addItem("tab_importar",   "📥", "Importar");
    nav->addGrupo("Cadastros");
    nav->addItem("tab_cartoes",    "💳", "Cartões");

    nav->addFooterItem("tab_prefs",  "⚙️", "Preferências");
    nav->addFooterItem("tab_ajuda",  "❓", "Ajuda");
    nav->addFooterItem("tab_tema",   "🎨", "Tema");

    nav->setUsuario("Petrus");
    nav->setAtivo("tab_gastos");

    return nav;
}

// Retorna true se a cor e "escura" (luminancia < 0.25, ou seja, navy/dark).
static bool isEscuro(const QColor &c)
{
    // Luminancia relativa simplificada: media linearizada dos canais.
    // Navy #1B2A41 tem R=27,G=42,B=65 → media ≈ 45 → 45/255 ≈ 0.176 → escuro.
    // Fundo app #EEF3F9 tem R=238,G=243,B=249 → media ≈ 243 → 0.953 → claro.
    const double lum = (c.red() + c.green() + c.blue()) / 3.0 / 255.0;
    return lum < 0.25;
}

// ── initTestCase ──────────────────────────────────────────────────────────────

void TestSidebarRender::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    // Aplica QSS + palette via aplicar() (precisa de QApplication, ja existente).
    Theme::aplicar();
}

// ── H1: QSS + palette (caminho normal) ──────────────────────────────────────

void TestSidebarRender::h1_fundoNavyComQss()
{
    QScopedPointer<SidebarNav> nav(criarNavCompleto());

    // QSS + palette normais: aplicar() ja chamou qApp->setStyleSheet() + setPalette().
    nav->show();
    nav->repaint();

    // grab() renderiza o widget em offscreen.
    QPixmap px = nav->grab();
    QVERIFY2(!px.isNull(), "grab() retornou pixmap nulo — render falhou");

    // Salva para inspecao manual.
    const bool saved = px.save(QStringLiteral("/tmp/sidebar_fix_h1.png"));
    QVERIFY2(saved, "Falha ao salvar /tmp/sidebar_fix_h1.png");

    // Pixel central da sidebar deve ser navy (escuro).
    const QImage img = px.toImage();
    const QColor pixelCentro = img.pixelColor(115, 300);  // centro ~(x=115, y=300)
    QVERIFY2(isEscuro(pixelCentro),
             qPrintable(QStringLiteral(
                 "H1: pixel central nao eh navy — cor: %1 (R=%2 G=%3 B=%4). "
                 "Gradiente QSS ou palette nao propagaram.")
                 .arg(pixelCentro.name())
                 .arg(pixelCentro.red())
                 .arg(pixelCentro.green())
                 .arg(pixelCentro.blue())));
}

// ── H2: sem gradiente QSS — somente QPalette (fallback) ─────────────────────

void TestSidebarRender::h2_fundoNavySemGradiente()
{
    QScopedPointer<SidebarNav> nav(criarNavCompleto());

    // Simula falha do gradiente: remove o QSS para que o gradiente
    // QWidget#sidebarNav { background: qlineargradient... } nao seja aplicado.
    // O fundo deve continuar navy via QPalette::Window definida no construtor.
    nav->setStyleSheet(QString()); // limpa QSS do widget (o qApp QSS ainda esta la,
                                   // mas aqui testamos o palette como fallback
                                   // quando o gradiente falha por razao de contexto).

    // Forca QPalette navy explicita (replica o que o construtor faz — garante
    // que o teste nao depende do estado do qApp palette).
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(Theme::sidebar1()));
    nav->setPalette(pal);
    nav->setAutoFillBackground(true);

    nav->show();
    nav->repaint();

    QPixmap px = nav->grab();
    QVERIFY2(!px.isNull(), "grab() retornou pixmap nulo — render falhou");

    const bool saved = px.save(QStringLiteral("/tmp/sidebar_fix_h2.png"));
    QVERIFY2(saved, "Falha ao salvar /tmp/sidebar_fix_h2.png");

    const QImage img = px.toImage();
    const QColor pixelCentro = img.pixelColor(115, 300);
    QVERIFY2(isEscuro(pixelCentro),
             qPrintable(QStringLiteral(
                 "H2 (sem gradiente): pixel central nao eh navy via palette — "
                 "cor: %1 (R=%2 G=%3 B=%4). QPalette fallback nao funcionou.")
                 .arg(pixelCentro.name())
                 .arg(pixelCentro.red())
                 .arg(pixelCentro.green())
                 .arg(pixelCentro.blue())));
}

QTEST_MAIN(TestSidebarRender)
#include "test_sidebar_render.moc"
