#pragma once

#include "MatchData.h"

#include <QList>

// IMatchFinder - pipeline para detectar se uma transacao importada eh
// duplicata, match preciso, match impreciso ou inedita perante o conjunto
// de candidatos (existentes no DB OU recorrentes agendadas). Padrao de
// kmymoney/converter/transactionmatchfinder.h:22-149, adaptado.
//
// Subclasses fornecem o conjunto de candidatos no construtor, mantendo o
// algoritmo de matching central testavel sem dependencia de DB (#84).
class IMatchFinder {
public:
    explicit IMatchFinder(QList<MatchData> candidates)
        : m_candidates(std::move(candidates)) {}
    virtual ~IMatchFinder() = default;

    // Janela de datas (em dias) usada para considerar candidato compativel.
    // Default 4 dias replica heuristica do kmymoney: extratos OFX costumam
    // postar 1-2 dias depois da data da transacao real.
    void setMatchWindow(int days) { m_matchWindow = days; }
    int  matchWindow() const      { return m_matchWindow; }

    // Aplica heuristicas em ordem (Duplicate > Precise > Imprecise > NotFound)
    // e retorna o melhor resultado. Implementacao default funciona para ambas
    // subclasses; ScheduledTransactionMatchFinder pode override se precisar
    // de regras distintas no futuro.
    virtual MatchResult find(const MatchData &input) const;

protected:
    QList<MatchData> m_candidates;
    int              m_matchWindow = 4;
};
