// PeuBud - #198 (Onda U-4, A11Y-01/A11Y-03, WCAG 4.1.2 / 2.1.1)
//
// Auditoria 2026-06-22 §3.10: `grep accessibleName src` = 1 hit em todo o
// /src (só o toggle do TabShell). Botões icon-only (◀️▶️ Dashboard, 👁 login,
// 🧮 CalculadoraEdit em 10 diálogos) ficavam mudos para Orca/NVDA/VoiceOver —
// só o glifo, sem nome acessível. 👁 e 🧮 também tinham Qt::NoFocus:
// inacessíveis por teclado (Tab pulava por cima).
//
// Este teste constrói instâncias REAIS de produção (não mocks/réplicas) e
// itera QAbstractButton na árvore: todo botão com ícone e SEM texto visível
// precisa de accessibleName não-vazio (contrato exato do work-order). Para
// 👁 (LoginDialog) e 🧮 (CalculadoraEdit) verifica também StrongFocus + que
// Espaço aciona o equivalente de teclado.
//
// LoginDialog: o botão 👁 só existe em modo release (`#ifndef QT_DEBUG`) —
// QT_NO_DEBUG é forçado neste target (abaixo, CMakeLists) para exercitar
// esse ramo, independente do CMAKE_BUILD_TYPE da árvore build/.
//
// Isolamento de config (#197, incidente documentado): QSettings::setPath()
// para IniFormat E NativeFormat (enums diferentes; um só não basta) ANTES de
// qualquer instância — AppSettings/UserSession tocam QSettings na construção
// (DashboardWidget::language(), LoginDialog::loadLastName()). Zero contato
// com ~/.config real.
//
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest/QtTest>
#include <QApplication>
#include <QAbstractButton>
#include <QToolButton>
#include <QLineEdit>
#include <QSettings>
#include <QTemporaryDir>
#include <QSignalSpy>

#include "ui/LoginDialog.h"
#include "ui/widgets/DashboardWidget.h"
#include "ui/widgets/ReceiptAttachmentWidget.h"
#include "ui/widgets/CalculadoraEdit.h"
#include "ui/Theme.h"

namespace {
// Falha se algum botão icon-only (ícone presente, texto vazio) não tiver
// accessibleName — contrato exato do work-order #198 (A11Y-01).
void verificarBotoesIconOnly(QWidget *root, const QString &contexto)
{
    const auto botoes = root->findChildren<QAbstractButton *>();
    int verificados = 0;
    for (auto *btn : botoes) {
        if (!btn->icon().isNull() && btn->text().isEmpty()) {
            ++verificados;
            QVERIFY2(!btn->accessibleName().isEmpty(),
                qPrintable(QStringLiteral(
                    "%1: botão icon-only (toolTip='%2', objectName='%3') sem accessibleName")
                    .arg(contexto, btn->toolTip(), btn->objectName())));
        }
    }
    QVERIFY2(verificados > 0,
        qPrintable(QStringLiteral(
            "%1: nenhum botão icon-only encontrado na árvore — teste não exercitou nada")
            .arg(contexto)));
}
}

class TestA11yBotoesIcone : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void dashboard_botoesNavegacaoMes_temAccessibleName();
    void receiptAttachment_botaoRemover_temAccessibleName();

    void calculadoraEdit_botao_temAccessibleName();
    void calculadoraEdit_botao_focoForte_naoMaisNoFocus();
    void calculadoraEdit_espacoAbreCalculadora_A11Y03();

    void loginDialog_botaoOlho_temAccessibleName();
    void loginDialog_botaoOlho_focoForte_naoMaisNoFocus();
    void loginDialog_espacoAlternaSenha_A11Y03();

private:
    QTemporaryDir m_settingsDir;
};

void TestA11yBotoesIcone::initTestCase()
{
    QVERIFY(m_settingsDir.isValid());
    // QSettings(org,app) resolve para NativeFormat, NÃO IniFormat — são enums
    // diferentes mesmo no Linux (lição do incidente #197: setPath(IniFormat)
    // sozinho não redireciona um QSettings(org,app) real). Registrar os dois
    // cobre qualquer construtor usado por AppSettings/UserSession.
    QSettings::setPath(QSettings::IniFormat,    QSettings::UserScope, m_settingsDir.path());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
    Theme::setMode(Theme::ThemeMode::Light);
}

// ── ◀️▶️🖨️🔒 (DashboardWidget) ───────────────────────────────────────────────

void TestA11yBotoesIcone::dashboard_botoesNavegacaoMes_temAccessibleName()
{
    // Sem abrir DB: mesmo contrato usado por test_dashboard_glossario (#196) —
    // os repositórios toleram QSqlDatabase inválida (q.exec() falha, listas
    // ficam vazias); suficiente para exercitar buildUi().
    DashboardWidget dash;
    verificarBotoesIconOnly(&dash, QStringLiteral("DashboardWidget"));
}

// ── 🗑️ (ReceiptAttachmentWidget) ─────────────────────────────────────────────

void TestA11yBotoesIcone::receiptAttachment_botaoRemover_temAccessibleName()
{
    ReceiptAttachmentWidget w;
    verificarBotoesIconOnly(&w, QStringLiteral("ReceiptAttachmentWidget"));
}

// ── 🧮 (CalculadoraEdit — reusado em 10 diálogos) ────────────────────────────

void TestA11yBotoesIcone::calculadoraEdit_botao_temAccessibleName()
{
    CalculadoraEdit edit;
    verificarBotoesIconOnly(&edit, QStringLiteral("CalculadoraEdit"));
}

void TestA11yBotoesIcone::calculadoraEdit_botao_focoForte_naoMaisNoFocus()
{
    CalculadoraEdit edit;
    auto *btn = edit.findChild<QToolButton *>();
    QVERIFY2(btn, "CalculadoraEdit sem QToolButton filho (m_btn)");
    QCOMPARE(btn->focusPolicy(), Qt::StrongFocus);
}

void TestA11yBotoesIcone::calculadoraEdit_espacoAbreCalculadora_A11Y03()
{
    CalculadoraEdit edit;
    auto *btn = edit.findChild<QToolButton *>();
    QVERIFY(btn);

    QSignalSpy spy(btn, &QToolButton::clicked);
    // QTest::keyClick entrega o evento direto ao widget (não depende de
    // show()/janela ativa) — Espaço deve disparar clicked() via animateClick()
    // padrão do QAbstractButton (StrongFocus habilita o Tab; o clicked() já
    // era conectado à abertura da calculadora — sem handler extra necessário).
    QTest::keyClick(btn, Qt::Key_Space);
    QVERIFY2(spy.count() >= 1,
        "Espaço no botão da calculadora não disparou clicked() — regressão A11Y-03");
}

// ── 👁 (LoginDialog, hold-to-reveal) ─────────────────────────────────────────
// Só existe em modo release (#ifndef QT_DEBUG) — este target força
// QT_NO_DEBUG (CMakeLists) para exercitar esse ramo.

void TestA11yBotoesIcone::loginDialog_botaoOlho_temAccessibleName()
{
    LoginDialog login;
    verificarBotoesIconOnly(&login, QStringLiteral("LoginDialog"));
}

void TestA11yBotoesIcone::loginDialog_botaoOlho_focoForte_naoMaisNoFocus()
{
    LoginDialog login;
    auto *btnOlho = login.findChild<QToolButton *>();
    QVERIFY2(btnOlho, "LoginDialog sem QToolButton (m_btnOlho) — build em QT_DEBUG?");
    QCOMPARE(btnOlho->focusPolicy(), Qt::StrongFocus);
}

void TestA11yBotoesIcone::loginDialog_espacoAlternaSenha_A11Y03()
{
    LoginDialog login;
    auto *btnOlho = login.findChild<QToolButton *>();
    QVERIFY(btnOlho);

    // O campo de senha é o único QLineEdit com echoMode==Password (o de nome
    // não define echoMode, fica no default Normal).
    QLineEdit *senha = nullptr;
    for (auto *le : login.findChildren<QLineEdit *>()) {
        if (le->echoMode() == QLineEdit::Password) { senha = le; break; }
    }
    QVERIFY2(senha, "LoginDialog sem QLineEdit de senha — build em QT_DEBUG?");

    QCOMPARE(senha->echoMode(), QLineEdit::Password); // estado inicial: oculta

    // #209: Espaço é HOLD-to-reveal, igual ao mouse. Contrato do eventFilter
    // do LoginDialog: KeyPress de Espaço revela (Normal); KeyRelease oculta
    // (Password). Ambos consumidos (return true), sem o animateClick() padrão.
    QTest::keyPress(btnOlho, Qt::Key_Space);
    QCOMPARE(senha->echoMode(), QLineEdit::Normal);   // segurando: revelada

    QTest::keyRelease(btnOlho, Qt::Key_Space);
    QCOMPARE(senha->echoMode(), QLineEdit::Password); // soltou: oculta de novo
}

QTEST_MAIN(TestA11yBotoesIcone)
#include "test_a11y_botoes_icone.moc"
