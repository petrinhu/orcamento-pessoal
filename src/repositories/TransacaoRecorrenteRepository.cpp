#include "TransacaoRecorrenteRepository.h"
#include "RepositoryUtils.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

static TransacaoRecorrente fromQuery(const QSqlQuery &q)
{
    TransacaoRecorrente t;
    t.id                = q.value(0).toInt();
    t.descricao         = q.value(1).toString();
    t.tipo              = q.value(2).toString();
    t.valor             = q.value(3).toLongLong();
    t.fonteId           = q.value(4).toInt();
    t.fonte             = q.value(5).toString();
    t.categoria         = q.value(6).toString();
    t.tipoPgto          = q.value(7).toString();
    t.obs               = q.value(8).toString();
    t.frequencia        = q.value(9).toString();
    t.dataInicio        = QDate::fromString(q.value(10).toString(), Qt::ISODate);
    const QString fim   = q.value(11).toString();
    if (!fim.isEmpty())
        t.dataFim       = QDate::fromString(fim, Qt::ISODate);
    t.ativo             = q.value(12).toInt() != 0;
    const QString ult   = q.value(13).toString();
    if (!ult.isEmpty())
        t.ultimoLancamento = QDate::fromString(ult, Qt::ISODate);
    t.criadoEm         = q.value(14).toString();
    return t;
}

bool TransacaoRecorrenteRepository::insert(TransacaoRecorrente &t)
{
    t.criadoEm = QDateTime::currentDateTime().toString(Qt::ISODate);
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "INSERT INTO transacoes_recorrentes "
        "(descricao, tipo, valor, fonte_id, fonte, categoria, tipo_pgto, obs, "
        " frequencia, data_inicio, data_fim, ativo, ultimo_lancamento, criado_em) "
        "VALUES (:descricao, :tipo, :valor, :fonte_id, :fonte, :categoria, :tipo_pgto, :obs, "
        "        :frequencia, :data_inicio, :data_fim, :ativo, :ultimo_lancamento, :criado_em)"));
    q.bindValue(QStringLiteral(":descricao"),         naoNulo(t.descricao));
    q.bindValue(QStringLiteral(":tipo"),              naoNulo(t.tipo));
    q.bindValue(QStringLiteral(":valor"),             t.valor);
    q.bindValue(QStringLiteral(":fonte_id"),          t.fonteId > 0 ? QVariant(t.fonteId) : QVariant());
    q.bindValue(QStringLiteral(":fonte"),             naoNulo(t.fonte));
    q.bindValue(QStringLiteral(":categoria"),         naoNulo(t.categoria));
    q.bindValue(QStringLiteral(":tipo_pgto"),         naoNulo(t.tipoPgto));
    q.bindValue(QStringLiteral(":obs"),               naoNulo(t.obs));
    q.bindValue(QStringLiteral(":frequencia"),        naoNulo(t.frequencia));
    q.bindValue(QStringLiteral(":data_inicio"),       naoNulo(t.dataInicio.toString(Qt::ISODate)));
    q.bindValue(QStringLiteral(":data_fim"),          t.dataFim.isValid() ? QVariant(t.dataFim.toString(Qt::ISODate)) : QVariant());
    q.bindValue(QStringLiteral(":ativo"),             t.ativo ? 1 : 0);
    q.bindValue(QStringLiteral(":ultimo_lancamento"), t.ultimoLancamento.isValid() ? QVariant(t.ultimoLancamento.toString(Qt::ISODate)) : QVariant());
    q.bindValue(QStringLiteral(":criado_em"),         naoNulo(t.criadoEm));
    if (!q.exec()) {
        qWarning() << "TransacaoRecorrenteRepository::insert:" << q.lastError().text();
        return false;
    }
    t.id = q.lastInsertId().toInt();
    return true;
}

bool TransacaoRecorrenteRepository::update(const TransacaoRecorrente &t)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "UPDATE transacoes_recorrentes SET "
        "descricao=:descricao, tipo=:tipo, valor=:valor, fonte_id=:fonte_id, fonte=:fonte, "
        "categoria=:categoria, tipo_pgto=:tipo_pgto, obs=:obs, frequencia=:frequencia, "
        "data_inicio=:data_inicio, data_fim=:data_fim, ativo=:ativo, ultimo_lancamento=:ultimo_lancamento "
        "WHERE id=:id"));
    q.bindValue(QStringLiteral(":descricao"),         naoNulo(t.descricao));
    q.bindValue(QStringLiteral(":tipo"),              naoNulo(t.tipo));
    q.bindValue(QStringLiteral(":valor"),             t.valor);
    q.bindValue(QStringLiteral(":fonte_id"),          t.fonteId > 0 ? QVariant(t.fonteId) : QVariant());
    q.bindValue(QStringLiteral(":fonte"),             naoNulo(t.fonte));
    q.bindValue(QStringLiteral(":categoria"),         naoNulo(t.categoria));
    q.bindValue(QStringLiteral(":tipo_pgto"),         naoNulo(t.tipoPgto));
    q.bindValue(QStringLiteral(":obs"),               naoNulo(t.obs));
    q.bindValue(QStringLiteral(":frequencia"),        naoNulo(t.frequencia));
    q.bindValue(QStringLiteral(":data_inicio"),       naoNulo(t.dataInicio.toString(Qt::ISODate)));
    q.bindValue(QStringLiteral(":data_fim"),          t.dataFim.isValid() ? QVariant(t.dataFim.toString(Qt::ISODate)) : QVariant());
    q.bindValue(QStringLiteral(":ativo"),             t.ativo ? 1 : 0);
    q.bindValue(QStringLiteral(":ultimo_lancamento"), t.ultimoLancamento.isValid() ? QVariant(t.ultimoLancamento.toString(Qt::ISODate)) : QVariant());
    q.bindValue(QStringLiteral(":id"),                t.id);
    if (!q.exec()) {
        qWarning() << "TransacaoRecorrenteRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool TransacaoRecorrenteRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM transacoes_recorrentes WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "TransacaoRecorrenteRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<TransacaoRecorrente> TransacaoRecorrenteRepository::findAll() const
{
    QSqlQuery q(QStringLiteral(
        "SELECT id, descricao, tipo, valor, fonte_id, fonte, categoria, tipo_pgto, obs, "
        "       frequencia, data_inicio, data_fim, ativo, ultimo_lancamento, criado_em "
        "FROM transacoes_recorrentes ORDER BY descricao"), db());
    QList<TransacaoRecorrente> list;
    while (q.next())
        list.append(fromQuery(q));
    return list;
}

std::optional<TransacaoRecorrente> TransacaoRecorrenteRepository::findById(int id) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT id, descricao, tipo, valor, fonte_id, fonte, categoria, tipo_pgto, obs, "
        "       frequencia, data_inicio, data_fim, ativo, ultimo_lancamento, criado_em "
        "FROM transacoes_recorrentes WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (q.exec() && q.next())
        return fromQuery(q);
    return std::nullopt;
}

bool TransacaoRecorrenteRepository::updateUltimoLancamento(int id, const QDate &data)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "UPDATE transacoes_recorrentes SET ultimo_lancamento = :data WHERE id = :id"));
    q.bindValue(QStringLiteral(":data"), data.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":id"),   id);
    if (!q.exec()) {
        qWarning() << "TransacaoRecorrenteRepository::updateUltimoLancamento:" << q.lastError().text();
        return false;
    }
    return true;
}
