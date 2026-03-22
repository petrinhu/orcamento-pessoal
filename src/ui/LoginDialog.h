#pragma once

#include <QDialog>
#include <QString>
#include <QByteArray>

class QLineEdit;
class QLabel;
class QPushButton;

// Splash/login dialog.
// QT_DEBUG: shows only name field, no password required.
// Release:  name + password with strength checklist.
class LoginDialog final : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);

    QString    nome()  const;
    QByteArray chave() const; // derived key (empty in debug)

private slots:
    void onNomeChanged(const QString &text);
    void onSenhaChanged(const QString &text);
    void onEntrar();

private:
    void buildUi();
    void updateChecklist();
    bool senhaValida() const;

    QLineEdit  *m_nome   = nullptr;
    QLineEdit  *m_senha  = nullptr;
    QPushButton *m_btnEntrar = nullptr;

    // Checklist labels (release only)
    QLabel *m_chkLen    = nullptr;
    QLabel *m_chkUpper  = nullptr;
    QLabel *m_chkDigit  = nullptr;

    QByteArray m_chave;
};
