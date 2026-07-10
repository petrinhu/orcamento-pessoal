#include "MarketProviderCatalog.h"

#include <QObject>

namespace MarketProviderCatalog {

namespace {

const QStringList kKeys = {
    QStringLiteral("none"),
    QStringLiteral("frankfurter"),
    QStringLiteral("brapi"),
    QStringLiteral("b3direct"),
    QStringLiteral("cotahist"),
};

// Contexto de tradução mantido como "QObject" (QObject::tr direto) para
// preservar a entrada já existente em peubud_en.ts ao mover este texto de
// MarketConfigWidget.cpp para cá (só a location muda, o <source> é igual).
const QStringList kLabels = {
    QObject::tr("Nenhum (manual)"),
    QStringLiteral("Frankfurter (ECB, sem chave)"),
    QStringLiteral("Brapi (token Bearer)"),
    QStringLiteral("B3 oficial (key id)"),
    QStringLiteral("COTAHIST (histórico)"),
};

} // namespace

const QStringList &keys() { return kKeys; }
const QStringList &labels() { return kLabels; }

QString labelFor(const QString &key)
{
    const int idx = kKeys.indexOf(key);
    return idx >= 0 ? kLabels.at(idx) : key;
}

} // namespace MarketProviderCatalog
