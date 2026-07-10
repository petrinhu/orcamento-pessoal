// PeuBud - PERF-02 (Onda U-5, #208): budget de startup das abas da MainWindow.
//
// Antes: MainWindow construía as 13 abas (widget + TabShell + wiring
// cross-widget) eager no boot, mesmo as que o usuário nunca abre (~285ms +
// query storm do PERF-03). Fix: LazyPageRouter — cada chave tem uma fábrica
// que só roda na 1ª navegação (garantir()); chamadas seguintes reusam o
// widget cacheado sem invocar a fábrica de novo.
//
// Este teste exercita o mecanismo em isolamento (sem arrastar a árvore
// inteira de widgets financeiros do app — só Qt::Core), com fábricas falsas
// que incrementam um contador. É a prova de "budget de startup": só a chave
// navegada é materializada; as demais nunca chamam a fábrica.
//
// Executar com QT_QPA_PLATFORM=offscreen (QWidget precisa de QApplication).

#include <QtTest>
#include <QApplication>
#include <QWidget>

#include "ui/LazyPageRouter.h"

class TestLazyPageRouter : public QObject
{
    Q_OBJECT
private slots:
    void nenhumaFabricaRodaAntesDeGarantir();
    void garantirSoConstroiAChaveNavegada();
    void garantirEIdempotenteNaoReconstroi();
    void chaveDesconhecidaRetornaNuloSemCrash();
    void paginaExistenteNaoDisparaConstrucao();
    void construidaReflete1aChamadaDeGarantir();
};

// ── nenhuma fábrica roda antes de garantir() ─────────────────────────────

void TestLazyPageRouter::nenhumaFabricaRodaAntesDeGarantir()
{
    LazyPageRouter router;
    int chamadasA = 0, chamadasB = 0, chamadasC = 0;

    router.registrarFabrica(QStringLiteral("a"), [&]() -> QWidget * {
        ++chamadasA; return new QWidget();
    });
    router.registrarFabrica(QStringLiteral("b"), [&]() -> QWidget * {
        ++chamadasB; return new QWidget();
    });
    router.registrarFabrica(QStringLiteral("c"), [&]() -> QWidget * {
        ++chamadasC; return new QWidget();
    });

    QCOMPARE(chamadasA, 0);
    QCOMPARE(chamadasB, 0);
    QCOMPARE(chamadasC, 0);
    QVERIFY2(!router.construida(QStringLiteral("a")),
        "nenhuma chave deveria estar construída antes do 1o garantir()");
}

// ── garantir() só constrói a chave navegada (budget de startup) ─────────

void TestLazyPageRouter::garantirSoConstroiAChaveNavegada()
{
    LazyPageRouter router;
    int chamadasA = 0, chamadasB = 0, chamadasC = 0;

    router.registrarFabrica(QStringLiteral("a"), [&]() -> QWidget * {
        ++chamadasA; return new QWidget();
    });
    router.registrarFabrica(QStringLiteral("b"), [&]() -> QWidget * {
        ++chamadasB; return new QWidget();
    });
    router.registrarFabrica(QStringLiteral("c"), [&]() -> QWidget * {
        ++chamadasC; return new QWidget();
    });

    QWidget *pagA = router.garantir(QStringLiteral("a"));

    QVERIFY2(pagA, "garantir() da chave registrada nao deveria retornar nullptr");
    QCOMPARE(chamadasA, 1);
    QCOMPARE(chamadasB, 0);
    QCOMPARE(chamadasC, 0);
    QVERIFY(router.construida(QStringLiteral("a")));
    QVERIFY2(!router.construida(QStringLiteral("b")),
        "aba nao navegada nao pode ser instanciada (#208 PERF-02)");
    QVERIFY2(!router.construida(QStringLiteral("c")),
        "aba nao navegada nao pode ser instanciada (#208 PERF-02)");

    delete pagA;
}

// ── garantir() é idempotente: 2a chamada não reconstrói ──────────────────

void TestLazyPageRouter::garantirEIdempotenteNaoReconstroi()
{
    LazyPageRouter router;
    int chamadas = 0;
    router.registrarFabrica(QStringLiteral("a"), [&]() -> QWidget * {
        ++chamadas; return new QWidget();
    });

    QWidget *primeira  = router.garantir(QStringLiteral("a"));
    QWidget *segunda   = router.garantir(QStringLiteral("a"));
    QWidget *terceira  = router.garantir(QStringLiteral("a"));

    QCOMPARE(chamadas, 1);
    QCOMPARE(primeira, segunda);
    QCOMPARE(segunda, terceira);

    delete primeira;
}

// ── chave desconhecida: sem crash, nullptr ────────────────────────────────

void TestLazyPageRouter::chaveDesconhecidaRetornaNuloSemCrash()
{
    LazyPageRouter router;
    router.registrarFabrica(QStringLiteral("a"), []() -> QWidget * {
        return new QWidget();
    });

    QVERIFY(router.garantir(QStringLiteral("inexistente")) == nullptr);
    QVERIFY(!router.registrada(QStringLiteral("inexistente")));
}

// ── paginaExistente() não dispara construção (uso em wiring cross-widget) ─

void TestLazyPageRouter::paginaExistenteNaoDisparaConstrucao()
{
    LazyPageRouter router;
    int chamadas = 0;
    router.registrarFabrica(QStringLiteral("a"), [&]() -> QWidget * {
        ++chamadas; return new QWidget();
    });

    QVERIFY2(router.paginaExistente(QStringLiteral("a")) == nullptr,
        "paginaExistente() antes de garantir() deve ser nullptr");
    QCOMPARE(chamadas, 0);

    QWidget *pag = router.garantir(QStringLiteral("a"));
    QCOMPARE(router.paginaExistente(QStringLiteral("a")), pag);
    QCOMPARE(chamadas, 1);

    delete pag;
}

// ── construida() reflete exatamente a 1a chamada de garantir() ──────────

void TestLazyPageRouter::construidaReflete1aChamadaDeGarantir()
{
    LazyPageRouter router;
    router.registrarFabrica(QStringLiteral("a"), []() -> QWidget * {
        return new QWidget();
    });

    QVERIFY(!router.construida(QStringLiteral("a")));
    QWidget *pag = router.garantir(QStringLiteral("a"));
    QVERIFY(router.construida(QStringLiteral("a")));
    delete pag;
}

QTEST_MAIN(TestLazyPageRouter)
#include "test_lazy_page_router.moc"
