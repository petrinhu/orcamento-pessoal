// PeuBud - golden-file de IExportWriter (#200/ARQ-05).
//
// ARQ-05: MainWindow::onExportar despachava por cadeia de
// `if (fmt == "csv") ... else if (fmt == "xlsx") ...` — formato novo exigia
// tocar 3 camadas (MainWindow + ExportService.h + .cpp). Fix: Strategy
// (IExportWriter) com ~10 writers finos que delegam 100% aos métodos
// estáticos já existentes de ExportService (nenhuma serialização reescrita).
//
// Este teste prova ZERO mudança de comportamento: para cada formato
// determinístico (csv/ofx/qif/json/md/html/txt) o writer produz saída
// byte-idêntica à chamada direta ao método estático correspondente
// (golden-file "auto-referente" — o baseline é o próprio ExportService, que
// não mudou uma linha). xlsx/pdf embutem timestamp (ZIP mtime / metadata de
// criação) — não comparáveis byte-a-byte de forma determinística; validados
// por sucesso + arquivo não-vazio.

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>

#include "services/ExportService.h"
#include "services/export/ExportWriterRegistry.h"
#include "services/export/ExportWriters.h"
#include "models/LancamentoEntrada.h"
#include "models/GastoAvulso.h"

class TestExportWriters : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir m_dir;

    static QByteArray readAll(const QString &path)
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return {};
        return f.readAll();
    }

    // Dataset de amostra reaproveitado em todos os formatos tabulares.
    QList<ExportService::Section> sampleSections() const
    {
        LancamentoEntrada l;
        l.id = 1; l.ano = 2025; l.mes = 3; l.dia = 15;
        l.valorBruto = 500000; l.irpf = 50000; l.inss = 20000;
        l.fonte = QStringLiteral("Salário");

        GastoAvulso g;
        g.id = 1; g.ano = 2025; g.mes = 3; g.dia = 20;
        g.valor = 8990; g.descricao = QStringLiteral("Conta de luz");

        return {
            { QStringLiteral("Entradas"), ExportService::buildTableEntradas({ l }) },
            { QStringLiteral("Gastos"),   ExportService::buildTableGastos({ g }) },
        };
    }

    QList<ExportService::OFXTx> sampleTxs() const
    {
        LancamentoEntrada l;
        l.id = 1; l.ano = 2025; l.mes = 3; l.dia = 15;
        l.valorBruto = 500000; l.irpf = 0; l.inss = 0;
        l.fonte = QStringLiteral("Salário");
        return ExportService::ofxFromEntradas({ l });
    }

private slots:
    // ── Registry ──────────────────────────────────────────────────────────

    void registryEncontraTodosOsFormatosConhecidos()
    {
        const QStringList formatos = {
            QStringLiteral("csv"), QStringLiteral("ofx"), QStringLiteral("qif"),
            QStringLiteral("xlsx"), QStringLiteral("xls"), QStringLiteral("json"),
            QStringLiteral("md"), QStringLiteral("html"), QStringLiteral("pdf"),
            QStringLiteral("txt"),
        };
        for (const QString &fmt : formatos) {
            const IExportWriter *w = ExportWriterRegistry::find(fmt);
            QVERIFY2(w != nullptr, qPrintable(QStringLiteral("formato sem writer: %1").arg(fmt)));
        }
    }

    void registryRetornaNullptrParaFormatoDesconhecido()
    {
        QVERIFY(ExportWriterRegistry::find(QStringLiteral("dbase3")) == nullptr);
        QVERIFY(ExportWriterRegistry::find(QString()) == nullptr);
    }

    void xlsxExlsCompartilhamOMesmoWriter()
    {
        QCOMPARE(ExportWriterRegistry::find(QStringLiteral("xlsx")),
                 ExportWriterRegistry::find(QStringLiteral("xls")));
    }

    // ── Golden-file: saída byte-idêntica ao ExportService direto ────────────

    void csvByteIdenticoAoBaseline()
    {
        const auto sections = sampleSections();
        const QString baseline = m_dir.filePath(QStringLiteral("baseline.csv"));
        const QString viaWriter = m_dir.filePath(QStringLiteral("writer.csv"));

        QVERIFY(ExportService::toMultiCsv(baseline, sections));
        const IExportWriter *w = ExportWriterRegistry::find(QStringLiteral("csv"));
        QVERIFY(w != nullptr);
        QVERIFY(w->write(viaWriter, sections, {}));

        QCOMPARE(readAll(viaWriter), readAll(baseline));
    }

    void ofxByteIdenticoAoBaseline()
    {
        const auto txs = sampleTxs();
        const QString baseline = m_dir.filePath(QStringLiteral("baseline.ofx"));
        const QString viaWriter = m_dir.filePath(QStringLiteral("writer.ofx"));

        QVERIFY(ExportService::toMultiOfx(baseline, txs));
        const IExportWriter *w = ExportWriterRegistry::find(QStringLiteral("ofx"));
        QVERIFY(w != nullptr);
        QVERIFY(w->write(viaWriter, {}, txs));

        QCOMPARE(readAll(viaWriter), readAll(baseline));
    }

    void qifByteIdenticoAoBaseline()
    {
        const auto txs = sampleTxs();
        const QString baseline = m_dir.filePath(QStringLiteral("baseline.qif"));
        const QString viaWriter = m_dir.filePath(QStringLiteral("writer.qif"));

        QVERIFY(ExportService::toMultiQif(baseline, txs));
        const IExportWriter *w = ExportWriterRegistry::find(QStringLiteral("qif"));
        QVERIFY(w != nullptr);
        QVERIFY(w->write(viaWriter, {}, txs));

        QCOMPARE(readAll(viaWriter), readAll(baseline));
    }

    void jsonByteIdenticoAoBaseline()
    {
        const auto sections = sampleSections();
        const QString baseline = m_dir.filePath(QStringLiteral("baseline.json"));
        const QString viaWriter = m_dir.filePath(QStringLiteral("writer.json"));

        QVERIFY(ExportService::toMultiJson(baseline, sections));
        const IExportWriter *w = ExportWriterRegistry::find(QStringLiteral("json"));
        QVERIFY(w != nullptr);
        QVERIFY(w->write(viaWriter, sections, {}));

        QCOMPARE(readAll(viaWriter), readAll(baseline));
    }

    void markdownByteIdenticoAoBaseline()
    {
        const auto sections = sampleSections();
        const QString baseline = m_dir.filePath(QStringLiteral("baseline.md"));
        const QString viaWriter = m_dir.filePath(QStringLiteral("writer.md"));

        QVERIFY(ExportService::toMultiMarkdown(baseline, sections));
        const IExportWriter *w = ExportWriterRegistry::find(QStringLiteral("md"));
        QVERIFY(w != nullptr);
        QVERIFY(w->write(viaWriter, sections, {}));

        QCOMPARE(readAll(viaWriter), readAll(baseline));
    }

    void htmlByteIdenticoAoBaseline()
    {
        const auto sections = sampleSections();
        const QString baseline = m_dir.filePath(QStringLiteral("baseline.html"));
        const QString viaWriter = m_dir.filePath(QStringLiteral("writer.html"));

        QVERIFY(ExportService::toMultiHtml(baseline, sections));
        const IExportWriter *w = ExportWriterRegistry::find(QStringLiteral("html"));
        QVERIFY(w != nullptr);
        QVERIFY(w->write(viaWriter, sections, {}));

        QCOMPARE(readAll(viaWriter), readAll(baseline));
    }

    void txtByteIdenticoAoBaseline()
    {
        const auto sections = sampleSections();
        const QString baseline = m_dir.filePath(QStringLiteral("baseline.txt"));
        const QString viaWriter = m_dir.filePath(QStringLiteral("writer.txt"));

        QVERIFY(ExportService::toMultiTxt(baseline, sections));
        const IExportWriter *w = ExportWriterRegistry::find(QStringLiteral("txt"));
        QVERIFY(w != nullptr);
        QVERIFY(w->write(viaWriter, sections, {}));

        QCOMPARE(readAll(viaWriter), readAll(baseline));
    }

    // ── xlsx/pdf: timestamp embutido não permite comparação byte-a-byte ─────
    // determinística; valida sucesso + arquivo gerado não-vazio.

    void xlsxViaWriterEquivaleAoBaseline()
    {
        // toMultiXlsx() embute timestamp (ZIP mtime) quando QXlsx está
        // compilado — não comparável byte-a-byte de forma determinística.
        // Quando NÃO está compilado (PEUBUD_HAS_XLSX=OFF), o fallback grava
        // texto fixo determinístico e o byte-compare volta a fazer sentido.
        // Em ambos os casos: `ok` do writer deve bater com `ok` do baseline
        // (mesma decisão de sucesso/falha) e, se ok, o arquivo não é vazio.
        const auto sections = sampleSections();
        const QString baseline  = m_dir.filePath(QStringLiteral("baseline.xlsx"));
        const QString viaWriter = m_dir.filePath(QStringLiteral("writer.xlsx"));

        const bool okBaseline = ExportService::toMultiXlsx(baseline, sections);
        const IExportWriter *w = ExportWriterRegistry::find(QStringLiteral("xlsx"));
        QVERIFY(w != nullptr);
        const bool okWriter = w->write(viaWriter, sections, {});

        QCOMPARE(okWriter, okBaseline);
        if (!okBaseline) {
            // Fallback sem QXlsx: conteúdo é texto fixo — byte-idêntico.
            QCOMPARE(readAll(viaWriter), readAll(baseline));
        } else {
            QVERIFY(QFileInfo::exists(viaWriter));
            QVERIFY(QFileInfo(viaWriter).size() > 0);
        }
    }

    void pdfViaWriterProduzArquivoValido()
    {
        const auto sections = sampleSections();
        const QString path = m_dir.filePath(QStringLiteral("writer.pdf"));
        const IExportWriter *w = ExportWriterRegistry::find(QStringLiteral("pdf"));
        QVERIFY(w != nullptr);
        QVERIFY(w->write(path, sections, {}));
        QVERIFY(QFileInfo::exists(path));
        QVERIFY(QFileInfo(path).size() > 0);
    }
};

// QTEST_MAIN (não QTEST_GUILESS_MAIN): toMultiPdf() usa QFont/QTextDocument,
// que exigem QGuiApplication (QFontDatabase). Roda offscreen via
// QT_QPA_PLATFORM=offscreen (setado no ambiente do ctest, ver CMakeLists.txt).
QTEST_MAIN(TestExportWriters)
#include "test_export_writers.moc"
