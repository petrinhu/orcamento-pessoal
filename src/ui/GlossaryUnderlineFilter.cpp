#include "GlossaryUnderlineFilter.h"

#include "widgets/GlossaryUnderline.h"

#include <QApplication>
#include <QEvent>
#include <QLabel>
#include <QPainter>

GlossaryUnderlineFilter *GlossaryUnderlineFilter::instance()
{
    // Owner = qApp para sobreviver enquanto qualquer label decorado existir.
    static GlossaryUnderlineFilter *inst = new GlossaryUnderlineFilter(qApp);
    return inst;
}

void GlossaryUnderlineFilter::decorar(QLabel *lbl)
{
    if (!lbl)
        return;
    // Idempotência: já decorado? não reinstalar o filtro.
    if (lbl->property("glossarioUnderlined").toBool())
        return;

    lbl->setProperty("glossarioUnderlined", true);
    lbl->installEventFilter(instance());
    lbl->update(); // força repaint para o cue aparecer imediatamente
}

bool GlossaryUnderlineFilter::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() != QEvent::Paint)
        return QObject::eventFilter(obj, ev);

    auto *lbl = qobject_cast<QLabel *>(obj);
    if (!lbl)
        return QObject::eventFilter(obj, ev);

    // O eventFilter roda ANTES do paintEvent do alvo. Para garantir que o
    // pontilhado fique POR CIMA do conteúdo do label sem ser apagado pelo
    // repaint do QLabel, re-despachamos o Paint ao label de forma síncrona
    // (passo 1), depois desenhamos o overlay (passo 2) e consumimos o evento
    // original (passo 3) para não repetir o paint base.
    //
    // A guarda de reentrância impede recursão: durante o re-despacho, o filtro
    // recebe o mesmo Paint de novo e o deixa seguir normal (return false), o que
    // executa QLabel::paintEvent (texto/background) uma única vez.
    if (m_reentrant)
        return QObject::eventFilter(obj, ev);

    m_reentrant = true;
    QApplication::sendEvent(lbl, ev); // executa QLabel::paintEvent (base) agora
    m_reentrant = false;

    {
        QPainter p(lbl);
        GlossaryUnderline::paint(lbl, &p); // overlay: pontilhado por cima da base
    }

    return true; // base + overlay já pintados; suprime o despacho duplicado
}
