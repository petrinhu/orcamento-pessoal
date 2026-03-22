#pragma once

#include <QString>
#include <QByteArray>

// Holds runtime session state: logged-in username and derived crypto key.
// Also persists the last-used username via QSettings.
class UserSession {
public:
    static UserSession &instance();

    void login(const QString &name, const QByteArray &key);
    void logout();

    bool      isLoggedIn() const;
    QString   name()       const { return m_name; }
    QByteArray key()       const { return m_key; }
    QString   slug()       const; // lowercase, spaces→underscore

    // Persisted across restarts via QSettings
    void    saveLastName(const QString &name);
    QString loadLastName() const;

private:
    UserSession() = default;

    QString    m_name;
    QByteArray m_key;
};
