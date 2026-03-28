#pragma once

#include <QDate>
#include <QString>
#include <QtTypes>

// Frequência do lembrete — controla geração automática da próxima ocorrência.
enum class FrequenciaLembrete { Unico, Mensal, Anual };

inline QString frequenciaLembreteParaTexto(FrequenciaLembrete f)
{
    switch (f) {
    case FrequenciaLembrete::Mensal: return QStringLiteral("mensal");
    case FrequenciaLembrete::Anual:  return QStringLiteral("anual");
    default:                         return QStringLiteral("unico");
    }
}

inline FrequenciaLembrete frequenciaLembreteDeTexto(const QString &s)
{
    if (s == QStringLiteral("mensal")) return FrequenciaLembrete::Mensal;
    if (s == QStringLiteral("anual"))  return FrequenciaLembrete::Anual;
    return FrequenciaLembrete::Unico;
}

inline QString frequenciaLembreteLabel(FrequenciaLembrete f)
{
    switch (f) {
    case FrequenciaLembrete::Mensal: return QStringLiteral("Mensal");
    case FrequenciaLembrete::Anual:  return QStringLiteral("Anual");
    default:                         return QStringLiteral("Único");
    }
}

// Status do lembrete.
//   pendente   → aguardando pagamento/ação
//   pago       → quitado pelo usuário ou debitado automaticamente
//   nao_pago   → débito automático não realizado — exige ação manual
//   ignorado   → usuário decidiu ignorar
enum class StatusLembrete { Pendente, Pago, NaoPago, Ignorado };

inline QString statusLembreteParaTexto(StatusLembrete s)
{
    switch (s) {
    case StatusLembrete::Pago:     return QStringLiteral("pago");
    case StatusLembrete::NaoPago:  return QStringLiteral("nao_pago");
    case StatusLembrete::Ignorado: return QStringLiteral("ignorado");
    default:                       return QStringLiteral("pendente");
    }
}

inline StatusLembrete statusLembreteDeTexto(const QString &s)
{
    if (s == QStringLiteral("pago"))     return StatusLembrete::Pago;
    if (s == QStringLiteral("nao_pago")) return StatusLembrete::NaoPago;
    if (s == QStringLiteral("ignorado")) return StatusLembrete::Ignorado;
    return StatusLembrete::Pendente;
}

inline QString statusLembreteLabel(StatusLembrete s)
{
    switch (s) {
    case StatusLembrete::Pago:     return QStringLiteral("Pago");
    case StatusLembrete::NaoPago:  return QStringLiteral("Não debitado");
    case StatusLembrete::Ignorado: return QStringLiteral("Ignorado");
    default:                       return QStringLiteral("Pendente");
    }
}

struct Lembrete {
    int     id              = 0;
    QString descricao;
    qint64  valor           = 0;    // centavos (0 = sem valor definido)
    QString dataVenc;               // ISO yyyy-MM-dd
    QString frequencia;             // ver FrequenciaLembrete helpers
    bool    debitoAutomatico = false; // true = debitado sem ação manual
    QString status;                 // ver StatusLembrete helpers
    QString obs;
    QString criadoEm;               // ISO 8601

    // Helpers de conveniência
    bool pendente() const { return status == QStringLiteral("pendente"); }
    bool pago()     const { return status == QStringLiteral("pago");     }
    bool naoPago()  const { return status == QStringLiteral("nao_pago"); }
    bool ignorado() const { return status == QStringLiteral("ignorado"); }

    // Dias até o vencimento (negativo = já venceu; 0 = hoje).
    int diasParaVencer() const
    {
        const QDate d = QDate::fromString(dataVenc, Qt::ISODate);
        return d.isValid() ? QDate::currentDate().daysTo(d) : 0;
    }

    bool vencido()  const { return pendente() && diasParaVencer() < 0; }
    bool proximo()  const { return pendente() && diasParaVencer() >= 0 && diasParaVencer() <= 7; }
};
