#include "CurrencyService.h"
#include "core/currency/FrankfurterProvider.h"

#include <cmath>
#include <stdexcept>

// __int128 é extensão GCC/Clang (não ISO C++), daí os avisos -Wpedantic.
// Escolha deliberada: é o único jeito portável (nesses dois compiladores,
// os únicos usados no build desta app — CLAUDE.md restringe a Linux/GCC-
// Clang) de fazer widening de int64*int64 sem sair de aritmética inteira.
// Alternativa seria uma lib de bignum (boost::multiprecision) só para isso
// — overkill. Silenciar -Wpedantic apenas neste bloco, escopo mínimo.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

namespace {

// Escala das taxas em micro-unidades inteiras: 1.0 (taxa) == 1'000'000
// (micro). 6 casas decimais de precisão — bem acima do que qualquer API de
// câmbio publica (Frankfurter/ECB usa tipicamente 4-5 casas significativas).
constexpr qint64 kMicroScale = 1'000'000;

// Único ponto de arredondamento double->inteiro no caminho de câmbio: a
// borda de parse da taxa vinda do provider (JSON). Dali em diante é tudo
// aritmética inteira exata.
qint64 paraMicro(double taxa)
{
    return static_cast<qint64>(std::llround(taxa * static_cast<double>(kMicroScale)));
}

// centavos * rParaMicro / rDeMicro em aritmética inteira exata: widening p/
// __int128 no produto intermediário (evita overflow — centavos até ~9.2e18
// * rPara até ~1e11 excede int64 mas cabe em __int128, que vai até ~1.7e38),
// arredondamento half-up explícito único no final. rDeMicro > 0 é garantido
// pelo chamador (CurrencyService::taxasMicro) antes de chegar aqui.
// __int128 fica 100% contido nesta função — nunca aparece na assinatura
// pública de CurrencyService.
qint64 converterInteiro(qint64 centavos, qint64 rParaMicro, qint64 rDeMicro)
{
    const __int128 numerador   = static_cast<__int128>(centavos) * static_cast<__int128>(rParaMicro);
    const __int128 denominador = static_cast<__int128>(rDeMicro);

    const bool negativo = (numerador < 0);
    const __int128 n = negativo ? -numerador : numerador;
    const __int128 q = (n + denominador / 2) / denominador;
    return static_cast<qint64>(negativo ? -q : q);
}

} // namespace

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

CurrencyService &CurrencyService::instance()
{
    static CurrencyService cs(std::make_unique<FrankfurterProvider>());
    return cs;
}

CurrencyService::CurrencyService(std::unique_ptr<ICurrencyProvider> provider)
    : m_provider(std::move(provider))
{}

void CurrencyService::ensureRates()
{
    const QMap<QString, double> raw = m_provider->fetchRates(QStringLiteral("BRL"));
    m_ratesMicro.clear();
    for (auto it = raw.constBegin(); it != raw.constEnd(); ++it)
        m_ratesMicro.insert(it.key(), paraMicro(it.value()));
}

bool CurrencyService::disponivel() const
{
    return m_provider->isAvailable();
}

std::pair<qint64, qint64> CurrencyService::taxasMicro(const QString &de, const QString &para)
{
    ensureRates();

    // Moeda AUSENTE do mapa do provider != taxa legitimamente 1.0 (a própria
    // base BRL, que sempre está presente). Não fabricar fallback — sinalizar
    // erro explícito (#156: evita ~288x de erro silencioso em BRL->ARS etc).
    if (!m_ratesMicro.contains(de))
        throw std::runtime_error(
            ("Cotacao indisponivel para a moeda de origem: " + de).toStdString());
    if (!m_ratesMicro.contains(para))
        throw std::runtime_error(
            ("Cotacao indisponivel para a moeda de destino: " + para).toStdString());

    const qint64 rDe = m_ratesMicro.value(de);
    if (rDe <= 0)
        throw std::runtime_error(
            ("Cotacao invalida (<= 0) para a moeda de origem: " + de).toStdString());

    return {rDe, m_ratesMicro.value(para)};
}

double CurrencyService::taxa(const QString &de, const QString &para)
{
    if (de == para) return 1.0;

    // rates are relative to BRL, so:
    //   rate[X] = how many X per 1 BRL
    //   to convert de→para: value_para = value_de * rate[para] / rate[de]
    const auto [rDe, rPara] = taxasMicro(de, para);
    return static_cast<double>(rPara) / static_cast<double>(rDe);
}

qint64 CurrencyService::converter(qint64 centavos, const QString &de, const QString &para)
{
    if (de == para) return centavos;

    const auto [rDe, rPara] = taxasMicro(de, para);
    return converterInteiro(centavos, rPara, rDe);
}

QList<QString> CurrencyService::moedas()
{
    ensureRates();
    return m_ratesMicro.keys();
}
