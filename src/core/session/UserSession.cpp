#include "UserSession.h"

#include "core/settings/AppSettings.h"

#include <QSettings>

#include <sodium.h>   // sodium_memzero — apaga a chave em memória no logout

UserSession &UserSession::instance()
{
    static UserSession inst;
    return inst;
}

void UserSession::login(const QString &name, const QByteArray &key, Role role)
{
    m_name = name.trimmed();
    m_key  = key;
    m_role = role;
    saveLastName(m_name);
}

void UserSession::logout()
{
    m_name.clear();

    // (#128) Zera o material da chave em memória antes de soltar o buffer.
    // QByteArray::clear() apenas dessacia o ponteiro (COW) — não apaga os bytes.
    // sodium_memzero(data()) força o detach e apaga ESTE buffer. Só executa
    // quando há chave (em QT_DEBUG a chave é vazia).
    // Limitação conhecida: se a chave foi copiada por valor para outras
    // instâncias COW (ex.: DatabaseManager::m_key), aquelas cópias têm seu
    // próprio wipe nos respectivos pontos de destruição — ver close().
    if (!m_key.isEmpty())
        sodium_memzero(m_key.data(), static_cast<size_t>(m_key.size()));
    m_key.clear();

    // (ADR-001 #212) Nunca deixa privilégio residual pós-logout — a próxima
    // sessão (ex.: outro usuário na mesma máquina) parte sempre de User.
    m_role = Role::User;
}

bool UserSession::isLoggedIn() const
{
    return !m_name.isEmpty();
}

QString UserSession::slug() const
{
    // (ADR-001 §6/D6, Fatia 4) Delega à fonte única de verdade — corrige o
    // bug pré-existente "João" -> "joo" (esta implementação não decompunha
    // Unicode antes do filtro [a-z0-9_]; AppSettings::slugify decompõe).
    return AppSettings::slugify(m_name);
}

void UserSession::saveLastName(const QString &name)
{
    QSettings s(QStringLiteral("OrcamentoPessoal"), QStringLiteral("Session"));
    s.setValue(QStringLiteral("lastUser"), name);
}

QString UserSession::loadLastName() const
{
    QSettings s(QStringLiteral("OrcamentoPessoal"), QStringLiteral("Session"));
    return s.value(QStringLiteral("lastUser")).toString();
}
