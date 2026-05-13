// PeuBud - IBankPDFParser TDD (#93)
// Testa parsers heuristicos com textos sample (simula saida de Poppler).
// Nao depende de Poppler-qt6 instalado.

#include <QtTest/QtTest>
#include "services/import/BankPDFParsers.h"

class TestPDFParsers : public QObject
{
    Q_OBJECT

private slots:
    void nubankDetectaCabecalho();
    void nubankParseDuasTransacoes();
    void itauDetectaCabecalho();
    void itauParseComDebitoCredito();
    void genericoExtraiQualquerPadrao();
    void parserNaoDetectadoCaiNoGenerico();
};

void TestPDFParsers::nubankDetectaCabecalho()
{
    NubankPDFParser p;
    QVERIFY(p.detectar(QStringLiteral("Fatura Nubank - 12/05/2026")));
    QVERIFY(p.detectar(QStringLiteral("Nu Pagamentos S.A.")));
    QVERIFY(!p.detectar(QStringLiteral("Banco do Brasil")));
}

void TestPDFParsers::nubankParseDuasTransacoes()
{
    const QString texto = QStringLiteral(
        "Nubank\n"
        "Fatura\n"
        "07 MAI  SUPERMERCADO XYZ   R$ 152,30\n"
        "15 MAI  ASSINATURA NETFLIX R$ 55,90\n"
    );
    NubankPDFParser p;
    const auto r = p.parse(texto);
    QVERIFY(r.isValid());
    QCOMPARE(r.compras.size(), 2);
    QCOMPARE(r.compras[0].nome, QStringLiteral("SUPERMERCADO XYZ"));
    QCOMPARE(r.compras[0].valorTotal, qint64(15230));
    QCOMPARE(r.compras[1].valorTotal, qint64(5590));
}

void TestPDFParsers::itauDetectaCabecalho()
{
    ItauPDFParser p;
    QVERIFY(p.detectar(QStringLiteral("BANCO ITAU UNIBANCO S.A.")));
    QVERIFY(p.detectar(QStringLiteral("Extrato Itaú")));
    QVERIFY(!p.detectar(QStringLiteral("Inter Bank")));
}

void TestPDFParsers::itauParseComDebitoCredito()
{
    const QString texto = QStringLiteral(
        "Itau Extrato\n"
        "07/05 PIX RECEBIDO         500,00 C\n"
        "08/05 PAGAMENTO BOLETO     250,75 D\n"
    );
    ItauPDFParser p;
    const auto r = p.parse(texto);
    QVERIFY(r.isValid());
    QCOMPARE(r.compras.size(), 2);
    // Itau parser registra valor absoluto; debito/credito sinaliza intern.
    QCOMPARE(r.compras[0].valorTotal, qint64(50000));
    QCOMPARE(r.compras[1].valorTotal, qint64(25075));
}

void TestPDFParsers::genericoExtraiQualquerPadrao()
{
    const QString texto = QStringLiteral(
        "Banco Desconhecido\n"
        "12/05/2026 Compra mercado R$ 89,90\n"
        "13/05/2026 Pix Joao        -R$ 1.500,00\n"
    );
    GenericoPDFParser p;
    const auto r = p.parse(texto);
    QVERIFY(r.isValid());
    QCOMPARE(r.compras.size(), 2);
    QCOMPARE(r.compras[0].valorTotal, qint64(8990));
    QCOMPARE(r.compras[1].valorTotal, qint64(150000));
}

void TestPDFParsers::parserNaoDetectadoCaiNoGenerico()
{
    // Texto sem assinatura Nubank: Nubank.detectar() falha
    const QString texto = QStringLiteral(
        "Extrato qualquer\n"
        "11/04/2026 Loja generica R$ 42,10\n"
    );
    NubankPDFParser p;
    QVERIFY(!p.detectar(texto));
    // Mas se for chamado, faz fallback interno e extrai
    const auto r = p.parse(texto);
    QVERIFY(r.isValid());
    QCOMPARE(r.compras.size(), 1);
    QCOMPARE(r.compras[0].valorTotal, qint64(4210));
}

QTEST_GUILESS_MAIN(TestPDFParsers)
#include "test_pdf_parsers.moc"
