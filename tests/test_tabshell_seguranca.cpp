// PeuBud — #201 (Onda U-4), 3 fixes bounded de AUDITORIA_2026-06-22 §3.6/3.7:
//
//   SEC-01 · LoginDialog::onEntrar() não limpava a senha em claro no sucesso —
//            m_senha vivia até o fim do processo (só a chave derivada era
//            zerada). Fix: m_senha->clear() antes de accept().
//   FE-01  · TabShell::montarCallout usava 3× setStyleSheet() inline no
//            construtor, matando o QSS global do Theme — callouts ficavam
//            presos na cor do boot ao trocar tema. Fix: removidos, o QSS
//            global (#helpDescBox/#helpInstrBox/#helpEyebrow) assume.
//   FE-03  · TabShell::setContent fazia setParent(nullptr) sem deleteLater()
//            — leak se setContent for reusado. Fix: deleteLater() antes de
//            trocar o ponteiro.
//
// Este target força QT_NO_DEBUG (CMakeLists) para exercitar o ramo release
// do LoginDialog (campo de senha + fluxo de cripto/libsecret real — mesmo
// padrão de test_a11y_botoes_icone). onEntrar() toca o cofre real do sistema
// (KWallet/GNOME Keyring via libsecret) sob a chave de teste única gerada em
// runtime; removida no cleanup para não deixar resíduo.
//
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest/QtTest>
#include <QApplication>
#include <QLineEdit>
#include <QFrame>
#include <QLabel>
#include <QPointer>
#include <QSettings>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QDateTime>

#include "ui/LoginDialog.h"
#include "ui/widgets/TabShell.h"
#include "ui/Theme.h"
#include "core/secret/SecretStoreService.h"

class TestTabShellSeguranca : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    // #201/SEC-01
    void loginDialog_senhaLimpaAposLoginValido();

    // #201/FE-01
    void tabShell_calloutSemStyleSheetInline_segueQssGlobal();

    // #201/FE-03
    void tabShell_setContent_deletaConteudoAntigo();

private:
    QTemporaryDir m_settingsDir;
};

void TestTabShellSeguranca::initTestCase()
{
    QVERIFY(m_settingsDir.isValid());
    // Isolamento de config (#197): sem isso, LoginDialog/AppSettings tocam o
    // ~/.config real (lastUser, tema etc.) durante o teste.
    QSettings::setPath(QSettings::IniFormat,    QSettings::UserScope, m_settingsDir.path());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestTabShellSeguranca::cleanup()
{
    // Cada teste de tema deixa o modo em Light para não vazar estado entre
    // funções de teste (ordem de execução do QtTest é sequencial, mas não
    // deve ser assumida entre arquivos/rodadas).
    Theme::setMode(Theme::ThemeMode::Light);
}

// ── SEC-01 ───────────────────────────────────────────────────────────────────

void TestTabShellSeguranca::loginDialog_senhaLimpaAposLoginValido()
{
    LoginDialog dlg;

    // O campo de senha é o único QLineEdit com echoMode==Password (mesmo
    // discriminador usado em test_a11y_botoes_icone); o de nome fica Normal.
    QLineEdit *nome  = nullptr;
    QLineEdit *senha = nullptr;
    for (auto *le : dlg.findChildren<QLineEdit *>()) {
        if (le->echoMode() == QLineEdit::Password) senha = le;
        else nome = le;
    }
    QVERIFY2(nome,  "LoginDialog sem QLineEdit de nome");
    QVERIFY2(senha, "LoginDialog sem QLineEdit de senha — build em QT_DEBUG?");

    // Usuário único por execução: evita colidir com salt de execuções
    // anteriores no cofre real do sistema (novo usuário → branch de geração
    // de salt em onEntrar()).
    const QString usuarioTeste = QStringLiteral("peubud_sec01_%1")
        .arg(QDateTime::currentMSecsSinceEpoch());

    nome->setText(usuarioTeste);
    senha->setText(QStringLiteral("Senha123X")); // 8+, maiúscula, dígito — passa senhaValida()

    QVERIFY2(!senha->text().isEmpty(), "pré-condição: senha deveria estar preenchida");

    // Dispara o mesmo caminho de produção que Enter no campo de senha aciona
    // (connect(m_senha, returnPressed, ...) só existe no ramo release).
    QSignalSpy spy(&dlg, &QDialog::accepted);
    QTest::keyClick(senha, Qt::Key_Return);
    QCOMPARE(spy.count(), 1);

    QVERIFY2(senha->text().isEmpty(),
        "m_senha->text() não está vazio após login bem-sucedido — regressão SEC-01 "
        "(senha em claro sobrevive na memória do widget)");

    // Cleanup: remove a entrada de teste do cofre real (salt de 16 bytes,
    // sem valor sensível, mas não deixar resíduo em KWallet/GNOME Keyring).
    SecretStoreService store(QStringLiteral("orcamento-pessoal"));
    store.remove(usuarioTeste.toLower() + QStringLiteral("/salt"));
}

// ── FE-01 ────────────────────────────────────────────────────────────────────

void TestTabShellSeguranca::tabShell_calloutSemStyleSheetInline_segueQssGlobal()
{
    Theme::setMode(Theme::ThemeMode::Light);
    TabShell shell(QStringLiteral("Teste"));

    auto *descBox  = shell.findChild<QFrame *>(QStringLiteral("helpDescBox"));
    auto *instrBox = shell.findChild<QFrame *>(QStringLiteral("helpInstrBox"));
    auto *eyebrow  = shell.findChild<QLabel *>(QStringLiteral("helpEyebrow"));
    QVERIFY2(descBox,  "TabShell sem QFrame#helpDescBox");
    QVERIFY2(instrBox, "TabShell sem QFrame#helpInstrBox");
    QVERIFY2(eyebrow,  "TabShell sem QLabel#helpEyebrow");

    // Contrato central do fix: nenhum dos 3 widgets pode ter setStyleSheet()
    // inline — senão a cascata do QSS global (seletor #objectName) é morta
    // para aquele widget e trocar de tema não muda mais a cor dele.
    QVERIFY2(descBox->styleSheet().isEmpty(),
        "helpDescBox com setStyleSheet() inline — mata o QSS global (regressão FE-01)");
    QVERIFY2(instrBox->styleSheet().isEmpty(),
        "helpInstrBox com setStyleSheet() inline — mata o QSS global (regressão FE-01)");
    QVERIFY2(eyebrow->styleSheet().isEmpty(),
        "helpEyebrow com setStyleSheet() inline — mata o QSS global (regressão FE-01)");

    // Prova positiva: os tokens Light/Dark do callout são distintos (Theme.cpp) —
    // trocar de tema deve refletir no QSS *global* da aplicação (é dali que o
    // callout passa a herdar a cor, sem inline pra travar o valor do boot).
    const QString bgLight      = Theme::helpDescBg();
    const QString eyebrowLight = Theme::helpEyebrow();

    Theme::setMode(Theme::ThemeMode::Dark);
    const QString bgDark      = Theme::helpDescBg();
    const QString eyebrowDark = Theme::helpEyebrow();

    QVERIFY2(bgLight != bgDark, "tokens de fundo Light/Dark deveriam ser distintos (Theme.cpp)");
    QVERIFY2(eyebrowLight != eyebrowDark, "tokens de eyebrow Light/Dark deveriam ser distintos (Theme.cpp)");

    QVERIFY2(qApp->styleSheet().contains(bgDark),
        "QSS global após Theme::setMode(Dark) não contém o token de fundo do "
        "callout de Descrição — regressão FE-01");
    QVERIFY2(qApp->styleSheet().contains(eyebrowDark),
        "QSS global após Theme::setMode(Dark) não contém o token de cor do "
        "eyebrow — regressão FE-01");
    QVERIFY2(!qApp->styleSheet().contains(bgLight) || bgLight == bgDark,
        "QSS global ainda referencia o token de fundo Light após trocar para Dark");
}

// ── FE-03 ────────────────────────────────────────────────────────────────────

void TestTabShellSeguranca::tabShell_setContent_deletaConteudoAntigo()
{
    TabShell shell(QStringLiteral("Teste"));

    auto *conteudoAntigo = new QWidget;
    QPointer<QWidget> guard(conteudoAntigo);
    shell.setContent(conteudoAntigo);
    QVERIFY2(!guard.isNull(), "pré-condição: conteúdo antigo deveria estar vivo");

    auto *conteudoNovo = new QWidget;
    shell.setContent(conteudoNovo);

    // deleteLater() só executa no próximo ciclo do event loop — drena a fila
    // de deferred deletes explicitamente (padrão QtTest para este cenário).
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QVERIFY2(guard.isNull(),
        "conteúdo antigo não foi destruído após o 2º setContent — leak (regressão FE-03)");
}

QTEST_MAIN(TestTabShellSeguranca)
#include "test_tabshell_seguranca.moc"
