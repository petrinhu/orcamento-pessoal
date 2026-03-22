#include "UserSession.h"

#include <QSettings>
#include <QRegularExpression>

UserSession &UserSession::instance()
{
    static UserSession inst;
    return inst;
}

void UserSession::login(const QString &name, const QByteArray &key)
{
    m_name = name.trimmed();
    m_key  = key;
    saveLastName(m_name);
}

void UserSession::logout()
{
    m_name.clear();
    m_key.clear();
}

bool UserSession::isLoggedIn() const
{
    return !m_name.isEmpty();
}

QString UserSession::slug() const
{
    return m_name.toLower()
                 .replace(QLatin1Char(' '), QLatin1Char('_'))
                 .remove(QRegularExpression(QStringLiteral("[^a-z0-9_]")));
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
