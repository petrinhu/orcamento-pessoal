#pragma once

#include <QDate>
#include <QString>
#include <QtTypes>

// MatchData - tupla de dados usada para detectar duplicatas / matches durante
// importacao de extratos (OFX/CSV/XLSX/...). Vide IMatchFinder (#84).
//
// Campos opcionais (vazios) NAO bloqueiam o matching: bankId vazio cai em
// matching heuristico por valor + payee + data.
struct MatchData {
    QString bankId;     // FITID/transactionId do banco (opaco)
    qint64  amount = 0; // centavos
    QString payee;      // descricao / memo (normalizada caller-side)
    QDate   date;       // data da transacao
    QString accountId;  // conta/cartao destino
};

// Resultado do matching. Ordem: do mais forte (Duplicate) ao mais fraco
// (NotFound). Caller decide como agir em cada caso (bloquear import, sinalizar
// review manual, etc).
enum class MatchResult {
    NotFound,
    Imprecise,
    Precise,
    Duplicate
};
