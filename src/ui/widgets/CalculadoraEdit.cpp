#include "CalculadoraEdit.h"
#include "core/CalculadoraHub.h"

#include <QToolButton>
#include <QResizeEvent>

CalculadoraEdit::CalculadoraEdit(QWidget *parent) : QLineEdit(parent)
{
    m_btn = new QToolButton(this);
    m_btn->setObjectName(QStringLiteral("calcEditBtn"));
    m_btn->setText(QStringLiteral("⊞"));
    m_btn->setCursor(Qt::ArrowCursor);
    m_btn->setFocusPolicy(Qt::NoFocus);
    m_btn->setFixedSize(22, 22);
    m_btn->setToolTip(tr("Abrir calculadora"));
    connect(m_btn, &QToolButton::clicked, this, [this]() {
        CalculadoraHub::instance().mostrar(this, window());
    });
}

void CalculadoraEdit::setReadOnly(bool ro)
{
    QLineEdit::setReadOnly(ro);
    m_btn->setVisible(!ro);
    ajustarBotao();
}

void CalculadoraEdit::resizeEvent(QResizeEvent *e)
{
    QLineEdit::resizeEvent(e);
    ajustarBotao();
}

void CalculadoraEdit::ajustarBotao()
{
    if (!m_btn->isVisible()) {
        setTextMargins(0, 0, 0, 0);
        return;
    }
    const int pad  = qMax(2, (height() - m_btn->height()) / 2);
    const int btnW = m_btn->width();
    m_btn->move(width() - btnW - pad, (height() - m_btn->height()) / 2);
    setTextMargins(0, 0, btnW + pad + 2, 0);
}
