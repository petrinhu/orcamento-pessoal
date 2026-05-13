#include "LancamentoEntradaRepository.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

static LancamentoEntrada fromQuery(const QSqlQuery &q)
{
    LancamentoEntrada l;
    l.id         = q.value(QStringLiteral("id")).toInt();
    l.mes        = q.value(QStringLiteral("mes")).toInt();
    l.ano        = q.value(QStringLiteral("ano")).toInt();
    l.dia        = q.value(QStringLiteral("dia")).toInt();
    l.fonte       = q.value(QStringLiteral("fonte")).toString();
    l.fonteId     = q.value(QStringLiteral("fonte_id")).toInt();
    l.categoria   = q.value(QStringLiteral("categoria")).toString();
    l.categoriaId = q.value(QStringLiteral("categoria_id")).toInt();
    l.valorBruto  = q.value(QStringLiteral("valor_bruto")).toLongLong();
    l.irpf       = q.value(QStringLiteral("irpf")).toLongLong();
    l.inss       = q.value(QStringLiteral("inss")).toLongLong();
    l.obs        = q.value(QStringLiteral("obs")).toString();
    l.criadoEm   = q.value(QStringLiteral("criado_em")).toString();
    return l;
}

bool LancamentoEntradaRepository::insert(LancamentoEntrada &l)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT INTO lancamentos_entradas
            (mes, ano, dia, fonte, fonte_id, categoria, categoria_id,
             valor_bruto, irpf, inss, obs, criado_em)
        VALUES
            (:mes, :ano, :dia, :fonte, :fonte_id, :categoria, :categoria_id,
             :valor_bruto, :irpf, :inss, :obs, :criado_em)
    )"));
    q.bindValue(QStringLiteral(":mes"),        l.mes);
    q.bindValue(QStringLiteral(":ano"),        l.ano);
    q.bindValue(QStringLiteral(":dia"),        l.dia);
    q.bindValue(QStringLiteral(":fonte"),       l.fonte);
    q.bindValue(QStringLiteral(":fonte_id"),    l.fonteId ? QVariant(l.fonteId) : QVariant());
    q.bindValue(QStringLiteral(":categoria"),   l.categoria);
    q.bindValue(QStringLiteral(":categoria_id"),
                l.categoriaId > 0 ? QVariant(l.categoriaId) : QVariant(QMetaType::fromType<int>()));
    q.bindValue(QStringLiteral(":valor_bruto"), l.valorBruto);
    q.bindValue(QStringLiteral(":irpf"),        l.irpf);
    q.bindValue(QStringLiteral(":inss"),        l.inss);
    q.bindValue(QStringLiteral(":obs"),         l.obs);
    q.bindValue(QStringLiteral(":criado_em"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "LancamentoEntradaRepository::insert:" << q.lastError().text();
        return false;
    }
    l.id = q.lastInsertId().toInt();
    return true;
}

bool LancamentoEntradaRepository::update(const LancamentoEntrada &l)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        UPDATE lancamentos_entradas SET
            mes         = :mes,
            ano         = :ano,
            dia         = :dia,
            fonte        = :fonte,
            fonte_id     = :fonte_id,
            categoria    = :categoria,
            categoria_id = :categoria_id,
            valor_bruto  = :valor_bruto,
            irpf         = :irpf,
            inss         = :inss,
            obs          = :obs
        WHERE id = :id
    )"));
    q.bindValue(QStringLiteral(":mes"),        l.mes);
    q.bindValue(QStringLiteral(":ano"),        l.ano);
    q.bindValue(QStringLiteral(":dia"),        l.dia);
    q.bindValue(QStringLiteral(":fonte"),       l.fonte);
    q.bindValue(QStringLiteral(":fonte_id"),    l.fonteId ? QVariant(l.fonteId) : QVariant());
    q.bindValue(QStringLiteral(":categoria"),   l.categoria);
    q.bindValue(QStringLiteral(":categoria_id"),
                l.categoriaId > 0 ? QVariant(l.categoriaId) : QVariant(QMetaType::fromType<int>()));
    q.bindValue(QStringLiteral(":valor_bruto"), l.valorBruto);
    q.bindValue(QStringLiteral(":irpf"),        l.irpf);
    q.bindValue(QStringLiteral(":inss"),        l.inss);
    q.bindValue(QStringLiteral(":obs"),         l.obs);
    q.bindValue(QStringLiteral(":id"),          l.id);
    if (!q.exec()) {
        qWarning() << "LancamentoEntradaRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool LancamentoEntradaRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM lancamentos_entradas WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "LancamentoEntradaRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<LancamentoEntrada> LancamentoEntradaRepository::findAll() const
{
    QSqlQuery q(QStringLiteral(
        "SELECT * FROM lancamentos_entradas ORDER BY ano, mes, dia, id"), db());
    QList<LancamentoEntrada> list;
    while (q.next())
        list.append(fromQuery(q));
    return list;
}

QList<LancamentoEntrada> LancamentoEntradaRepository::findByMesAno(int mes, int ano) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT * FROM lancamentos_entradas WHERE mes = :mes AND ano = :ano ORDER BY dia, id"));
    q.bindValue(QStringLiteral(":mes"), mes);
    q.bindValue(QStringLiteral(":ano"), ano);
    QList<LancamentoEntrada> list;
    if (q.exec())
        while (q.next())
            list.append(fromQuery(q));
    return list;
}

std::optional<LancamentoEntrada> LancamentoEntradaRepository::findById(int id) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT * FROM lancamentos_entradas WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (q.exec() && q.next())
        return fromQuery(q);
    return std::nullopt;
}
