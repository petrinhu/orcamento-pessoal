#pragma once

#include "services/ImportService.h"
#include <QString>

// IBankPDFParser (#93) - interface para parsers heuristicos de extratos PDF
// brasileiros. Cada banco tem layout proprio; implementacoes detectam o
// formato via assinatura textual (cabecalho, padrao de regex de transacao)
// e fazem parse linha-a-linha.
//
// Pipeline: `ImportService::fromPdf(path)` extrai texto via Poppler-qt6,
// itera parsers registrados em ordem de prioridade, retorna o primeiro
// que `detectar()` aprovar. Fallback eh `GenericoPDFParser` (tabular puro).
class IBankPDFParser {
public:
    virtual ~IBankPDFParser() = default;

    // Identificador do banco (e.g. "Nubank", "Itau") para diagnostico/log.
    [[nodiscard]] virtual QString nomeBanco() const = 0;

    // Heuristica de deteccao via assinatura no texto. Caller normalmente
    // checa em ordem prioritaria e usa o primeiro `true`.
    [[nodiscard]] virtual bool detectar(const QString &texto) const = 0;

    // Executa parse no texto inteiro do PDF. Retorna Result preenchido
    // ou com `error` setado em falha.
    [[nodiscard]] virtual ImportService::Result parse(const QString &texto) const = 0;
};
