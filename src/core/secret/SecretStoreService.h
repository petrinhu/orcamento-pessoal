#pragma once

#include "core/interfaces/ISecretStore.h"

// Stores secrets in the OS keychain via libsecret (Secret Service API).
// On KDE Plasma, this uses KWallet; on GNOME, GNOME Keyring.
class SecretStoreService final : public ISecretStore {
public:
    explicit SecretStoreService(const QString &appName);

    bool store(const QString &key, const QByteArray &secret) override;
    QByteArray retrieve(const QString &key) override;
    bool remove(const QString &key) override;
    bool contains(const QString &key) override;

private:
    QString m_appName;
};
