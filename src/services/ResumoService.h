#pragma once

#include "models/ResumoAnual.h"
#include "models/ResumoMensal.h"
#include <QVector>

class QDate;

// Camada Mid: agrega Entradas, Gastos, Parcelas, Investimentos e Proventos
// em resumos mensais/anuais consumíveis pela UI (Dashboard, Tendências).
//
// Não inclui Qt Widgets/Charts — apenas QDate/QtTypes/models/services/repos.
// Services/repos são instanciados direto a cada chamada (padrão do projeto).
class ResumoService {
public:
    ResumoService() = default;

    // Agrega um único mês (1–12) de um ano.
    ResumoMensal resumoDoMes(int mes, int ano) const;

    // Agrega o ano inteiro (entradas líquidas, gastos, saldo e proventos).
    ResumoAnual resumoDoAno(int ano) const;

    // Série cronológica inclusiva de [ini, fim], um ResumoMensal por mês.
    // Meses sem dado retornam ResumoMensal zerado (presentes, não pulados).
    QVector<ResumoMensal> serie(const QDate &ini, const QDate &fim) const;
};
