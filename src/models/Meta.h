#pragma once

#include <QDate>
#include <QString>
#include <QtTypes>

// Categoria da meta financeira.
enum class CategoriaMeta {
    Emergencia, Viagem, Imovel, Educacao, Aposentadoria, Outro
};

inline QString categoriaMetaParaTexto(CategoriaMeta c)
{
    switch (c) {
    case CategoriaMeta::Emergencia:    return QStringLiteral("emergencia");
    case CategoriaMeta::Viagem:        return QStringLiteral("viagem");
    case CategoriaMeta::Imovel:        return QStringLiteral("imovel");
    case CategoriaMeta::Educacao:      return QStringLiteral("educacao");
    case CategoriaMeta::Aposentadoria: return QStringLiteral("aposentadoria");
    default:                           return QStringLiteral("outro");
    }
}

inline CategoriaMeta categoriaMetaDeTexto(const QString &s)
{
    if (s == QStringLiteral("emergencia"))    return CategoriaMeta::Emergencia;
    if (s == QStringLiteral("viagem"))        return CategoriaMeta::Viagem;
    if (s == QStringLiteral("imovel"))        return CategoriaMeta::Imovel;
    if (s == QStringLiteral("educacao"))      return CategoriaMeta::Educacao;
    if (s == QStringLiteral("aposentadoria")) return CategoriaMeta::Aposentadoria;
    return CategoriaMeta::Outro;
}

inline QString categoriaMetaLabel(CategoriaMeta c)
{
    switch (c) {
    case CategoriaMeta::Emergencia:    return QStringLiteral("Emergência");
    case CategoriaMeta::Viagem:        return QStringLiteral("Viagem");
    case CategoriaMeta::Imovel:        return QStringLiteral("Imóvel");
    case CategoriaMeta::Educacao:      return QStringLiteral("Educação");
    case CategoriaMeta::Aposentadoria: return QStringLiteral("Aposentadoria");
    default:                           return QStringLiteral("Outro");
    }
}

// Status da meta.
enum class StatusMeta { EmAndamento, Concluida, Cancelada };

inline QString statusMetaParaTexto(StatusMeta s)
{
    switch (s) {
    case StatusMeta::Concluida:  return QStringLiteral("concluida");
    case StatusMeta::Cancelada:  return QStringLiteral("cancelada");
    default:                     return QStringLiteral("em_andamento");
    }
}

inline StatusMeta statusMetaDeTexto(const QString &s)
{
    if (s == QStringLiteral("concluida")) return StatusMeta::Concluida;
    if (s == QStringLiteral("cancelada")) return StatusMeta::Cancelada;
    return StatusMeta::EmAndamento;
}

inline QString statusMetaLabel(StatusMeta s)
{
    switch (s) {
    case StatusMeta::Concluida: return QStringLiteral("Concluída");
    case StatusMeta::Cancelada: return QStringLiteral("Cancelada");
    default:                    return QStringLiteral("Em andamento");
    }
}

struct Meta {
    int     id         = 0;
    QString nome;
    QString categoria;              // ver CategoriaMeta helpers
    qint64  valorMeta  = 0;         // centavos — objetivo
    qint64  valorAtual = 0;         // centavos — acumulado até agora
    QString dataInicio;             // ISO yyyy-MM-dd
    QString dataPrazo;              // ISO yyyy-MM-dd (vazio = sem prazo)
    QString status;                 // "em_andamento" | "concluida" | "cancelada"
    QString obs;
    QString criadoEm;               // ISO 8601

    // Progresso em percentual (0–100+).
    int progresso() const
    {
        return valorMeta > 0
            ? static_cast<int>(valorAtual * 100 / valorMeta)
            : 0;
    }

    // Dias restantes até o prazo (negativo = atrasado; 0 = sem prazo).
    int diasRestantes() const
    {
        if (dataPrazo.isEmpty()) return 0;
        const QDate prazo = QDate::fromString(dataPrazo, Qt::ISODate);
        return prazo.isValid() ? QDate::currentDate().daysTo(prazo) : 0;
    }
};
