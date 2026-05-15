#include "OnboardingDialog.h"

#include "core/settings/AppSettings.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

OnboardingDialog::OnboardingDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Bem-vindo ao PeuBud"));
    setModal(true);
    setMinimumSize(560, 380);
    buildUi();
    atualizarBotoes();
}

void OnboardingDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 16);
    root->setSpacing(16);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(makePagina1());
    m_stack->addWidget(makePagina2());
    m_stack->addWidget(makePagina3());
    root->addWidget(m_stack, /*stretch*/ 1);

    auto *footer = new QHBoxLayout;
    footer->setSpacing(8);

    m_lblIndicador = new QLabel(this);
    m_lblIndicador->setStyleSheet(QStringLiteral("color: gray;"));
    footer->addWidget(m_lblIndicador);
    footer->addStretch(1);

    m_btnPular = new QPushButton(tr("Pular"), this);
    m_btnVoltar = new QPushButton(tr("Voltar"), this);
    m_btnProximo = new QPushButton(tr("Próximo"), this);
    m_btnProximo->setDefault(true);

    footer->addWidget(m_btnPular);
    footer->addWidget(m_btnVoltar);
    footer->addWidget(m_btnProximo);

    root->addLayout(footer);

    connect(m_btnPular,   &QPushButton::clicked, this, &OnboardingDialog::onPular);
    connect(m_btnVoltar,  &QPushButton::clicked, this, &OnboardingDialog::onVoltar);
    connect(m_btnProximo, &QPushButton::clicked, this, &OnboardingDialog::onProximo);
}

QWidget *OnboardingDialog::makePagina1()
{
    auto *w = new QWidget;
    auto *lay = new QVBoxLayout(w);
    lay->setSpacing(12);

    auto *titulo = new QLabel(tr("Bem-vindo ao PeuBud! 🎉"), w);
    QFont fTit = titulo->font();
    fTit.setPointSize(fTit.pointSize() + 6);
    fTit.setBold(true);
    titulo->setFont(fTit);

    auto *corpo = new QLabel(w);
    corpo->setWordWrap(true);
    corpo->setTextFormat(Qt::RichText);
    corpo->setText(tr(
        "<p>Seu <b>assistente financeiro pessoal</b> em C++/Qt — leve, rápido "
        "e sem nuvem.</p>"
        "<ul>"
        "<li>🔒 Tudo fica no <b>seu computador</b>, criptografado (AES-256).</li>"
        "<li>📶 <b>Funciona offline</b> — sem login obrigatório em servidor externo.</li>"
        "<li>🐧 100% software livre, Fedora/Debian/Arch.</li>"
        "</ul>"
        "<p>Vamos te mostrar o essencial em 3 telas. Você pode pular a qualquer "
        "momento.</p>"));

    lay->addWidget(titulo);
    lay->addWidget(corpo);
    lay->addStretch(1);
    return w;
}

QWidget *OnboardingDialog::makePagina2()
{
    auto *w = new QWidget;
    auto *lay = new QVBoxLayout(w);
    lay->setSpacing(12);

    auto *titulo = new QLabel(tr("3 conceitos básicos"), w);
    QFont fTit = titulo->font();
    fTit.setPointSize(fTit.pointSize() + 6);
    fTit.setBold(true);
    titulo->setFont(fTit);

    auto *corpo = new QLabel(w);
    corpo->setWordWrap(true);
    corpo->setTextFormat(Qt::RichText);
    corpo->setText(tr(
        "<p>Tudo no PeuBud gira em torno de três ideias:</p>"
        "<ul>"
        "<li>💰 <b>Receita</b> — o que <b>entra</b> (salário, freelas, aluguéis recebidos).</li>"
        "<li>💸 <b>Despesa</b> — o que <b>sai</b> (contas, cartão, mercado, lazer).</li>"
        "<li>📈 <b>Investimento</b> — o que <b>cresce com o tempo</b> (CDB, ações, FIIs).</li>"
        "</ul>"
        "<p>O objetivo é simples: <b>Receita &gt; Despesa</b>, e a diferença vira "
        "Investimento.</p>"));

    lay->addWidget(titulo);
    lay->addWidget(corpo);
    lay->addStretch(1);
    return w;
}

QWidget *OnboardingDialog::makePagina3()
{
    auto *w = new QWidget;
    auto *lay = new QVBoxLayout(w);
    lay->setSpacing(12);

    auto *titulo = new QLabel(tr("Pronto para começar! ✓"), w);
    QFont fTit = titulo->font();
    fTit.setPointSize(fTit.pointSize() + 6);
    fTit.setBold(true);
    titulo->setFont(fTit);

    auto *corpo = new QLabel(w);
    corpo->setWordWrap(true);
    corpo->setTextFormat(Qt::RichText);
    corpo->setText(tr(
        "<p>Algumas sugestões de primeiros passos:</p>"
        "<ol>"
        "<li>🎯 Vá na aba <b>Entradas</b> e cadastre seu salário ou fonte principal.</li>"
        "<li>🛒 Use <b>Gastos</b> para registrar despesas avulsas e <b>Cartões</b> para parcelas.</li>"
        "<li>📊 O <b>Dashboard</b> mostra o resumo de tudo, mês a mês.</li>"
        "</ol>"
        "<p><b>Dica:</b> em <b>Preferências → Geral → Apresentação</b> você pode trocar para "
        "<b>Modo Avançado</b> se preferir ver siglas técnicas (CET, FIRE, DY, IRPF…) "
        "em vez dos rótulos coloquiais.</p>"
        "<p>Bons estudos financeiros! 📚</p>"));

    lay->addWidget(titulo);
    lay->addWidget(corpo);
    lay->addStretch(1);
    return w;
}

void OnboardingDialog::atualizarBotoes()
{
    const int idx   = m_stack->currentIndex();
    const int total = m_stack->count();
    m_btnVoltar->setEnabled(idx > 0);
    m_btnProximo->setText(idx == total - 1 ? tr("Concluir") : tr("Próximo"));
    m_lblIndicador->setText(tr("%1 / %2").arg(idx + 1).arg(total));
    // Em "Concluir" o botao Pular vira redundante — esconde.
    m_btnPular->setVisible(idx < total - 1);
}

void OnboardingDialog::onProximo()
{
    const int idx   = m_stack->currentIndex();
    const int total = m_stack->count();
    if (idx == total - 1) {
        // Conclui: marca flag e aceita.
        AppSettings::instance().setOnboardingFeito(true);
        accept();
        return;
    }
    m_stack->setCurrentIndex(idx + 1);
    atualizarBotoes();
}

void OnboardingDialog::onVoltar()
{
    const int idx = m_stack->currentIndex();
    if (idx == 0) return;
    m_stack->setCurrentIndex(idx - 1);
    atualizarBotoes();
}

void OnboardingDialog::onPular()
{
    // Pular tambem marca como feito — nao re-exibir em proximas sessoes.
    AppSettings::instance().setOnboardingFeito(true);
    reject();
}
