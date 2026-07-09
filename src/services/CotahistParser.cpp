#include "services/CotahistParser.h"

#include <QDate>
#include <QDateTime>

namespace cotahist {

bool isValidRecord(const QString &line)
{
    if (line.length() < 200) {
        return false;
    }
    const QString tipo = line.mid(0, 2);
    return (tipo == QStringLiteral("01")); // apenas registros de preço
}

PrecoAtivo parseLine(const QString &line)
{
    PrecoAtivo p;
    if (line.length() < 200) return p;

    // Tipo (1-indexed: 1-2)
    const QString tipo = line.mid(0, 2);
    if (tipo != QStringLiteral("01")) return p;

    // Data do pregão (positions 3-10, 0-indexed mid(2,8))
    const QString dataStr = line.mid(2, 8);
    const QDate data = QDate::fromString(dataStr, QStringLiteral("yyyyMMdd"));
    if (!data.isValid()) return p;

    // Ticker (positions 13-24, 0-indexed mid(12,12))
    const QString ticker = line.mid(12, 12).trimmed();
    if (ticker.isEmpty()) return p;

    // Helper para converter string monetária (ex: "  12345,67") para centavos
    auto monetToCents = [](const QString &s) -> qint64 {
        if (s.isEmpty()) return 0;
        const QString limpo = s.trimmed();
        if (limpo.length() < 3) return 0;
        // Separar parte inteira e decimal (2 dígitos)
        const QString parteInt = limpo.left(limpo.length() - 2);
        const QString parteDec = limpo.mid(limpo.length() - 2);
        bool ok1 = false, ok2 = false;
        const int i = parteInt.toInt(&ok1);
        const int d = parteDec.toInt(&ok2);
        if (!ok1 || !ok2) return 0;
        return static_cast<qint64>(i) * 100 + d;
    };

    // Helper para volume (inteiro longo)
    auto strToLongLong = [](const QString &s) -> qint64 {
        if (s.isEmpty()) return 0;
        const QString limpo = s.trimmed();
        bool ok = false;
        const qint64 v = limpo.toLongLong(&ok);
        return ok ? v : 0;
    };

    // Preços (layout posicional B3 COTAHIST, 1-indexed):
    //   abertura (PREABE) 57-69, maxima (PREMAX) 70-82, minima (PREMIN) 83-95,
    //   media    (PREMED) 96-108, ultimo/fechamento (PREULT) 109-121.
    // 0-indexed p/ QString::mid: abertura mid(56,13), maxima mid(69,13),
    // minima mid(82,13), ultimo(PREULT) mid(108,13).
    // #199 (QA-01, auditoria 2026-06-22 §3.4): campo `fechamento` lia
    // mid(95,13) = PREMED (preco medio), NAO PREULT (fechamento). Layout B3
    // conferido; corrigido para mid(108,13).
    const QString aberturaStr = line.mid(56, 13);
    const QString maximaStr   = line.mid(69, 13);
    const QString minimaStr   = line.mid(82, 13);
    const QString ultimoStr   = line.mid(108, 13);
    // Volume: positions 171-188, 0-indexed mid(170, 18)
    const QString volumeStr   = line.mid(170, 18);

    p.ticker = ticker;
    p.data = data;
    p.abertura = monetToCents(aberturaStr);
    p.maxima = monetToCents(maximaStr);
    p.minima = monetToCents(minimaStr);
    p.fechamento = monetToCents(ultimoStr);
    p.volume = strToLongLong(volumeStr);
    p.fonte = QStringLiteral("COTAHIST_B3");
    p.atualizado = QDateTime::currentDateTime();

    return p;
}

} // namespace cotahist
