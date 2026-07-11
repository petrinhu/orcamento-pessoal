// PeuBud - suite de testes do BuscaService (#139, Onda 7 — arquitetura).
//
// Casos golden: busca por termo em cada uma das tabelas cobertas pela busca
// global (lancamentos_entradas, lancamentos_gastos, lancamentos_investimentos,
// dividas, compras+cartoes) deve retornar os mesmos registros que a query SQL
// crua que vivia antes em BuscaGlobalDialog::popularResultados. Prova que a
// extração para a camada Mid (#139) NÃO mudou comportamento.
//
// Setup de DB: mesmo padrão de tests/test_resumo.cpp — DatabaseManager num
// diretório temporário isolado (HOME + cwd trocados em initTestCase),
// AppBootstrap::registerCoreSchemaProviders() antes do open(), triggers de
// auditoria removidos (bug de produto conhecido — INSERT nas tabelas
// auditadas falha com "no such column"; fora de escopo desta suite).
// Inserção via API pública (Services/Repositories), nunca INSERT cru.

#include <QtTest/QtTest>

#include <QDir>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "core/AppBootstrap.h"
#include "core/database/DatabaseManager.h"
#include "models/Cartao.h"
#include "models/Compra.h"
#include "models/Divida.h"
#include "models/GastoAvulso.h"
#include "models/LancamentoEntrada.h"
#include "models/LancamentoInvestimento.h"
#include "models/Parcela.h"
#include "repositories/CartaoRepository.h"
#include "repositories/DividaRepository.h"
#include "repositories/GastoAvulsoRepository.h"
#include "repositories/LancamentoInvestimentoRepository.h"
#include "services/BuscaService.h"
#include "services/CompraService.h"
#include "services/EntradaService.h"

class TestBuscaService : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tmp;
    QString       m_prevCwd;
    QByteArray    m_prevHome;
    bool          m_prevHomeSet = false;

    void dropTriggersAudit()
    {
        QSqlDatabase db = DatabaseManager::instance().db();
        QSqlQuery list(db);
        list.exec(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE type='trigger' "
            "AND name LIKE 'audit_%'"));
        QStringList nomes;
        while (list.next()) nomes << list.value(0).toString();
        for (const QString &n : nomes) {
            QSqlQuery d(db);
            d.exec(QStringLiteral("DROP TRIGGER IF EXISTS %1").arg(n));
        }
    }

    void limparTabelas()
    {
        QSqlDatabase db = DatabaseManager::instance().db();
        const QStringList tabelas = {
            QStringLiteral("lancamentos_entradas"),
            QStringLiteral("lancamentos_gastos"),
            QStringLiteral("lancamentos_investimentos"),
            QStringLiteral("dividas"),
            QStringLiteral("parcelas"),
            QStringLiteral("compras"),
            QStringLiteral("cartoes"),
            QStringLiteral("fontes_renda"),
        };
        for (const QString &t : tabelas) {
            QSqlQuery q(db);
            q.exec(QStringLiteral("DELETE FROM %1").arg(t));
        }
    }

    // ── Helpers de inserção (via API pública de produção) ────────────────────

    void addEntrada(int mes, int ano, int dia, const QString &fonte,
                    qint64 bruto, qint64 irpf, qint64 inss,
                    const QString &obs = QString::fromLatin1(""))
    {
        LancamentoEntrada l;
        l.mes        = mes;
        l.ano        = ano;
        l.dia        = dia;
        l.fonte      = fonte;
        l.categoria  = QString::fromLatin1("");
        l.valorBruto = bruto;
        l.irpf       = irpf;
        l.inss       = inss;
        l.obs        = obs;
        EntradaService svc;
        QVERIFY2(svc.salvarComFonte(l), "falha ao inserir LancamentoEntrada");
    }

    void addGasto(int mes, int ano, int dia, const QString &descricao,
                 const QString &categoria, qint64 valor,
                 const QString &obs = QString::fromLatin1(""))
    {
        GastoAvulso g;
        g.mes       = mes;
        g.ano       = ano;
        g.dia       = dia;
        g.descricao = descricao;
        g.categoria = categoria;
        g.tipoPgto  = QStringLiteral("pix");
        g.valor     = valor;
        g.obs       = obs;
        g.pagador   = QString::fromLatin1("");
        GastoAvulsoRepository repo;
        QVERIFY2(repo.insert(g), "falha ao inserir GastoAvulso");
    }

    void addInvestimento(int mes, int ano, int dia, const QString &nome,
                         const QString &tipoAtivo, const QString &op,
                         qint64 valor, const QString &obs = QString::fromLatin1(""))
    {
        LancamentoInvestimento inv;
        inv.mes          = mes;
        inv.ano          = ano;
        inv.dia          = dia;
        inv.nome         = nome;
        inv.tipoAtivo    = tipoAtivo;
        inv.tipoOperacao = op;
        inv.valor        = valor;
        inv.categoria    = QString::fromLatin1("");
        inv.obs          = obs;
        inv.ticker       = QString::fromLatin1("");
        LancamentoInvestimentoRepository repo;
        QVERIFY2(repo.insert(inv), "falha ao inserir LancamentoInvestimento");
    }

    void addDivida(const QString &contraparte, const QString &descricao,
                   qint64 valorTotal, const QString &dataInicio,
                   const QString &obs = QString::fromLatin1(""))
    {
        Divida d;
        d.tipo        = QStringLiteral("divida");
        d.contraparte = contraparte;
        d.descricao   = descricao;
        d.valorTotal  = valorTotal;
        d.dataInicio  = dataInicio;
        d.status      = QStringLiteral("em_andamento");
        d.obs         = obs;
        DividaRepository repo;
        QVERIFY2(repo.insert(d), "falha ao inserir Divida");
    }

    int addCartao(const QString &apelido)
    {
        Cartao c;
        c.apelido = apelido;
        CartaoRepository repo;
        const bool ok = repo.insert(c);
        Q_ASSERT(ok);
        Q_UNUSED(ok);
        return c.id;
    }

    void addCompra(const QString &nome, qint64 valorTotal, int cartaoId = 0)
    {
        Compra c;
        c.nome           = nome;
        c.valorTotal     = valorTotal;
        c.numParcelas    = 1;
        c.parcelaInicial = 1;
        c.mesInicio      = 6;
        c.anoInicio      = 2025;
        c.cartaoId       = cartaoId;

        QList<Parcela> parcelas;
        Parcela p;
        p.numero = 1;
        p.valor  = valorTotal;
        parcelas.append(p);

        CompraService svc;
        const bool ok = svc.inserir(c, parcelas);
        Q_ASSERT(ok);
        Q_UNUSED(ok);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_tmp.isValid());
        m_prevCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(m_tmp.path()));
        m_prevHome    = qgetenv("HOME");
        m_prevHomeSet = qEnvironmentVariableIsSet("HOME");
        qputenv("HOME", m_tmp.path().toUtf8());

        AppBootstrap::registerCoreSchemaProviders();

        const bool ok = DatabaseManager::instance().open(
            QStringLiteral("test-busca-service"));
        QVERIFY2(ok, "DatabaseManager::open falhou");
        QVERIFY(DatabaseManager::instance().isOpen());

        dropTriggersAudit();
    }

    void cleanupTestCase()
    {
        if (m_prevHomeSet)
            qputenv("HOME", m_prevHome);
        else
            qunsetenv("HOME");
        if (!m_prevCwd.isEmpty())
            QDir::setCurrent(m_prevCwd);
    }

    void init()
    {
        limparTabelas();
    }

    // ── Golden: entradas ──────────────────────────────────────────────────

    void buscaPorTermoEmEntradas()
    {
        addEntrada(6, 2025, 5, QStringLiteral("Salario Junho"), 500000, 50000, 30000);
        addEntrada(6, 2025, 10, QStringLiteral("Freela"), 100000, 0, 0);

        const auto r = BuscaService::buscar(QStringLiteral("Salario"),
                                            QStringLiteral("tudo"),
                                            QStringLiteral("todos"));

        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].categoria, QString::fromLatin1(BuscaService::kCategoriaEntrada));
        QCOMPARE(r[0].descricao, QStringLiteral("Salario Junho"));
        QCOMPARE(r[0].data, QStringLiteral("2025-06-05"));
        QCOMPARE(r[0].valor, qint64(500000 - 50000 - 30000)); // liquido
        QCOMPARE(r[0].tabIndex, BuscaService::kTabEntradas);
    }

    // ── Golden: gastos ────────────────────────────────────────────────────

    void buscaPorTermoEmGastos()
    {
        addGasto(6, 2025, 3, QStringLiteral("Supermercado Extra"),
                QStringLiteral("mercado"), 25000);
        addGasto(6, 2025, 8, QStringLiteral("Farmacia"),
                QStringLiteral("saude"), 8000);

        const auto r = BuscaService::buscar(QStringLiteral("Extra"),
                                            QStringLiteral("tudo"),
                                            QStringLiteral("todos"));

        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].categoria, QString::fromLatin1(BuscaService::kCategoriaGasto));
        QCOMPARE(r[0].descricao, QStringLiteral("Supermercado Extra"));
        QCOMPARE(r[0].valor, qint64(-25000)); // gasto = negativo
        QCOMPARE(r[0].categoriaObs, QStringLiteral("mercado"));
        QCOMPARE(r[0].tabIndex, BuscaService::kTabGastos);
    }

    // ── Golden: investimentos ─────────────────────────────────────────────

    void buscaPorTermoEmInvestimentos()
    {
        addInvestimento(6, 2025, 12, QStringLiteral("PETR4"),
                        QStringLiteral("acao"), QStringLiteral("aporte"), 300000);
        addInvestimento(6, 2025, 15, QStringLiteral("VALE3"),
                        QStringLiteral("acao"), QStringLiteral("aporte"), 150000);

        const auto r = BuscaService::buscar(QStringLiteral("PETR4"),
                                            QStringLiteral("tudo"),
                                            QStringLiteral("todos"));

        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].categoria, QString::fromLatin1(BuscaService::kCategoriaInvestimento));
        QCOMPARE(r[0].descricao, QStringLiteral("PETR4"));
        QCOMPARE(r[0].valor, qint64(300000));
        QCOMPARE(r[0].tabIndex, BuscaService::kTabInvestimentos);
    }

    // ── Golden: dívidas ───────────────────────────────────────────────────

    void buscaPorTermoEmDividas()
    {
        addDivida(QStringLiteral("Banco XYZ"), QStringLiteral("Emprestimo pessoal"),
                  1000000, QStringLiteral("2025-01-10"));
        addDivida(QStringLiteral("Joao"), QStringLiteral("Emprestimo amigo"),
                  50000, QStringLiteral("2025-02-01"));

        const auto r = BuscaService::buscar(QStringLiteral("XYZ"),
                                            QStringLiteral("tudo"),
                                            QStringLiteral("todos"));

        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].categoria, QString::fromLatin1(BuscaService::kCategoriaDivida));
        QVERIFY(r[0].descricao.contains(QStringLiteral("Banco XYZ")));
        QCOMPARE(r[0].data, QStringLiteral("2025-01-10"));
        QCOMPARE(r[0].valor, qint64(1000000));
        QCOMPARE(r[0].tabIndex, BuscaService::kTabDividas);
    }

    // ── Golden: cartões (compras, via nome da compra) ─────────────────────

    void buscaPorTermoEmCartoesViaNomeCompra()
    {
        const int cartaoId = addCartao(QStringLiteral("Nubank"));
        addCompra(QStringLiteral("Notebook Dell"), 450000, cartaoId);
        addCompra(QStringLiteral("Mouse sem fio"), 8000, cartaoId);

        const auto r = BuscaService::buscar(QStringLiteral("Notebook"),
                                            QStringLiteral("tudo"),
                                            QStringLiteral("todos"));

        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].categoria, QString::fromLatin1(BuscaService::kCategoriaCartao));
        QCOMPARE(r[0].descricao, QStringLiteral("Notebook Dell"));
        QCOMPARE(r[0].valor, qint64(450000));
        QCOMPARE(r[0].categoriaObs, QStringLiteral("Nubank"));
        QCOMPARE(r[0].tabIndex, BuscaService::kTabCartoes);
    }

    // ── Golden: cartões, via apelido do cartão (LEFT JOIN) ─────────────────

    void buscaPorTermoEmCartoesViaApelido()
    {
        const int cartaoId = addCartao(QStringLiteral("Inter Gold"));
        addCompra(QStringLiteral("Passagem aerea"), 120000, cartaoId);
        addCompra(QStringLiteral("Sem cartao"), 5000, 0); // cartaoId=0 -> LEFT JOIN nulo

        const auto r = BuscaService::buscar(QStringLiteral("Inter"),
                                            QStringLiteral("tudo"),
                                            QStringLiteral("todos"));

        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].descricao, QStringLiteral("Passagem aerea"));
        QCOMPARE(r[0].categoriaObs, QStringLiteral("Inter Gold"));
    }

    // ── Filtro por tipo ───────────────────────────────────────────────────

    void filtroTipoRestringeCategorias()
    {
        addEntrada(6, 2025, 1, QStringLiteral("Bonus"), 200000, 0, 0);
        addGasto(6, 2025, 1, QStringLiteral("Bonus gasto"), QStringLiteral(""), 10000);

        const auto soEntradas = BuscaService::buscar(QStringLiteral("Bonus"),
                                                      QStringLiteral("tudo"),
                                                      QStringLiteral("entradas"));
        QCOMPARE(soEntradas.size(), 1);
        QCOMPARE(soEntradas[0].categoria, QString::fromLatin1(BuscaService::kCategoriaEntrada));

        const auto soGastos = BuscaService::buscar(QStringLiteral("Bonus"),
                                                    QStringLiteral("tudo"),
                                                    QStringLiteral("gastos"));
        QCOMPARE(soGastos.size(), 1);
        QCOMPARE(soGastos[0].categoria, QString::fromLatin1(BuscaService::kCategoriaGasto));

        const auto todos = BuscaService::buscar(QStringLiteral("Bonus"),
                                                 QStringLiteral("tudo"),
                                                 QStringLiteral("todos"));
        QCOMPARE(todos.size(), 2);
    }

    // ── Filtro por período ────────────────────────────────────────────────

    void filtroPeriodoMesExcluiLancamentoAntigo()
    {
        const QDate hoje = QDate::currentDate();
        addEntrada(hoje.month(), hoje.year(), 1,
                  QStringLiteral("Recente Alvo"), 100000, 0, 0);
        // Lançamento de 2 anos atrás cai fora de "mes" e "3meses"/"6meses"/"ano".
        const QDate antiga = hoje.addYears(-2);
        addEntrada(antiga.month(), antiga.year(), 1,
                  QStringLiteral("Antigo Alvo"), 100000, 0, 0);

        const auto rMes = BuscaService::buscar(QStringLiteral("Alvo"),
                                               QStringLiteral("mes"),
                                               QStringLiteral("todos"));
        QCOMPARE(rMes.size(), 1);
        QCOMPARE(rMes[0].descricao, QStringLiteral("Recente Alvo"));

        const auto rTudo = BuscaService::buscar(QStringLiteral("Alvo"),
                                                QStringLiteral("tudo"),
                                                QStringLiteral("todos"));
        QCOMPARE(rTudo.size(), 2);
    }

    // ── Sem resultado ─────────────────────────────────────────────────────

    void termoSemCorrespondenciaRetornaVazio()
    {
        addEntrada(6, 2025, 1, QStringLiteral("Salario"), 100000, 0, 0);

        const auto r = BuscaService::buscar(QStringLiteral("XXNaoExiste"),
                                            QStringLiteral("tudo"),
                                            QStringLiteral("todos"));
        QVERIFY(r.isEmpty());
    }
};

QTEST_MAIN(TestBuscaService)
#include "test_busca_service.moc"
