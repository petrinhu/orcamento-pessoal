#include "CalculadoraEdit.h"
#include "ui/Theme.h"
#include "core/CalculadoraHub.h"

#include <QLineEdit>
#include <QToolButton>
#include <QHBoxLayout>
#include <QIcon>

CalculadoraEdit::CalculadoraEdit(QWidget *parent) : QWidget(parent)
{
    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);

    m_edit = new QLineEdit(this);
    lay->addWidget(m_edit, 1);

    m_btn = new QToolButton(this);
    m_btn->setToolTip(tr("Abrir calculadora"));
    m_btn->setCursor(Qt::ArrowCursor);
    m_btn->setFocusPolicy(Qt::NoFocus);
    m_btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    const QIcon icon = QIcon::fromTheme(QStringLiteral("accessories-calculator"));
    if (!icon.isNull()) {
        m_btn->setIcon(icon);
        m_btn->setIconSize(QSize(22, 22));
    } else {
        m_btn->setIcon(Theme::emojiIcon(QStringLiteral("🧮"), 22));
        m_btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }

    lay->addWidget(m_btn);

    connect(m_btn, &QToolButton::clicked, this, [this]() {
        CalculadoraHub::instance().mostrar(m_edit, window());
    });
}

QString CalculadoraEdit::text() const          { return m_edit->text(); }
void CalculadoraEdit::setText(const QString &t) { m_edit->setText(t); }
void CalculadoraEdit::setPlaceholderText(const QString &t) { m_edit->setPlaceholderText(t); }
void CalculadoraEdit::setMaxLength(int max)    { m_edit->setMaxLength(max); }

void CalculadoraEdit::setReadOnly(bool ro)
{
    m_edit->setReadOnly(ro);
    m_btn->setVisible(!ro);
}

bool CalculadoraEdit::isReadOnly() const { return m_edit->isReadOnly(); }
void CalculadoraEdit::clear()            { m_edit->clear(); }
