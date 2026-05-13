#include "FileObfuscator.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QHash>
#include <QString>

namespace {

struct TabelaInfo {
    const char *tabela;
    const char *colunaIdentificavel;
    const char *prefixoDummy;
};

constexpr TabelaInfo kTabelas[] = {
    { "fontes_renda",                "nome",      "Fonte" },
    { "pagadores",                   "nome",      "Pessoa" },
    { "categorias",                  "nome",      "Categoria" },
    { "cartoes",                     "nome",      "Cartao" },
    { "lancamentos_entradas",        "descricao", "d_" },
    { "lancamentos_gastos",          "descricao", "d_" },
    { "lancamentos_investimentos",   "descricao", "d_" },
    { "compras",                     "descricao", "d_" },
    { "metas",                       "nome",      "Meta" },
    { "lembretes",                   "titulo",    "Lembrete" },
    { "dividas",                     "descricao", "d_" },
    { "transacoes_recorrentes",      "descricao", "d_" },
};

QString anonimizar(const QString &original, const char *prefixoDummy, int seqId)
{
    if (original.isEmpty()) return {};
    const QString prefixo = QString::fromLatin1(prefixoDummy);
    if (prefixo == QStringLiteral("d_")) {
        const QByteArray h = QCryptographicHash::hash(
            original.toUtf8(), QCryptographicHash::Md5);
        return prefixo + QString::fromLatin1(h.toHex().left(10));
    }
    return QStringLiteral("%1 %2").arg(prefixo).arg(seqId);
}

QJsonArray tabelaParaJson(QSqlDatabase &db, const TabelaInfo &info)
{
    QJsonArray rows;
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("SELECT * FROM %1")
                    .arg(QString::fromLatin1(info.tabela)))) {
        return rows;
    }

    int seq = 0;
    while (q.next()) {
        ++seq;
        const QSqlRecord rec = q.record();
        QJsonObject row;
        for (int i = 0; i < rec.count(); ++i) {
            const QString col = rec.fieldName(i);
            QVariant val = q.value(i);

            if (info.colunaIdentificavel != nullptr
                && col == QString::fromLatin1(info.colunaIdentificavel)) {
                val = anonimizar(val.toString(), info.prefixoDummy, seq);
            }

            switch (val.typeId()) {
            case QMetaType::Int:
            case QMetaType::LongLong:
                row.insert(col, QJsonValue(val.toLongLong()));
                break;
            case QMetaType::Double:
                row.insert(col, QJsonValue(val.toDouble()));
                break;
            default:
                row.insert(col, val.toString());
                break;
            }
        }
        rows.append(row);
    }
    return rows;
}

}

QJsonObject FileObfuscator::obfuscate(QSqlDatabase &db)
{
    QJsonObject root;
    root.insert(QStringLiteral("schema_version"), 1);
    root.insert(QStringLiteral("obfuscated"),     true);

    QJsonObject tabelas;
    for (const auto &info : kTabelas) {
        tabelas.insert(QString::fromLatin1(info.tabela),
                       tabelaParaJson(db, info));
    }
    root.insert(QStringLiteral("tabelas"), tabelas);
    return root;
}

bool FileObfuscator::saveToFile(const QString &filePath, QSqlDatabase &db,
                                QString *errorOut)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorOut != nullptr) *errorOut = f.errorString();
        return false;
    }
    const QJsonDocument doc(obfuscate(db));
    const QByteArray bytes = doc.toJson(QJsonDocument::Indented);
    const qint64 written = f.write(bytes);
    f.close();
    if (written != bytes.size()) {
        if (errorOut != nullptr) *errorOut = QStringLiteral("Escrita incompleta");
        return false;
    }
    return true;
}
