// PeuBud - testes do GastoService (#120, Onda 2).
//
// Sob teste: src/services/GastoService.{h,cpp} (camada Mid). Consolida os gastos
// avulsos de um mes/ano em fixos vs variaveis, resolvendo o tipo pela categoria
// do gasto (via categoria_id). Regra de negocio (#118/#119):
//   - tipo do gasto = TipoDespesa da categoria apontada por categoriaId;
//   - categoriaId <= 0 (NULL) -> Variavel (default seguro);
//   - categoriaId orfao (id ausente em categorias) -> Variavel;
//   - SEM heranca pai->filho: a marcacao do pai nao vale para a filha.
//
// Setup: DatabaseManager real em QTemporaryDir (mesmo padrao de test_categoria/
// test_resumo). open() roda createSchema() => todas as tabelas + triggers de
// auditoria (#142) ativos. Cada teste limpa as tabelas em init() para isolamento.
//
// IMPORTANTE: o enum e GLOBAL `TipoDespesa { Variavel=0, Fixo=1 }` (Categoria.h),
// NAO `Categoria::TipoDespesa`.

#include <QtTest/QtTest>
#include <QByteArray>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>

#include "core/database/DatabaseManager.h"
#include "core/AppBootstrap.h"
#include "models/Categoria.h"
#include "models/GastoAvulso.h"
#include "repositories/CategoriaRepository.h"
#include "repositories/GastoAvulsoRepository.h"
#include "services/GastoService.h"

class TestGastoService : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tmp;
    QString       m_prevCwd;
    QByteArray    m_prevHome;
    bool          m_prevHomeSet = false;

    QSqlDatabase db() const { return DatabaseManager::instance().db(); }

    // --- helpers de fixture -------------------------------------------------

    // Cria uma categoria de gasto com o TipoDespesa indicado; devolve o id.
    int criarCategoria(const QString &nome, TipoDespesa tipo, int paiId = 0)
    {
        Categoria c;
        c.nome        = nome;
        c.tipo        = QStringLiteral("gasto");
        c.paiId       = paiId;
        c.tipoDespesa = tipo;
        const int id = CategoriaRepository().insert(c);
        Q_ASSERT(id > 0);
        return id;
    }

    // Insere um gasto avulso via repo (caminho de producao). categoriaId<=0 vira
    // NULL no banco. Devolve o id persistido.
    int criarGasto(qint64 valor, int categoriaId, int mes, int ano,
                   const QString &descricao = QStringLiteral("gasto"))
    {
        GastoAvulso g;
        g.mes         = mes;
        g.ano         = ano;
        g.dia         = 1;
        g.descricao   = descricao;
        // `categoria` (snapshot do nome) e TEXT NOT NULL no schema; QString()
        // default vira NULL e violaria a constraint. Preenchemos com string vazia
        // explicita (irrelevante para o GastoService, que resolve por categoriaId).
        // Colunas TEXT NOT NULL do schema (categoria, obs) precisam de string
        // nao-nula: QString() default vira NULL e violaria a constraint. Sao
        // irrelevantes para o GastoService, que resolve o tipo por categoriaId.
        g.categoria   = QString::fromLatin1("");
        g.obs         = QString::fromLatin1("");
        g.pagador     = QString::fromLatin1("");
        g.categoriaId = categoriaId;
        g.tipoPgto    = QStringLiteral("pix");
        g.valor       = valor;
        const bool ok = GastoAvulsoRepository().insert(g);
        Q_ASSERT(ok);
        return g.id;
    }

    // Insere um gasto com categoria_id apontando para id INEXISTENTE (orfao
    // "puro"). Como ha FK categoria_id REFERENCES categorias(id) e o schema roda
    // com PRAGMA foreign_keys=ON, um insert normal violaria a FK. Desligamos a FK
    // apenas para este insert -- estamos reproduzindo o ESTADO que findByMesAno()
    // entregaria ao GastoService (um gasto cujo categoria_id nao casa com nenhuma
    // categoria) para exercitar o ramo "orfao -> Variavel" de resolverTipo().
    void criarGastoOrfao(qint64 valor, int categoriaIdInexistente, int mes, int ano)
    {
        QSqlQuery off(db());
        QVERIFY(off.exec(QStringLiteral("PRAGMA foreign_keys = OFF")));
        QSqlQuery ins(db());
        ins.prepare(QStringLiteral(
            "INSERT INTO lancamentos_gastos "
            "(mes, ano, dia, descricao, categoria, categoria_id, tipo_pgto, valor, obs, criado_em) "
            "VALUES (:mes, :ano, 1, 'orfao', '', :cid, 'pix', :valor, '', :criado)"));
        ins.bindValue(QStringLiteral(":mes"),    mes);
        ins.bindValue(QStringLiteral(":ano"),    ano);
        ins.bindValue(QStringLiteral(":cid"),    categoriaIdInexistente);
        ins.bindValue(QStringLiteral(":valor"),  valor);
        ins.bindValue(QStringLiteral(":criado"), QStringLiteral("2026-01-01T00:00:00"));
        QVERIFY2(ins.exec(), qPrintable(ins.lastError().text()));
        QSqlQuery on(db());
        QVERIFY(on.exec(QStringLiteral("PRAGMA foreign_keys = ON")));
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_tmp.isValid());
        m_prevCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(m_tmp.path()));
        // (#192) AppSettings::workingDbPath() usa QStandardPaths::AppDataLocation
        // (HOME/XDG_DATA_HOME), nao mais CWD-relativo -- isolar HOME tambem,
        // senao o .db em claro deste teste vaza pro ambiente real do dev.
        m_prevHome    = qgetenv("HOME");
        m_prevHomeSet = qEnvironmentVariableIsSet("HOME");
        qputenv("HOME", m_tmp.path().toUtf8());

        // (#200/ARQ-01) registra os providers de schema Mid/Backend ANTES
        // do open() -- ver AppBootstrap.h.
        AppBootstrap::registerCoreSchemaProviders();

        const bool ok = DatabaseManager::instance().open(
            QStringLiteral("test-gasto-service"));
        QVERIFY2(ok, "DatabaseManager::open falhou");
        QVERIFY(DatabaseManager::instance().isOpen());
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

    // Isolamento: cada teste comeca com tabelas vazias.
    void init()
    {
        QSqlQuery q(db());
        QVERIFY(q.exec(QStringLiteral("DELETE FROM lancamentos_gastos")));
        QVERIFY(q.exec(QStringLiteral("DELETE FROM categorias")));
    }

    // ESPEC 1: gasto numa categoria Fixo conta como fixo.
    void categoriaFixoContaFixo()
    {
        // arrange
        const int catId = criarCategoria(QStringLiteral("Aluguel"), TipoDespesa::Fixo);
        criarGasto(50000, catId, 3, 2026);

        // act
        const ResumoGastos r = GastoService().resumoDoMes(3, 2026);

        // assert
        QCOMPARE(r.totalFixos, qint64(50000));
        QCOMPARE(r.cntFixos, 1);
        QCOMPARE(r.totalVariaveis, qint64(0));
        QCOMPARE(r.cntVariaveis, 0);
    }

    // ESPEC 2: gasto numa categoria Variavel conta como variavel.
    void categoriaVariavelContaVariavel()
    {
        const int catId = criarCategoria(QStringLiteral("Lazer"), TipoDespesa::Variavel);
        criarGasto(30000, catId, 3, 2026);

        const ResumoGastos r = GastoService().resumoDoMes(3, 2026);

        QCOMPARE(r.totalVariaveis, qint64(30000));
        QCOMPARE(r.cntVariaveis, 1);
        QCOMPARE(r.totalFixos, qint64(0));
        QCOMPARE(r.cntFixos, 0);
    }

    // ESPEC 3: categoriaId NULL (<=0) -> Variavel.
    void categoriaNulaContaVariavel()
    {
        criarGasto(12345, /*categoriaId=*/0, 3, 2026); // 0 -> NULL no banco

        const ResumoGastos r = GastoService().resumoDoMes(3, 2026);

        QCOMPARE(r.totalVariaveis, qint64(12345));
        QCOMPARE(r.cntVariaveis, 1);
        QCOMPARE(r.totalFixos, qint64(0));
        QCOMPARE(r.cntFixos, 0);
    }

    // ESPEC 4: categoriaId orfao (id ausente em categorias) -> Variavel.
    void categoriaOrfaContaVariavel()
    {
        criarGastoOrfao(7777, /*categoriaIdInexistente=*/9999, 3, 2026);

        const ResumoGastos r = GastoService().resumoDoMes(3, 2026);

        QCOMPARE(r.totalVariaveis, qint64(7777));
        QCOMPARE(r.cntVariaveis, 1);
        QCOMPARE(r.totalFixos, qint64(0));
        QCOMPARE(r.cntFixos, 0);
    }

    // ESPEC 5: SEM heranca. Categoria-filha Variavel com pai Fixo -> o gasto na
    // filha conta como Variavel; a marcacao Fixo do pai nao se propaga.
    void semHerancaPaiNaoContamina()
    {
        const int paiId   = criarCategoria(QStringLiteral("Casa"), TipoDespesa::Fixo);
        const int filhaId = criarCategoria(QStringLiteral("Casa > Decoracao"),
                                           TipoDespesa::Variavel, paiId);
        criarGasto(20000, filhaId, 3, 2026);

        const ResumoGastos r = GastoService().resumoDoMes(3, 2026);

        QCOMPARE(r.totalVariaveis, qint64(20000));
        QCOMPARE(r.cntVariaveis, 1);
        QCOMPARE(r.totalFixos, qint64(0));
        QCOMPARE(r.cntFixos, 0);
    }

    // ESPEC 6: mix de fixos + variaveis. total == totalFixos+totalVariaveis e
    // contadores corretos.
    void mixSomaETotaisCorretos()
    {
        const int fixoId = criarCategoria(QStringLiteral("Internet"), TipoDespesa::Fixo);
        const int varId  = criarCategoria(QStringLiteral("Restaurante"), TipoDespesa::Variavel);

        criarGasto(10000, fixoId, 4, 2026);
        criarGasto(25000, fixoId, 4, 2026);
        criarGasto(30000, varId, 4, 2026);
        criarGasto(5000, /*categoriaId=*/0, 4, 2026); // sem categoria -> variavel

        const ResumoGastos r = GastoService().resumoDoMes(4, 2026);

        QCOMPARE(r.totalFixos, qint64(35000));
        QCOMPARE(r.cntFixos, 2);
        QCOMPARE(r.totalVariaveis, qint64(35000));
        QCOMPARE(r.cntVariaveis, 2);
        QCOMPARE(r.total, r.totalFixos + r.totalVariaveis);
        QCOMPARE(r.total, qint64(70000));
    }

    // ESPEC 7a: pctFixosBps = fixos*10000/total. fixos=60000, var=40000 -> 6000.
    void pctFixosBpsCalculado()
    {
        const int fixoId = criarCategoria(QStringLiteral("Plano"), TipoDespesa::Fixo);
        const int varId  = criarCategoria(QStringLiteral("Compras"), TipoDespesa::Variavel);
        criarGasto(60000, fixoId, 5, 2026);
        criarGasto(40000, varId, 5, 2026);

        const ResumoGastos r = GastoService().resumoDoMes(5, 2026);

        QCOMPARE(r.total, qint64(100000));
        QCOMPARE(r.pctFixosBps, 6000);
    }

    // ESPEC 7b: total == 0 -> pctFixosBps == 0 (sem divisao por zero).
    void pctFixosBpsZeroSemGastos()
    {
        const ResumoGastos r = GastoService().resumoDoMes(6, 2026);

        QCOMPARE(r.total, qint64(0));
        QCOMPARE(r.pctFixosBps, 0);
    }

    // ESPEC 8: filtro estrito por mes/ano. Gasto de outro mes/ano nao vaza.
    void filtroMesAnoEstrito()
    {
        const int fixoId = criarCategoria(QStringLiteral("Aluguel"), TipoDespesa::Fixo);
        criarGasto(50000, fixoId, 7, 2026);          // alvo
        criarGasto(99999, fixoId, 8, 2026);          // outro mes
        criarGasto(88888, fixoId, 7, 2025);          // outro ano

        const ResumoGastos r = GastoService().resumoDoMes(7, 2026);

        QCOMPARE(r.totalFixos, qint64(50000));
        QCOMPARE(r.cntFixos, 1);
        QCOMPARE(r.total, qint64(50000));
    }

    // ESPEC 9: porTipo retorna exatamente os gastos do tipo pedido no mes.
    void porTipoRetornaApenasDoTipo()
    {
        const int fixoId = criarCategoria(QStringLiteral("Seguro"), TipoDespesa::Fixo);
        const int varId  = criarCategoria(QStringLiteral("Cinema"), TipoDespesa::Variavel);
        const int gFixo  = criarGasto(11000, fixoId, 9, 2026);
        criarGasto(22000, varId, 9, 2026);
        criarGasto(33000, /*categoriaId=*/0, 9, 2026); // variavel (sem cat)

        const QList<GastoAvulso> fixos = GastoService().porTipo(TipoDespesa::Fixo, 9, 2026);
        QCOMPARE(fixos.size(), 1);
        QCOMPARE(fixos.first().id, gFixo);
        QCOMPARE(fixos.first().valor, qint64(11000));

        const QList<GastoAvulso> variaveis = GastoService().porTipo(TipoDespesa::Variavel, 9, 2026);
        QCOMPARE(variaveis.size(), 2);
        qint64 somaVar = 0;
        for (const auto &g : variaveis)
            somaVar += g.valor;
        QCOMPARE(somaVar, qint64(55000));
    }

    // #146: insert de GastoAvulso com campos TEXT NOT NULL deixados no default
    // (QString() null) DEVE persistir. Antes da correcao, o repo bindava NULL
    // explicito em colunas NOT NULL DEFAULT '' (categoria, descricao, tipo_pgto,
    // obs, pagador) e o INSERT falhava silenciosamente (qWarning + return false).
    // O repo agora normaliza null -> "" antes do bind.
    void insertComCamposTextNulosPersiste()
    {
        // arrange: NAO preenche descricao/categoria/tipoPgto/obs/pagador.
        // Permanecem QString() null (o caso que reproduz o bug).
        GastoAvulso g;
        g.mes   = 11;
        g.ano   = 2026;
        g.dia   = 1;
        g.valor = 9900;
        QVERIFY(g.categoria.isNull());   // garante o cenario (null, nao "")
        QVERIFY(g.obs.isNull());
        QVERIFY(g.pagador.isNull());
        QVERIFY(g.descricao.isNull());
        QVERIFY(g.tipoPgto.isNull());

        // act: hoje (sem o fix) retornaria false e id continuaria 0.
        const bool ok = GastoAvulsoRepository().insert(g);

        // assert: persistiu de fato.
        QVERIFY2(ok, "insert deveria persistir mesmo com campos TEXT nulos (#146)");
        QVERIFY(g.id > 0);

        // round-trip: null virou "" no banco (nao NULL).
        const auto lido = GastoAvulsoRepository().findById(g.id);
        QVERIFY(lido.has_value());
        QCOMPARE(lido->valor, qint64(9900));
        QCOMPARE(lido->categoria, QString::fromLatin1(""));
        QCOMPARE(lido->obs,       QString::fromLatin1(""));
        QCOMPARE(lido->pagador,   QString::fromLatin1(""));
        QCOMPARE(lido->descricao, QString::fromLatin1(""));
        QCOMPARE(lido->tipoPgto,  QString::fromLatin1(""));
    }

    // ESPEC 10: totalPorTipo bate com o campo correspondente do resumoDoMes.
    void totalPorTipoConsistenteComResumo()
    {
        const int fixoId = criarCategoria(QStringLiteral("Energia"), TipoDespesa::Fixo);
        const int varId  = criarCategoria(QStringLiteral("Mercado"), TipoDespesa::Variavel);
        criarGasto(15000, fixoId, 10, 2026);
        criarGasto(45000, fixoId, 10, 2026);
        criarGasto(27000, varId, 10, 2026);

        const ResumoGastos r = GastoService().resumoDoMes(10, 2026);
        const qint64 totFixos = GastoService().totalPorTipo(TipoDespesa::Fixo, 10, 2026);
        const qint64 totVar   = GastoService().totalPorTipo(TipoDespesa::Variavel, 10, 2026);

        QCOMPARE(totFixos, r.totalFixos);
        QCOMPARE(totVar, r.totalVariaveis);
        QCOMPARE(totFixos, qint64(60000));
        QCOMPARE(totVar, qint64(27000));
    }
};

QTEST_MAIN(TestGastoService)
#include "test_gasto_service.moc"
