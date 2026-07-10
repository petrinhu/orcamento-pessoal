// PeuBud - #197 (Onda U-4, I18N-01): UiFormatters::formatCentavosBRL
// concatenava "R$" + vírgula fixos, ignorando AppSettings::language()/
// defaultCurrency() (~24 call-sites, auditoria 2026-06-22 §3.8). Em modo
// EN/USD mostrava "R$ 1.234,56" -- moeda e separadores errados.
//
// Fix: aritmética 100% INTEIRA (regra do projeto -- sem double). Agrupamento
// de milhar via QLocale::toString(qint64); símbolo via
// ParcelaService::simboloMoeda(defaultCurrency()); separador decimal e
// posição do símbolo por switch(AppSettings::language()).
//
// AppSettings é singleton sobre QSettings("petrinhu","PeuBud") -- MESMA
// config real do app instalado. Ao contrário de outros testes que só tocam
// um bool simples (ex. test_table_skin), setDefaultCurrency() aqui grava a
// flag interna "currency_manual" (sem setter público para desfazer) -- um
// save/restore de valor não é suficiente para não deixar resíduo. Fix:
// QSettings::setPath() ANTES do 1º AppSettings::instance() redireciona todo
// QSettings(org,app) subsequente para um QTemporaryDir isolado -- zero
// contato com a config real do usuário.

#include <QtTest/QtTest>
#include <QSettings>
#include <QTemporaryDir>

#include "core/settings/AppSettings.h"
#include "ui/UiFormatters.h"

class TestLocaleMoeda : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void enIntlUsdFormataComoDolar();
    void ptBrBrlFormataComoReal();
    void enIntlNegativoSinalAntesDoSimbolo();
    void ptBrNegativoSinalDepoisDoSimbolo();
    void zeroEmAmbosLocales();
    void agrupamentoDeMilharPorLocale();
    void moedaAtivaNaoFicaHardcodedEmBrl();

private:
    QTemporaryDir m_settingsDir;
};

void TestLocaleMoeda::initTestCase()
{
    QVERIFY(m_settingsDir.isValid());
    // Precisa rodar ANTES de qualquer AppSettings::instance() (singleton
    // Meyer's, 1ª chamada constrói o QSettings interno) -- initTestCase()
    // roda antes de todos os slots de teste, então é seguro aqui.
    //
    // IMPORTANTE: AppSettings::m_s usa o construtor QSettings(org,app), que
    // resolve para QSettings::NativeFormat -- NÃO QSettings::IniFormat (são
    // enums DIFERENTES, mesmo no Linux onde os dois formatos são
    // ini-compatíveis por baixo). setPath(IniFormat, ...) sozinho NÃO
    // redireciona um QSettings(org,app): ele ainda vai para o
    // ~/.config/petrinhu/PeuBud.conf real. Registrar os dois formatos cobre
    // qualquer construtor usado.
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
}

// ESPEC RED->GREEN (#197/I18N-01): EN + USD -> "$1,234.56", NUNCA "R$ ...".
void TestLocaleMoeda::enIntlUsdFormataComoDolar()
{
    AppSettings::instance().setLanguage(AppSettings::Language::EnIntl);
    AppSettings::instance().setDefaultCurrency(QStringLiteral("USD"));

    const QString out = uifmt::formatCentavosBRL(123456); // R$ 1.234,56 em centavos
    QCOMPARE(out, QStringLiteral("$1,234.56"));
    QVERIFY2(!out.contains(QStringLiteral("R$")),
        "regressao: moeda EN ainda mostrando R$ hardcoded");
}

void TestLocaleMoeda::ptBrBrlFormataComoReal()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    AppSettings::instance().setDefaultCurrency(QStringLiteral("BRL"));

    QCOMPARE(uifmt::formatCentavosBRL(123456), QStringLiteral("R$ 1.234,56"));
}

void TestLocaleMoeda::enIntlNegativoSinalAntesDoSimbolo()
{
    AppSettings::instance().setLanguage(AppSettings::Language::EnIntl);
    AppSettings::instance().setDefaultCurrency(QStringLiteral("USD"));

    QCOMPARE(uifmt::formatCentavosBRL(-50), QStringLiteral("-$0.50"));
}

void TestLocaleMoeda::ptBrNegativoSinalDepoisDoSimbolo()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    AppSettings::instance().setDefaultCurrency(QStringLiteral("BRL"));

    QCOMPARE(uifmt::formatCentavosBRL(-50), QStringLiteral("R$ -0,50"));
}

void TestLocaleMoeda::zeroEmAmbosLocales()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    AppSettings::instance().setDefaultCurrency(QStringLiteral("BRL"));
    QCOMPARE(uifmt::formatCentavosBRL(0), QStringLiteral("R$ 0,00"));

    AppSettings::instance().setLanguage(AppSettings::Language::EnIntl);
    AppSettings::instance().setDefaultCurrency(QStringLiteral("USD"));
    QCOMPARE(uifmt::formatCentavosBRL(0), QStringLiteral("$0.00"));
}

// Aritmética inteira (sem double): grouping via QLocale::toString(qint64).
void TestLocaleMoeda::agrupamentoDeMilharPorLocale()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    AppSettings::instance().setDefaultCurrency(QStringLiteral("BRL"));
    QCOMPARE(uifmt::formatCentavosBRL(123456789), QStringLiteral("R$ 1.234.567,89"));

    AppSettings::instance().setLanguage(AppSettings::Language::EnIntl);
    AppSettings::instance().setDefaultCurrency(QStringLiteral("USD"));
    QCOMPARE(uifmt::formatCentavosBRL(123456789), QStringLiteral("$1,234,567.89"));
}

// Antes do fix, formatCentavosBRL SEMPRE mostrava "R$" mesmo com outra moeda
// ativa (o nome do método era literal -- sempre BRL). Agora segue a moeda
// padrão ativa (AppSettings::defaultCurrency()), símbolo via
// ParcelaService::simboloMoeda.
void TestLocaleMoeda::moedaAtivaNaoFicaHardcodedEmBrl()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    AppSettings::instance().setDefaultCurrency(QStringLiteral("EUR"));

    const QString out = uifmt::formatCentavosBRL(123456);
    QVERIFY2(out.startsWith(QStringLiteral("€")),
        qPrintable(QStringLiteral("esperado simbolo EUR, obtido '%1'").arg(out)));
    QVERIFY(!out.contains(QStringLiteral("R$")));
}

QTEST_GUILESS_MAIN(TestLocaleMoeda)
#include "test_locale_moeda.moc"
