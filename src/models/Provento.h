#pragma once

#include <QDate>
#include <QString>
#include <QtTypes>

// Provento (#66) - recebimento de dividendo, JCP ou rendimento de FII por
// ativo. Persistido em proventos_recebidos. Permite calculo DY% por ativo
// + total recebido por periodo. Snapshot ticker para preservar historico
// mesmo se LancamentoInvestimento mudar nome.
//
// Tipos:
//   "dividendo"  - distribuicao tributada na fonte (isenta IRPF PF)
//   "jcp"        - juros sobre capital proprio (IR 15% retido)
//   "rendimento" - rendimento de FII (isento ate certo limite)
//   "bonus"      - bonus em acoes (sem cash)
struct Provento {
    qint64  id              = 0;
    QString ticker;                 // PETR4, HGLG11, ...
    QString tipo;                   // dividendo|jcp|rendimento|bonus
    QDate   dataEvento;             // data do recebimento (com)
    qint64  valorBrutoCent  = 0;    // total bruto em centavos
    qint64  valorLiquidoCent = 0;   // pos IR retido em centavos
    double  porAcao         = 0.0;  // R$ por papel (para historico)
    double  quantidade      = 0.0;  // quantidade detida no com
    QString observacao;
    QString criadoEm;               // ISO 8601
};
