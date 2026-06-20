#include "BaseWizardDialog.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>

#include "ui/Theme.h"

BaseWizardDialog::BaseWizardDialog(QWidget *parent)
    : QDialog(parent)
{
    buildUi();
}

void BaseWizardDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Body: sidebar (esq) + paginas (dir)
    auto *body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    m_sidebar = new QListWidget(this);
    m_sidebar->setObjectName(QStringLiteral("wizardSidebar"));
    m_sidebar->setFixedWidth(180);
    m_sidebar->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sidebar->setFocusPolicy(Qt::NoFocus);
    body->addWidget(m_sidebar);

    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    body->addWidget(sep);

    m_pages = new QStackedWidget(this);
    body->addWidget(m_pages, 1);

    root->addLayout(body, 1);

    // Botoes inferiores
    auto *btnSep = new QFrame(this);
    btnSep->setFrameShape(QFrame::HLine);
    btnSep->setFrameShadow(QFrame::Sunken);
    root->addWidget(btnSep);

    auto *btnBar = new QHBoxLayout;
    btnBar->setContentsMargins(12, 8, 12, 8);

    m_btnCancel = new QPushButton(tr("Cancelar"), this);
    m_btnPrev   = new QPushButton(tr("Anterior"), this);
    m_btnPrev->setIcon(Theme::emojiIcon(QStringLiteral("◀️")));
    m_btnNext   = new QPushButton(tr("Próximo"), this);
    m_btnNext->setIcon(Theme::emojiIcon(QStringLiteral("▶️")));
    m_btnNext->setObjectName(QStringLiteral("primary"));

    btnBar->addWidget(m_btnCancel);
    btnBar->addStretch();
    btnBar->addWidget(m_btnPrev);
    btnBar->addWidget(m_btnNext);
    root->addLayout(btnBar);

    connect(m_btnCancel, &QPushButton::clicked, this, [this]() {
        emit cancelado();
        reject();
    });
    connect(m_btnPrev,    &QPushButton::clicked, this, &BaseWizardDialog::onAnterior);
    connect(m_btnNext,    &QPushButton::clicked, this, &BaseWizardDialog::onProximo);
    connect(m_sidebar,    &QListWidget::currentRowChanged,
            this,         &BaseWizardDialog::onSidebarClick);

    atualizarBotoes();
}

void BaseWizardDialog::addStep(const QString &titulo, QWidget *page)
{
    m_sidebar->addItem(titulo);
    m_pages->addWidget(page);
    if (m_pages->count() == 1) {
        m_sidebar->setCurrentRow(0);
    }
    atualizarBotoes();
}

int BaseWizardDialog::stepCount() const
{
    return m_pages ? m_pages->count() : 0;
}

int BaseWizardDialog::currentStep() const
{
    return m_pages ? m_pages->currentIndex() : -1;
}

void BaseWizardDialog::setCurrentStep(int idx)
{
    if (idx < 0 || idx >= stepCount()) return;
    const int anterior = currentStep();
    m_pages->setCurrentIndex(idx);
    m_sidebar->setCurrentRow(idx);
    atualizarBotoes();
    if (anterior != idx) emit mudouPasso(anterior, idx);
}

bool BaseWizardDialog::validarPasso(int /*idx*/)
{
    return true;
}

void BaseWizardDialog::aoConcluir()
{
    // Hook para subclasses; padrao nao faz nada.
}

void BaseWizardDialog::onProximo()
{
    const int idx = currentStep();
    if (!validarPasso(idx)) return;

    if (idx == stepCount() - 1) {
        // Ultima pagina: concluir
        aoConcluir();
        emit concluido();
        accept();
        return;
    }
    setCurrentStep(idx + 1);
}

void BaseWizardDialog::onAnterior()
{
    const int idx = currentStep();
    if (idx > 0) setCurrentStep(idx - 1);
}

void BaseWizardDialog::onSidebarClick(int row)
{
    if (row >= 0 && row != currentStep()) {
        setCurrentStep(row);
    }
}

void BaseWizardDialog::atualizarBotoes()
{
    if (!m_btnPrev || !m_btnNext) return;
    const int idx   = currentStep();
    const int total = stepCount();

    m_btnPrev->setEnabled(idx > 0);
    if (idx == total - 1 && total > 0) {
        m_btnNext->setText(tr("Concluir"));
        m_btnNext->setIcon(Theme::emojiIcon(QStringLiteral("✅")));
    } else {
        m_btnNext->setText(tr("Próximo"));
        m_btnNext->setIcon(Theme::emojiIcon(QStringLiteral("▶️")));
    }
}
