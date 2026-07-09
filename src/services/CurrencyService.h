#pragma once

#include "core/currency/ICurrencyProvider.h"

#include <QMap>
#include <QString>
#include <memory>
#include <utility>

// Converts values between currencies using a provider (DIP).
// Caches rates; refreshes at most once per calendar day.
//
// Regra dura (prioridade nº1: precisão financeira): dinheiro NUNCA passa por
// `double` no caminho de conversão. As taxas do provider (double, vindas do
// JSON) são arredondadas UMA única vez, na borda de `ensureRates()`, para
// micro-unidades inteiras (`qint64`, taxa × 1e6). Todo o resto — inclusive
// `converter()` — é aritmética inteira exata (widening `__int128` para evitar
// overflow no produto intermediário) com arredondamento half-up explícito
// único no final. Erro máximo introduzido: o da taxa arredondada para a
// micro-unidade (≤ 5e-7 relativo) — nunca erro de ponto flutuante sobre o
// valor monetário.
class CurrencyService {
public:
    // Singleton initialized with FrankfurterProvider (lazy, safe to call after QApplication).
    static CurrencyService &instance();

    explicit CurrencyService(std::unique_ptr<ICurrencyProvider> provider);

    // Convert centavos from one currency to another.
    // Returns converted centavos (rounded half-up, aritmética inteira).
    // Lança std::runtime_error se "de" ou "para" não têm cotação no provider
    // (moeda ausente do mapa — nunca fabrica taxa 1.0, ver #156).
    qint64 converter(qint64 centavos, const QString &de, const QString &para);

    // Rate de → para (e.g., 1 BRL = X USD). Só para exibição/leitura — não
    // usar o double retornado aqui em cálculo monetário; usar converter().
    // Mesma regra de exceção de converter() para moeda ausente.
    double taxa(const QString &de, const QString &para);

    // All supported currencies (from last fetch)
    QList<QString> moedas();

    bool disponivel() const;

private:
    std::unique_ptr<ICurrencyProvider> m_provider;
    QMap<QString, qint64>              m_ratesMicro;  // relative to BRL, ×1e6 (micro-units)
    void ensureRates();

    // Resolve as taxas de "de" e "para" em micro-unidades, lançando
    // std::runtime_error se qualquer uma estiver ausente do mapa ou se a
    // taxa de origem for <= 0 (cotação inválida — evita divisão por zero).
    std::pair<qint64, qint64> taxasMicro(const QString &de, const QString &para);
};
