#include "HelpDialog.h"

#include <QVBoxLayout>
#include <QTextBrowser>
#include <QDialogButtonBox>

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Ajuda — PeuBud"));
    resize(720, 560);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 12);

    auto *browser = new QTextBrowser(this);
    browser->setOpenExternalLinks(false);
    browser->setSource(QUrl(QStringLiteral("qrc:/help/help.html")));
    layout->addWidget(browser);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Close, this);
    layout->addWidget(btns);
    layout->setContentsMargins(12, 0, 12, 12);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::accept);
}
