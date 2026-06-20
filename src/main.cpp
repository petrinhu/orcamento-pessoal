#include "ui/Theme.h"
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"
#include "ui/widgets/OnboardingDialog.h"
#include "core/database/DatabaseManager.h"
#include "core/session/UserSession.h"
#include "core/settings/AppSettings.h"

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QMessageBox>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("PeuBud"));
    app.setApplicationDisplayName(QStringLiteral("PeuBud — Orçamento Pessoal"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setOrganizationName(QStringLiteral("petrinhu"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icon/icon.png")));

    // ── Translator ─────────────────────────────────────────────────────────
    QTranslator translator;
    if (AppSettings::instance().language() == AppSettings::Language::EnIntl
        && translator.load(QStringLiteral(":/translations/peubud_en.qm"))) {
        app.installTranslator(&translator);
    }

    Theme::aplicar();

    // ── Login + retry loop ─────────────────────────────────────────────────
    // Cria o diretório de dados uma única vez, antes do loop.
    QDir().mkpath(QStringLiteral("data"));

    LoginDialog login;
    for (;;) {
        if (login.exec() != QDialog::Accepted)
            return 0; // usuário cancelou/fechou → saída limpa

        const QByteArray chave    = login.chave();
        const QByteArray chaveOld = login.chaveOld(); // PBKDF2 — for legacy file migration

        if (DatabaseManager::instance().open(UserSession::instance().slug(), chave, chaveOld))
            break; // sucesso → segue para onboarding/MainWindow

        QMessageBox::warning(&login,
            QStringLiteral("Senha incorreta"),
            QStringLiteral("Não foi possível abrir o cofre com essa senha.\n"
                           "Verifique a senha e tente novamente, ou clique em Cancelar para sair."));
        login.limparSenha(); // mantém o nome, limpa a senha — volta ao topo do loop
    }

    const QString nome = login.nome();

    // ── Onboarding first-run (#109) ───────────────────────────────────────
    // Exibido apenas se a flag nunca foi marcada. "Pular" e "Concluir"
    // ambos persistem a flag — nao re-exibe em proximas sessoes.
    if (!AppSettings::instance().onboardingFeito()) {
        OnboardingDialog onb;
        onb.exec();
    }

    // ── Main window ────────────────────────────────────────────────────────
    MainWindow w;
    w.show();

    const int ret = app.exec();

    DatabaseManager::instance().close();
    UserSession::instance().logout();

    return ret;
}
