#include "ui/Theme.h"
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"
#include "ui/IconGenerator.h"
#include "core/database/DatabaseManager.h"
#include "core/session/UserSession.h"

#include <QApplication>
#include <QDir>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("PeuBud"));
    app.setApplicationDisplayName(QStringLiteral("PeuBud — Orçamento Pessoal"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setOrganizationName(QStringLiteral("petrinhu"));
    app.setWindowIcon(IconGenerator::generate());

    Theme::aplicar();

    // ── Login ──────────────────────────────────────────────────────────────
    LoginDialog login;
    if (login.exec() != QDialog::Accepted)
        return 0;

    const QString    nome  = login.nome();
    const QByteArray chave = login.chave();

    // ── Open database ──────────────────────────────────────────────────────
    QDir().mkpath(QStringLiteral("data"));

    const QString slug = UserSession::instance().slug();
    if (!DatabaseManager::instance().open(slug, chave)) {
        QMessageBox::critical(nullptr,
            QStringLiteral("Erro ao abrir banco de dados"),
            QStringLiteral("Não foi possível abrir o banco de dados.\n"
                           "Verifique se a senha está correta."));
        return 1;
    }

    // ── Main window ────────────────────────────────────────────────────────
    MainWindow w;
    w.show();

    const int ret = app.exec();

    DatabaseManager::instance().close();
    UserSession::instance().logout();

    return ret;
}
