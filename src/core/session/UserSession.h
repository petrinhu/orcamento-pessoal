#pragma once

#include <QString>
#include <QByteArray>

#include "models/UserAccountRecord.h"   // enum class Role — ADR-001 #212 §6

// Holds runtime session state: logged-in username, derived crypto key and
// role. Also persists the last-used username via QSettings.
class UserSession {
public:
    static UserSession &instance();

    // role tem default (User) para não quebrar callers pré-ADR-001 que ainda
    // não sabem sobre papel (LoginDialog é tocado só na fatia de UI, §6).
    void login(const QString &name, const QByteArray &key, Role role = Role::User);
    void logout();

    bool      isLoggedIn() const;
    QString   name()       const { return m_name; }
    QByteArray key()       const { return m_key; }
    Role      role()       const { return m_role; }
    QString   slug()       const; // lowercase, spaces→underscore

    // Persisted across restarts via QSettings
    void    saveLastName(const QString &name);
    QString loadLastName() const;

private:
    UserSession() = default;

    QString    m_name;
    QByteArray m_key;
    Role       m_role = Role::User;
};
