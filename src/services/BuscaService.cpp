#include "BuscaService.h"
#include "core/database/DatabaseManager.h"

#include <QDate>
#include <QPair>
#include <QSqlQuery>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QString buildIsoDate(int day, int month, int year)
{
    return QDate(year, month, day).toString(Qt::ISODate);
}

static QPair<QString, QString> periodoParaDatas(const QString &periodo)
{
    const QDate hoje = QDate::currentDate();
    QString dataInicio, dataFim;
    dataFim = hoje.toString(Qt::ISODate);

    if (periodo == QStringLiteral("mes")) {
        dataInicio = QDate(hoje.year(), hoje.month(), 1).toString(Qt::ISODate);
    } else if (periodo == QStringLiteral("3meses")) {
        dataInicio = hoje.addMonths(-3).toString(Qt::ISODate);
    } else if (periodo == QStringLiteral("6meses")) {
        dataInicio = hoje.addMonths(-6).toString(Qt::ISODate);
    } else if (periodo == QStringLiteral("ano")) {
        dataInicio = hoje.addYears(-1).toString(Qt::ISODate);
    } else {
        dataInicio.clear(); // "tudo" (ou valor desconhecido) — sem filtro de data
    }
    return {dataInicio, dataFim};
}

// ── BuscaService::buscar ─────────────────────────────────────────────────────

QList<BuscaService::Resultado> BuscaService::buscar(const QString &termo,
                                                     const QString &periodo,
                                                     const QString &tipo)
{
    const auto [dataInicio, dataFim] = periodoParaDatas(periodo);
    const QString like = QStringLiteral("%%1%").arg(termo);

    auto db = DatabaseManager::instance().db();
    QList<Resultado> resultados;

    // ── Entradas ──────────────────────────────────────────────────────────
    if (tipo == QStringLiteral("todos") || tipo == QStringLiteral("entradas")) {
        QSqlQuery q(db);
        const bool filtrarData = !dataInicio.isEmpty();
        const QString sql = filtrarData
            ? QStringLiteral(
                "SELECT ano, mes, dia, fonte, valor_bruto, irpf, inss, obs "
                "FROM lancamentos_entradas "
                "WHERE date(ano||'-'||printf('%02d',mes)||'-'||printf('%02d',dia)) "
                "      BETWEEN :data_ini AND :data_fim "
                "  AND (fonte LIKE :like OR obs LIKE :like) "
                "ORDER BY ano DESC, mes DESC, dia DESC LIMIT 500")
            : QStringLiteral(
                "SELECT ano, mes, dia, fonte, valor_bruto, irpf, inss, obs "
                "FROM lancamentos_entradas "
                "WHERE (fonte LIKE :like OR obs LIKE :like) "
                "ORDER BY ano DESC, mes DESC, dia DESC LIMIT 500");
        q.prepare(sql);
        if (filtrarData) {
            q.bindValue(QStringLiteral(":data_ini"), dataInicio);
            q.bindValue(QStringLiteral(":data_fim"), dataFim);
        }
        q.bindValue(QStringLiteral(":like"), like);
        q.exec();
        while (q.next()) {
            Resultado r;
            r.categoria    = QString::fromLatin1(kCategoriaEntrada);
            r.data         = buildIsoDate(q.value(2).toInt(),
                                          q.value(1).toInt(),
                                          q.value(0).toInt());
            r.descricao    = q.value(3).toString();
            const qint64 bruto = q.value(4).toLongLong();
            const qint64 irpf  = q.value(5).toLongLong();
            const qint64 inss  = q.value(6).toLongLong();
            r.valor        = bruto - irpf - inss;
            r.categoriaObs = q.value(7).toString();
            r.tabIndex     = kTabEntradas;
            resultados << r;
        }
    }

    // ── Gastos ────────────────────────────────────────────────────────────
    if (tipo == QStringLiteral("todos") || tipo == QStringLiteral("gastos")) {
        QSqlQuery q(db);
        const bool filtrarData = !dataInicio.isEmpty();
        const QString sql = filtrarData
            ? QStringLiteral(
                "SELECT ano, mes, dia, descricao, valor, categoria, obs "
                "FROM lancamentos_gastos "
                "WHERE date(ano||'-'||printf('%02d',mes)||'-'||printf('%02d',dia)) "
                "      BETWEEN :data_ini AND :data_fim "
                "  AND (descricao LIKE :like OR categoria LIKE :like OR obs LIKE :like) "
                "ORDER BY ano DESC, mes DESC, dia DESC LIMIT 500")
            : QStringLiteral(
                "SELECT ano, mes, dia, descricao, valor, categoria, obs "
                "FROM lancamentos_gastos "
                "WHERE (descricao LIKE :like OR categoria LIKE :like OR obs LIKE :like) "
                "ORDER BY ano DESC, mes DESC, dia DESC LIMIT 500");
        q.prepare(sql);
        if (filtrarData) {
            q.bindValue(QStringLiteral(":data_ini"), dataInicio);
            q.bindValue(QStringLiteral(":data_fim"), dataFim);
        }
        q.bindValue(QStringLiteral(":like"), like);
        q.exec();
        while (q.next()) {
            Resultado r;
            r.categoria    = QString::fromLatin1(kCategoriaGasto);
            r.data         = buildIsoDate(q.value(2).toInt(),
                                          q.value(1).toInt(),
                                          q.value(0).toInt());
            r.descricao    = q.value(3).toString();
            r.valor        = -q.value(4).toLongLong();
            r.categoriaObs = q.value(5).toString();
            if (!q.value(6).toString().isEmpty())
                r.categoriaObs += QStringLiteral(" — ") + q.value(6).toString();
            r.tabIndex     = kTabGastos;
            resultados << r;
        }
    }

    // ── Investimentos ─────────────────────────────────────────────────────
    if (tipo == QStringLiteral("todos") || tipo == QStringLiteral("investimentos")) {
        QSqlQuery q(db);
        const bool filtrarData = !dataInicio.isEmpty();
        const QString sql = filtrarData
            ? QStringLiteral(
                "SELECT ano, mes, dia, nome, valor, tipo_ativo, obs "
                "FROM lancamentos_investimentos "
                "WHERE date(ano||'-'||printf('%02d',mes)||'-'||printf('%02d',dia)) "
                "      BETWEEN :data_ini AND :data_fim "
                "  AND (nome LIKE :like OR tipo_ativo LIKE :like OR obs LIKE :like) "
                "ORDER BY ano DESC, mes DESC, dia DESC LIMIT 500")
            : QStringLiteral(
                "SELECT ano, mes, dia, nome, valor, tipo_ativo, obs "
                "FROM lancamentos_investimentos "
                "WHERE (nome LIKE :like OR tipo_ativo LIKE :like OR obs LIKE :like) "
                "ORDER BY ano DESC, mes DESC, dia DESC LIMIT 500");
        q.prepare(sql);
        if (filtrarData) {
            q.bindValue(QStringLiteral(":data_ini"), dataInicio);
            q.bindValue(QStringLiteral(":data_fim"), dataFim);
        }
        q.bindValue(QStringLiteral(":like"), like);
        q.exec();
        while (q.next()) {
            Resultado r;
            r.categoria    = QString::fromLatin1(kCategoriaInvestimento);
            r.data         = buildIsoDate(q.value(2).toInt(),
                                          q.value(1).toInt(),
                                          q.value(0).toInt());
            r.descricao    = q.value(3).toString();
            r.valor        = q.value(4).toLongLong();
            r.categoriaObs = q.value(5).toString();
            if (!q.value(6).toString().isEmpty())
                r.categoriaObs += QStringLiteral(" — ") + q.value(6).toString();
            r.tabIndex     = kTabInvestimentos;
            resultados << r;
        }
    }

    // ── Dívidas ───────────────────────────────────────────────────────────
    if (tipo == QStringLiteral("todos") || tipo == QStringLiteral("dividas")) {
        QSqlQuery q(db);
        const QString sql = QStringLiteral(
            "SELECT data_inicio, contraparte, descricao, valor_total, obs "
            "FROM dividas "
            "WHERE (contraparte LIKE :like OR descricao LIKE :like OR obs LIKE :like) "
            "ORDER BY data_inicio DESC LIMIT 200");
        q.prepare(sql);
        q.bindValue(QStringLiteral(":like"), like);
        q.exec();
        while (q.next()) {
            Resultado r;
            r.categoria    = QString::fromLatin1(kCategoriaDivida);
            r.data         = q.value(0).toString();
            r.descricao    = q.value(1).toString();
            if (!q.value(2).toString().isEmpty())
                r.descricao += QStringLiteral(": ") + q.value(2).toString();
            r.valor        = q.value(3).toLongLong();
            r.categoriaObs = q.value(4).toString();
            r.tabIndex     = kTabDividas;
            resultados << r;
        }
    }

    // ── Cartões (compras) ─────────────────────────────────────────────────
    if (tipo == QStringLiteral("todos") || tipo == QStringLiteral("cartoes")) {
        QSqlQuery q(db);
        const QString sql = QStringLiteral(
            "SELECT c.criado_em, c.nome, c.valor_total, ca.apelido "
            "FROM compras c "
            "LEFT JOIN cartoes ca ON ca.id = c.cartao_id "
            "WHERE (c.nome LIKE :like OR ca.apelido LIKE :like) "
            "ORDER BY c.criado_em DESC LIMIT 200");
        q.prepare(sql);
        q.bindValue(QStringLiteral(":like"), like);
        q.exec();
        while (q.next()) {
            Resultado r;
            r.categoria    = QString::fromLatin1(kCategoriaCartao);
            r.data         = q.value(0).toString().left(10);
            r.descricao    = q.value(1).toString();
            r.valor        = q.value(2).toLongLong();
            r.categoriaObs = q.value(3).toString();
            r.tabIndex     = kTabCartoes;
            resultados << r;
        }
    }

    return resultados;
}
