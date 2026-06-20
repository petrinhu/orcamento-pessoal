#pragma once

#include <QObject>

// App-wide event filter that gives every QPushButton a drop shadow (it floats)
// and shrinks that shadow on press (it sinks). Paired with the QSS padding-sink
// in Theme::gerarQss, this yields tactile "Shadow"-style click feedback.
//
// Install once on qApp via ButtonShadowFilter::install(). Idempotent.
//   - Polish/Show on a QPushButton with no graphicsEffect()  -> apply rest shadow.
//   - MouseButtonPress   -> tighten shadow (looks pressed-in).
//   - MouseButtonRelease -> restore rest shadow.
// QToolButton, flat() and autoRaise() buttons are skipped (toolbar stays flat).
class ButtonShadowFilter : public QObject {
    Q_OBJECT
public:
    static void install();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    using QObject::QObject;
};
