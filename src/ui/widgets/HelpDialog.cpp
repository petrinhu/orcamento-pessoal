#include "HelpDialog.h"
#include "ui/TabHelp.h"

#include <QVBoxLayout>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QFile>
#include <QStringList>

namespace {

// Monta a seção "Ajuda por aba" a partir do MESMO registro que alimenta os
// TabShells (fonte única, DRY). Cresce sozinha conforme entradas são
// adicionadas em TabHelp.cpp; mostra apenas o que houver.
QString montarAjudaPorAba()
{
    const auto &mapa = tabHelp();
    if (mapa.isEmpty()) return {};

    QString html = QStringLiteral("<h2>Ajuda por aba</h2>");
    for (const QString &key : tabHelpOrder()) {
        if (!mapa.contains(key)) continue;
        const TabHelpEntry &e = mapa.value(key);

        html += QStringLiteral("<h3>%1</h3>").arg(e.titulo.toHtmlEscaped());
        if (!e.descricao.trimmed().isEmpty())
            html += QStringLiteral("<p>%1</p>").arg(e.descricao);

        if (!e.passos.isEmpty()) {
            html += QStringLiteral("<ol>");
            for (const QString &passo : e.passos)
                html += QStringLiteral("<li>%1</li>").arg(passo);
            html += QStringLiteral("</ol>");
        }
    }
    return html;
}

} // namespace

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent)
{
    // #197 (I18N-03): "Ajuda" traduzível; "PeuBud" é marca, fica fora do tr().
    setWindowTitle(tr("Ajuda — %1").arg(QStringLiteral("PeuBud")));
    resize(720, 560);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 12);

    auto *browser = new QTextBrowser(this);
    browser->setOpenExternalLinks(false);

    // Conteúdo base (qrc) + seção gerada do registro de abas, no mesmo documento.
    QString base;
    QFile f(QStringLiteral(":/help/help.html"));
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
        base = QString::fromUtf8(f.readAll());

    const QString porAba = montarAjudaPorAba();
    if (!porAba.isEmpty()) {
        const int fim = base.lastIndexOf(QStringLiteral("</body>"), -1, Qt::CaseInsensitive);
        if (fim >= 0) base.insert(fim, porAba);
        else          base += porAba;
    }

    if (!base.isEmpty()) browser->setHtml(base);
    else                 browser->setSource(QUrl(QStringLiteral("qrc:/help/help.html")));

    layout->addWidget(browser);

    // Margem interna no box garante folga para a sombra do botão (#176):
    // ButtonShadowFilter blur=8 → extensão ~5px; 10px right/bottom são seguros.
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Close, this);
    btns->setContentsMargins(0, 6, 10, 10);
    layout->addWidget(btns);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::accept);
}
