#pragma once

#include "models/PrecoAtivo.h"
#include <QList>
#include <QStringList>
#include <optional>

// Acesso ao banco para precos_ativos (OHLCV histórico) e cotacoes_tempo_real.
//
// Convenção de cache:
//   - Data passada   → verifica banco; busca internet apenas se ausente.
//   - Data de hoje   → banco tem prioridade; providers buscam e chamam upsert.
//   - Tempo real     → CotacaoRT via upsertRT / findRT (tabela separada).
class PriceRepository {
public:
    // ── OHLCV histórico ──────────────────────────────────────────────────────

    // INSERT OR REPLACE em precos_ativos.
    bool upsert(const PrecoAtivo &p);

    // Insere/substitui uma lista inteira em uma única transação.
    // Ideal para importar COTAHIST (milhares de registros de uma vez).
    bool upsertBatch(const QList<PrecoAtivo> &lista);

    // Último registro disponível para o ticker (maior data).
    std::optional<PrecoAtivo> findLatest(const QString &ticker) const;

    // Registro exato de ticker + data. nullopt se não existir.
    std::optional<PrecoAtivo> findByTickerAndDate(const QString &ticker,
                                                   const QDate   &data) const;

    // Série histórica de ticker no intervalo [inicio, fim] ordenada por data.
    QList<PrecoAtivo> findByTickerRange(const QString &ticker,
                                        const QDate   &inicio,
                                        const QDate   &fim) const;

    // Lista de tickers que possuem pelo menos um registro histórico.
    QStringList tickersComHistorico() const;

    // Quantos registros existem para o ticker.
    int countByTicker(const QString &ticker) const;

    // Remove todos os registros de um ticker (para limpar histórico).
    bool removeByTicker(const QString &ticker);

    // Remove todos os registros históricos (botão "Limpar histórico").
    bool removeAll();

    // ── Cotação em tempo real ─────────────────────────────────────────────────

    // INSERT OR REPLACE em cotacoes_tempo_real.
    bool upsertRT(const CotacaoRT &c);

    // Último snapshot real-time do ticker. nullopt se nunca foi buscado.
    std::optional<CotacaoRT> findRT(const QString &ticker) const;

    // Remove snapshot de tempo real de um ticker.
    bool removeRT(const QString &ticker);
};
