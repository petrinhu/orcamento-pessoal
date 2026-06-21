#pragma once

#include <QObject>

// App-wide event filter that emulates the web `:focus-visible` behaviour for
// icon-only buttons (#iconButton): the focus ring (border + tint of the
// QPushButton#iconButton:focus QSS rule) must appear ONLY when focus arrives
// from the keyboard (Tab / Backtab / shortcut), never from the automatic focus
// Qt assigns to the first focusable widget when a window becomes active.
//
// Why this exists (#165): the first #iconButton in a window (the ◀️ month-back
// button on the Dashboard) receives focus with Qt::ActiveWindowFocusReason on
// the first show. The :focus QSS rule then paints a box around it, before the
// user has interacted — the bug reported as "caixa no 1º render". A repolish
// does not fix it (the widget keeps focus, so :focus keeps matching); the fix
// is to drop the focus when it did not come from the keyboard.
//
// Install once on qApp via FocusVisibleFilter::install(). Idempotent.
//   - FocusIn on a QWidget with objectName "iconButton":
//       reason is keyboard (Tab/Backtab/Shortcut) -> keep focus (ring shows).
//       otherwise (ActiveWindow/Mouse/Other/Popup) -> clearFocus() (no ring).
//
// clearFocus() inside FocusIn is safe: it turns into a FocusOut on the same
// widget, never re-entering FocusIn — no recursion.
class FocusVisibleFilter : public QObject {
    Q_OBJECT
public:
    static void install();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    using QObject::QObject;
};
