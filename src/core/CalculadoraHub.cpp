#include "CalculadoraHub.h"
#include "ui/widgets/CalculadoraDialog.h"
#include "ui/widgets/WizardGuiadoDialog.h"
#include "services/CalculadoraService.h"
#include "core/settings/AppSettings.h"

#include <QLineEdit>

CalculadoraHub &CalculadoraHub::instance()
{
    static CalculadoraHub inst;
    return inst;
}

CalculadoraHub::CalculadoraHub() : QObject(nullptr) {}

void CalculadoraHub::ensureDialog()
{
    if (!m_dialog) {
        m_dialog = new CalculadoraDialog(nullptr); // top-level, non-modal
        connect(m_dialog, &CalculadoraDialog::resultadoDisponivel,
                this, &CalculadoraHub::onResultado);
        connect(m_dialog, &CalculadoraDialog::transferirSolicitado,
                this, [this](double valor) {
            if (m_invoker)
                m_invoker->setText(CalculadoraService::formatarDisplay(valor, 2));
            m_invoker = nullptr;
            m_dialog->setTemInvoker(false);
            m_dialog->hide();
            emit resultadoDisponivel(valor);
        });
        connect(m_dialog, &CalculadoraDialog::tvmParaCartoes,
                this, &CalculadoraHub::tvmParaCartoes);
    }
}

void CalculadoraHub::mostrar(QLineEdit *invoker, QWidget *parent)
{
    Q_UNUSED(parent)
    ensureDialog();
    m_invoker = invoker;
    m_dialog->setTemInvoker(invoker != nullptr);
    if (invoker)
        m_dialog->setValorInicial(CalculadoraService::parsearDisplay(invoker->text()));
    m_dialog->show();
    m_dialog->raise();
    m_dialog->activateWindow();
}

void CalculadoraHub::mostrarGlobal(QWidget *parent)
{
    // #108 - Modo Iniciante substitui calculadora densa por wizard guiado
    // pergunta-a-pergunta. Apenas no acesso global (toolbar): campos
    // monetarios usam mostrar(invoker, ...) e mantem fluxo da calculadora.
    if (AppSettings::instance().modoIniciante()) {
        WizardGuiadoDialog wiz(parent);
        wiz.exec();
        return;
    }
    mostrar(nullptr, parent);
}

void CalculadoraHub::fechar()
{
    if (m_dialog)
        m_dialog->hide();
}

void CalculadoraHub::onResultado(double valor)
{
    if (m_invoker)
        m_invoker->setText(CalculadoraService::formatarDisplay(valor, 2));
    emit resultadoDisponivel(valor);
}
