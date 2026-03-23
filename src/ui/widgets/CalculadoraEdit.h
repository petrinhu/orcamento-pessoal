#pragma once
#include <QWidget>

class QLineEdit;
class QToolButton;

// Composite widget: QLineEdit + calculator button side by side.
// The button opens CalculadoraHub pre-filled with the current value;
// the result is injected back into the internal QLineEdit.
// The button is hidden when the field is set read-only.
class CalculadoraEdit final : public QWidget {
    Q_OBJECT
public:
    explicit CalculadoraEdit(QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);
    void setPlaceholderText(const QString &text);
    void setReadOnly(bool ro);
    bool isReadOnly() const;
    void clear();
    void setMaxLength(int max);

    QLineEdit *lineEdit() const { return m_edit; }

private:
    QLineEdit   *m_edit = nullptr;
    QToolButton *m_btn  = nullptr;
};
