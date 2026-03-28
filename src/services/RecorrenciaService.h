#pragma once

#include <QDate>

// Generates pending lançamentos for all active TransacaoRecorrente up to `ate`.
// Returns the count of entries created.
class RecorrenciaService {
public:
    int gerarPendentes(QDate ate);

    // Next occurrence after `d` for the given frequencia.
    static QDate proximaApos(QDate d, const QString &frequencia);
};
