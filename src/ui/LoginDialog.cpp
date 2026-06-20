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
#include <QToolButton>
#include <QRegularExpression>
#include <QMessageBox>
#include <QPalette>

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
    // ── Correção #1: contraste em dialog frameless ────────────────────────────
    // Qt::FramelessWindowHint impede o gerenciador de janela de pintar o fundo.
    // Sem setAutoFillBackground, o Fusion pode deixar a área do widget transparente
    // ou usar a cor errada da palette herdada — texto fica invisível ou de baixo
    // contraste. Fixar a palette do dialog com o fundo @@BG@@ do tema garante
    // que todo texto dentro do dialog sempre contrasta sobre o fundo correto.
    {
        QPalette pal = palette();
        pal.setColor(QPalette::Window, QColor(Theme::background()));
        setPalette(pal);
        setAutoFillBackground(true);
    }

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
    // ── Anti-clip: campo expande horizontalmente mas não encolhe abaixo do hint
    m_nome->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    root->addWidget(m_nome);
    connect(m_nome, &QLineEdit::textChanged, this, &LoginDialog::onNomeChanged);

#ifndef QT_DEBUG
    // Senha
    auto *lblSenha = new QLabel(tr("Senha"), this);
    root->addWidget(lblSenha);

    // ── Correção #3: linha senha + botão olho em HBoxLayout ──────────────────
    auto *rowSenha = new QHBoxLayout();
    rowSenha->setSpacing(4);

    m_senha = new QLineEdit(this);
    m_senha->setEchoMode(QLineEdit::Password);
    m_senha->setPlaceholderText(tr("Mínimo 8 caracteres"));
    m_senha->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rowSenha->addWidget(m_senha);
    connect(m_senha, &QLineEdit::textChanged, this, &LoginDialog::onSenhaChanged);

    // Botão olho hold-to-reveal: revela a senha APENAS enquanto o botão está
    // pressionado (pressed → Normal; released → Password). O toolTip instrui o
    // usuário. Usa emojiIcon para renderizar o glifo colorido via Noto Emoji —
    // consistent com a política do projeto (emoji nunca em texto de botão).
    m_btnOlho = new QToolButton(this);
    m_btnOlho->setObjectName(QStringLiteral("iconButton")); // sem borda (#165)
    m_btnOlho->setIcon(Theme::emojiIcon(QStringLiteral("👁"), 18));
    m_btnOlho->setToolTip(tr("Segure para ver a senha"));
    m_btnOlho->setFixedSize(32, 32); // quadrado mantido (spec §5 item 7)
    m_btnOlho->setFocusPolicy(Qt::NoFocus);  // não entra no Tab order de auth
    rowSenha->addWidget(m_btnOlho);

    connect(m_btnOlho, &QToolButton::pressed,  this, [this]() {
        m_senha->setEchoMode(QLineEdit::Normal);
    });
    connect(m_btnOlho, &QToolButton::released, this, [this]() {
        m_senha->setEchoMode(QLineEdit::Password);
    });

    root->addLayout(rowSenha);

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

    // ── Correção #2: botões anti-clip ─────────────────────────────────────────
    // MinimumExpanding: os botões crescem para preencher a linha, mas nunca
    // ficam abaixo do sizeHint — o que impede o clip quando a janela estreita.
    // O minimumSize do dialog garante que a janela nunca encolhe além do ponto
    // onde os botões caberiam confortavelmente.
    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    m_btnCancelar = new QPushButton(tr("Cancelar"), this);
    m_btnCancelar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    // (#174) Impede o Cancelar de capturar Enter — nem como default implícito
    // nem como fallback quando o Entrar está desabilitado.
    m_btnCancelar->setAutoDefault(false);
    btnRow->addWidget(m_btnCancelar);
    connect(m_btnCancelar, &QPushButton::clicked, this, &QDialog::reject);

    m_btnEntrar = new QPushButton(tr("Entrar"), this);
    m_btnEntrar->setObjectName(QStringLiteral("primary"));
    m_btnEntrar->setEnabled(false);
    m_btnEntrar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    // (#174) Garante que Enter sempre dispara Entrar, não Cancelar.
    // O Cancelar foi criado primeiro → Qt o elegeria como default implícito
    // sem este setDefault explícito.
    m_btnEntrar->setDefault(true);
    btnRow->addWidget(m_btnEntrar);
    connect(m_btnEntrar, &QPushButton::clicked, this, &LoginDialog::onEntrar);

    root->addLayout(btnRow);

    // (#174) Belt-and-suspenders: Enter nos campos de texto submete o form
    // quando o botão Entrar está habilitado, independente do foco atual.
    connect(m_nome, &QLineEdit::returnPressed, this, [this]{
        if (m_btnEntrar->isEnabled()) onEntrar();
    });
#ifndef QT_DEBUG
    connect(m_senha, &QLineEdit::returnPressed, this, [this]{
        if (m_btnEntrar->isEnabled()) onEntrar();
    });
#endif

    // Largura mínima: 360px comporta os dois botões (≥80px cada) + margens 2×40
    // + spacing 8px. Altura mínima: garante que checklist + campos + botões
    // nunca ficam cortados ao redimensionar a janela para o menor tamanho útil.
    setMinimumSize(360, 340);
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
