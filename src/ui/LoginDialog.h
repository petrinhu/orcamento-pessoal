#pragma once

#include <QDialog>
#include <QString>
#include <QByteArray>

class QLineEdit;
class QLabel;
class QPushButton;
class QToolButton;

// Splash/login dialog.
// QT_DEBUG: shows only name field, no password required.
// Release:  name + password with strength checklist.
class LoginDialog final : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog() override;   // (#128) zera o material de chave em memória

    QString    nome()     const;
    QByteArray chave()    const; // Argon2id-derived key (empty in debug)
    QByteArray chaveOld() const; // PBKDF2-derived key for legacy file migration (empty in debug)

    void limparSenha(); // limpa campo de senha e reposiciona foco (usado em retry)

protected:
#ifndef QT_DEBUG
    // #198 (A11Y-03, WCAG 2.1.1): intercepta Espaço em m_btnOlho para ALTERNAR
    // a visibilidade da senha (toggle), em vez do animateClick() padrão do
    // QAbstractButton (que dispararia pressed()/released() automaticamente
    // após ~100ms — um "flash" rápido demais para ler a senha, não o
    // hold-to-reveal real do mouse). Ver LoginDialog.cpp.
    bool eventFilter(QObject *watched, QEvent *event) override;
#endif

private slots:
    void onNomeChanged(const QString &text);
    void onSenhaChanged(const QString &text);
    void onEntrar();

private:
    void buildUi();
    void updateChecklist();
    bool senhaValida() const;
#ifndef QT_DEBUG
    void atualizarIconeOlho();  // 🔒 = senha oculta, 👁 = senha visível
#endif

    QLineEdit   *m_nome        = nullptr;
    QLineEdit   *m_senha       = nullptr;
    QPushButton *m_btnEntrar   = nullptr;
    QPushButton *m_btnCancelar = nullptr;

    // Checklist labels (release only)
    QLabel *m_chkLen    = nullptr;
    QLabel *m_chkUpper  = nullptr;
    QLabel *m_chkDigit  = nullptr;

#ifndef QT_DEBUG
    // Botão olho hold-to-reveal (release only — só existe quando há senha)
    QToolButton *m_btnOlho = nullptr;
#endif

    QByteArray m_chave;     // Argon2id key
    QByteArray m_chaveOld;  // PBKDF2 key (for legacy file backward compat)
};
