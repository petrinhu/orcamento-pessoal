#pragma once

#include <QString>
#include <QDateTime>
#include <QList>

class QSqlDatabase;

// AuditLogService (#92) - trilha de auditoria via triggers SQLite.
//
// Cria tabela audit_log + triggers AFTER INSERT/UPDATE/DELETE nas tabelas
// financeiras chave. Cada operacao loga dados antes/depois em JSON
// (sqlite json_object). Recovery de incidentes ou conferencia LGPD.
//
// Schema:
//   audit_log(
//     id INTEGER PRIMARY KEY AUTOINCREMENT,
//     tabela TEXT NOT NULL,
//     op TEXT NOT NULL CHECK(op IN ('INSERT','UPDATE','DELETE')),
//     registro_id INTEGER,
//     dados_antigos TEXT,    -- JSON ou NULL para INSERT
//     dados_novos   TEXT,    -- JSON ou NULL para DELETE
//     ts            TEXT DEFAULT CURRENT_TIMESTAMP,
//     usuario       TEXT
//   )
//
// Padrao: Hibernate Envers usado em web-budget; aqui usamos triggers
// nativos para evitar overhead em runtime do app.

struct AuditLogEntry {
    qint64    id          = 0;
    QString   tabela;
    QString   op;          // INSERT, UPDATE, DELETE
    qint64    registroId   = 0;
    QString   dadosAntigos; // JSON
    QString   dadosNovos;   // JSON
    QDateTime timestamp;
    QString   usuario;
};

class AuditLogService
{
public:
    // Cria audit_log + indices + triggers nas tabelas chave. Idempotente
    // (CREATE IF NOT EXISTS). Chamar uma vez no startup, apos ensureSchema
    // de cada Repository.
    [[nodiscard]] static bool ensureSchema(QSqlDatabase &db,
                                            QString      *errorOut = nullptr);

    // Lista entradas filtradas. tabela vazia = todas; intervalo vazio = sem
    // filtro de data. Limite default 500 entradas (mais recentes primeiro).
    [[nodiscard]] static QList<AuditLogEntry> query(QSqlDatabase   &db,
                                                     const QString  &tabela = {},
                                                     const QDateTime &inicio = {},
                                                     const QDateTime &fim    = {},
                                                     int             limite  = 500);
};
