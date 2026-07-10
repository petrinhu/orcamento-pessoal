#include "ExportWriters.h"

bool CsvExportWriter::write(const QString &path, const QList<ExportService::Section> &sections,
                             const QList<ExportService::OFXTx> &) const
{
    return ExportService::toMultiCsv(path, sections);
}

bool OfxExportWriter::write(const QString &path, const QList<ExportService::Section> &,
                             const QList<ExportService::OFXTx> &txs) const
{
    return ExportService::toMultiOfx(path, txs);
}

bool QifExportWriter::write(const QString &path, const QList<ExportService::Section> &,
                             const QList<ExportService::OFXTx> &txs) const
{
    return ExportService::toMultiQif(path, txs);
}

bool XlsxExportWriter::write(const QString &path, const QList<ExportService::Section> &sections,
                              const QList<ExportService::OFXTx> &) const
{
    return ExportService::toMultiXlsx(path, sections);
}

bool JsonExportWriter::write(const QString &path, const QList<ExportService::Section> &sections,
                              const QList<ExportService::OFXTx> &) const
{
    return ExportService::toMultiJson(path, sections);
}

bool MarkdownExportWriter::write(const QString &path, const QList<ExportService::Section> &sections,
                                  const QList<ExportService::OFXTx> &) const
{
    return ExportService::toMultiMarkdown(path, sections);
}

bool HtmlExportWriter::write(const QString &path, const QList<ExportService::Section> &sections,
                              const QList<ExportService::OFXTx> &) const
{
    return ExportService::toMultiHtml(path, sections);
}

bool PdfExportWriter::write(const QString &path, const QList<ExportService::Section> &sections,
                             const QList<ExportService::OFXTx> &) const
{
    return ExportService::toMultiPdf(path, sections);
}

bool TxtExportWriter::write(const QString &path, const QList<ExportService::Section> &sections,
                             const QList<ExportService::OFXTx> &) const
{
    return ExportService::toMultiTxt(path, sections);
}
