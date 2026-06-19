// PeuBud - ExportService (#133): protecao da precisao de centavos na
// formatacao machine-readable (OFX/QIF usam PONTO decimal; nunca virgula).
//
// Estrategia:
//  1. Asserts sobre o TEXTO gerado (OFX/QIF) — TRNAMT com ponto decimal,
//     sinal correto (CREDIT positivo / DEBIT negativo).
//  2. Round-trip OFX: gerar via toMultiOfx -> reimportar via fromOfx ->
//     conferir que os centavos batem (modulo std::abs do importador).
//  3. fmtBrl via buildTableEntradas: liquido = bruto - irpf - inss em cents.

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include "services/ExportService.h"
#include "services/ImportService.h"
#include "models/LancamentoEntrada.h"
#include "models/GastoAvulso.h"

class TestExport : public QObject
{
    Q_OBJECT
private:
    QString readAll(const QString &path);
    QTemporaryDir m_dir;

private slots:
    // ── Formato machine-readable ────────────────────────────────────────────
    void ofxUsaPontoDecimalCredito();
    void ofxUsaPontoDecimalDebito();
    void ofxNuncaUsaVirgulaNoValor();
    void qifUsaPontoDecimalComSinal();
    void ofxCentavosComZeroAEsquerda();

    // ── Round-trip OFX ──────────────────────────────────────────────────────
    void roundTripOfxPreservaCentavos();

    // ── fmtBrl / liquido ────────────────────────────────────────────────────
    void buildTableEntradasLiquidoEmCentavos();
};

QString TestExport::readAll(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    return ts.readAll();
}

// ── OFX/QIF formato ───────────────────────────────────────────────────────────

void TestExport::ofxUsaPontoDecimalCredito()
{
    // Entrada de R$ 1.234,56 liquido -> CREDIT, TRNAMT 1234.56 (ponto).
    LancamentoEntrada l;
    l.id = 1; l.ano = 2025; l.mes = 3; l.dia = 15;
    l.valorBruto = 123456; l.irpf = 0; l.inss = 0;
    l.fonte = QStringLiteral("Salario");

    const auto txs = ExportService::ofxFromEntradas({ l });
    QCOMPARE(txs.size(), 1);
    QCOMPARE(txs[0].valor, qint64(123456)); // positivo = credito

    const QString path = m_dir.filePath(QStringLiteral("ent.ofx"));
    QVERIFY(ExportService::toMultiOfx(path, txs));
    const QString ofx = readAll(path);

    QVERIFY2(ofx.contains(QStringLiteral("<TRNAMT>1234.56</TRNAMT>")),
        qPrintable(QStringLiteral("OFX deve usar ponto decimal. Gerado:\n%1").arg(ofx)));
    QVERIFY(ofx.contains(QStringLiteral("<TRNTYPE>CREDIT</TRNTYPE>")));
}

void TestExport::ofxUsaPontoDecimalDebito()
{
    // Gasto de R$ 89,90 -> DEBIT, TRNAMT -89.90 (negativo, ponto).
    GastoAvulso g;
    g.id = 7; g.ano = 2025; g.mes = 6; g.dia = 1;
    g.valor = 8990; g.descricao = QStringLiteral("Conta de luz");

    const auto txs = ExportService::ofxFromGastos({ g });
    QCOMPARE(txs.size(), 1);
    QCOMPARE(txs[0].valor, qint64(-8990)); // negativo = debito

    const QString path = m_dir.filePath(QStringLiteral("gst.ofx"));
    QVERIFY(ExportService::toMultiOfx(path, txs));
    const QString ofx = readAll(path);

    QVERIFY2(ofx.contains(QStringLiteral("<TRNAMT>-89.90</TRNAMT>")),
        qPrintable(QStringLiteral("OFX debito deve ser -89.90. Gerado:\n%1").arg(ofx)));
    QVERIFY(ofx.contains(QStringLiteral("<TRNTYPE>DEBIT</TRNTYPE>")));
}

void TestExport::ofxNuncaUsaVirgulaNoValor()
{
    // Valor "quebrado" 1.234.567,89 -> TRNAMT 1234567.89; jamais virgula.
    LancamentoEntrada l;
    l.id = 2; l.ano = 2025; l.mes = 1; l.dia = 10;
    l.valorBruto = 123456789; l.irpf = 0; l.inss = 0;
    l.fonte = QStringLiteral("Bonus");

    const auto txs = ExportService::ofxFromEntradas({ l });
    const QString path = m_dir.filePath(QStringLiteral("big.ofx"));
    QVERIFY(ExportService::toMultiOfx(path, txs));
    const QString ofx = readAll(path);

    // Extrai a linha do TRNAMT e garante que nao ha virgula nela.
    const int p = ofx.indexOf(QStringLiteral("<TRNAMT>"));
    QVERIFY(p >= 0);
    const int e = ofx.indexOf(QStringLiteral("</TRNAMT>"), p);
    const QString trnamt = ofx.mid(p, e - p);
    QVERIFY2(!trnamt.contains(QLatin1Char(',')),
        qPrintable(QStringLiteral("TRNAMT contem virgula: %1").arg(trnamt)));
    QVERIFY(ofx.contains(QStringLiteral("<TRNAMT>1234567.89</TRNAMT>")));
}

void TestExport::qifUsaPontoDecimalComSinal()
{
    GastoAvulso g;
    g.id = 9; g.ano = 2025; g.mes = 6; g.dia = 1;
    g.valor = 8990; g.descricao = QStringLiteral("Conta de luz");

    const auto txs = ExportService::ofxFromGastos({ g });
    const QString path = m_dir.filePath(QStringLiteral("gst.qif"));
    QVERIFY(ExportService::toMultiQif(path, txs));
    const QString qif = readAll(path);

    // QIF: linha de valor comeca com 'T'; debito -> T-89.90 (ponto, sinal).
    QVERIFY2(qif.contains(QStringLiteral("T-89.90")),
        qPrintable(QStringLiteral("QIF deve ter T-89.90. Gerado:\n%1").arg(qif)));
    QVERIFY(qif.startsWith(QStringLiteral("!Type:Bank")));
}

void TestExport::ofxCentavosComZeroAEsquerda()
{
    // Valor R$ 100,05 -> centavos componente "05" deve ter zero a esquerda.
    LancamentoEntrada l;
    l.id = 3; l.ano = 2025; l.mes = 2; l.dia = 5;
    l.valorBruto = 10005; l.irpf = 0; l.inss = 0;
    l.fonte = QStringLiteral("X");

    const auto txs = ExportService::ofxFromEntradas({ l });
    const QString path = m_dir.filePath(QStringLiteral("z.ofx"));
    QVERIFY(ExportService::toMultiOfx(path, txs));
    const QString ofx = readAll(path);

    QVERIFY2(ofx.contains(QStringLiteral("<TRNAMT>100.05</TRNAMT>")),
        qPrintable(QStringLiteral("centavos 05 mal formatados. Gerado:\n%1").arg(ofx)));
}

// ── Round-trip OFX ────────────────────────────────────────────────────────────

void TestExport::roundTripOfxPreservaCentavos()
{
    // Gera OFX a partir de gastos conhecidos e reimporta. O importador aplica
    // std::abs, entao comparamos contra o valor absoluto em centavos.
    QList<GastoAvulso> gastos;
    GastoAvulso g1; g1.id = 1; g1.ano = 2025; g1.mes = 3; g1.dia = 15;
    g1.valor = 123456; g1.descricao = QStringLiteral("Mercado"); gastos << g1;
    GastoAvulso g2; g2.id = 2; g2.ano = 2025; g2.mes = 3; g2.dia = 20;
    g2.valor = 8990;   g2.descricao = QStringLiteral("Luz");     gastos << g2;
    GastoAvulso g3; g3.id = 3; g3.ano = 2025; g3.mes = 3; g3.dia = 25;
    g3.valor = 5;      g3.descricao = QStringLiteral("Taxa 5c"); gastos << g3;

    const auto txs = ExportService::ofxFromGastos(gastos);
    const QString path = m_dir.filePath(QStringLiteral("rt.ofx"));
    QVERIFY(ExportService::toMultiOfx(path, txs));

    const auto r = ImportService::fromOfx(path);
    QVERIFY2(r.isValid(), qPrintable(r.error));
    QCOMPARE(r.compras.size(), gastos.size());

    // OFX 2.x preserva ordem das transacoes; centavos devem bater (abs).
    QCOMPARE(r.compras[0].valorTotal, qint64(123456));
    QCOMPARE(r.compras[1].valorTotal, qint64(8990));
    QCOMPARE(r.compras[2].valorTotal, qint64(5));
    QCOMPARE(r.compras[0].mesInicio, 3);
    QCOMPARE(r.compras[0].anoInicio, 2025);
}

// ── fmtBrl / liquido ──────────────────────────────────────────────────────────

void TestExport::buildTableEntradasLiquidoEmCentavos()
{
    // liquido = bruto - irpf - inss. R$ 5.000,00 bruto, R$ 500 IRPF, R$ 200 INSS
    // -> liquido R$ 4.300,00. fmtBrl usa virgula decimal (celula de planilha).
    LancamentoEntrada l;
    l.id = 1; l.ano = 2025; l.mes = 3; l.dia = 5;
    l.valorBruto = 500000; l.irpf = 50000; l.inss = 20000;
    l.fonte = QStringLiteral("Salario");

    const auto td = ExportService::buildTableEntradas({ l });
    QVERIFY(!td.rows.isEmpty());

    // Algum campo da primeira linha deve conter o liquido formatado "4300,00".
    bool achouLiquido = false;
    for (const auto &cell : td.rows.first())
        if (cell.contains(QStringLiteral("4300,00"))) achouLiquido = true;
    QVERIFY2(achouLiquido,
        qPrintable(QStringLiteral("liquido 4300,00 ausente. Linha: %1")
            .arg(td.rows.first().join(QStringLiteral(" | ")))));
}

QTEST_GUILESS_MAIN(TestExport)
#include "test_export.moc"
