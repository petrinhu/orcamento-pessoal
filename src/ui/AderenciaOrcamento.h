#pragma once

#include <QString>
#include <QtGlobal>

// Classificação de aderência real×planejado (Planejamento) e o sufixo textual
// redundante à cor — WCAG 1.4.1 (#203b/A11Y-07, auditoria 2026-06-22 §3.10):
// a coluna "%" usava só a cor (verde/azul/vermelho) para sinalizar a
// aderência, invisível para daltônicos. Extraído de PlanejamentoWidget para
// ser testável sem QApplication/DB.
namespace aderencia {

enum class Nivel {
    SemMeta,   // plan <= 0 — sem cor, exibido como "—"
    Bom,       // bem dentro (despesa) / acima da meta (receita)
    Moderado,  // dentro da faixa de tolerância ±10%
    Alerta     // estourou (despesa) / abaixo da meta (receita)
};

// Tolerância ±10%, comparação em inteiros (regra do projeto — sem double).
//   Despesa: real ≤ plan·0,90 → Bom; ±10% → Moderado; real > plan·1,10 → Alerta.
//   Receita (invertido): real ≥ plan·1,10 → Bom; ±10% → Moderado; < plan·0,90 → Alerta.
Nivel calcular(qint64 real, qint64 plan, bool isDespesa);

// Sufixo textual redundante à cor. Bom=" ✓", Alerta=" ⚠", Moderado/SemMeta=""
// (estado neutro — já distinguível de Bom/Alerta pela ausência de sufixo).
QString sufixo(Nivel nivel);

} // namespace aderencia
