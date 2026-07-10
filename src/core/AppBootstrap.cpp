#include "AppBootstrap.h"
#include "core/database/DatabaseManager.h"
#include "services/FechamentoMensalService.h"
#include "services/AuditLogService.h"
#include "repositories/ProventoRepository.h"
#include "services/AjusteCorporativoService.h"

#include <QSqlDatabase>
#include <QDebug>

void AppBootstrap::registerCoreSchemaProviders()
{
    static bool done = false;
    if (done) return;
    done = true;

    // Ordem preservada — idêntica à sequência que existia inline em
    // DatabaseManager::createSchema() antes do #200/ARQ-01.

    // #66: tabela proventos_recebidos (tracker dividendos/JCP/rendimento FII)
    DatabaseManager::registerSchemaProvider([](QSqlDatabase &) {
        if (!ProventoRepository::ensureSchema())
            qWarning() << "AppBootstrap: falha ao criar schema de proventos_recebidos";
    });

    // #65: tabela ajustes_corporativos (splits/inplits/bonificacoes)
    DatabaseManager::registerSchemaProvider([](QSqlDatabase &) {
        if (!AjusteCorporativoService::ensureSchema())
            qWarning() << "AppBootstrap: falha ao criar schema de ajustes_corporativos";
    });

    // #88: tabela fechamentos_mensais (snapshot imutavel mes contabil)
    DatabaseManager::registerSchemaProvider([](QSqlDatabase &db) {
        if (!FechamentoMensalService::ensureSchema(db))
            qWarning() << "AppBootstrap: falha ao criar schema de fechamentos_mensais";
    });

    // #92: audit_log + triggers AFTER INSERT/UPDATE/DELETE em 7 tabelas
    DatabaseManager::registerSchemaProvider([](QSqlDatabase &db) {
        if (!AuditLogService::ensureSchema(db))
            qWarning() << "AppBootstrap: falha ao criar schema de audit_log";
    });
}
