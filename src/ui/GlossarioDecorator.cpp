#include "GlossarioDecorator.h"

#include "GlossaryUnderlineFilter.h"
#include "core/settings/AppSettings.h"
#include "services/GlossarioService.h"

#include <QCheckBox>
#include <QCursor>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QWidget>

namespace GlossarioDecorator {

// aplicarTooltips (#166 Peca 3 — §9.6 mecanismo)
//
// Nao-destrutivo: nunca altera text() nem setText() — so seta toolTip() e cursor.
// Affordanca de "termo explicavel": cursor WhatsThisCursor (?) nos QLabel que
// recebem tooltip, sinalizando "passe o mouse para saber mais". Em Qt Widgets o
// sublinhado pontilhado por-palavra exigiria converter o label para rich-text HTML,
// o que seria destrutivo (altera text()). O cursor e a alternativa nao-destrutiva
// viavel — especialmente util em desktops onde cursor e visivel (#166 decisao).
//
// O tooltip escuro (fundo navy #1B2A41 do QToolTip) e definido no QSS global em
// Theme::gerarQss() — os tooltips serao escuros automaticamente ao usar setToolTip().
namespace {

// Decide se o label deve receber o sublinhado pontilhado ESTÁTICO (§9.6) além
// do tooltip. Critério de elegância: o cue marca um TERMO, não rica uma frase.
// Sublinhar a largura inteira de um label de prosa longa ("Taxa CET ao ano com
// IOF incluído…") ficaria pesado. Decoramos quando o texto é essencialmente o
// termo — label-chave curto (ex.: "CET:", "PV:", "Dividend Yield (DY)").
//
// Regra (robusta, sem falso-positivo em prosa):
//   - texto plano (rich-text é descartado pelo helper de pintura), E
//   - texto curto: ≤ 28 caracteres visíveis, OU o termo cobre ≥ 55% do texto.
bool deveSublinhar(const QString &texto, const QString &termo)
{
    if (termo.isEmpty()) return false;
    const QString t = texto.trimmed();
    if (t.isEmpty()) return false;

    if (t.length() <= 28)
        return true;

    // Texto longo: só se o termo dominar (label-chave com sufixo explicativo).
    const double cobertura = static_cast<double>(termo.length()) / t.length();
    return cobertura >= 0.55;
}

} // namespace

void aplicarTooltips(QWidget *root)
{
    if (!root) return;
    const auto labels = root->findChildren<QLabel*>();
    for (auto *lbl : labels) {
        if (lbl->property("glossarioIgnore").toBool()) continue;
        const QString termo = GlossarioService::primeiroTermoEncontrado(lbl->text());
        if (termo.isEmpty()) continue;

        if (lbl->toolTip().isEmpty()) {
            lbl->setToolTip(GlossarioService::definicao(termo));
            // Affordanca: cursor de interrogacao indica "tem explicacao aqui".
            // Nao-destrutivo: nao altera text(), nao altera estrutura do widget.
            lbl->setCursor(Qt::WhatsThisCursor);
        }

        // Cue estático (§9.6): sublinhado pontilhado nos labels-chave curtos.
        // Cobertura AMPLA — mesma varredura dos tooltips. Não-destrutivo (event
        // filter pinta por cima; não troca classe nem mexe no layout).
        if (deveSublinhar(lbl->text(), termo))
            GlossaryUnderlineFilter::decorar(lbl);
    }
    const auto buttons = root->findChildren<QPushButton*>();
    for (auto *b : buttons) {
        const QString termo = GlossarioService::primeiroTermoEncontrado(b->text());
        if (!termo.isEmpty() && b->toolTip().isEmpty()) {
            b->setToolTip(GlossarioService::definicao(termo));
            // Botoes ja tem cursor de ponteiro; nao sobrescrever (nao acrescenta).
            // Sem sublinhado em botões: já têm chrome próprio (borda/sombra).
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
