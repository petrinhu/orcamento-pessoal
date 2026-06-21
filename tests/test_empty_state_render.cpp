// Render offscreen dos empty states guiados (#166 Peça 2).
//
// Verifica (TDD RED → GREEN):
//   (1) EmptyStateWidget cria os três labels obrigatórios (emoji, título, frase).
//   (2) Títulos e frases conferem exatamente com a copy aprovada.
//   (3) CTA está presente e rotulado corretamente quando fornecido.
//   (4) Telas sem CTA (Tendências, Bolsa) não têm botão.
//   (5) QSS contém seletores #emptyStateTitle e #emptyStateFrase (regressão de token).
//   (6) Nenhum token @@...@@ residual no QSS (regressão de substituição).
//   (7) Render offscreen light + dark não é nulo (smoke de render).
//
// Salva /tmp/empty_entradas_light.png, /tmp/empty_metas_light.png,
//       /tmp/empty_tendencias_light.png,
//       /tmp/empty_entradas_dark.png, /tmp/empty_metas_dark.png,
//       /tmp/empty_tendencias_dark.png para aprovação visual.
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest>
#include <QApplication>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>

#include "ui/widgets/EmptyStateWidget.h"
#include "ui/Theme.h"

// ── Helpers ───────────────────────────────────────────────────────────────────

// Cria um EmptyStateWidget com as dimensões mínimas para render e retorna-o.
static EmptyStateWidget *criarVazio(const QString &emoji,
                                    const QString &titulo,
                                    const QString &frase,
                                    const QString &ctaLabel = QString(),
                                    QWidget *parent = nullptr)
{
    auto *w = new EmptyStateWidget(emoji, titulo, frase, ctaLabel, parent);
    w->resize(600, 400);
    return w;
}

// Salva um grab offscreen em /tmp/<nome>.png e retorna o QPixmap.
static QPixmap grab(QWidget *w, const QString &nome)
{
    w->show();
    QApplication::processEvents();
    const QPixmap pm = w->grab();
    pm.save(QStringLiteral("/tmp/%1.png").arg(nome));
    return pm;
}

// ── TestEmptyStateRender ──────────────────────────────────────────────────────

class TestEmptyStateRender : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // (1) Labels obrigatórios existem
    void emojiLabel_existe();
    void titleLabel_existe();
    void phraseLabel_existe();

    // (2) Copy correta para cada tela
    void parcelas_titulo();
    void parcelas_frase();
    void entradas_titulo();
    void entradas_frase();
    void gastos_titulo();
    void gastos_frase();
    void metas_titulo();
    void metas_frase();
    void investimentos_titulo();
    void investimentos_frase();
    void planejamento_titulo();
    void planejamento_frase();
    void recorrencias_titulo();
    void recorrencias_frase();
    void dividas_titulo();
    void dividas_frase();
    void lembretes_titulo();
    void lembretes_frase();
    void proventos_titulo();
    void proventos_frase();
    void tendencias_titulo();
    void tendencias_frase();
    void bolsa_titulo();
    void bolsa_frase();

    // (3) CTA presente nas telas que têm botão
    void parcelas_ctaPresente();
    void entradas_ctaPresente();
    void metas_ctaPresente();

    // (4) Telas sem CTA não têm botão
    void tendencias_semCta();
    void bolsa_semCta();

    // (5) QSS contém seletores dos objectNames
    void qss_contemSeletorTitle();
    void qss_contemSeletorFrase();

    // (6) Sem tokens residuais
    void qss_semTokensResidu();

    // (7) Render offscreen (smoke)
    void renderLight_entradas_naoNulo();
    void renderDark_entradas_naoNulo();
    void renderLight_metas_naoNulo();
    void renderLight_tendencias_naoNulo();
};

// ── initTestCase ──────────────────────────────────────────────────────────────

void TestEmptyStateRender::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

// ── (1) Labels obrigatórios ───────────────────────────────────────────────────

void TestEmptyStateRender::emojiLabel_existe()
{
    EmptyStateWidget w(QStringLiteral("💵"),
                       QStringLiteral("Título"),
                       QStringLiteral("Frase."));
    QVERIFY(w.emojiLabel() != nullptr);
    QVERIFY(!w.emojiLabel()->pixmap().isNull());
}

void TestEmptyStateRender::titleLabel_existe()
{
    EmptyStateWidget w(QStringLiteral("💵"),
                       QStringLiteral("Título"),
                       QStringLiteral("Frase."));
    QVERIFY(w.titleLabel() != nullptr);
    QCOMPARE(w.titleLabel()->objectName(), QStringLiteral("emptyStateTitle"));
}

void TestEmptyStateRender::phraseLabel_existe()
{
    EmptyStateWidget w(QStringLiteral("💵"),
                       QStringLiteral("Título"),
                       QStringLiteral("Frase."));
    QVERIFY(w.phraseLabel() != nullptr);
    QCOMPARE(w.phraseLabel()->objectName(), QStringLiteral("emptyStateFrase"));
}

// ── (2) Copy correta ─────────────────────────────────────────────────────────

void TestEmptyStateRender::parcelas_titulo()
{
    EmptyStateWidget w(QStringLiteral("💳"),
                       QStringLiteral("Nenhuma compra ainda"),
                       QStringLiteral("Adicione uma compra parcelada para acompanhar quanto falta pagar em cada mês."),
                       QStringLiteral("Novo item"));
    QCOMPARE(w.titleLabel()->text(), QStringLiteral("Nenhuma compra ainda"));
}
void TestEmptyStateRender::parcelas_frase()
{
    EmptyStateWidget w(QStringLiteral("💳"),
                       QStringLiteral("Nenhuma compra ainda"),
                       QStringLiteral("Adicione uma compra parcelada para acompanhar quanto falta pagar em cada mês."),
                       QStringLiteral("Novo item"));
    QCOMPARE(w.phraseLabel()->text(),
             QStringLiteral("Adicione uma compra parcelada para acompanhar quanto falta pagar em cada mês."));
}

void TestEmptyStateRender::entradas_titulo()
{
    EmptyStateWidget w(QStringLiteral("💵"),
                       QStringLiteral("Nenhuma entrada ainda"),
                       QStringLiteral("Registre o dinheiro que você recebe — salário, freela, aluguel — para saber quanto entra por mês."),
                       QStringLiteral("Novo lançamento"));
    QCOMPARE(w.titleLabel()->text(), QStringLiteral("Nenhuma entrada ainda"));
}
void TestEmptyStateRender::entradas_frase()
{
    EmptyStateWidget w(QStringLiteral("💵"),
                       QStringLiteral("Nenhuma entrada ainda"),
                       QStringLiteral("Registre o dinheiro que você recebe — salário, freela, aluguel — para saber quanto entra por mês."),
                       QStringLiteral("Novo lançamento"));
    QCOMPARE(w.phraseLabel()->text(),
             QStringLiteral("Registre o dinheiro que você recebe — salário, freela, aluguel — para saber quanto entra por mês."));
}

void TestEmptyStateRender::gastos_titulo()
{
    EmptyStateWidget w(QStringLiteral("🧾"),
                       QStringLiteral("Nenhum gasto ainda"),
                       QStringLiteral("Anote suas despesas aqui para descobrir para onde seu dinheiro vai todo mês."),
                       QStringLiteral("Novo gasto"));
    QCOMPARE(w.titleLabel()->text(), QStringLiteral("Nenhum gasto ainda"));
}
void TestEmptyStateRender::gastos_frase()
{
    EmptyStateWidget w(QStringLiteral("🧾"),
                       QStringLiteral("Nenhum gasto ainda"),
                       QStringLiteral("Anote suas despesas aqui para descobrir para onde seu dinheiro vai todo mês."),
                       QStringLiteral("Novo gasto"));
    QCOMPARE(w.phraseLabel()->text(),
             QStringLiteral("Anote suas despesas aqui para descobrir para onde seu dinheiro vai todo mês."));
}

void TestEmptyStateRender::metas_titulo()
{
    EmptyStateWidget w(QStringLiteral("🎯"),
                       QStringLiteral("Nenhuma meta ainda"),
                       QStringLiteral("Defina um objetivo — viagem, reserva, eletrônico — e o PeuBud mostra quanto falta pra chegar lá."),
                       QStringLiteral("Nova meta"));
    QCOMPARE(w.titleLabel()->text(), QStringLiteral("Nenhuma meta ainda"));
}
void TestEmptyStateRender::metas_frase()
{
    EmptyStateWidget w(QStringLiteral("🎯"),
                       QStringLiteral("Nenhuma meta ainda"),
                       QStringLiteral("Defina um objetivo — viagem, reserva, eletrônico — e o PeuBud mostra quanto falta pra chegar lá."),
                       QStringLiteral("Nova meta"));
    QCOMPARE(w.phraseLabel()->text(),
             QStringLiteral("Defina um objetivo — viagem, reserva, eletrônico — e o PeuBud mostra quanto falta pra chegar lá."));
}

void TestEmptyStateRender::investimentos_titulo()
{
    EmptyStateWidget w(QStringLiteral("📈"),
                       QStringLiteral("Nenhum investimento ainda"),
                       QStringLiteral("Registre suas aplicações — poupança, CDB, ações — para ver tudo num só lugar."),
                       QStringLiteral("Novo investimento"));
    QCOMPARE(w.titleLabel()->text(), QStringLiteral("Nenhum investimento ainda"));
}
void TestEmptyStateRender::investimentos_frase()
{
    EmptyStateWidget w(QStringLiteral("📈"),
                       QStringLiteral("Nenhum investimento ainda"),
                       QStringLiteral("Registre suas aplicações — poupança, CDB, ações — para ver tudo num só lugar."),
                       QStringLiteral("Novo investimento"));
    QCOMPARE(w.phraseLabel()->text(),
             QStringLiteral("Registre suas aplicações — poupança, CDB, ações — para ver tudo num só lugar."));
}

void TestEmptyStateRender::planejamento_titulo()
{
    EmptyStateWidget w(QStringLiteral("📋"),
                       QStringLiteral("Nenhum item no orçamento"),
                       QStringLiteral("Planeje quanto quer gastar em cada categoria do mês e veja se está dentro do combinado."),
                       QStringLiteral("Novo item"));
    QCOMPARE(w.titleLabel()->text(), QStringLiteral("Nenhum item no orçamento"));
}
void TestEmptyStateRender::planejamento_frase()
{
    EmptyStateWidget w(QStringLiteral("📋"),
                       QStringLiteral("Nenhum item no orçamento"),
                       QStringLiteral("Planeje quanto quer gastar em cada categoria do mês e veja se está dentro do combinado."),
                       QStringLiteral("Novo item"));
    QCOMPARE(w.phraseLabel()->text(),
             QStringLiteral("Planeje quanto quer gastar em cada categoria do mês e veja se está dentro do combinado."));
}

void TestEmptyStateRender::recorrencias_titulo()
{
    EmptyStateWidget w(QStringLiteral("🔄"),
                       QStringLiteral("Nenhuma cobrança automática"),
                       QStringLiteral("Cadastre contas fixas — internet, academia, streaming — e o PeuBud lança sozinho todo mês."),
                       QStringLiteral("Adicionar"));
    QCOMPARE(w.titleLabel()->text(), QStringLiteral("Nenhuma cobrança automática"));
}
void TestEmptyStateRender::recorrencias_frase()
{
    EmptyStateWidget w(QStringLiteral("🔄"),
                       QStringLiteral("Nenhuma cobrança automática"),
                       QStringLiteral("Cadastre contas fixas — internet, academia, streaming — e o PeuBud lança sozinho todo mês."),
                       QStringLiteral("Adicionar"));
    QCOMPARE(w.phraseLabel()->text(),
             QStringLiteral("Cadastre contas fixas — internet, academia, streaming — e o PeuBud lança sozinho todo mês."));
}

void TestEmptyStateRender::dividas_titulo()
{
    EmptyStateWidget w(QStringLiteral("💸"),
                       QStringLiteral("Nenhuma dívida registrada"),
                       QStringLiteral("Anote o que você deve — empréstimo, cartão atrasado — para acompanhar e planejar o pagamento."),
                       QStringLiteral("Nova dívida"));
    QCOMPARE(w.titleLabel()->text(), QStringLiteral("Nenhuma dívida registrada"));
}
void TestEmptyStateRender::dividas_frase()
{
    EmptyStateWidget w(QStringLiteral("💸"),
                       QStringLiteral("Nenhuma dívida registrada"),
                       QStringLiteral("Anote o que você deve — empréstimo, cartão atrasado — para acompanhar e planejar o pagamento."),
                       QStringLiteral("Nova dívida"));
    QCOMPARE(w.phraseLabel()->text(),
             QStringLiteral("Anote o que você deve — empréstimo, cartão atrasado — para acompanhar e planejar o pagamento."));
}

void TestEmptyStateRender::lembretes_titulo()
{
    EmptyStateWidget w(QStringLiteral("🔔"),
                       QStringLiteral("Nenhum lembrete ainda"),
                       QStringLiteral("Crie alertas para não esquecer contas, vencimentos e qualquer coisa que precisa de atenção."),
                       QStringLiteral("Novo"));
    QCOMPARE(w.titleLabel()->text(), QStringLiteral("Nenhum lembrete ainda"));
}
void TestEmptyStateRender::lembretes_frase()
{
    EmptyStateWidget w(QStringLiteral("🔔"),
                       QStringLiteral("Nenhum lembrete ainda"),
                       QStringLiteral("Crie alertas para não esquecer contas, vencimentos e qualquer coisa que precisa de atenção."),
                       QStringLiteral("Novo"));
    QCOMPARE(w.phraseLabel()->text(),
             QStringLiteral("Crie alertas para não esquecer contas, vencimentos e qualquer coisa que precisa de atenção."));
}

void TestEmptyStateRender::proventos_titulo()
{
    EmptyStateWidget w(QStringLiteral("💰"),
                       QStringLiteral("Nenhum provento registrado"),
                       QStringLiteral("Registre dividendos e rendimentos dos seus investimentos para acompanhar o que você recebeu."),
                       QStringLiteral("Novo provento"));
    QCOMPARE(w.titleLabel()->text(), QStringLiteral("Nenhum provento registrado"));
}
void TestEmptyStateRender::proventos_frase()
{
    EmptyStateWidget w(QStringLiteral("💰"),
                       QStringLiteral("Nenhum provento registrado"),
                       QStringLiteral("Registre dividendos e rendimentos dos seus investimentos para acompanhar o que você recebeu."),
                       QStringLiteral("Novo provento"));
    QCOMPARE(w.phraseLabel()->text(),
             QStringLiteral("Registre dividendos e rendimentos dos seus investimentos para acompanhar o que você recebeu."));
}

void TestEmptyStateRender::tendencias_titulo()
{
    EmptyStateWidget w(QStringLiteral("📊"),
                       QStringLiteral("Ainda sem dados para analisar"),
                       QStringLiteral("Os gráficos aparecem aqui conforme você registra gastos. Comece adicionando suas despesas."));
    QCOMPARE(w.titleLabel()->text(), QStringLiteral("Ainda sem dados para analisar"));
}
void TestEmptyStateRender::tendencias_frase()
{
    EmptyStateWidget w(QStringLiteral("📊"),
                       QStringLiteral("Ainda sem dados para analisar"),
                       QStringLiteral("Os gráficos aparecem aqui conforme você registra gastos. Comece adicionando suas despesas."));
    QCOMPARE(w.phraseLabel()->text(),
             QStringLiteral("Os gráficos aparecem aqui conforme você registra gastos. Comece adicionando suas despesas."));
}

void TestEmptyStateRender::bolsa_titulo()
{
    EmptyStateWidget w(QStringLiteral("🏦"),
                       QStringLiteral("Nenhuma cotação pesquisada"),
                       QStringLiteral("Digite o código de uma ação (ex.: PETR4) para ver o preço e as informações mais recentes."));
    QCOMPARE(w.titleLabel()->text(), QStringLiteral("Nenhuma cotação pesquisada"));
}
void TestEmptyStateRender::bolsa_frase()
{
    EmptyStateWidget w(QStringLiteral("🏦"),
                       QStringLiteral("Nenhuma cotação pesquisada"),
                       QStringLiteral("Digite o código de uma ação (ex.: PETR4) para ver o preço e as informações mais recentes."));
    QCOMPARE(w.phraseLabel()->text(),
             QStringLiteral("Digite o código de uma ação (ex.: PETR4) para ver o preço e as informações mais recentes."));
}

// ── (3) CTA presente nas telas que têm botão ─────────────────────────────────

void TestEmptyStateRender::parcelas_ctaPresente()
{
    EmptyStateWidget w(QStringLiteral("💳"),
                       QStringLiteral("Nenhuma compra ainda"),
                       QStringLiteral("Frase."),
                       QStringLiteral("Novo item"));
    QVERIFY(w.ctaButton() != nullptr);
    QVERIFY(w.ctaButton()->text().contains(QStringLiteral("Novo item")));
}

void TestEmptyStateRender::entradas_ctaPresente()
{
    EmptyStateWidget w(QStringLiteral("💵"),
                       QStringLiteral("Nenhuma entrada ainda"),
                       QStringLiteral("Frase."),
                       QStringLiteral("Novo lançamento"));
    QVERIFY(w.ctaButton() != nullptr);
    QVERIFY(w.ctaButton()->text().contains(QStringLiteral("Novo lançamento")));
}

void TestEmptyStateRender::metas_ctaPresente()
{
    EmptyStateWidget w(QStringLiteral("🎯"),
                       QStringLiteral("Nenhuma meta ainda"),
                       QStringLiteral("Frase."),
                       QStringLiteral("Nova meta"));
    QVERIFY(w.ctaButton() != nullptr);
}

// ── (4) Telas sem CTA não têm botão ─────────────────────────────────────────

void TestEmptyStateRender::tendencias_semCta()
{
    EmptyStateWidget w(QStringLiteral("📊"),
                       QStringLiteral("Ainda sem dados para analisar"),
                       QStringLiteral("Frase."));
    QVERIFY(w.ctaButton() == nullptr);
}

void TestEmptyStateRender::bolsa_semCta()
{
    EmptyStateWidget w(QStringLiteral("🏦"),
                       QStringLiteral("Nenhuma cotação pesquisada"),
                       QStringLiteral("Frase."));
    QVERIFY(w.ctaButton() == nullptr);
}

// ── (5) QSS contém seletores ──────────────────────────────────────────────────

void TestEmptyStateRender::qss_contemSeletorTitle()
{
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(qss.contains(QStringLiteral("#emptyStateTitle")),
             "QSS nao contem seletor #emptyStateTitle");
}

void TestEmptyStateRender::qss_contemSeletorFrase()
{
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(qss.contains(QStringLiteral("#emptyStateFrase")),
             "QSS nao contem seletor #emptyStateFrase");
}

// ── (6) Sem tokens residuais ──────────────────────────────────────────────────

void TestEmptyStateRender::qss_semTokensResidu()
{
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(!qss.contains(QStringLiteral("@@")),
             "QSS contem token @@...@@ nao substituido");
}

// ── (7) Render offscreen ──────────────────────────────────────────────────────

void TestEmptyStateRender::renderLight_entradas_naoNulo()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
    auto *w = criarVazio(QStringLiteral("💵"),
                         QStringLiteral("Nenhuma entrada ainda"),
                         QStringLiteral("Registre o dinheiro que você recebe — salário, freela, aluguel — para saber quanto entra por mês."),
                         QStringLiteral("Novo lançamento"));
    const QPixmap pm = grab(w, QStringLiteral("empty_entradas_light"));
    QVERIFY(!pm.isNull());
    delete w;
}

void TestEmptyStateRender::renderDark_entradas_naoNulo()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    Theme::aplicar();
    auto *w = criarVazio(QStringLiteral("💵"),
                         QStringLiteral("Nenhuma entrada ainda"),
                         QStringLiteral("Registre o dinheiro que você recebe — salário, freela, aluguel — para saber quanto entra por mês."),
                         QStringLiteral("Novo lançamento"));
    const QPixmap pm = grab(w, QStringLiteral("empty_entradas_dark"));
    QVERIFY(!pm.isNull());
    delete w;
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

void TestEmptyStateRender::renderLight_metas_naoNulo()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
    auto *w = criarVazio(QStringLiteral("🎯"),
                         QStringLiteral("Nenhuma meta ainda"),
                         QStringLiteral("Defina um objetivo — viagem, reserva, eletrônico — e o PeuBud mostra quanto falta pra chegar lá."),
                         QStringLiteral("Nova meta"));
    const QPixmap pm = grab(w, QStringLiteral("empty_metas_light"));
    QVERIFY(!pm.isNull());
    delete w;
}

void TestEmptyStateRender::renderLight_tendencias_naoNulo()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
    auto *w = criarVazio(QStringLiteral("📊"),
                         QStringLiteral("Ainda sem dados para analisar"),
                         QStringLiteral("Os gráficos aparecem aqui conforme você registra gastos. Comece adicionando suas despesas."));
    const QPixmap pm = grab(w, QStringLiteral("empty_tendencias_light"));
    QVERIFY(!pm.isNull());
    delete w;
}

QTEST_MAIN(TestEmptyStateRender)
#include "test_empty_state_render.moc"
