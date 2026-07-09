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

    // #167 T1: sincroniza a densidade das tabelas persistida em AppSettings
    // para o estado em memória do Theme (Theme.cpp fica desacoplado de
    // AppSettings — ver comentário em Theme.h). PrefsWidget refaz essa
    // sincronização ao vivo quando o toggle "Compacta" muda.
    Theme::setDensidadeTabelasCompacta(AppSettings::instance().tabelaDensidadeCompacta());

    // ── Login + retry loop ─────────────────────────────────────────────────
    // Cria o diretório temporário data/ (banco decifrado em runtime).
    // O cofre cifrado vai para ~/peubud/save/<slug>/database.pbd (#181).
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

    // (#191/FIN-02) close() cifra o .db → .pbd (release). Se o integrity_check
    // reprovar o banco (corrupção), o save é ABORTADO e o cofre anterior
    // (último .pbd bom) é preservado — alertar aqui é o "alertar" do work
    // order, já que closeEvent() já tratou o caso normal de "Salvar" via
    // onSalvar()/save() (que também está sob o mesmo gate).
    if (!DatabaseManager::instance().close()) {
        QMessageBox::critical(nullptr,
            QStringLiteral("Falha ao salvar"),
            QStringLiteral(
                "O banco de dados apresentou um problema de integridade ao "
                "fechar e a gravação foi cancelada para não sobrescrever o "
                "último arquivo salvo válido.\n\n"
                "O cofre da sessão anterior permanece intacto, mas as "
                "alterações feitas agora podem ter sido perdidas."));
    }
    UserSession::instance().logout();

    return ret;
}
