/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is plugin for tip of day.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtipofdayboardwidget.h"

#include <qfileinfo.h>

#include <kcolorscheme.h>
#include <klocalizedstring.h>

#include "skgmainpanel.h"
#include "skgservices.h"
#include "skgtraces.h"

SKGTipOfDayBoardWidget::SKGTipOfDayBoardWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGBoardWidget(iParent, iDocument, i18nc("Dashboard widget title", "Tip of the day"))
{
    SKGTRACEINFUNC(10)

    auto f = new QFrame();
    ui.setupUi(f);
    setMainWidget(f);
    ui.kIcon->setIcon(SKGServices::fromTheme(QStringLiteral("ktip")));

    onModified();

    connect(ui.kIcon, &QPushButton::clicked, this, &SKGTipOfDayBoardWidget::onModified);
    connect(ui.kText, &QLabel::linkActivated, this, [](const QString &val) {
        SKGMainPanel::getMainPanel()->openPage(val);
    });

    // Refresh
    connect(getDocument(), &SKGDocument::transactionSuccessfullyEnded, this, &SKGTipOfDayBoardWidget::onModified, Qt::QueuedConnection);
}

SKGTipOfDayBoardWidget::~SKGTipOfDayBoardWidget()
{
    SKGTRACEINFUNC(10)
}

void SKGTipOfDayBoardWidget::onModified()
{
    auto text = SKGMainPanel::getMainPanel()->getTipOfDay();

    // Remove color of hyperlinks
    KColorScheme scheme(QPalette::Normal, KColorScheme::Window);
    auto color = scheme.foreground(KColorScheme::NormalText).color().name().right(6);
    text = text.replace(QStringLiteral("<a href"), QStringLiteral("<a style=\"color: #") + color + ";\" href");

    ui.kText->setText(text);
}
