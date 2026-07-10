#include "ExportWriterRegistry.h"
#include "ExportWriters.h"

#include <memory>
#include <vector>

namespace {

const std::vector<std::unique_ptr<IExportWriter>> &writers()
{
    static const std::vector<std::unique_ptr<IExportWriter>> registry = [] {
        std::vector<std::unique_ptr<IExportWriter>> v;
        v.push_back(std::make_unique<CsvExportWriter>());
        v.push_back(std::make_unique<OfxExportWriter>());
        v.push_back(std::make_unique<QifExportWriter>());
        v.push_back(std::make_unique<XlsxExportWriter>());
        v.push_back(std::make_unique<JsonExportWriter>());
        v.push_back(std::make_unique<MarkdownExportWriter>());
        v.push_back(std::make_unique<HtmlExportWriter>());
        v.push_back(std::make_unique<PdfExportWriter>());
        v.push_back(std::make_unique<TxtExportWriter>());
        return v;
    }();
    return registry;
}

} // namespace

const IExportWriter *ExportWriterRegistry::find(const QString &formatId)
{
    for (const auto &w : writers()) {
        if (w->formatIds().contains(formatId))
            return w.get();
    }
    return nullptr;
}
