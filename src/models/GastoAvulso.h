#pragma once

#include <QByteArray>
#include <QString>
#include <QtTypes>

// Tipo de pagamento para lançamentos de gastos avulsos.
// Armazenado como TEXT no banco ("pix", "debito", …).
enum class TipoPagamento {
    Pix, Debito, Dinheiro, Boleto, Transferencia, Fixo, Outro
};

inline QString tipoPgtoParaTexto(TipoPagamento t)
{
    switch (t) {
    case TipoPagamento::Pix:          return QStringLiteral("pix");
    case TipoPagamento::Debito:       return QStringLiteral("debito");
    case TipoPagamento::Dinheiro:     return QStringLiteral("dinheiro");
    case TipoPagamento::Boleto:       return QStringLiteral("boleto");
    case TipoPagamento::Transferencia:return QStringLiteral("transferencia");
    case TipoPagamento::Fixo:         return QStringLiteral("fixo");
    default:                          return QStringLiteral("outro");
    }
}

inline TipoPagamento tipoPgtoDeTexto(const QString &s)
{
    if (s == QStringLiteral("pix"))           return TipoPagamento::Pix;
    if (s == QStringLiteral("debito"))        return TipoPagamento::Debito;
    if (s == QStringLiteral("dinheiro"))      return TipoPagamento::Dinheiro;
    if (s == QStringLiteral("boleto"))        return TipoPagamento::Boleto;
    if (s == QStringLiteral("transferencia")) return TipoPagamento::Transferencia;
    if (s == QStringLiteral("fixo"))          return TipoPagamento::Fixo;
    return TipoPagamento::Outro;
}

// Label legível para exibição na tabela.
inline QString tipoPgtoLabel(TipoPagamento t)
{
    switch (t) {
    case TipoPagamento::Pix:          return QStringLiteral("PIX");
    case TipoPagamento::Debito:       return QStringLiteral("Débito");
    case TipoPagamento::Dinheiro:     return QStringLiteral("Dinheiro");
    case TipoPagamento::Boleto:       return QStringLiteral("Boleto");
    case TipoPagamento::Transferencia:return QStringLiteral("Transf.");
    case TipoPagamento::Fixo:         return QStringLiteral("Fixo");
    default:                          return QStringLiteral("Outro");
    }
}

inline QString tipoPgtoLabel(const QString &raw)
{
    return tipoPgtoLabel(tipoPgtoDeTexto(raw));
}

struct GastoAvulso {
    int        id        = 0;
    int        mes       = 1;    // 1–12
    int        ano       = 2025;
    int        dia       = 1;
    QString    descricao;
    QString    categoria;        // snapshot do nome da categoria
    int        categoriaId = 0; // FK → categorias(id); 0 = sem vínculo
    QString    tipoPgto;         // raw text: "pix"|"debito"|…
    qint64     valor     = 0;    // centavos
    QString    obs;
    QString    pagador;          // snapshot do nome do beneficiário/pagador
    int        pagadorId = 0;    // FK → pagadores(id); 0 = sem vínculo
    QString    criadoEm;         // ISO 8601
    QByteArray receipt;          // anexo do comprovante (#90); vazio = sem anexo
};
