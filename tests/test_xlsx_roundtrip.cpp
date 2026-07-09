// PeuBud - #199 (Onda U, QA-06): round-trip XLSX so estava provado para OFX
// (test_export::roundTripOfxPreservaCentavos). CSV/XLSX nunca tiveram um
// teste export->import->igual.
//
// Achado (RED antes do fix): ImportService::fromXlsx chamava
// `parseAmount(vAmt.toString(), QLatin1Char('.'))` com o separador decimal
// FIXO em ponto. Celulas de planilha geradas por
// ExportService::buildTableGastos usam ExportService::fmtBrl, que formata em
// pt-BR (virgula decimal, ex. "1234,56", sem separador de milhar). Com
// decimalSep fixo em '.', parseAmount tratava a virgula como separador de
// milhar (remove) -> "123456" -> interpretado como R$ 123.456,00 -> 100x o
// valor real em centavos.
//
// Fix: fromXlsx agora usa detectDecimal(raw) por celula (mesma heuristica ja
// usada em fromCsv), corrigindo o round-trip.
//
// Alvo de build separado (test_xlsx_roundtrip) porque precisa linkar
// QXlsx::QXlsx + QXLSX_LIBRARY -- os alvos test_export/test_import existentes
// compilam ExportService.cpp/ImportService.cpp SEM QXlsx (caem no stub
// #else), entao nunca exercitam o parser real de XLSX.

#include <QtTest/QtTest>
#include <QSet>
#include <QTemporaryDir>

#include "services/ExportService.h"
#include "services/ImportService.h"
#include "models/GastoAvulso.h"

class TestXlsxRoundtrip : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir m_dir;

private slots:
    // ESPEC RED->GREEN (#199/QA-06): export -> import deve preservar os
    // centavos EXATOS. Antes do fix, 123456 centavos viravam 12345600 (100x).
    void roundTripXlsxPreservaCentavos()
    {
        QList<GastoAvulso> gastos;
        GastoAvulso g1; g1.id = 1; g1.ano = 2025; g1.mes = 3; g1.dia = 15;
        g1.valor = 123456; g1.descricao = QStringLiteral("Mercado");
        g1.categoria = QStringLiteral("alimentacao");
        gastos << g1;
        GastoAvulso g2; g2.id = 2; g2.ano = 2025; g2.mes = 3; g2.dia = 20;
        g2.valor = 8990; g2.descricao = QStringLiteral("Luz");
        g2.categoria = QStringLiteral("moradia");
        gastos << g2;
        GastoAvulso g3; g3.id = 3; g3.ano = 2025; g3.mes = 3; g3.dia = 25;
        g3.valor = 5; g3.descricao = QStringLiteral("Taxa 5c");
        g3.categoria = QStringLiteral("outros");
        gastos << g3;

        const ExportService::TableData td = ExportService::buildTableGastos(gastos);
        QVERIFY(!td.rows.isEmpty());

        ExportService::Section sec;
        sec.name = QStringLiteral("Gastos");
        sec.data = td;

        const QString path = m_dir.filePath(QStringLiteral("gastos.xlsx"));
        QVERIFY2(ExportService::toMultiXlsx(path, { sec }),
            "toMultiXlsx falhou (QXlsx nao compilado neste alvo de teste?)");

        const auto r = ImportService::fromXlsx(path);
        QVERIFY2(r.isValid(), qPrintable(r.error));

        // fromXlsx nao distingue a linha TOTAL (comportamento pre-existente,
        // fora do escopo de QA-06) -- ela tambem vira uma "transacao" lida.
        // O que travamos aqui e a MAGNITUDE de cada centavo lido, nao a
        // contagem/ordem de linhas.
        const qint64 total = qint64(123456) + qint64(8990) + qint64(5);
        QSet<qint64> esperados = { qint64(123456), qint64(8990), qint64(5), total };
        QSet<qint64> lidos;
        for (const auto &c : r.compras)
            lidos.insert(c.valorTotal);

        QCOMPARE(lidos, esperados);
        QVERIFY2(!lidos.contains(qint64(12345600)),
            "regressao: valor inflado 100x (decimalSep fixo em '.')");
    }
};

QTEST_MAIN(TestXlsxRoundtrip)
#include "test_xlsx_roundtrip.moc"
