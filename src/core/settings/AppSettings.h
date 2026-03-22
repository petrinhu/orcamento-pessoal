#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

// Singleton — persists language + default currency across sessions.
// Uses QSettings("petrinhu", "PeuBud").
class AppSettings : public QObject {
    Q_OBJECT
public:
    static AppSettings &instance();

    enum class Language { PtBr, EnIntl };

    Language language() const;
    void     setLanguage(Language lang);

    // ISO 4217 code: "BRL", "USD", "EUR", …
    QString defaultCurrency() const;
    void    setDefaultCurrency(const QString &code);

    // Convenience: display symbol for the current default currency
    QString currencySymbol() const;

signals:
    void languageChanged(Language lang);
    void currencyChanged(const QString &code);

private:
    explicit AppSettings();
    QSettings m_s;
};
