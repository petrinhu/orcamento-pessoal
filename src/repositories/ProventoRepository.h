#pragma once

#include "models/Provento.h"

#include <QDate>
#include <QList>
#include <QString>
#include <optional>

// ProventoRepository (#66) - CRUD de proventos_recebidos.
// Operacoes especificas: findByTicker, totaisPorAno, dy12m (DY trailing 12m).
class ProventoRepository {
public:
    static bool ensureSchema();

    [[nodiscard]] bool insert(Provento &p);
    [[nodiscard]] bool update(const Provento &p);
    [[nodiscard]] bool remove(qint64 id);
    [[nodiscard]] std::optional<Provento> findById(qint64 id) const;

    [[nodiscard]] QList<Provento> findAll() const;
    [[nodiscard]] QList<Provento> findByTicker(const QString &ticker) const;
    [[nodiscard]] QList<Provento> findByPeriod(const QDate &inicio,
                                                const QDate &fim) const;

    // Total liquido recebido (somatorio em centavos) por ano.
    [[nodiscard]] qint64 totalLiquidoAno(int ano) const;

    // Dividend Yield trailing 12 meses para um ticker.
    // dy12m = soma(valorLiquido ultimos 365d) / cotacaoAtual / quantidade
    // Retorna fracao decimal (0.06 = 6% a.a.). Se quantidade=0 ou
    // cotacaoAtual=0, retorna 0.
    [[nodiscard]] double dy12m(const QString &ticker,
                                qint64 cotacaoAtualCent,
                                double quantidade,
                                const QDate &refData = QDate::currentDate()) const;
};
