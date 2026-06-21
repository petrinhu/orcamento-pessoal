#include "FocusVisibleFilter.h"

#include <QApplication>
#include <QEvent>
#include <QFocusEvent>
#include <QWidget>

namespace {

// Focus ring is desirable only for keyboard-driven focus (the :focus-visible
// contract). ActiveWindow/Mouse/Other/Popup must NOT paint the ring on an
// #iconButton — they would draw a box without any user interaction.
bool isKeyboardFocus(Qt::FocusReason r)
{
    return r == Qt::TabFocusReason
        || r == Qt::BacktabFocusReason
        || r == Qt::ShortcutFocusReason;
}

} // namespace

void FocusVisibleFilter::install()
{
    static FocusVisibleFilter *instance = nullptr;
    if (instance)
        return;
    instance = new FocusVisibleFilter(qApp);
    qApp->installEventFilter(instance);
}

bool FocusVisibleFilter::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() != QEvent::FocusIn)
        return QObject::eventFilter(obj, ev);

    auto *w = qobject_cast<QWidget *>(obj);
    if (!w || w->objectName() != QLatin1String("iconButton"))
        return QObject::eventFilter(obj, ev);

    const auto *fe = static_cast<QFocusEvent *>(ev);
    if (!isKeyboardFocus(fe->reason()))
        w->clearFocus();

    return QObject::eventFilter(obj, ev);
}
