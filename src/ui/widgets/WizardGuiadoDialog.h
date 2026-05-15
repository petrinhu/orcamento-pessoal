#pragma once

#include <QWizard>

// WizardGuiadoDialog (#108) - 4 wizards guiados pergunta-a-pergunta
// para modo Iniciante. Cobre Reserva Emergencia, FIRE, Cartao Parcelado,
// IRPF. Cada wizard eh selecionado na pagina inicial (escolha).
//
// Cada fluxo segue o padrao:
//   1 pergunta por pagina (QLabel grande + QDoubleSpinBox/QSpinBox)
//   pagina final exibe o resultado calculado via FinanceEngine/FiscalService.
//
// O dialog substitui CalculadoraDialog quando AppSettings::modoIniciante()
// retorna true; ver CalculadoraHub::mostrarGlobal.
class WizardGuiadoDialog final : public QWizard {
    Q_OBJECT
public:
    enum FluxoId {
        Escolher          = 0,
        ReservaEmergencia = 1,   // pergunta 1: despesa mensal
        ReservaMeses      = 2,   // pergunta 2: meses de reserva
        Aposentadoria     = 3,   // pergunta unica: despesa mensal (FIRE 25x)
        CartaoValor       = 4,   // pergunta 1: valor da compra
        CartaoParcelas    = 5,   // pergunta 2: numero de parcelas
        CartaoJuros       = 6,   // pergunta 3: juros mensal
        SimulacaoIrpf     = 7,   // pergunta unica: rendimento anual
        ResultadoReserva  = 10,
        ResultadoFire     = 11,
        ResultadoCartao   = 12,
        ResultadoIrpf     = 13,
    };

    explicit WizardGuiadoDialog(QWidget *parent = nullptr);

    int nextId() const override;
};
