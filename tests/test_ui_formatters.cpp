// PeuBud - #199 (Onda U, QA-03): uifmt::parseCentavos("1.234") devolvia
// R$ 1,23 (truncamento ~1000x) em vez de R$ 1.234,00.
//
// Causa: ramo "só ponto, sem vírgula" caía direto em QString::toDouble(),
// que interpreta o ponto como decimal -- "1.234".toDouble() == 1.234.
//
// Decisão de produto (documentada na auditoria, 2 opções levantadas):
//   (a) heurística de milhar: grupos EXATOS de 3 dígitos após ponto(s) são
//       separador de milhar (dinheiro nunca tem 3 casas decimais, então
//       nunca é ambíguo com decimal) -- esforço P, sem tocar UI.
//   (b) QValidator/máscara "0,00" em ~10 diálogos -- esforço bem maior.
// Adotada (a) nesta rodada; ver comentário em UiFormatters.h/.cpp.
//
// Gêmeo: ParcelaService::parsearValor recebeu o mesmo fix (ver
// test_parcela_service.cpp::parsearValorBRComDecimal para o caso já
// coberto; este arquivo foca no ramo "só ponto" especificamente).

#include <QtTest/QtTest>

#include "ui/UiFormatters.h"

class TestUiFormatters : public QObject
{
    Q_OBJECT
private slots:
    // ESPEC RED->GREEN (#199/QA-03): "1.234" -> 123400 centavos (R$ 1.234,00),
    // NAO 123 centavos (R$ 1,23 -- bug original).
    void parseCentavosSoPontoGrupoDeMilharTratadoComoMilhar()
    {
        QCOMPARE(uifmt::parseCentavos(QStringLiteral("1.234")), qint64(123400));
        QVERIFY2(uifmt::parseCentavos(QStringLiteral("1.234")) != qint64(123),
            "regressao: '1.234' voltou a truncar para R$ 1,23");
    }

    void parseCentavosSoPontoMultiplosGruposDeMilhar()
    {
        QCOMPARE(uifmt::parseCentavos(QStringLiteral("12.345.678")), qint64(1234567800));
    }

    // Decimal com ponto continua funcionando (nao regressao): "1234.56" nao
    // casa com grupos de 3 digitos EXATOS apos o ponto (so 2), entao segue
    // sendo tratado como decimal.
    void parseCentavosDecimalComPontoContinuaDecimal()
    {
        QCOMPARE(uifmt::parseCentavos(QStringLiteral("1234.56")), qint64(123456));
    }

    void parseCentavosDecimalCurtoComPontoContinuaDecimal()
    {
        // "12.5" nao casa com grupo de 3 digitos -> decimal (12,50).
        QCOMPARE(uifmt::parseCentavos(QStringLiteral("12.5")), qint64(1250));
    }

    // Formato pt-BR completo (milhar + decimal) continua funcionando.
    void parseCentavosMilharEDecimalCombinados()
    {
        QCOMPARE(uifmt::parseCentavos(QStringLiteral("1.234,56")), qint64(123456));
    }

    void parseCentavosApenasVirgulaContinuaDecimal()
    {
        QCOMPARE(uifmt::parseCentavos(QStringLiteral("1234,56")), qint64(123456));
    }

    void parseCentavosComPrefixoRsEEspacos()
    {
        QCOMPARE(uifmt::parseCentavos(QStringLiteral("R$ 1.234")), qint64(123400));
    }

    void parseCentavosTextoInvalidoRetornaZero()
    {
        QCOMPARE(uifmt::parseCentavos(QStringLiteral("abc")), qint64(0));
    }
};

QTEST_GUILESS_MAIN(TestUiFormatters)
#include "test_ui_formatters.moc"
