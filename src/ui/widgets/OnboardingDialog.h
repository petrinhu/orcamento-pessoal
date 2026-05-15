#pragma once

#include <QDialog>

class QStackedWidget;
class QPushButton;
class QLabel;

// OnboardingDialog (#109) — wizard first-run, 3 telas.
//
// Exibido apenas na primeira sessao apos login (controlado por
// AppSettings::onboardingFeito; default false). Apresenta:
//   1. Boas-vindas + politica de privacidade (tudo local, criptografado)
//   2. 3 conceitos basicos: Receita / Despesa / Investimento
//   3. Proximos passos + sugestao do Modo Avancado em Preferencias
//
// Botao "Pular" sempre visivel. "Pular" e "Concluir" ambos marcam a flag
// onboardingFeito para true.
class OnboardingDialog final : public QDialog {
    Q_OBJECT
public:
    explicit OnboardingDialog(QWidget *parent = nullptr);

private slots:
    void onProximo();
    void onVoltar();
    void onPular();

private:
    void buildUi();
    QWidget *makePagina1();
    QWidget *makePagina2();
    QWidget *makePagina3();
    void atualizarBotoes();

    QStackedWidget *m_stack         = nullptr;
    QPushButton    *m_btnVoltar     = nullptr;
    QPushButton    *m_btnProximo    = nullptr;
    QPushButton    *m_btnPular      = nullptr;
    QLabel         *m_lblIndicador  = nullptr;
};
