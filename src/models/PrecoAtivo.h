#pragma once

#include <QString>
#include <QDate>
#include <QDateTime>

// OHLCV de um ativo para uma data (fechamento diário ou intraday).
// Valores monetários em centavos (qint64). Volume em unidades (ações/cotas).
struct PrecoAtivo {
    QString   ticker;
    QDate     data;
    qint64    abertura   = 0;   // centavos
    qint64    maxima     = 0;   // centavos
    qint64    minima     = 0;   // centavos
    qint64    fechamento = 0;   // centavos
    qint64    volume     = 0;   // unidades
    QString   fonte;            // "cotahist" | "brapi" | "b3direct" | "yahoo" | …
    QDateTime atualizado;
};

// Snapshot de cotação em tempo real (durante pregão).
// Preenchido pelo B3DirectQuoteProvider a cada poll.
struct CotacaoRT {
    QString   ticker;
    qint64    preco       = 0;  // centavos
    qint32    variacaoPct = 0;  // % × 10000 (ex: 2,83% → 28300)
    qint64    abertura    = 0;  // centavos
    qint64    minima      = 0;  // centavos
    qint64    maxima      = 0;  // centavos
    qint64    volume      = 0;  // unidades
    QDateTime timestamp;
};
