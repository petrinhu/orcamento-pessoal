#pragma once

#include "IExportWriter.h"

// ExportWriterRegistry (#200/ARQ-05) - ponto único de lookup dos writers de
// exportação. MainWindow::onExportar consulta `find(fmt)` em vez de
// despachar por cadeia de `if (fmt == "csv") ... else if (fmt == "xlsx")`.
// Adicionar um formato novo = implementar IExportWriter + registrar aqui;
// MainWindow e ExportService.h/.cpp não precisam mudar (OCP).
class ExportWriterRegistry {
public:
    // Retorna o writer registrado para `formatId` (chave vinda do combo/menu
    // de formato — ex. "csv", "xlsx", "xls"...) ou nullptr se desconhecido.
    // Ponteiro tem tempo de vida estático (registry function-local static);
    // não liberar/deletar.
    [[nodiscard]] static const IExportWriter *find(const QString &formatId);
};
