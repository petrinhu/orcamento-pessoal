#pragma once

#include "IExportWriter.h"

// ExportWriters (#200/ARQ-05) - implementações finas de IExportWriter, uma
// por formato de exportação multi-seção. Cada write() é um forward de UMA
// linha para o método estático correspondente já existente em ExportService
// — zero mudança de comportamento/serialização (golden-file em
// tests/test_export_writers.cpp prova saída byte-idêntica ao caminho antigo).

class CsvExportWriter final : public IExportWriter {
public:
    [[nodiscard]] QStringList formatIds() const override { return { QStringLiteral("csv") }; }
    [[nodiscard]] QString nome() const override { return QStringLiteral("CSV"); }
    [[nodiscard]] bool write(const QString &path, const QList<ExportService::Section> &sections,
                              const QList<ExportService::OFXTx> &) const override;
};

class OfxExportWriter final : public IExportWriter {
public:
    [[nodiscard]] QStringList formatIds() const override { return { QStringLiteral("ofx") }; }
    [[nodiscard]] QString nome() const override { return QStringLiteral("OFX"); }
    [[nodiscard]] bool write(const QString &path, const QList<ExportService::Section> &,
                              const QList<ExportService::OFXTx> &txs) const override;
};

class QifExportWriter final : public IExportWriter {
public:
    [[nodiscard]] QStringList formatIds() const override { return { QStringLiteral("qif") }; }
    [[nodiscard]] QString nome() const override { return QStringLiteral("QIF"); }
    [[nodiscard]] bool write(const QString &path, const QList<ExportService::Section> &,
                              const QList<ExportService::OFXTx> &txs) const override;
};

// Um único serializador (ExportService::toMultiXlsx) atende os dois aliases
// "xlsx" e "xls" — mesmo comportamento do switch original.
class XlsxExportWriter final : public IExportWriter {
public:
    [[nodiscard]] QStringList formatIds() const override
    { return { QStringLiteral("xlsx"), QStringLiteral("xls") }; }
    [[nodiscard]] QString nome() const override { return QStringLiteral("Excel (XLSX)"); }
    [[nodiscard]] bool write(const QString &path, const QList<ExportService::Section> &sections,
                              const QList<ExportService::OFXTx> &) const override;
};

class JsonExportWriter final : public IExportWriter {
public:
    [[nodiscard]] QStringList formatIds() const override { return { QStringLiteral("json") }; }
    [[nodiscard]] QString nome() const override { return QStringLiteral("JSON"); }
    [[nodiscard]] bool write(const QString &path, const QList<ExportService::Section> &sections,
                              const QList<ExportService::OFXTx> &) const override;
};

class MarkdownExportWriter final : public IExportWriter {
public:
    [[nodiscard]] QStringList formatIds() const override { return { QStringLiteral("md") }; }
    [[nodiscard]] QString nome() const override { return QStringLiteral("Markdown"); }
    [[nodiscard]] bool write(const QString &path, const QList<ExportService::Section> &sections,
                              const QList<ExportService::OFXTx> &) const override;
};

class HtmlExportWriter final : public IExportWriter {
public:
    [[nodiscard]] QStringList formatIds() const override { return { QStringLiteral("html") }; }
    [[nodiscard]] QString nome() const override { return QStringLiteral("HTML"); }
    [[nodiscard]] bool write(const QString &path, const QList<ExportService::Section> &sections,
                              const QList<ExportService::OFXTx> &) const override;
};

class PdfExportWriter final : public IExportWriter {
public:
    [[nodiscard]] QStringList formatIds() const override { return { QStringLiteral("pdf") }; }
    [[nodiscard]] QString nome() const override { return QStringLiteral("PDF"); }
    [[nodiscard]] bool write(const QString &path, const QList<ExportService::Section> &sections,
                              const QList<ExportService::OFXTx> &) const override;
};

class TxtExportWriter final : public IExportWriter {
public:
    [[nodiscard]] QStringList formatIds() const override { return { QStringLiteral("txt") }; }
    [[nodiscard]] QString nome() const override { return QStringLiteral("Texto"); }
    [[nodiscard]] bool write(const QString &path, const QList<ExportService::Section> &sections,
                              const QList<ExportService::OFXTx> &) const override;
};
