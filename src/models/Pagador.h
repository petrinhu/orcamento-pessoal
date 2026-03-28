#pragma once

#include <QString>

// Tipo de relacionamento do contato com o usuário.
// Armazenado como TEXT no banco ("pagador", "recebedor", "ambos").
enum class TipoPagador { Pagador, Recebedor, Ambos };

inline QString tipoPagadorParaTexto(TipoPagador t)
{
    switch (t) {
    case TipoPagador::Pagador:   return QStringLiteral("pagador");
    case TipoPagador::Recebedor: return QStringLiteral("recebedor");
    default:                     return QStringLiteral("ambos");
    }
}

inline TipoPagador tipoPagadorDeTexto(const QString &s)
{
    if (s == QStringLiteral("pagador"))   return TipoPagador::Pagador;
    if (s == QStringLiteral("recebedor")) return TipoPagador::Recebedor;
    return TipoPagador::Ambos;
}

inline QString tipoPagadorLabel(TipoPagador t)
{
    switch (t) {
    case TipoPagador::Pagador:   return QStringLiteral("Pagador");
    case TipoPagador::Recebedor: return QStringLiteral("Recebedor");
    default:                     return QStringLiteral("Ambos");
    }
}

inline QString tipoPagadorLabel(const QString &raw)
{
    return tipoPagadorLabel(tipoPagadorDeTexto(raw));
}

struct Pagador {
    int     id   = 0;
    QString nome;
    QString tipo = QStringLiteral("ambos"); // raw text: "pagador"|"recebedor"|"ambos"
    QString obs;
};
