#include "AderenciaOrcamento.h"

namespace aderencia {

Nivel calcular(qint64 real, qint64 plan, bool isDespesa)
{
    if (plan <= 0) return Nivel::SemMeta;

    const bool dentroBaixo = real * 100 <= plan * 90;   // real ≤ 90% do plan
    const bool acimaAlto   = real * 100 >  plan * 110;  // real > 110% do plan

    if (isDespesa) {
        if (dentroBaixo)      return Nivel::Bom;     // gastou bem menos
        else if (acimaAlto)   return Nivel::Alerta;  // estourou
        else                  return Nivel::Moderado; // dentro de ±10%
    }

    const bool acimaMeta = real * 100 >= plan * 110;  // real ≥ 110% do plan
    if (acimaMeta)        return Nivel::Bom;     // recebeu bem mais
    else if (dentroBaixo) return Nivel::Alerta;  // recebeu bem menos
    else                  return Nivel::Moderado; // dentro de ±10%
}

QString sufixo(Nivel nivel)
{
    switch (nivel) {
    case Nivel::Bom:    return QStringLiteral(" ✓"); // ✓
    case Nivel::Alerta: return QStringLiteral(" ⚠"); // ⚠
    case Nivel::Moderado:
    case Nivel::SemMeta:
    default:
        return QString();
    }
}

} // namespace aderencia
