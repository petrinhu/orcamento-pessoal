// PeuBud - UX-02 (auditoria 2026-06-22, Onda U-5): combo de provider da aba
// Bolsa tinha 3 defeitos:
//   1. Jargao cru — mostrava a chave interna ("frankfurter"/"b3direct"/...)
//      direto pro usuario leigo, em vez de um rotulo amigavel.
//   2. Status falso — o default "frankfurter" nunca e registrado no
//      MarketDataService (so "brapi" e); setActiveProvider("frankfurter")
//      falha e cai em silencio pro provider ja ativo, mas a UI confirmava
//      "Provider ativo: frankfurter" mesmo sem ser verdade.
//   3. Sem estado de erro na busca — se a busca nao respondesse, ficava
//      "Buscando..." pra sempre (sem timeout, sem mensagem de erro).
//
// Fix: MarketProviderCatalog::labelFor (rotulo unico compartilhado),
// MarketDataService::activeProviderName() (honestidade na origem — reflete
// quem esta REALMENTE ativo, nao quem foi pedido), BolsaWidget::
// mensagemStatusProvider/mensagemTimeoutBusca (logica pura extraida da UI,
// testavel sem rede) + timeout de busca com QTimer.

#include <QtTest/QtTest>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>

#include "services/MarketDataService.h"
#include "services/MarketProviderCatalog.h"
#include "services/IMarketDataProvider.h"
#include "ui/widgets/BolsaWidget.h"
#include "core/settings/AppSettings.h"

namespace {

// Provider fake — sem rede. `search()` pode devolver resultado na hora ou
// nunca chamar o callback (simula servidor pendurado, sem resposta) para
// exercitar o timeout sem esperar rede real.
class FakeMarketProvider final : public IMarketDataProvider {
    Q_OBJECT
public:
    explicit FakeMarketProvider(QObject *parent = nullptr) : IMarketDataProvider(parent) {}

    void fetchQuote(const QString &, std::function<void(const CotacaoRT&)> callback) override
    {
        CotacaoRT rt;
        callback(rt);
    }

    void fetchOHLCV(const QString &, const QDate &, const QDate &, const QString &,
                    std::function<void(const QList<PrecoAtivo>&)> callback) override
    {
        QList<PrecoAtivo> list;
        callback(list);
    }

    void search(const QString &, std::function<void(const QStringList&)> callback) override
    {
        if (travarBusca) return; // nunca chama o callback -> simula rede pendurada
        callback(resultados);
    }

    QString name() const override { return QStringLiteral("fake"); }

    bool travarBusca = false;
    QStringList resultados;
};

} // namespace

class TestUx02ProviderBolsa : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        // AppSettings usa QSettings — isola em diretorio temporario por teste
        // para nao ler/escrever preferencia real do usuario.
        m_tmpDir = new QTemporaryDir();
        QVERIFY(m_tmpDir->isValid());
        QSettings::setPath(QSettings::IniFormat,    QSettings::UserScope, m_tmpDir->path());
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_tmpDir->path());
    }

    void cleanup()
    {
        delete m_tmpDir;
        m_tmpDir = nullptr;
    }

    // ── MarketProviderCatalog::labelFor ──────────────────────────────────────

    void labelForRotuloAmigavelDiferenteDaChaveInterna()
    {
        for (const QString &key : MarketProviderCatalog::keys()) {
            const QString label = MarketProviderCatalog::labelFor(key);
            QVERIFY2(!label.isEmpty(), qPrintable(key));
            QVERIFY2(label != key,
                      qPrintable(QStringLiteral("chave '%1' vazou crua no rotulo").arg(key)));
        }
    }

    void labelForChaveDesconhecidaFallbackSeguro()
    {
        QCOMPARE(MarketProviderCatalog::labelFor(QStringLiteral("xyz-nao-existe")),
                 QStringLiteral("xyz-nao-existe"));
    }

    // ── MarketDataService::activeProviderName — honestidade na origem ───────

    void setActiveProviderComNomeInexistenteNaoMudaOAtivoRealNemMente()
    {
        MarketDataService svc;
        auto *brapi = new FakeMarketProvider(&svc);
        svc.registerProvider(QStringLiteral("brapi"), brapi); // 1o registrado -> vira ativo
        QCOMPARE(svc.activeProviderName(), QStringLiteral("brapi"));

        // "frankfurter" nunca foi registrado (bug real do #46/#58: default
        // da AppSettings nao tem provider IMarketDataProvider correspondente).
        const bool ok = svc.setActiveProvider(QStringLiteral("frankfurter"));
        QVERIFY(!ok);

        // honestidade: o ativo real continua sendo quem realmente esta
        // servindo requisicoes, nao o nome que falhou.
        QCOMPARE(svc.activeProviderName(), QStringLiteral("brapi"));
        QCOMPARE(svc.activeProvider(), static_cast<IMarketDataProvider*>(brapi));
    }

    void activeProviderNameVazioQuandoNenhumProviderRegistrado()
    {
        MarketDataService svc;
        QVERIFY(svc.activeProviderName().isEmpty());
        QVERIFY(!svc.setActiveProvider(QStringLiteral("qualquer")));
        QVERIFY(svc.activeProviderName().isEmpty());
    }

    // ── BolsaWidget::mensagemStatusProvider — pura, sem rede/widget ─────────

    void mensagemStatusProviderSucessoMostraOProviderRealComRotuloAmigavel()
    {
        const QString msg = BolsaWidget::mensagemStatusProvider(
            QStringLiteral("brapi"), /*sucesso=*/true, QStringLiteral("brapi"));
        QVERIFY(msg.contains(MarketProviderCatalog::labelFor(QStringLiteral("brapi"))));
    }

    void mensagemStatusProviderFalhaNuncaConfirmaOSolicitadoComoAtivo()
    {
        // O nucleo do bug #2 da auditoria: pedir "frankfurter" e a UI dizer
        // que "frankfurter" esta ativo quando na verdade e "brapi".
        const QString msg = BolsaWidget::mensagemStatusProvider(
            QStringLiteral("frankfurter"), /*sucesso=*/false, QStringLiteral("brapi"));

        const QString labelFrank = MarketProviderCatalog::labelFor(QStringLiteral("frankfurter"));
        const QString labelBrapi = MarketProviderCatalog::labelFor(QStringLiteral("brapi"));

        QVERIFY2(msg.contains(labelBrapi),
                  qPrintable(QStringLiteral("mensagem devia citar o provider REAL ativo: ") + msg));
        // Comportamento antigo (bug): "Provider ativo: frankfurter" mesmo
        // sem frankfurter estar realmente ativo. Nunca mais pode aparecer.
        QVERIFY2(!msg.toLower().contains(QStringLiteral("ativo: %1").arg(labelFrank).toLower()),
                  qPrintable(msg));
    }

    void mensagemStatusProviderSemAtivoAvisaParaConfigurar()
    {
        const QString msg = BolsaWidget::mensagemStatusProvider(
            QStringLiteral("none"), /*sucesso=*/false, QString());
        QVERIFY(!msg.isEmpty());
        QVERIFY(msg.toLower().contains(QStringLiteral("preferências")));
    }

    // ── BolsaWidget::mensagemTimeoutBusca — pura ─────────────────────────────

    void mensagemTimeoutBuscaNaoDizBuscandoENemFicaVazia()
    {
        const QString msg = BolsaWidget::mensagemTimeoutBusca(QStringLiteral("PETR4"));
        QVERIFY(!msg.isEmpty());
        QVERIFY(!msg.toLower().contains(QStringLiteral("buscando")));
        QVERIFY(msg.contains(QStringLiteral("PETR4")));
    }

    // ── Integracao: BolsaWidget real, sem rede (FakeMarketProvider) ─────────

    void integracaoTrocaDeProviderParaChaveNaoRegistradaMostraStatusHonesto()
    {
        MarketDataService svc;
        auto *brapi = new FakeMarketProvider(&svc);
        svc.registerProvider(QStringLiteral("brapi"), brapi);

        BolsaWidget widget(&svc);
        auto *combo = widget.findChild<QComboBox*>();
        QVERIFY(combo);

        const int idxFrankfurter = combo->findData(QStringLiteral("frankfurter"));
        QVERIFY(idxFrankfurter >= 0);
        // Garante que a mudanca de index dispare o sinal dentro do teste (nao
        // so na construcao, caso o default salvo ja seja "frankfurter").
        combo->setCurrentIndex(idxFrankfurter == 0 ? 1 : 0);
        combo->setCurrentIndex(idxFrankfurter);

        // Procura o label que contem o texto de status (nao o placeholder de
        // busca) e confirma que ele cita o provider REAL ativo (brapi), nunca
        // confirmando frankfurter (que falhou) como ativo.
        bool achouStatusHonesto = false;
        for (QLabel *lbl : widget.findChildren<QLabel*>()) {
            if (lbl->text().contains(MarketProviderCatalog::labelFor(QStringLiteral("brapi")))) {
                achouStatusHonesto = true;
                QVERIFY2(!lbl->text().toLower().contains(QStringLiteral("ativo: frankfurter")),
                          qPrintable(lbl->text()));
                break;
            }
        }
        QVERIFY2(achouStatusHonesto,
                  "status apos trocar pra provider inexistente deveria citar o provider REAL (brapi)");
    }

    void integracaoBuscaSemRespostaVirimErroAcionavelNaoBuscandoEterno()
    {
        MarketDataService svc;
        auto *fake = new FakeMarketProvider(&svc);
        fake->travarBusca = true; // nunca chama o callback -> simula timeout
        svc.registerProvider(QStringLiteral("brapi"), fake);

        BolsaWidget widget(&svc);
        widget.setBuscaTimeoutMsParaTeste(80); // curto p/ nao segurar o teste

        auto *edit = widget.findChild<QLineEdit*>();
        auto *btn  = widget.findChild<QPushButton*>();
        QVERIFY(edit && btn);
        edit->setText(QStringLiteral("PETR4"));
        btn->click();

        // Logo apos o clique ainda deve estar "Buscando..." (nao adiantou o
        // timeout artificialmente).
        QLabel *statusLabel = nullptr;
        for (QLabel *lbl : widget.findChildren<QLabel*>()) {
            if (lbl->text().contains(QStringLiteral("Buscando"))) { statusLabel = lbl; break; }
        }
        QVERIFY2(statusLabel, "esperava ver 'Buscando...' logo apos o clique");

        // Espera o timeout curto vencer.
        QTest::qWait(300);

        QVERIFY2(!statusLabel->text().contains(QStringLiteral("Buscando")),
                  qPrintable(QStringLiteral("ainda preso em 'Buscando...' apos o timeout: ") + statusLabel->text()));
        QVERIFY2(statusLabel->text().contains(QStringLiteral("PETR4")),
                  qPrintable(statusLabel->text()));
    }

private:
    QTemporaryDir *m_tmpDir = nullptr;
};

QTEST_MAIN(TestUx02ProviderBolsa)
#include "test_ux02_provider_bolsa.moc"
