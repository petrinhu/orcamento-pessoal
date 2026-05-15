#include "WizardGuiadoDialog.h"

#include "services/FinanceEngine.h"
#include "services/FiscalService.h"

#include <QDoubleSpinBox>
#include <QLabel>
#include <QLocale>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWizardPage>

#include <cmath>

namespace {

// QWizardPage com registerField publico (o original eh protected, impedindo
// uso a partir de helpers nao-membros).
class Pagina final : public QWizardPage {
public:
    using QWizardPage::QWizardPage;
    using QWizardPage::registerField;
};

// Helper: cria pagina simples com pergunta + 1 campo numerico (double).
QWizardPage *paginaPerguntaDouble(const QString &titulo,
                                  const QString &pergunta,
                                  const QString &fieldName,
                                  const QString &sufixo,
                                  double minimo,
                                  double maximo,
                                  double passo,
                                  int decimais = 2)
{
    auto *page = new Pagina;
    page->setTitle(titulo);
    auto *v = new QVBoxLayout(page);
    auto *lblPergunta = new QLabel(pergunta, page);
    lblPergunta->setWordWrap(true);
    lblPergunta->setTextFormat(Qt::RichText);
    lblPergunta->setStyleSheet(QStringLiteral("font-size: 14px; padding: 12px;"));
    v->addWidget(lblPergunta);

    auto *spn = new QDoubleSpinBox(page);
    spn->setRange(minimo, maximo);
    spn->setDecimals(decimais);
    spn->setSingleStep(passo);
    if (!sufixo.isEmpty())
        spn->setSuffix(sufixo);
    spn->setStyleSheet(QStringLiteral("font-size: 16px; padding: 6px;"));
    v->addWidget(spn);
    v->addStretch();
    page->registerField(fieldName + QStringLiteral("*"), spn,
                        "value", SIGNAL(valueChanged(double)));
    return page;
}

// Helper: cria pagina com pergunta + QSpinBox (inteiro).
QWizardPage *paginaPerguntaInt(const QString &titulo,
                                const QString &pergunta,
                                const QString &fieldName,
                                const QString &sufixo,
                                int minimo,
                                int maximo,
                                int valorPadrao)
{
    auto *page = new Pagina;
    page->setTitle(titulo);
    auto *v = new QVBoxLayout(page);
    auto *lblPergunta = new QLabel(pergunta, page);
    lblPergunta->setWordWrap(true);
    lblPergunta->setTextFormat(Qt::RichText);
    lblPergunta->setStyleSheet(QStringLiteral("font-size: 14px; padding: 12px;"));
    v->addWidget(lblPergunta);

    auto *spn = new QSpinBox(page);
    spn->setRange(minimo, maximo);
    spn->setValue(valorPadrao);
    if (!sufixo.isEmpty())
        spn->setSuffix(sufixo);
    spn->setStyleSheet(QStringLiteral("font-size: 16px; padding: 6px;"));
    v->addWidget(spn);
    v->addStretch();
    page->registerField(fieldName + QStringLiteral("*"), spn);
    return page;
}

// Formata valor monetario com pt_BR (R$ 1.234,56).
QString brl(double v)
{
    const QLocale ptBR(QLocale::Portuguese, QLocale::Brazil);
    return QStringLiteral("R$ ") + ptBR.toString(v, 'f', 2);
}

// Pagina final que recalcula no initializePage().
class PaginaResultado final : public QWizardPage {
public:
    using Calcular = std::function<QString(const QWizard *)>;

    PaginaResultado(const QString &titulo, Calcular calc, QWidget *parent = nullptr)
        : QWizardPage(parent), m_calc(std::move(calc))
    {
        setTitle(titulo);
        setFinalPage(true);
        auto *v = new QVBoxLayout(this);
        m_lbl = new QLabel(this);
        m_lbl->setWordWrap(true);
        m_lbl->setTextFormat(Qt::RichText);
        m_lbl->setStyleSheet(QStringLiteral("font-size: 13px; padding: 12px;"));
        v->addWidget(m_lbl);
        v->addStretch();
    }

    void initializePage() override
    {
        m_lbl->setText(m_calc(wizard()));
    }

private:
    QLabel *m_lbl = nullptr;
    Calcular m_calc;
};

} // namespace

WizardGuiadoDialog::WizardGuiadoDialog(QWidget *parent) : QWizard(parent)
{
    setWindowTitle(tr("Assistente financeiro"));
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::HaveHelpButton, false);
    setOption(QWizard::NoBackButtonOnStartPage, true);
    resize(580, 440);

    // ── Pagina 0: escolha do fluxo ────────────────────────────────────────
    auto *p0 = new Pagina;
    p0->setTitle(tr("O que você quer calcular?"));
    p0->setSubTitle(tr("Escolha um assistente. Vamos perguntar passo a passo."));
    auto *v0 = new QVBoxLayout(p0);
    auto *rbReserva = new QRadioButton(
        tr("Reserva de Emergência — quanto guardar para imprevistos"), p0);
    auto *rbFire = new QRadioButton(
        tr("Aposentadoria Antecipada — quando posso parar de trabalhar (FIRE)"), p0);
    auto *rbCartao = new QRadioButton(
        tr("Cartão Parcelado — quanto vou pagar de juros"), p0);
    auto *rbIrpf = new QRadioButton(
        tr("Imposto de Renda — quanto vou pagar ao Leão"), p0);
    rbReserva->setChecked(true);
    v0->addWidget(rbReserva);
    v0->addWidget(rbFire);
    v0->addWidget(rbCartao);
    v0->addWidget(rbIrpf);
    v0->addStretch();
    p0->registerField(QStringLiteral("fluxoReserva"), rbReserva);
    p0->registerField(QStringLiteral("fluxoFire"),    rbFire);
    p0->registerField(QStringLiteral("fluxoCartao"),  rbCartao);
    p0->registerField(QStringLiteral("fluxoIrpf"),    rbIrpf);
    setPage(Escolher, p0);

    // ── Fluxo Reserva Emergencia (2 perguntas + resultado) ────────────────
    setPage(ReservaEmergencia, paginaPerguntaDouble(
        tr("Reserva de Emergência — passo 1 de 2"),
        tr("Quanto você gasta por mês em <b>despesas essenciais</b>?<br>"
           "<i>aluguel + comida + contas básicas + transporte</i>"),
        QStringLiteral("reservaDesp"), QStringLiteral(" R$"),
        0.0, 1'000'000.0, 100.0));

    setPage(ReservaMeses, paginaPerguntaInt(
        tr("Reserva de Emergência — passo 2 de 2"),
        tr("Quantos meses de despesa você quer guardar?<br>"
           "<i>Recomendação: 6 meses se CLT estável; 12 meses se autônomo/PJ</i>"),
        QStringLiteral("reservaMeses"), QStringLiteral(" meses"),
        1, 60, 6));

    setPage(ResultadoReserva, new PaginaResultado(
        tr("Sua meta de Reserva de Emergência"),
        [](const QWizard *w) {
            const double desp = w->field(QStringLiteral("reservaDesp")).toDouble();
            const int meses = w->field(QStringLiteral("reservaMeses")).toInt();
            const double meta = desp * static_cast<double>(meses);
            return WizardGuiadoDialog::tr(
                "<h2>Você precisa de %1</h2>"
                "<p>= %2 × %3 meses</p>"
                "<p style='color: gray'>Recomendação geral: 6 meses para "
                "CLT estável, 12 meses para autônomo/PJ. Mantenha o dinheiro "
                "em aplicação <b>liquidez diária</b> (Tesouro Selic, CDB liquidez "
                "diária, ou poupança).</p>")
                .arg(brl(meta), brl(desp), QString::number(meses));
        }));

    // ── Fluxo FIRE (1 pergunta + resultado, regra dos 25) ─────────────────
    setPage(Aposentadoria, paginaPerguntaDouble(
        tr("Aposentadoria Antecipada (FIRE)"),
        tr("Quanto você gasta por mês <b>hoje</b> (estilo de vida atual)?<br>"
           "<i>Vamos calcular o patrimônio que rende isso para sempre, "
           "usando a regra dos 25 (taxa segura de retirada de 4% a.a.)</i>"),
        QStringLiteral("fireDesp"), QStringLiteral(" R$"),
        0.0, 1'000'000.0, 100.0));

    setPage(ResultadoFire, new PaginaResultado(
        tr("Sua meta de Aposentadoria"),
        [](const QWizard *w) {
            const double despMensal = w->field(QStringLiteral("fireDesp")).toDouble();
            const double despAnual = despMensal * 12.0;
            const double patrimonio = despAnual * 25.0;
            return WizardGuiadoDialog::tr(
                "<h2>Você precisa de %1</h2>"
                "<p>= %2/mês × 12 meses × 25 anos</p>"
                "<p>Com esse patrimônio investido a 4% real ao ano, você pode "
                "retirar <b>%2/mês</b> indefinidamente (regra dos 25 / "
                "Trinity Study).</p>"
                "<p style='color: gray'>Premissa: carteira diversificada com "
                "retorno real (acima da inflação) de 4-7% a.a. Para um plano "
                "personalizado considere idade, expectativa de vida e "
                "aposentadoria pública (INSS).</p>")
                .arg(brl(patrimonio), brl(despMensal));
        }));

    // ── Fluxo Cartao Parcelado (3 perguntas + resultado) ──────────────────
    setPage(CartaoValor, paginaPerguntaDouble(
        tr("Cartão Parcelado — passo 1 de 3"),
        tr("Qual o <b>valor total</b> da compra?<br>"
           "<i>Valor à vista (sem juros)</i>"),
        QStringLiteral("cartaoValor"), QStringLiteral(" R$"),
        0.01, 1'000'000.0, 100.0));

    setPage(CartaoParcelas, paginaPerguntaInt(
        tr("Cartão Parcelado — passo 2 de 3"),
        tr("Em quantas parcelas você vai pagar?"),
        QStringLiteral("cartaoN"), QStringLiteral("x"),
        1, 360, 12));

    setPage(CartaoJuros, paginaPerguntaDouble(
        tr("Cartão Parcelado — passo 3 de 3"),
        tr("Qual o <b>juros mensal</b> cobrado?<br>"
           "<i>Procure no contrato/fatura: \"taxa de juros ao mês\". "
           "Se for parcelamento \"sem juros\" da loja, digite 0.</i>"),
        QStringLiteral("cartaoI"), QStringLiteral(" % a.m."),
        0.0, 50.0, 0.1, 4));

    setPage(ResultadoCartao, new PaginaResultado(
        tr("Custo real do parcelamento"),
        [](const QWizard *w) {
            const double pv = w->field(QStringLiteral("cartaoValor")).toDouble();
            const int n = w->field(QStringLiteral("cartaoN")).toInt();
            const double iPct = w->field(QStringLiteral("cartaoI")).toDouble();
            const double i = iPct / 100.0;

            double parcela;
            double total;
            double juros;
            if (i <= 0.0) {
                parcela = pv / static_cast<double>(n);
                total = pv;
                juros = 0.0;
            } else {
                // FinanceEngine::pmt assina convencao TVM: pv positivo,
                // retorna pmt negativo (saida de caixa) -> usar -pmt.
                parcela = -FinanceEngine::pmt(pv, 0.0, i, n, false);
                total = parcela * static_cast<double>(n);
                juros = total - pv;
            }

            const double iAnual = (i > 0.0)
                ? (std::pow(1.0 + i, 12.0) - 1.0) * 100.0
                : 0.0;

            return WizardGuiadoDialog::tr(
                "<h2>Parcela mensal: %1</h2>"
                "<p>Total pago em %2 meses: <b>%3</b></p>"
                "<p>Juros total: <b>%4</b> (taxa anual equivalente: %5%)</p>"
                "<p style='color: gray'>Dica: se a taxa for &gt; 2% a.m., considere "
                "guardar e pagar à vista. Cartão de crédito rotativo no Brasil "
                "tem média de 14% a.m. (~400% a.a.).</p>")
                .arg(brl(parcela))
                .arg(n)
                .arg(brl(total))
                .arg(brl(juros))
                .arg(QString::number(iAnual, 'f', 2));
        }));

    // ── Fluxo IRPF (1 pergunta + resultado) ───────────────────────────────
    setPage(SimulacaoIrpf, paginaPerguntaDouble(
        tr("Simulação IRPF"),
        tr("Qual seu <b>rendimento bruto anual</b> (CLT)?<br>"
           "<i>Soma de todos os salários do ano (13 + férias inclusos). "
           "Vamos descontar INSS e aplicar a tabela 2024.</i>"),
        QStringLiteral("irpfAnual"), QStringLiteral(" R$"),
        0.0, 100'000'000.0, 1000.0));

    setPage(ResultadoIrpf, new PaginaResultado(
        tr("Estimativa de Imposto de Renda"),
        [](const QWizard *w) {
            const double rendaAnual = w->field(QStringLiteral("irpfAnual")).toDouble();
            const double rendaMensal = rendaAnual / 12.0;

            const qint64 brutoCent = static_cast<qint64>(std::llround(rendaMensal * 100.0));
            const qint64 inssCent = FiscalService::inssMensal2024(brutoCent);
            const qint64 baseCent = brutoCent - inssCent;
            const qint64 irMensalCent = FiscalService::irpfMensal2024(baseCent);
            const qint64 irAnualCent = irMensalCent * 12;

            const double inss = inssCent / 100.0;
            const double irMensal = irMensalCent / 100.0;
            const double irAnual = irAnualCent / 100.0;
            const double liquidoMensal = rendaMensal - inss - irMensal;
            const double aliquota = (rendaAnual > 0.0)
                ? (irAnual / rendaAnual) * 100.0
                : 0.0;

            return WizardGuiadoDialog::tr(
                "<h2>IR a pagar (ano): %1</h2>"
                "<p>= aproximadamente %2 por mês</p>"
                "<p>Renda bruta mensal: %3<br>"
                "INSS mensal: %4<br>"
                "Base de cálculo: %5<br>"
                "IR mensal: %2<br>"
                "<b>Líquido mensal: %6</b></p>"
                "<p>Alíquota efetiva: <b>%7%</b> sobre renda bruta anual.</p>"
                "<p style='color: gray'>Cálculo simplificado (tabela 2024, "
                "sem deduções de dependentes/saúde/educação/previdência). "
                "Para declaração real use o programa da Receita.</p>")
                .arg(brl(irAnual))
                .arg(brl(irMensal))
                .arg(brl(rendaMensal))
                .arg(brl(inss))
                .arg(brl(baseCent / 100.0))
                .arg(brl(liquidoMensal))
                .arg(QString::number(aliquota, 'f', 2));
        }));
}

int WizardGuiadoDialog::nextId() const
{
    const int cur = currentId();
    switch (cur) {
    case Escolher:
        if (field(QStringLiteral("fluxoReserva")).toBool()) return ReservaEmergencia;
        if (field(QStringLiteral("fluxoFire")).toBool())    return Aposentadoria;
        if (field(QStringLiteral("fluxoCartao")).toBool())  return CartaoValor;
        if (field(QStringLiteral("fluxoIrpf")).toBool())    return SimulacaoIrpf;
        return -1;

    // Reserva: 2 perguntas -> Resultado
    case ReservaEmergencia: return ReservaMeses;
    case ReservaMeses:      return ResultadoReserva;
    case ResultadoReserva:  return -1;

    // FIRE: 1 pergunta -> Resultado
    case Aposentadoria:     return ResultadoFire;
    case ResultadoFire:     return -1;

    // Cartao: 3 perguntas -> Resultado
    case CartaoValor:       return CartaoParcelas;
    case CartaoParcelas:    return CartaoJuros;
    case CartaoJuros:       return ResultadoCartao;
    case ResultadoCartao:   return -1;

    // IRPF: 1 pergunta -> Resultado
    case SimulacaoIrpf:     return ResultadoIrpf;
    case ResultadoIrpf:     return -1;

    default: return -1;
    }
}
