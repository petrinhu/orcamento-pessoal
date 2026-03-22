#include "SecretStoreService.h"

// glib/gio headers use 'signals' as a field name, which conflicts with
// Qt's 'signals' keyword. Temporarily undefine/redefine around the include.
#undef signals
#include <libsecret/secret.h>
#define signals Q_SIGNALS

#include <QByteArray>

static const SecretSchema *appSchema(const QString &appName)
{
    static SecretSchema schema = {
        nullptr, SECRET_SCHEMA_NONE,
        {
            { "app",  SECRET_SCHEMA_ATTRIBUTE_STRING },
            { "key",  SECRET_SCHEMA_ATTRIBUTE_STRING },
            { nullptr, SecretSchemaAttributeType(0) }
        },
        0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
    };
    // schema.name set once
    static QByteArray nameBytes;
    if (!schema.name) {
        nameBytes = appName.toUtf8();
        schema.name = nameBytes.constData();
    }
    return &schema;
}

SecretStoreService::SecretStoreService(const QString &appName)
    : m_appName(appName)
{
    // Initialise schema name
    appSchema(m_appName);
}

bool SecretStoreService::store(const QString &key, const QByteArray &secret)
{
    GError *err = nullptr;
    const QByteArray keyBytes = key.toUtf8();
    const QByteArray appBytes = m_appName.toUtf8();

    gboolean ok = secret_password_store_sync(
        appSchema(m_appName),
        SECRET_COLLECTION_DEFAULT,
        keyBytes.constData(),          // label
        secret.toBase64().constData(), // value (base64 for binary safety)
        nullptr, &err,
        "app", appBytes.constData(),
        "key", keyBytes.constData(),
        nullptr
    );

    if (err) { g_error_free(err); return false; }
    return ok;
}

QByteArray SecretStoreService::retrieve(const QString &key)
{
    GError *err = nullptr;
    const QByteArray keyBytes = key.toUtf8();
    const QByteArray appBytes = m_appName.toUtf8();

    gchar *value = secret_password_lookup_sync(
        appSchema(m_appName),
        nullptr, &err,
        "app", appBytes.constData(),
        "key", keyBytes.constData(),
        nullptr
    );

    if (err) { g_error_free(err); return {}; }
    if (!value) return {};

    QByteArray result = QByteArray::fromBase64(QByteArray(value));
    secret_password_free(value);
    return result;
}

bool SecretStoreService::remove(const QString &key)
{
    GError *err = nullptr;
    const QByteArray keyBytes = key.toUtf8();
    const QByteArray appBytes = m_appName.toUtf8();

    gboolean ok = secret_password_clear_sync(
        appSchema(m_appName),
        nullptr, &err,
        "app", appBytes.constData(),
        "key", keyBytes.constData(),
        nullptr
    );

    if (err) { g_error_free(err); return false; }
    return ok;
}

bool SecretStoreService::contains(const QString &key)
{
    return !retrieve(key).isEmpty();
}
