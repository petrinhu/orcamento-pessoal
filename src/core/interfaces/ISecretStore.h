#pragma once

#include <QByteArray>
#include <QString>

class ISecretStore {
public:
    virtual ~ISecretStore() = default;

    virtual bool store(const QString &key, const QByteArray &secret) = 0;
    virtual QByteArray retrieve(const QString &key) = 0;
    virtual bool remove(const QString &key) = 0;
    virtual bool contains(const QString &key) = 0;
};
