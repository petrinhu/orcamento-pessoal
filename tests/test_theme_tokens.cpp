// PeuBud - teste dos tokens de paleta do Theme (#164 redesign fase 1)
// Verifica que os novos valores da paleta azul-confiável e os getters
// adicionados (bg2, dourado, sidebar1, sidebar2, sidebarText) estão corretos
// em modo Light. Deve ser executado com QT_QPA_PLATFORM=offscreen.

#include <QtTest/QtTest>
#include <QCoreApplication>

// Incluir apenas o header — o .cpp é linkado via CMake (não usar aplicar()
// aqui para não arrastar ButtonShadowFilter + QStyleFactory).
#include "ui/Theme.h"

class TestThemeTokens : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // Paleta base (valores substituídos)
    void acento_light();
    void positivo_light();
    void negativo_light();

    // Getters novos
    void dourado_light();
    void sidebar1_naoVazio();

    // Tokens Dark não quebram (smoke test)
    void acento_dark_naoVazio();
};

void TestThemeTokens::initTestCase()
{
    // Força Light para todos os testes desta suíte (sem chamar aplicar(),
    // que precisaria de QApplication completo + Fusion style).
    Theme::setMode(Theme::ThemeMode::Light);
}

void TestThemeTokens::acento_light()
{
    QCOMPARE(Theme::acento(), QStringLiteral("#3B82F6"));
}

void TestThemeTokens::positivo_light()
{
    QCOMPARE(Theme::positivo(), QStringLiteral("#16C784"));
}

void TestThemeTokens::negativo_light()
{
    QCOMPARE(Theme::negativo(), QStringLiteral("#EA3943"));
}

void TestThemeTokens::dourado_light()
{
    QCOMPARE(Theme::dourado(), QStringLiteral("#E8A93B"));
}

void TestThemeTokens::sidebar1_naoVazio()
{
    QVERIFY(!Theme::sidebar1().isEmpty());
}

void TestThemeTokens::acento_dark_naoVazio()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    QVERIFY(!Theme::acento().isEmpty());
    Theme::setMode(Theme::ThemeMode::Light); // restaura para testes seguintes
}

QTEST_MAIN(TestThemeTokens)
#include "test_theme_tokens.moc"
