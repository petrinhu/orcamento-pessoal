#pragma once

#include "models/FechamentoMensal.h"
#include <QString>
#include <QDate>
#include <optional>

class QSqlDatabase;

// FechamentoMensalService (#88) - close/reopen/simulate de mes contabil.
//
// Snapshot imutavel: edicoes em lancamentos do mes apos fechamento NAO
// alteram o dadosJson armazenado. Para refazer, fazer reopen() (apaga
// o registro de fechamento e recalcula via simulate).
//
// Padrao: web-budget/.../ClosingService.java:43-110.

struct FechamentoResumo {
    qint64  entradasCentavos     = 0;
    qint64  gastosCentavos       = 0;
    qint64  parcelasCentavos     = 0;
    qint64  investimentosCentavos = 0;
    qint64  saldoLiquidoCentavos = 0; // entradas - gastos - parcelas
    int     numLancamentos       = 0;
};

class FechamentoMensalService
{
public:
    // Cria tabela fechamentos_mensais se nao existir (idempotente).
    [[nodiscard]] static bool ensureSchema(QSqlDatabase &db,
                                            QString      *errorOut = nullptr);

    // Agrega dados do mes sem persistir. Reutilizavel para previa visual.
    [[nodiscard]] static FechamentoResumo simulate(QSqlDatabase &db,
                                                    const QDate  &mes);

    // Fecha o mes (simulate + persiste). Retorna snapshot persistido.
    // Se ja existe fechamento para o periodo: retorna std::nullopt
    // (nao sobrescreve - caller deve fazer reopen() antes).
    [[nodiscard]] static std::optional<FechamentoMensal> close(
        QSqlDatabase  &db,
        const QDate   &mes,
        const QString &fechadoPor);

    // Apaga o registro de fechamento (dados originais ficam intactos).
    // Idempotente: retorna true mesmo se nao havia fechamento.
    static bool reopen(QSqlDatabase &db, const QDate &mes);

    // Busca fechamento existente ou std::nullopt.
    [[nodiscard]] static std::optional<FechamentoMensal> findByPeriod(
        QSqlDatabase &db, const QDate &mes);
};
