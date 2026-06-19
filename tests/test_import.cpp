// PeuBud - ImportService parsers (#133): parseAmount/parseDate/detect* sao
// private static; testados via a API publica fromCsv()/fromOfx() com samples
// minimos escritos em arquivo temporario.
//
// Foco no risco financeiro: centavos parseados corretamente sob diferentes
// locales (pt-BR "1.234,56" vs en "1,234.56"), datas dd/MM vs ISO, e o
// tratamento de valores negativos (parseAmount aplica std::abs -> sempre
// positivo; o importador descarta val<=0).

#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QTextStream>
#include "services/ImportService.h"
#include "models/Compra.h"

class TestImport : public QObject
{
    Q_OBJECT
private:
    // Escreve `content` num arquivo temporario com sufixo e devolve o path.
    // O QTemporaryFile e mantido vivo na lista m_temps ate o fim do teste.
    QString writeTemp(const QString &content, const QString &suffix);
    QList<QTemporaryFile *> m_temps;

private slots:
    void cleanup();

    // ── CSV ─────────────────────────────────────────────────────────────────
    void csvPtBrMilharVirgula();
    void csvEnPontoDecimal();
    void csvEnMilharEntreAspasQuebra();
    void csvSemSeparadorMilhar();
    void csvDataIso();
    void csvSeparadorPontoEVirgula();
    void csvNegativoViraPositivo();
    void csvComSimboloReal();

    // ── OFX ─────────────────────────────────────────────────────────────────
    void ofxXmlPontoDecimal();
    void ofxSgmlPontoDecimal();
};

QString TestImport::writeTemp(const QString &content, const QString &suffix)
{
    auto *tmp = new QTemporaryFile(QDir::tempPath() +
        QStringLiteral("/peubud_import_XXXXXX") + suffix);
    tmp->setAutoRemove(true);
    if (!tmp->open()) { delete tmp; return {}; }
    {
        QTextStream ts(tmp);
        ts.setEncoding(QStringConverter::Utf8);
        ts << content;
    }
    tmp->flush();
    const QString path = tmp->fileName();
    m_temps.append(tmp); // mantem aberto/vivo; fechamos no cleanup
    return path;
}

void TestImport::cleanup()
{
    qDeleteAll(m_temps);
    m_temps.clear();
}

// ── CSV ───────────────────────────────────────────────────────────────────────

void TestImport::csvPtBrMilharVirgula()
{
    // pt-BR: ponto e milhar, virgula e decimal. R$ 1.234,56 -> 123456 cents.
    const QString csv =
        QStringLiteral("data;descricao;valor\n"
                       "15/03/2025;Mercado;1.234,56\n");
    const QString path = writeTemp(csv, QStringLiteral(".csv"));

    const auto r = ImportService::fromCsv(path);
    QVERIFY2(r.isValid(), qPrintable(r.error));
    QCOMPARE(r.compras.size(), 1);
    QCOMPARE(r.compras[0].valorTotal, qint64(123456));
    QCOMPARE(r.compras[0].mesInicio, 3);
    QCOMPARE(r.compras[0].anoInicio, 2025);
}

void TestImport::csvEnPontoDecimal()
{
    // en SEM milhar: ponto e decimal, sem virgula no numero. Aqui o separador
    // de campo e ',' e o numero "1234.56" nao contem virgula -> parseia OK.
    const QString csv =
        QStringLiteral("date,description,amount\n"
                       "2025-03-15,Market,1234.56\n");
    const QString path = writeTemp(csv, QStringLiteral(".csv"));

    const auto r = ImportService::fromCsv(path);
    QVERIFY2(r.isValid(), qPrintable(r.error));
    QCOMPARE(r.compras.size(), 1);
    QCOMPARE(r.compras[0].valorTotal, qint64(123456));
}

void TestImport::csvEnMilharEntreAspasQuebra()
{
    // ACHADO (#133): o parser CSV faz split(sep) ingenuo, SEM respeitar aspas.
    // Em CSV en-US o separador de campo e ',' E o milhar tambem e ',':
    //   2025-03-15,Market,"1,234.56"
    // O split em ',' parte o campo citado em "1 | 234.56", desalinhando as
    // colunas. Resultado REAL: a coluna "amount" (ultima) vira 234.56" e/ou a
    // transacao e descartada -> erro "Nenhuma transacao encontrada".
    // Este teste FIXA o comportamento defeituoso (caracterizacao). NAO e o
    // comportamento desejado: um CSV en-US valido deveria importar 1234.56.
    const QString csv =
        QStringLiteral("date,description,amount\n"
                       "2025-03-15,Market,\"1,234.56\"\n");
    const QString path = writeTemp(csv, QStringLiteral(".csv"));

    const auto r = ImportService::fromCsv(path);
    // Comportamento observado: NAO importa o valor correto de 123456 cents.
    // Ou falha (sem transacao) ou importa um valor errado. Garantimos que
    // NAO produz o valor correto — sinalizando o bug a ser corrigido.
    const bool importouCorreto =
        r.isValid() && r.compras.size() == 1 &&
        r.compras[0].valorTotal == qint64(123456);
    QVERIFY2(!importouCorreto,
        "CSV en-US com milhar entre aspas importou correto inesperadamente — "
        "se o parser foi corrigido, atualize este teste de caracterizacao.");
}

void TestImport::csvSemSeparadorMilhar()
{
    const QString csv =
        QStringLiteral("date,description,amount\n"
                       "2025-03-15,Market,1234.56\n");
    const QString path = writeTemp(csv, QStringLiteral(".csv"));

    const auto r = ImportService::fromCsv(path);
    QVERIFY2(r.isValid(), qPrintable(r.error));
    QCOMPARE(r.compras[0].valorTotal, qint64(123456));
}

void TestImport::csvDataIso()
{
    const QString csv =
        QStringLiteral("date,description,amount\n"
                       "2025-12-31,Fim de ano,100.00\n");
    const QString path = writeTemp(csv, QStringLiteral(".csv"));

    const auto r = ImportService::fromCsv(path);
    QVERIFY2(r.isValid(), qPrintable(r.error));
    QCOMPARE(r.compras[0].mesInicio, 12);
    QCOMPARE(r.compras[0].anoInicio, 2025);
    QCOMPARE(r.compras[0].valorTotal, qint64(10000));
}

void TestImport::csvSeparadorPontoEVirgula()
{
    // Separador de campo ';' detectado por contagem; decimal ',' detectado.
    const QString csv =
        QStringLiteral("data;descricao;valor\n"
                       "01/06/2025;Conta de luz;89,90\n");
    const QString path = writeTemp(csv, QStringLiteral(".csv"));

    const auto r = ImportService::fromCsv(path);
    QVERIFY2(r.isValid(), qPrintable(r.error));
    QCOMPARE(r.compras.size(), 1);
    QCOMPARE(r.compras[0].valorTotal, qint64(8990));
}

void TestImport::csvNegativoViraPositivo()
{
    // parseAmount aplica std::abs -> "-50,00" vira 5000 cents (positivo).
    // FIXA o comportamento REAL: o sinal e descartado (modulo).
    const QString csv =
        QStringLiteral("data;descricao;valor\n"
                       "10/04/2025;Estorno;-50,00\n");
    const QString path = writeTemp(csv, QStringLiteral(".csv"));

    const auto r = ImportService::fromCsv(path);
    QVERIFY2(r.isValid(), qPrintable(r.error));
    QCOMPARE(r.compras.size(), 1);
    QCOMPARE(r.compras[0].valorTotal, qint64(5000));
}

void TestImport::csvComSimboloReal()
{
    // "R$ 1.234,56" — simbolo removido por parseAmount.
    const QString csv =
        QStringLiteral("data;descricao;valor\n"
                       "15/03/2025;Compra;R$ 1.234,56\n");
    const QString path = writeTemp(csv, QStringLiteral(".csv"));

    const auto r = ImportService::fromCsv(path);
    QVERIFY2(r.isValid(), qPrintable(r.error));
    QCOMPARE(r.compras[0].valorTotal, qint64(123456));
}

// ── OFX ───────────────────────────────────────────────────────────────────────

void TestImport::ofxXmlPontoDecimal()
{
    // OFX 2.x XML. TRNAMT sempre usa ponto decimal (machine-readable).
    const QString ofx = QStringLiteral(
        "<?xml version=\"1.0\"?>\n"
        "<OFX><BANKMSGSRSV1><STMTTRNRS><STMTRS><BANKTRANLIST>\n"
        "<STMTTRN>\n"
        "<TRNTYPE>DEBIT</TRNTYPE>\n"
        "<DTPOSTED>20250315</DTPOSTED>\n"
        "<TRNAMT>-1234.56</TRNAMT>\n"
        "<MEMO>Mercado</MEMO>\n"
        "</STMTTRN>\n"
        "</BANKTRANLIST></STMTRS></STMTTRNRS></BANKMSGSRSV1></OFX>\n");
    const QString path = writeTemp(ofx, QStringLiteral(".ofx"));

    const auto r = ImportService::fromOfx(path);
    QVERIFY2(r.isValid(), qPrintable(r.error));
    QCOMPARE(r.compras.size(), 1);
    QCOMPARE(r.compras[0].valorTotal, qint64(123456)); // abs aplicado
    QCOMPARE(r.compras[0].mesInicio, 3);
    QCOMPARE(r.compras[0].anoInicio, 2025);
    QCOMPARE(r.compras[0].nome, QStringLiteral("Mercado"));
}

void TestImport::ofxSgmlPontoDecimal()
{
    // OFX 1.x SGML (sem fechamento de tags, line-by-line).
    const QString ofx = QStringLiteral(
        "OFXHEADER:100\n"
        "DATA:OFXSGML\n"
        "<OFX>\n"
        "<STMTTRN>\n"
        "<TRNTYPE>DEBIT\n"
        "<DTPOSTED>20250601120000\n"
        "<TRNAMT>-89.90\n"
        "<MEMO>Conta de luz\n"
        "</STMTTRN>\n"
        "</OFX>\n");
    const QString path = writeTemp(ofx, QStringLiteral(".ofx"));

    const auto r = ImportService::fromOfx(path);
    QVERIFY2(r.isValid(), qPrintable(r.error));
    QCOMPARE(r.compras.size(), 1);
    QCOMPARE(r.compras[0].valorTotal, qint64(8990));
    QCOMPARE(r.compras[0].mesInicio, 6);
    QCOMPARE(r.compras[0].anoInicio, 2025);
}

QTEST_GUILESS_MAIN(TestImport)
#include "test_import.moc"
