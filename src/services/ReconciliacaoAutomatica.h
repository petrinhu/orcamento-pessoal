#pragma once

#include <QList>
#include <QString>
#include <QtTypes>

// ReconciliacaoAutomatica (#85) - subset-sum/knapsack puro para reconciliacao
// automatica de lancamentos contra saldo-alvo. Inspirado em
// gnucash/libgnucash/app-utils/gnc-autoclear.cpp.
//
// Stateless, sem Qt SQL/Network. Trabalha em centavos (qint64) com timeout
// para evitar explosao em conjuntos grandes (>50 candidatos) ou alvos
// elevados (>1M centavos).
class ReconciliacaoAutomatica
{
public:
    struct Lancamento {
        qint64  id;
        qint64  valorCent;   // sempre positivo
        QString descricao;
    };

    struct Resultado {
        QList<qint64> ids;             // ids escolhidos cuja soma == alvo
        bool          encontrado;
        int           iteracoes;       // diagnostico (celulas DP visitadas)
        bool          timeoutAtingido;
    };

    // DP subset-sum com timeout (default 2s).
    // Procura primeiro subconjunto cuja soma == saldoAlvoCent.
    // Aplica bound rem_pos (soma restante possivel) tipico gnc-autoclear:
    // se rem_pos < target_restante, pode parar ramo.
    //
    // Edge cases:
    //   - saldoAlvoCent == 0          -> encontrado, ids vazio
    //   - saldoAlvoCent < 0           -> nao encontrado
    //   - candidatos vazio + alvo > 0 -> nao encontrado
    //   - timeout                     -> timeoutAtingido = true
    [[nodiscard]] static Resultado resolver(
        const QList<Lancamento> &candidatos,
        qint64 saldoAlvoCent,
        int    timeoutMs = 2000);
};
