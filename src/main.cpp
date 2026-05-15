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

    // ── Login ──────────────────────────────────────────────────────────────
    LoginDialog login;
    if (login.exec() != QDialog::Accepted)
        return 0;

    const QString    nome     = login.nome();
    const QByteArray chave    = login.chave();
    const QByteArray chaveOld = login.chaveOld(); // PBKDF2 — for legacy file migration

    // ── Open database ──────────────────────────────────────────────────────
    QDir().mkpath(QStringLiteral("data"));

    const QString slug = UserSession::instance().slug();
    if (!DatabaseManager::instance().open(slug, chave, chaveOld)) {
        QMessageBox::critical(nullptr,
            QStringLiteral("Erro ao abrir banco de dados"),
            QStringLiteral("Não foi possível abrir o banco de dados.\n"
                           "Verifique se a senha está correta."));
        return 1;
    }

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
