#include "GlossarioDecorator.h"

#include "core/settings/AppSettings.h"
#include "services/GlossarioService.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QWidget>

namespace GlossarioDecorator {

void aplicarTooltips(QWidget *root)
{
    if (!root) return;
    const auto labels = root->findChildren<QLabel*>();
    for (auto *lbl : labels) {
        if (lbl->property("glossarioIgnore").toBool()) continue;
        const QString termo = GlossarioService::primeiroTermoEncontrado(lbl->text());
        if (!termo.isEmpty() && lbl->toolTip().isEmpty()) {
            lbl->setToolTip(GlossarioService::definicao(termo));
        }
    }
    const auto buttons = root->findChildren<QPushButton*>();
    for (auto *b : buttons) {
        const QString termo = GlossarioService::primeiroTermoEncontrado(b->text());
        if (!termo.isEmpty() && b->toolTip().isEmpty()) {
            b->setToolTip(GlossarioService::definicao(termo));
        }
    }
}

void aplicarLabelsColoquiais(QWidget *root)
{
    if (!root) return;
    if (!AppSettings::instance().modoIniciante()) return;

    // Helper para widgets com text()/setText() (QLabel/QPushButton/QCheckBox/QRadioButton).
    auto aplicarText = [](auto *w) {
        if (w->property("glossarioIgnore").toBool()) return;
        const QString termo = GlossarioService::primeiroTermoEncontrado(w->text());
        if (termo.isEmpty()) return;
        const QString coloq = GlossarioService::labelColoquial(termo);
        if (coloq.isEmpty()) return;
        w->setText(coloq);
    };

    for (auto *lbl : root->findChildren<QLabel*>())       aplicarText(lbl);
    for (auto *btn : root->findChildren<QPushButton*>())  aplicarText(btn);
    for (auto *cb  : root->findChildren<QCheckBox*>())    aplicarText(cb);
    for (auto *rb  : root->findChildren<QRadioButton*>()) aplicarText(rb);

    // QGroupBox usa title()/setTitle().
    for (auto *grp : root->findChildren<QGroupBox*>()) {
        const QString termo = GlossarioService::primeiroTermoEncontrado(grp->title());
        if (termo.isEmpty()) continue;
        const QString coloq = GlossarioService::labelColoquial(termo);
        if (coloq.isEmpty()) continue;
        grp->setTitle(coloq);
    }
}

void aplicarMicrocopies(QWidget *root)
{
    if (!root) return;

    // Coletar todos os QFormLayout descendentes (inclusive em filhos diretos).
    const auto layouts = root->findChildren<QFormLayout*>();
    for (auto *form : layouts) {
        // Iterar de tras pra frente: insertRow desloca indices das linhas
        // seguintes mas nao das anteriores.
        for (int row = form->rowCount() - 1; row >= 0; --row) {
            auto *labelItem = form->itemAt(row, QFormLayout::LabelRole);
            auto *fieldItem = form->itemAt(row, QFormLayout::FieldRole);
            if (!labelItem || !fieldItem) continue;

            auto *lbl = qobject_cast<QLabel*>(labelItem->widget());
            if (!lbl) continue;

            const QString termo = GlossarioService::primeiroTermoEncontrado(lbl->text());
            if (termo.isEmpty()) continue;

            const QString micro = GlossarioService::microcopy(termo);
            if (micro.isEmpty()) continue;

            // Idempotencia: se proxima linha ja for microcopy, pula.
            if (row + 1 < form->rowCount()) {
                auto *nextField = form->itemAt(row + 1, QFormLayout::FieldRole);
                if (nextField) {
                    auto *nextLbl = qobject_cast<QLabel*>(nextField->widget());
                    if (nextLbl && nextLbl->property("microcopy").toBool())
                        continue;
                }
            }

            auto *microLbl = new QLabel(micro);
            microLbl->setProperty("microcopy", true);
            microLbl->setStyleSheet(
                QStringLiteral("color: gray; font-size: 11px;"));
            microLbl->setWordWrap(true);
            // SpanningRole ocuparia label+field; usar FieldRole alinha embaixo do campo.
            form->insertRow(row + 1, QString(), microLbl);
        }
    }
}

} // namespace GlossarioDecorator
