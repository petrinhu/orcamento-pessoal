#include "ButtonShadowFilter.h"

#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>

namespace {

// Rest state: sombra discreta, moderna — blur menor reduz extensão além do
// botão e minimiza clip em containers com margem apertada (#176).
constexpr int    kRestBlur     = 8;
constexpr int    kRestOffsetY  = 1;
// Pressed state: shadow collapses toward the button — it sinks into the surface.
constexpr int    kPressBlur    = 3;
constexpr int    kPressOffsetY = 1;

QColor restColor()  { return QColor(0, 0, 0, 70);  }
QColor pressColor() { return QColor(0, 0, 0, 110); }

void applyShadow(QGraphicsDropShadowEffect *fx, int blur, int offsetY, const QColor &c)
{
    fx->setBlurRadius(blur);
    fx->setXOffset(0);
    fx->setYOffset(offsetY);
    fx->setColor(c);
}

// Returns our shadow effect on the button, or nullptr if the button has no
// effect or carries a foreign one (which we must not touch).
QGraphicsDropShadowEffect *ourShadow(QPushButton *btn)
{
    return qobject_cast<QGraphicsDropShadowEffect *>(btn->graphicsEffect());
}

} // namespace

void ButtonShadowFilter::install()
{
    static ButtonShadowFilter *instance = nullptr;
    if (instance)
        return;
    instance = new ButtonShadowFilter(qApp);
    qApp->installEventFilter(instance);
}

bool ButtonShadowFilter::eventFilter(QObject *obj, QEvent *ev)
{
    auto *btn = qobject_cast<QPushButton *>(obj);
    if (!btn)
        return QObject::eventFilter(obj, ev);

    if (btn->isFlat())
        return QObject::eventFilter(obj, ev);

    // Itens da sidebar nao recebem drop-shadow: o pill colorido sobre o fundo navy
    // nao tem superfície de elevacao — sombra retangular seria visualmente errada.
    // Cobre objectName "navItem" (itens normais e footer items do SidebarNav).
    if (btn->objectName() == QLatin1String("navItem"))
        return QObject::eventFilter(obj, ev);

    // Botões só-ícone (#iconButton) não recebem sombra: em repouso devem ser
    // invisíveis sobre o fundo (border:none, background:transparent). A sombra
    // criaria um contorno/caixa ao redor do quadrado de 32px — exatamente o
    // "chrome que compete com o conteúdo" que a spec #165 quer eliminar.
    if (btn->objectName() == QLatin1String("iconButton"))
        return QObject::eventFilter(obj, ev);

    switch (ev->type()) {
    case QEvent::Polish:
    case QEvent::Show:
        // A widget accepts only one QGraphicsEffect: only install if none is
        // present, so we never clobber an effect set elsewhere.
        if (!btn->graphicsEffect()) {
            auto *fx = new QGraphicsDropShadowEffect(btn);
            applyShadow(fx, kRestBlur, kRestOffsetY, restColor());
            btn->setGraphicsEffect(fx);
        }
        break;

    case QEvent::MouseButtonPress:
        if (auto *fx = ourShadow(btn))
            applyShadow(fx, kPressBlur, kPressOffsetY, pressColor());
        break;

    case QEvent::MouseButtonRelease:
        if (auto *fx = ourShadow(btn))
            applyShadow(fx, kRestBlur, kRestOffsetY, restColor());
        break;

    default:
        break;
    }

    return QObject::eventFilter(obj, ev);
}
