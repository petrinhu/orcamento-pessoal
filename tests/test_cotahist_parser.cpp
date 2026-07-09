// PeuBud - #199 (Onda U, QA-01): cotahist::parseLine lia o
// campo errado do layout posicional B3 COTAHIST.
//
// Layout B3 COTAHIST (registro tipo "01", 1-indexed):
//   PREMED (preco medio)      96-108
//   PREULT (fechamento/ultimo) 109-121
// O codigo original fazia `line.mid(95, 13)` (0-indexed) para popular
// `p.fechamento` -- isso cobre as posicoes 96-108 (1-indexed), ou seja,
// PREMED, NAO PREULT. O proprio comentario do codigo documentava o campo
// errado. Fix: `line.mid(108, 13)` (posicoes 109-121 = PREULT).
//
// CotahistProvider.cpp/.h (rede/DB/download, Bolsa congelada) permanecem
// FORA do CMakeLists do app. O parser puro foi extraido para
// services/CotahistParser.h/.cpp (funcoes livres, sem QObject/rede/DB,
// mesmo padrao de BankPDFParsers) -- SO esse pedaco e compilado aqui, sem
// reintroduzir o provider (e seu codigo de download nunca portado p/ Qt6)
// no build.

#include <QtTest/QtTest>

#include "services/CotahistParser.h"
#include "models/PrecoAtivo.h"

class TestCotahistParser : public QObject
{
    Q_OBJECT

private:
    // Monta uma linha COTAHIST sintetica (245 chars) com campos conhecidos.
    // PREABE=25.50 PREMAX=26.00 PREMIN=25.00 PREMED=25.75 PREULT=25.99.
    static QString linhaFixture()
    {
        QString l;
        l += QStringLiteral("01");                    // 1-2   TIPREG
        l += QStringLiteral("20240102");               // 3-10  DATA_PREGAO
        l += QStringLiteral("02");                    // 11-12 CODBDI
        l += QStringLiteral("PETR4").leftJustified(12); // 13-24 CODNEG
        l += QStringLiteral("010");                    // 25-27 TPMERC
        l += QStringLiteral("PETROBRAS PN").leftJustified(12); // 28-39 NOMRES
        l += QStringLiteral("PN").leftJustified(10);    // 40-49 ESPECI
        l += QString().leftJustified(3);                // 50-52 PRAZOT
        l += QStringLiteral("R$").leftJustified(4);      // 53-56 MODREF
        l += QStringLiteral("0000000002550");           // 57-69  PREABE  = 25,50
        l += QStringLiteral("0000000002600");           // 70-82  PREMAX  = 26,00
        l += QStringLiteral("0000000002500");           // 83-95  PREMIN  = 25,00
        l += QStringLiteral("0000000002575");           // 96-108 PREMED  = 25,75 (NAO e o fechamento)
        l += QStringLiteral("0000000002599");           // 109-121 PREULT = 25,99 (fechamento correto)
        l += QString().leftJustified(13, QLatin1Char('0')); // 122-134 PREOFC
        l += QString().leftJustified(13, QLatin1Char('0')); // 135-147 PREOFV
        l += QString().leftJustified(5,  QLatin1Char('0')); // 148-152 TOTNEG
        l += QString().leftJustified(18, QLatin1Char('0')); // 153-170 QUATOT
        l += QString().leftJustified(18, QLatin1Char('0')); // 171-188 VOLTOT
        l += QString().leftJustified(13, QLatin1Char('0')); // 189-201 PREEXE
        l += QStringLiteral("0");                            // 202 INDOPC
        l += QString().leftJustified(8,  QLatin1Char('0')); // 203-210 DATVEN
        l += QStringLiteral("0000001");                      // 211-217 FATCOT
        l += QString().leftJustified(13, QLatin1Char('0')); // 218-230 PTOEXE
        l += QString().leftJustified(12);                    // 231-242 CODISI
        l += QStringLiteral("000");                          // 243-245 DISMES
        return l;
    }

private slots:
    // ESPEC RED->GREEN (#199/QA-01): fechamento DEVE ser PREULT (25,99),
    // nao PREMED (25,75). Antes do fix, este teste falhava com 2575.
    void fechamentoUsaPreultNaoPremed()
    {
        const QString linha = linhaFixture();
        QCOMPARE(linha.length(), 245);

        const PrecoAtivo p = cotahist::parseLine(linha);

        QCOMPARE(p.ticker, QStringLiteral("PETR4"));
        QCOMPARE(p.data, QDate(2024, 1, 2));
        QCOMPARE(p.abertura,   qint64(2550));
        QCOMPARE(p.maxima,     qint64(2600));
        QCOMPARE(p.minima,     qint64(2500));
        QVERIFY2(p.fechamento != qint64(2575),
            "regressao: fechamento voltou a ler PREMED (mid(95,13))");
        QCOMPARE(p.fechamento, qint64(2599)); // PREULT — fechamento real
    }

    void isValidRecordAceitaTipo01ComTamanhoMinimo()
    {
        QVERIFY(cotahist::isValidRecord(linhaFixture()));
    }

    void isValidRecordRejeitaLinhaCurta()
    {
        QVERIFY(!cotahist::isValidRecord(QStringLiteral("01curta")));
    }

    void isValidRecordRejeitaTipoDiferenteDe01()
    {
        QString l = linhaFixture();
        l.replace(0, 2, QStringLiteral("99")); // registro de cabecalho/trailer
        QVERIFY(!cotahist::isValidRecord(l));
    }

    void parseCotahistLineRejeitaLinhaVazia()
    {
        const PrecoAtivo p = cotahist::parseLine(QString());
        QVERIFY(p.ticker.isEmpty());
    }
};

QTEST_MAIN(TestCotahistParser)
#include "test_cotahist_parser.moc"
