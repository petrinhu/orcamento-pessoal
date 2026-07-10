// PeuBud — regressão #174: Enter no login dispara Entrar, não Cancelar.
//
// Verifica três contratos estabelecidos pelo fix:
//   1. m_btnEntrar->isDefault() == true    — Enter dispara Entrar.
//   2. m_btnCancelar->autoDefault() == false — Cancelar nunca captura Enter.
//   3. returnPressed no campo nome invoca onEntrar() quando habilitado.
//
// Executar com QT_QPA_PLATFORM=offscreen (constrói QDialog/QPushButton/QLineEdit).
// Em QT_DEBUG (modo headless de CI): não há campo senha; onEntrar() aceita
// direto sem criptografia — só nome não-vazio é necessário.

#include <QtTest/QtTest>
#include <QApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QDialog>
#include <QSettings>
#include <QTemporaryDir>

#include "ui/LoginDialog.h"

class TestLoginEnter : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // #174-C1: Entrar deve ser o botão default (Enter o aciona).
    void entrar_ehDefault();

    // #174-C2: Cancelar não deve ser autoDefault (não captura Enter como fallback).
    void cancelar_naoEhAutoDefault();

    // #174-C3: returnPressed no campo nome invoca accept() quando form válido.
    void returnPressed_nome_aceita();

private:
    QTemporaryDir m_settingsDir;
};

// #198 (achado durante a varredura de A11Y-01/03): returnPressed_nome_aceita()
// completa o fluxo onEntrar() → UserSession::login("TestUser", …) →
// QSettings("OrcamentoPessoal","Session").setValue("lastUser","TestUser") —
// SEM isolamento, sobrescrevendo o lastUser REAL do usuário em
// ~/.config/OrcamentoPessoal/Session.conf. Fix: mesmo padrão de
// QSettings::setPath (Ini+Native — enums diferentes, os dois cobrem qualquer
// construtor) usado em test_locale_moeda/test_locale_datas (#197).
void TestLoginEnter::initTestCase()
{
    QVERIFY(m_settingsDir.isValid());
    QSettings::setPath(QSettings::IniFormat,    QSettings::UserScope, m_settingsDir.path());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
}

// ── Helpers internos ─────────────────────────────────────────────────────────

// Localiza um filho por classe e objectName opcional.
// Lança QFAIL se não encontrar — evita nullptr deref silencioso no teste.
template<typename T>
static T *findBtn(QWidget *parent, const QString &name = {})
{
    const auto children = parent->findChildren<T *>();
    for (auto *c : children) {
        if (name.isEmpty() || c->text() == name)
            return c;
    }
    return nullptr;
}

// ── Testes ───────────────────────────────────────────────────────────────────

void TestLoginEnter::entrar_ehDefault()
{
    LoginDialog dlg;

    // Localiza o botão "Entrar" pelo texto.
    auto *btnEntrar = findBtn<QPushButton>(&dlg, dlg.tr("Entrar"));
    QVERIFY2(btnEntrar, "Botão 'Entrar' não encontrado no LoginDialog");

    // Contrato #174-C1: Enter deve acionar Entrar.
    QVERIFY2(btnEntrar->isDefault(),
             "m_btnEntrar->isDefault() é false — Enter pode acionar o Cancelar (regressão #174)");
}

void TestLoginEnter::cancelar_naoEhAutoDefault()
{
    LoginDialog dlg;

    auto *btnCancelar = findBtn<QPushButton>(&dlg, dlg.tr("Cancelar"));
    QVERIFY2(btnCancelar, "Botão 'Cancelar' não encontrado no LoginDialog");

    // Contrato #174-C2: Cancelar nunca captura Enter — nem como default explícito
    // nem como fallback quando Entrar está desabilitado.
    QVERIFY2(!btnCancelar->autoDefault(),
             "m_btnCancelar->autoDefault() é true — pode capturar Enter (regressão #174)");
}

void TestLoginEnter::returnPressed_nome_aceita()
{
    // Constrói dialog e localiza campo nome.
    LoginDialog dlg;
    auto *nome = dlg.findChild<QLineEdit *>();
    QVERIFY2(nome, "QLineEdit de nome não encontrado no LoginDialog");

    auto *btnEntrar = findBtn<QPushButton>(&dlg, dlg.tr("Entrar"));
    QVERIFY2(btnEntrar, "Botão 'Entrar' não encontrado");

    // Garante estado limpo: limpa o campo (pode estar pre-filled via UserSession)
    // e verifica que Entrar fica desabilitado com nome vazio.
    nome->clear();
    QVERIFY2(!btnEntrar->isEnabled(),
             "Entrar deve estar desabilitado com nome vazio");

    // Preenche nome → habilita Entrar (em QT_DEBUG não há senha).
    nome->setText(QStringLiteral("TestUser"));
    // onNomeChanged é conectado via textChanged — dispara automaticamente.
    QVERIFY2(btnEntrar->isEnabled(),
             "Entrar permanece desabilitado após nome preenchido (QT_DEBUG)");

    // Contrato #174-C3: returnPressed com form válido chama accept().
    // Usa QSignalSpy para capturar o sinal accepted() do QDialog.
    QSignalSpy spy(&dlg, &QDialog::accepted);
    QTest::keyClick(nome, Qt::Key_Return);
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestLoginEnter)
#include "test_login_enter.moc"
