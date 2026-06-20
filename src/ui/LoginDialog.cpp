#include "LoginDialog.h"
#include "Theme.h"
#include "core/crypto/AesCryptoService.h"
#include "core/crypto/KeyDerivation.h"
#include "core/crypto/SodiumCryptoService.h"
#include "core/secret/SecretStoreService.h"
#include "core/session/UserSession.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QMessageBox>

#include <sodium.h>  // sodium_memzero — apaga chaves derivadas no destrutor

static const QString APP_NAME   = QStringLiteral("orcamento-pessoal");
static const QString SALT_SUFFIX = QStringLiteral("/salt");

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Orçamento Pessoal"));
    setWindowIcon(QIcon(QStringLiteral(":/icon/icon.png")));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    buildUi();

    // Pre-fill last used name
    const QString last = UserSession::instance().loadLastName();
    if (!last.isEmpty())
        m_nome->setText(last);
}

LoginDialog::~LoginDialog()
{
    // (#128) Zera as chaves derivadas mantidas pelo dialog. O chamador (main)
    // já copiou os valores para variáveis locais antes de usá-los; estas são as
    // cópias COW residentes no dialog, vivas enquanto o objeto existir.
    if (!m_chave.isEmpty())
        sodium_memzero(m_chave.data(), static_cast<size_t>(m_chave.size()));
    if (!m_chaveOld.isEmpty())
        sodium_memzero(m_chaveOld.data(), static_cast<size_t>(m_chaveOld.size()));
}

QString    LoginDialog::nome()     const { return m_nome->text().trimmed(); }
QByteArray LoginDialog::chave()    const { return m_chave; }
QByteArray LoginDialog::chaveOld() const { return m_chaveOld; }

void LoginDialog::limparSenha()
{
#ifndef QT_DEBUG
    if (m_senha) {
        m_senha->clear();
        m_senha->setFocus();
    }
    // Zera a chave derivada da tentativa anterior para não vazar em memória
    if (!m_chave.isEmpty())
        sodium_memzero(m_chave.data(), static_cast<size_t>(m_chave.size()));
    m_chave.clear();
    if (!m_chaveOld.isEmpty())
        sodium_memzero(m_chaveOld.data(), static_cast<size_t>(m_chaveOld.size()));
    m_chaveOld.clear();
#endif
    // Em QT_DEBUG não há campo de senha; no-op intencional.
}

void LoginDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(40, 40, 40, 40);
    root->setSpacing(16);

    // Title
    auto *title = new QLabel(tr("Orçamento Pessoal"), this);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    root->addSpacing(8);

    // Nome
    auto *lblNome = new QLabel(tr("Nome"), this);
    root->addWidget(lblNome);
    m_nome = new QLineEdit(this);
    m_nome->setPlaceholderText(tr("Seu nome"));
    root->addWidget(m_nome);
    connect(m_nome, &QLineEdit::textChanged, this, &LoginDialog::onNomeChanged);

#ifndef QT_DEBUG
    // Senha
    auto *lblSenha = new QLabel(tr("Senha"), this);
    root->addWidget(lblSenha);
    m_senha = new QLineEdit(this);
    m_senha->setEchoMode(QLineEdit::Password);
    m_senha->setPlaceholderText(tr("Mínimo 8 caracteres"));
    root->addWidget(m_senha);
    connect(m_senha, &QLineEdit::textChanged, this, &LoginDialog::onSenhaChanged);

    // Checklist
    auto *checkFrame = new QFrame(this);
    auto *checkLayout = new QVBoxLayout(checkFrame);
    checkLayout->setContentsMargins(8, 8, 8, 8);
    checkLayout->setSpacing(4);

    m_chkLen   = new QLabel(tr("✗  Mínimo 8 caracteres"), this);
    m_chkUpper = new QLabel(tr("✗  Uma letra maiúscula"), this);
    m_chkDigit = new QLabel(tr("✗  Um número"), this);
    for (auto *lbl : {m_chkLen, m_chkUpper, m_chkDigit}) {
        lbl->setObjectName(QStringLiteral("subtitle"));
        checkLayout->addWidget(lbl);
    }
    root->addWidget(checkFrame);
#endif

    root->addSpacing(8);

    // Botões Cancelar + Entrar (mesma linha)
    auto *btnRow = new QHBoxLayout();

    m_btnCancelar = new QPushButton(tr("Cancelar"), this);
    btnRow->addWidget(m_btnCancelar);
    connect(m_btnCancelar, &QPushButton::clicked, this, &QDialog::reject);

    m_btnEntrar = new QPushButton(tr("Entrar"), this);
    m_btnEntrar->setObjectName(QStringLiteral("primary"));
    m_btnEntrar->setEnabled(false);
    btnRow->addWidget(m_btnEntrar);
    connect(m_btnEntrar, &QPushButton::clicked, this, &LoginDialog::onEntrar);

    root->addLayout(btnRow);

    setMinimumWidth(360);
}

void LoginDialog::onNomeChanged(const QString &)
{
#ifdef QT_DEBUG
    m_btnEntrar->setEnabled(!nome().isEmpty());
#else
    m_btnEntrar->setEnabled(!nome().isEmpty() && senhaValida());
#endif
}

void LoginDialog::onSenhaChanged(const QString &)
{
#ifndef QT_DEBUG
    updateChecklist();
    m_btnEntrar->setEnabled(!nome().isEmpty() && senhaValida());
#endif
}

void LoginDialog::updateChecklist()
{
#ifndef QT_DEBUG
    if (!m_senha) return;
    const QString s = m_senha->text();
    const bool okLen   = s.length() >= 8;
    const bool okUpper = s.contains(QRegularExpression(QStringLiteral("[A-Z]")));
    const bool okDigit = s.contains(QRegularExpression(QStringLiteral("[0-9]")));
    const QString ok  = QStringLiteral("✓  ");
    const QString bad = QStringLiteral("✗  ");
    m_chkLen->setText  ((okLen   ? ok : bad) + tr("Mínimo 8 caracteres"));
    m_chkUpper->setText((okUpper ? ok : bad) + tr("Uma letra maiúscula"));
    m_chkDigit->setText((okDigit ? ok : bad) + tr("Um número"));
    const QString green = Theme::positivo();
    const QString red   = Theme::negativo();
    m_chkLen->setStyleSheet  (QStringLiteral("color: %1").arg(okLen   ? green : red));
    m_chkUpper->setStyleSheet(QStringLiteral("color: %1").arg(okUpper ? green : red));
    m_chkDigit->setStyleSheet(QStringLiteral("color: %1").arg(okDigit ? green : red));
#endif
}

bool LoginDialog::senhaValida() const
{
#ifdef QT_DEBUG
    return true;
#else
    if (!m_senha) return false;
    const QString s = m_senha->text();
    return s.length() >= 8
        && s.contains(QRegularExpression(QStringLiteral("[A-Z]")))
        && s.contains(QRegularExpression(QStringLiteral("[0-9]")));
#endif
}

void LoginDialog::onEntrar()
{
    const QString n = nome();
    if (n.isEmpty()) return;

#ifdef QT_DEBUG
    m_chave    = QByteArray();
    m_chaveOld = QByteArray();
    UserSession::instance().login(n, m_chave);
    accept();
    return;
#else
    // Retrieve or create salt from system keychain.
    // The salt is 16 bytes — compatible with both Argon2id and PBKDF2.
    SecretStoreService store(APP_NAME);
    const QString saltKey = n.toLower() + SALT_SUFFIX;
    QByteArray salt = store.retrieve(saltKey);

    if (salt.isEmpty()) {
        // New user: generate salt with SodiumCryptoService (uses randombytes_buf).
        SodiumCryptoService sodium;
        salt = sodium.generateSalt();
        if (!store.store(saltKey, salt)) {
            QMessageBox::warning(this, tr("Erro"),
                tr("Não foi possível acessar o cofre de senhas do sistema.\n"
                   "Verifique se o KWallet ou GNOME Keyring está disponível."));
            return;
        }
        // New user: no legacy file exists, skip legacy derivation.
        m_chave    = sodium.deriveKey(m_senha->text(), salt);
        m_chaveOld = QByteArray();
    } else {
        // Existing user: derive with Argon2id (new format) and PBKDF2 (legacy compat).
        // Both use the same stored salt (same size: 16 bytes).
        SodiumCryptoService sodium;
        m_chave    = sodium.deriveKey(m_senha->text(), salt);
        AesCryptoService legacy;
        m_chaveOld = legacy.deriveKey(m_senha->text(), salt);
    }

    UserSession::instance().login(n, m_chave);
    accept();
#endif
}
