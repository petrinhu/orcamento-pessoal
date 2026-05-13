#pragma once

#include <QDialog>
#include <QVector>

class QListWidget;
class QStackedWidget;
class QPushButton;

// BaseWizardDialog (#87) - dialogo base para wizards multi-passo nao-lineares.
//
// Layout: sidebar de steps a esquerda + QStackedWidget de paginas a direita +
// botoes Anterior/Proximo (ou Concluir na ultima)/Cancelar embaixo.
//
// Padrao de kmymoney/wizards/kmymoneywizard.h: usa QDialog puro em vez de
// QWizard (mais flexivel para casos nao-lineares dos wizards do Bloco 2).
//
// Uso:
//   class MeuWizard : public BaseWizardDialog {
//   public:
//       MeuWizard(QWidget *parent) : BaseWizardDialog(parent) {
//           addStep(tr("Dados"),      construirDados());
//           addStep(tr("Calculos"),   construirCalculos());
//           addStep(tr("Resumo"),     construirResumo());
//       }
//   protected:
//       bool validarPasso(int idx) override { ... }
//   };
class BaseWizardDialog : public QDialog {
    Q_OBJECT
public:
    explicit BaseWizardDialog(QWidget *parent = nullptr);

    // Adiciona um passo. Ordem de chamada = ordem de exibicao.
    // Toma posse do widget (parent passa a ser o stack).
    void addStep(const QString &titulo, QWidget *page);

    [[nodiscard]] int stepCount() const;
    [[nodiscard]] int currentStep() const;
    void setCurrentStep(int idx);

signals:
    void concluido();           // emitido apos validarPasso() OK na ultima pagina
    void cancelado();
    void mudouPasso(int idxAnterior, int idxNovo);

protected:
    // Override para validar antes de avancar. Retornar true permite; false bloqueia.
    // Padrao: aceita sempre. Subclasse pode mostrar QMessageBox antes de retornar.
    virtual bool validarPasso(int idx);

    // Override para acao customizada ao concluir (alem de emitir concluido()).
    virtual void aoConcluir();

private slots:
    void onProximo();
    void onAnterior();
    void onSidebarClick(int row);

private:
    void buildUi();
    void atualizarBotoes();

    QListWidget    *m_sidebar = nullptr;
    QStackedWidget *m_pages   = nullptr;
    QPushButton    *m_btnPrev = nullptr;
    QPushButton    *m_btnNext = nullptr;
    QPushButton    *m_btnCancel = nullptr;
};
