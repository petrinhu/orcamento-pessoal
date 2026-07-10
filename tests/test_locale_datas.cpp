// PeuBud - #197 (Onda U-4, I18N-05): formato de data "dd/MM/yyyy" hardcoded
// em 29 pontos / 22 arquivos de UI (fora ImportService/ExportService,
// corretamente fixos -- formato de arquivo/interop). Fix: uifmt::
// dataDisplayFormat() (+ variantes curta/ano-curto/mês-ano) resolve
// dd/MM x MM/dd por AppSettings::language() -- ordem dia/mês é ambígua
// entre locales (ao contrário de mês/ano).
//
// Isolamento de QSettings: ver comentário em test_locale_moeda.cpp — mesmo
// padrão (QSettings::setPath para QTemporaryDir antes do 1º
// AppSettings::instance()).

#include <QtTest/QtTest>
#include <QDate>
#include <QSettings>
#include <QTemporaryDir>

#include "core/settings/AppSettings.h"
#include "ui/UiFormatters.h"

class TestLocaleDatas : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void ptBrDiaAntesDoMes();
    void enIntlMesAntesDoDia();
    void formatoCurtoSemAno();
    void formatoAnoCurto();
    void mesAnoNaoMudaEntreLocales();
    void aplicadoViaQDateToString();

private:
    QTemporaryDir m_settingsDir;
};

void TestLocaleDatas::initTestCase()
{
    QVERIFY(m_settingsDir.isValid());
    // Ver comentário equivalente em test_locale_moeda.cpp: NativeFormat !=
    // IniFormat para fins de setPath(), mesmo no Linux -- registrar os dois.
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestLocaleDatas::ptBrDiaAntesDoMes()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    QCOMPARE(uifmt::dataDisplayFormat(), QStringLiteral("dd/MM/yyyy"));
}

// ESPEC RED->GREEN (#197/I18N-05): antes o app SEMPRE usava "dd/MM/yyyy",
// mesmo em EN (onde a convenção é mês antes do dia).
void TestLocaleDatas::enIntlMesAntesDoDia()
{
    AppSettings::instance().setLanguage(AppSettings::Language::EnIntl);
    QCOMPARE(uifmt::dataDisplayFormat(), QStringLiteral("MM/dd/yyyy"));
}

void TestLocaleDatas::formatoCurtoSemAno()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    QCOMPARE(uifmt::dataDisplayFormatCurta(), QStringLiteral("dd/MM"));

    AppSettings::instance().setLanguage(AppSettings::Language::EnIntl);
    QCOMPARE(uifmt::dataDisplayFormatCurta(), QStringLiteral("MM/dd"));
}

void TestLocaleDatas::formatoAnoCurto()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    QCOMPARE(uifmt::dataDisplayFormatAnoCurto(), QStringLiteral("dd/MM/yy"));

    AppSettings::instance().setLanguage(AppSettings::Language::EnIntl);
    QCOMPARE(uifmt::dataDisplayFormatAnoCurto(), QStringLiteral("MM/dd/yy"));
}

// Mês/ano não é ambíguo entre locales (ao contrário de dia/mês) -- mesmo
// formato nos dois idiomas, mas centralizado (1 único ponto de verdade).
void TestLocaleDatas::mesAnoNaoMudaEntreLocales()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    QCOMPARE(uifmt::mesAnoDisplayFormat(), QStringLiteral("MM/yyyy"));

    AppSettings::instance().setLanguage(AppSettings::Language::EnIntl);
    QCOMPARE(uifmt::mesAnoDisplayFormat(), QStringLiteral("MM/yyyy"));
}

// Prova de integração: QDate::toString(uifmt::dataDisplayFormat()) produz o
// texto esperado nos dois idiomas para a MESMA data (25/dez/2025).
void TestLocaleDatas::aplicadoViaQDateToString()
{
    const QDate natal(2025, 12, 25);

    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    QCOMPARE(natal.toString(uifmt::dataDisplayFormat()), QStringLiteral("25/12/2025"));

    AppSettings::instance().setLanguage(AppSettings::Language::EnIntl);
    QCOMPARE(natal.toString(uifmt::dataDisplayFormat()), QStringLiteral("12/25/2025"));
}

QTEST_GUILESS_MAIN(TestLocaleDatas)
#include "test_locale_datas.moc"
