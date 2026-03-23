#pragma once
#include <QObject>
#include <QPointer>

class CalculadoraDialog;
class QLineEdit;
class QWidget;

// Singleton that owns the single CalculadoraDialog instance and routes
// results back to whichever monetary field invoked the calculator.
class CalculadoraHub final : public QObject {
    Q_OBJECT
public:
    static CalculadoraHub &instance();

    // Open calculator tied to a specific QLineEdit — result is injected back.
    void mostrar(QLineEdit *invoker, QWidget *parent = nullptr);

    // Open calculator standalone (from toolbar / menu) — no field injection.
    void mostrarGlobal(QWidget *parent = nullptr);

    // Close and hide the calculator (called when main window closes).
    void fechar();

signals:
    void resultadoDisponivel(double valor);

private:
    explicit CalculadoraHub();
    void ensureDialog();
    void onResultado(double valor);

    CalculadoraDialog   *m_dialog  = nullptr;
    QPointer<QLineEdit>  m_invoker;
};
