// src/ui/NavRegistry.cpp
// Implementação do registro central de navegação (#165).

#include "NavRegistry.h"

const QList<NavItem> &navItens()
{
    static const QList<NavItem> lista = {
        // Sem grupo — Dashboard sempre primeiro
        {QStringLiteral("tab_dashboard"),    QStringLiteral("🏠"), QStringLiteral("Dashboard"),            QStringLiteral("")},

        // Grupo "Dia a dia"
        {QStringLiteral("tab_entradas"),     QStringLiteral("💵"), QStringLiteral("Entradas"),             QStringLiteral("Dia a dia")},
        {QStringLiteral("tab_gastos"),       QStringLiteral("🧾"), QStringLiteral("Gastos"),               QStringLiteral("Dia a dia")},
        {QStringLiteral("tab_cartoes"),      QStringLiteral("💳"), QStringLiteral("Cartões"),              QStringLiteral("Dia a dia")},
        {QStringLiteral("tab_recorrencias"), QStringLiteral("🔁"), QStringLiteral("Recorrências"),         QStringLiteral("Dia a dia")},
        {QStringLiteral("tab_lembretes"),    QStringLiteral("🔔"), QStringLiteral("Lembretes"),            QStringLiteral("Dia a dia")},

        // Grupo "Planejar"
        {QStringLiteral("tab_planejamento"), QStringLiteral("📋"), QStringLiteral("Planejamento/Orçamento"), QStringLiteral("Planejar")},
        {QStringLiteral("tab_metas"),        QStringLiteral("🎯"), QStringLiteral("Metas"),                QStringLiteral("Planejar")},
        {QStringLiteral("tab_dividas"),      QStringLiteral("📉"), QStringLiteral("Dívidas"),              QStringLiteral("Planejar")},
        {QStringLiteral("tab_tendencias"),   QStringLiteral("📈"), QStringLiteral("Tendências"),           QStringLiteral("Planejar")},

        // Grupo "Investir"
        {QStringLiteral("tab_investimentos"),QStringLiteral("📊"), QStringLiteral("Investimentos"),        QStringLiteral("Investir")},
        {QStringLiteral("tab_proventos"),    QStringLiteral("💰"), QStringLiteral("Proventos"),            QStringLiteral("Investir")},
        {QStringLiteral("tab_bolsa"),        QStringLiteral("🏦"), QStringLiteral("Bolsa"),                QStringLiteral("Investir")},
    };
    return lista;
}
