#pragma once

#include "services/ExportService.h"
#include <QString>
#include <QStringList>

// IExportWriter (#200/ARQ-05) - interface Strategy para os formatos de
// exportação multi-seção, espelhando IBankPDFParser (import, #93).
//
// Antes: MainWindow::onExportar tinha um `if/else if (fmt == "csv") ... else
// if (fmt == "xlsx") ...` que exigia tocar 3 camadas (MainWindow + ExportService
// .h + .cpp) para adicionar um formato novo — violação de OCP (ARQ-05).
//
// Depois: cada writer encapsula UM formato (ou um grupo de aliases do mesmo
// serializador, ex. "xlsx"/"xls") e delega 100% aos métodos estáticos que já
// existiam em ExportService (toMultiCsv, toMultiXlsx, ...) — nenhuma
// serialização foi reescrita, só encapsulada atrás de uma interface pequena.
// MainWindow::onExportar passa a fazer um lookup em ExportWriterRegistry em
// vez de despachar por string.
class IExportWriter {
public:
    virtual ~IExportWriter() = default;

    // Identificador(es) usados no dispatch a partir do combo/menu de formato
    // (ex.: XlsxExportWriter responde a "xlsx" E "xls" — mesmo serializador,
    // igual ao comportamento anterior do switch em MainWindow::onExportar).
    [[nodiscard]] virtual QStringList formatIds() const = 0;

    // Nome legível do formato (diagnóstico/log) — paridade com
    // IBankPDFParser::nomeBanco().
    [[nodiscard]] virtual QString nome() const = 0;

    // Executa a exportação. `sections` é usado pelos formatos tabulares
    // (csv/xlsx/json/md/html/pdf/txt); `txs` é usado pelos formatos
    // transacionais (ofx/qif). Cada writer usa apenas o parâmetro que precisa.
    [[nodiscard]] virtual bool write(const QString                        &path,
                                      const QList<ExportService::Section> &sections,
                                      const QList<ExportService::OFXTx>   &txs) const = 0;
};
