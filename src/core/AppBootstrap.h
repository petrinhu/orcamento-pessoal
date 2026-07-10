#pragma once

// AppBootstrap (#200/ARQ-01) - ponto de composição único que liga o
// DatabaseManager (Foundation) aos 4 providers de schema Mid/Backend
// (FechamentoMensalService #88, AuditLogService #92, ProventoRepository #66,
// AjusteCorporativoService #65), sem que o DatabaseManager precise
// `#include` ou conhecer essas classes concretas — inversão de dependência
// via DatabaseManager::registerSchemaProvider (DI por std::function, mesmo
// padrão já usado em BackupService::ExportFn, #79).
//
// Chamar registerCoreSchemaProviders() UMA VEZ, ANTES do primeiro
// DatabaseManager::instance().open():
//   - app real: em main(), antes do login loop;
//   - testes: no initTestCase() de cada suíte que abre o DB diretamente
//     (essas suítes não passam por main(), então precisam registrar também).
//
// Idempotente: chamadas repetidas no mesmo processo são ignoradas após a
// primeira (guarda estática) — seguro chamar de múltiplos pontos.
class AppBootstrap
{
public:
    static void registerCoreSchemaProviders();
};
