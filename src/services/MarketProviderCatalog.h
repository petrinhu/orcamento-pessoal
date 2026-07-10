#pragma once

#include <QString>
#include <QStringList>

// UX-02 (auditoria 2026-06-22): catálogo único de providers de mercado
// (chave interna <-> rótulo amigável). Antes existia só dentro de
// MarketConfigWidget.cpp (anônimo, não reaproveitável) e o BolsaWidget
// mostrava a chave crua ("frankfurter"/"b3direct"/"cotahist") direto pro
// usuário leigo. Fonte única de verdade agora — os dois widgets consomem
// daqui, sem duplicar a lista.
namespace MarketProviderCatalog {

// Chaves internas na ordem canônica de exibição.
const QStringList &keys();

// Rótulos amigáveis (mesma ordem/índice de keys()).
const QStringList &labels();

// Rótulo amigável para uma chave interna. Chave desconhecida -> ela mesma
// (fallback seguro, nunca devolve string vazia para chave não vazia).
QString labelFor(const QString &key);

} // namespace MarketProviderCatalog
