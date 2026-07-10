#pragma once

#include <QByteArray>
#include <QString>

// Stores secrets in the OS keychain via libsecret (Secret Service API).
// On KDE Plasma, this uses KWallet; on GNOME, GNOME Keyring.
//
// (#203a/ARQ-04) Classe concreta, sem interface — `ISecretStore` foi
// removida (era abstração morta: nenhum consumidor a usava
// polimorficamente, todos instanciam `SecretStoreService` diretamente;
// YAGNI/DRY). Se surgir necessidade de dublê em teste (ex.: isolar
// libsecret/D-Bus em CI headless), reintroduzir a interface então.
class SecretStoreService final {
public:
    explicit SecretStoreService(const QString &appName);

    bool store(const QString &key, const QByteArray &secret);
    QByteArray retrieve(const QString &key);
    bool remove(const QString &key);
    bool contains(const QString &key);

private:
    QString m_appName;
};
