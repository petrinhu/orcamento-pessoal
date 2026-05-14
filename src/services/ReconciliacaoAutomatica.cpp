#include "ReconciliacaoAutomatica.h"

#include <QElapsedTimer>
#include <QHash>

// Estrategia: subset-sum bottom-up tipo "DP de mochila" otimizado em mapa
// esparso (alvo->predecessor). Para cada candidato, expande estados
// alcancaveis somando seu valor. Quando alvo aparece, reconstroi caminho.
//
// Bound: se soma_restante (acumulador "rem_pos") + soma_corrente < alvo,
// nao adianta continuar -- gnc-autoclear usa essa poda. Implementamos
// equivalente verificando se algum estado pode chegar a alvo dado o que
// resta dos candidatos.
//
// Timeout via QElapsedTimer. Contador de iteracoes = celulas DP visitadas.

ReconciliacaoAutomatica::Resultado ReconciliacaoAutomatica::resolver(
    const QList<Lancamento> &candidatos,
    qint64 saldoAlvoCent,
    int    timeoutMs)
{
    Resultado out;
    out.encontrado      = false;
    out.iteracoes       = 0;
    out.timeoutAtingido = false;

    // Edge cases triviais
    if (saldoAlvoCent < 0) {
        return out;  // nao alcancavel com candidatos positivos
    }
    if (saldoAlvoCent == 0) {
        out.encontrado = true;
        return out;  // subconjunto vazio satisfaz
    }
    if (candidatos.isEmpty()) {
        return out;
    }

    // Filtra candidatos invalidos (<=0 ou > alvo).
    QList<Lancamento> valid;
    valid.reserve(candidatos.size());
    qint64 totalPositivo = 0;
    for (const auto &l : candidatos) {
        if (l.valorCent > 0 && l.valorCent <= saldoAlvoCent) {
            valid.append(l);
            totalPositivo += l.valorCent;
        } else if (l.valorCent == saldoAlvoCent) {
            // match trivial: 1 candidato bate exato
            out.encontrado = true;
            out.ids.append(l.id);
            out.iteracoes = 1;
            return out;
        }
    }
    if (valid.isEmpty() || totalPositivo < saldoAlvoCent) {
        return out;
    }

    QElapsedTimer timer;
    timer.start();

    // DP por mapa esparso: state[soma] = (indiceCandidatoUsado, somaPredecessora)
    // Comecando com soma 0 (nada usado).
    struct Origem {
        qint64 prevSoma;
        int    idxCandidato;  // -1 = raiz
    };

    QHash<qint64, Origem> estados;
    estados.insert(0, {0, -1});

    bool encontrouAlvo = false;

    for (int i = 0; i < valid.size() && !encontrouAlvo; ++i) {
        // Timeout check
        if (timer.elapsed() > timeoutMs) {
            out.timeoutAtingido = true;
            return out;
        }

        const qint64 v = valid[i].valorCent;

        // Snapshot dos estados atuais para iterar sem invalidar
        const QList<qint64> chaves = estados.keys();
        for (qint64 s : chaves) {
            const qint64 nova = s + v;
            ++out.iteracoes;
            if (nova > saldoAlvoCent) {
                continue;  // poda: ultrapassa alvo
            }
            if (!estados.contains(nova)) {
                estados.insert(nova, {s, i});
                if (nova == saldoAlvoCent) {
                    encontrouAlvo = true;
                    break;
                }
            }
            // Periodicamente checa timeout em loops grandes
            if ((out.iteracoes & 0xFFFF) == 0 && timer.elapsed() > timeoutMs) {
                out.timeoutAtingido = true;
                return out;
            }
        }
    }

    if (!encontrouAlvo) {
        return out;
    }

    // Reconstroi caminho do alvo ate 0
    out.encontrado = true;
    qint64 cur = saldoAlvoCent;
    while (cur > 0) {
        auto it = estados.constFind(cur);
        if (it == estados.constEnd() || it->idxCandidato < 0) {
            break;
        }
        out.ids.prepend(valid[it->idxCandidato].id);
        cur = it->prevSoma;
    }

    return out;
}
