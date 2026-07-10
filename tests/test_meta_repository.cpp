// PeuBud - regressao FIN-01 / #146 (Onda U-1): perda de dado silenciosa em
// Meta "sem prazo".
//
// MetaDialog::meta() usa QDateEdit + setSpecialValueText("Sem prazo") — o
// ESTADO PADRAO da tela (QDateEdit inicializado em minimumDate()). Quando o
// usuario nao mexe no campo (fluxo mais comum), meta() devolve dataPrazo =
// QString() (NULL), nao "". A coluna `metas.data_prazo` e TEXT NOT NULL
// DEFAULT '' — bindValue de um QString() null insere NULL explicito, o que
// viola a constraint NOT NULL (DEFAULT so vale quando a coluna e OMITIDA do
// INSERT, nunca quando ela recebe NULL). insert() retornava false e o widget
// (MetasWidget) descartava o bool -> a meta "desaparecia" silenciosamente.
//
// Fix: MetaRepository::insert()/update() normalizam QString null -> "" via
// naoNulo() (RepositoryUtils.h) antes de bindar, para TODOS os campos TEXT
// NOT NULL (nome, categoria, data_inicio, data_prazo, status, obs).
//
// Setup: DatabaseManager real em QTemporaryDir (mesmo padrao de
// test_categoria/test_gasto_service). open() roda createSchema() => schema
// completo + triggers de auditoria ativos.

#include <QtTest/QtTest>
#include <QByteArray>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "core/database/DatabaseManager.h"
#include "core/AppBootstrap.h"
#include "models/Meta.h"
#include "repositories/MetaRepository.h"

class TestMetaRepository : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tmp;
    QString       m_prevCwd;
    QByteArray    m_prevHome;
    bool          m_prevHomeSet = false;

    QSqlDatabase db() const { return DatabaseManager::instance().db(); }

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
            QStringLiteral("test-meta-repository"));
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

    // Isolamento: cada teste comeca com a tabela vazia.
    void init()
    {
        QSqlQuery q(db());
        QVERIFY(q.exec(QStringLiteral("DELETE FROM metas")));
    }

    // ESPEC RED->GREEN (FIN-01/#146): insert de Meta "sem prazo" (o estado
    // padrao real do MetaDialog) DEVE persistir. dataPrazo fica QString()
    // null -- exatamente o valor que MetaDialog::meta() devolve quando o
    // usuario nao mexe no QDateEdit (fluxo mais comum).
    void insertMetaSemPrazoPersiste()
    {
        Meta m;
        m.nome        = QStringLiteral("Viagem para a praia");
        m.categoria   = QStringLiteral("viagem");
        m.valorMeta   = 500000; // R$ 5.000,00
        m.valorAtual  = 0;
        m.dataInicio  = QDate::currentDate().toString(Qt::ISODate);
        m.status      = QStringLiteral("em_andamento");
        // dataPrazo e obs ficam no default de MetaDialog::meta() quando o
        // QDateEdit continua em minimumDate() ("Sem prazo"): QString() null.
        QVERIFY(m.dataPrazo.isNull());
        QVERIFY(m.obs.isNull());

        const bool ok = MetaRepository().insert(m);

        QVERIFY2(ok, "insert deveria persistir a meta mesmo com dataPrazo/obs nulos (FIN-01/#146)");
        QVERIFY(m.id > 0);

        const auto lido = MetaRepository().findById(m.id);
        QVERIFY(lido.has_value());
        QCOMPARE(lido->nome, QStringLiteral("Viagem para a praia"));
        QCOMPARE(lido->valorMeta, qint64(500000));
        QCOMPARE(lido->dataPrazo, QString::fromLatin1("")); // null -> "" persistido
        QCOMPARE(lido->obs,       QString::fromLatin1(""));
    }

    // ESPEC: update de Meta ja persistida, voltando o prazo para "sem prazo"
    // (dataPrazo null de novo), tambem nao pode falhar.
    void updateMetaVoltandoParaSemPrazoPersiste()
    {
        Meta m;
        m.nome       = QStringLiteral("Reserva de emergencia");
        m.categoria  = QStringLiteral("emergencia");
        m.valorMeta  = 1000000;
        m.dataInicio = QDate::currentDate().toString(Qt::ISODate);
        m.dataPrazo  = QDate::currentDate().addMonths(6).toString(Qt::ISODate);
        m.status     = QStringLiteral("em_andamento");
        m.obs        = QStringLiteral("prazo inicial definido");
        QVERIFY(MetaRepository().insert(m));
        QVERIFY(m.id > 0);

        // Usuario reabre a meta e limpa o prazo -> volta a ser null.
        m.dataPrazo = QString();
        m.obs       = QString();

        const bool ok = MetaRepository().update(m);
        QVERIFY2(ok, "update deveria persistir mesmo voltando dataPrazo/obs para null");

        const auto lido = MetaRepository().findById(m.id);
        QVERIFY(lido.has_value());
        QCOMPARE(lido->dataPrazo, QString::fromLatin1(""));
        QCOMPARE(lido->obs,       QString::fromLatin1(""));
    }

    // ESPEC: insert com TODOS os campos TEXT deixados no default (QString()
    // null) persiste -- cobertura ampla dos campos TEXT NOT NULL de `metas`.
    void insertComTodosCamposTextNulosPersiste()
    {
        Meta m;
        // nome vazio e permitido pelo schema (so NOT NULL, sem CHECK de
        // tamanho) -- widgets bloqueiam nome vazio na UI, nao no repo.
        QVERIFY(m.nome.isNull());
        QVERIFY(m.categoria.isNull());
        QVERIFY(m.dataInicio.isNull());
        QVERIFY(m.dataPrazo.isNull());
        QVERIFY(m.status.isNull());
        QVERIFY(m.obs.isNull());

        const bool ok = MetaRepository().insert(m);
        QVERIFY2(ok, "insert deveria persistir com todos os campos TEXT nulos");
        QVERIFY(m.id > 0);
    }
};

QTEST_MAIN(TestMetaRepository)
#include "test_meta_repository.moc"
