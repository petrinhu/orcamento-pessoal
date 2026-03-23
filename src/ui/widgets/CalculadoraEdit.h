#pragma once
#include <QLineEdit>

class QToolButton;

// QLineEdit subclass that embeds a small ⊞ button on the trailing edge.
// Clicking the button opens CalculadoraHub's calculator pre-filled with the
// current field value; accepting the result writes it back to the field.
// The button is hidden automatically when the field is read-only.
class CalculadoraEdit final : public QLineEdit {
    Q_OBJECT
public:
    explicit CalculadoraEdit(QWidget *parent = nullptr);

    // Override to show/hide the embedded button.
    // (setReadOnly is not virtual in QLineEdit, but since we store as
    //  CalculadoraEdit* everywhere it compiles to the right override.)
    void setReadOnly(bool ro);

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    void ajustarBotao();
    QToolButton *m_btn = nullptr;
};
