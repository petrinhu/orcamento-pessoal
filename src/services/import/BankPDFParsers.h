#pragma once

#include "IBankPDFParser.h"

// Implementacoes concretas dos parsers PDF de extrato bancario (#93).
// Cabecalho consolidado para evitar dezenas de .h triviais. Cada classe
// segue a mesma estrutura: nome, deteccao por regex no cabecalho, parse
// linha-a-linha via QRegularExpression.

class NubankPDFParser final : public IBankPDFParser {
public:
    [[nodiscard]] QString nomeBanco() const override
        { return QStringLiteral("Nubank"); }
    [[nodiscard]] bool detectar(const QString &texto) const override;
    [[nodiscard]] ImportService::Result parse(const QString &texto) const override;
};

class ItauPDFParser final : public IBankPDFParser {
public:
    [[nodiscard]] QString nomeBanco() const override
        { return QStringLiteral("Itaú"); }
    [[nodiscard]] bool detectar(const QString &texto) const override;
    [[nodiscard]] ImportService::Result parse(const QString &texto) const override;
};

class BradescoPDFParser final : public IBankPDFParser {
public:
    [[nodiscard]] QString nomeBanco() const override
        { return QStringLiteral("Bradesco"); }
    [[nodiscard]] bool detectar(const QString &texto) const override;
    [[nodiscard]] ImportService::Result parse(const QString &texto) const override;
};

class BBPDFParser final : public IBankPDFParser {
public:
    [[nodiscard]] QString nomeBanco() const override
        { return QStringLiteral("Banco do Brasil"); }
    [[nodiscard]] bool detectar(const QString &texto) const override;
    [[nodiscard]] ImportService::Result parse(const QString &texto) const override;
};

class CaixaPDFParser final : public IBankPDFParser {
public:
    [[nodiscard]] QString nomeBanco() const override
        { return QStringLiteral("Caixa"); }
    [[nodiscard]] bool detectar(const QString &texto) const override;
    [[nodiscard]] ImportService::Result parse(const QString &texto) const override;
};

class InterPDFParser final : public IBankPDFParser {
public:
    [[nodiscard]] QString nomeBanco() const override
        { return QStringLiteral("Inter"); }
    [[nodiscard]] bool detectar(const QString &texto) const override;
    [[nodiscard]] ImportService::Result parse(const QString &texto) const override;
};

// Fallback - extrai transacoes de qualquer texto contendo padroes
// "dd/MM/yyyy ... R$ XX,XX" sem regras especificas. Sempre `detectar() == true`.
class GenericoPDFParser final : public IBankPDFParser {
public:
    [[nodiscard]] QString nomeBanco() const override
        { return QStringLiteral("Genérico (heurístico)"); }
    [[nodiscard]] bool detectar(const QString &) const override { return true; }
    [[nodiscard]] ImportService::Result parse(const QString &texto) const override;
};
