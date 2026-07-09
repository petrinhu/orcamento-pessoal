// PeuBud - regressao FIN-01 / #146 (Onda U-1): perda de dado silenciosa em
// Divida "sem vencimento".
//
// DividaDialog::divida() usa QDateEdit + setSpecialValueText("Sem vencimento")
// — o ESTADO PADRAO da tela (QDateEdit inicializado em minimumDate()). Quando
// o usuario nao mexe no campo (fluxo mais comum), divida() devolve dataVenc =
// QString() (NULL), nao "". A coluna `dividas.data_venc` e TEXT NOT NULL
// DEFAULT '' — bindValue de um QString() null insere NULL explicito, o que
// viola a constraint NOT NULL. insert() retornava false e o widget
// (DividasWidget) descartava o bool -> a divida "desaparecia" silenciosamente.
//
// Fix: DividaRepository::insert()/update() normalizam QString null -> "" via
// naoNulo() (RepositoryUtils.h) antes de bindar, para TODOS os campos TEXT
// NOT NULL (tipo, contraparte, descricao, data_inicio, data_venc, status, obs).
//
// Setup: DatabaseManager real em QTemporaryDir (mesmo padrao de
// test_categoria/test_meta_repository).

#include <QtTest/QtTest>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "core/database/DatabaseManager.h"
#include "models/Divida.h"
#include "repositories/DividaRepository.h"

class TestDividaRepository : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tmp;
    QString       m_prevCwd;

    QSqlDatabase db() const { return DatabaseManager::instance().db(); }

private slots:
    void initTestCase()
    {
        QVERIFY(m_tmp.isValid());
        m_prevCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(m_tmp.path()));

        const bool ok = DatabaseManager::instance().open(
            QStringLiteral("test-divida-repository"));
        QVERIFY2(ok, "DatabaseManager::open falhou");
        QVERIFY(DatabaseManager::instance().isOpen());
    }

    void cleanupTestCase()
    {
        if (!m_prevCwd.isEmpty())
            QDir::setCurrent(m_prevCwd);
    }

    void init()
    {
        QSqlQuery q(db());
        QVERIFY(q.exec(QStringLiteral("DELETE FROM dividas")));
    }

    // ESPEC RED->GREEN (FIN-01/#146): insert de Divida "sem vencimento" (o
    // estado padrao real do DividaDialog) DEVE persistir. dataVenc fica
    // QString() null -- exatamente o valor que DividaDialog::divida() devolve
    // quando o usuario nao mexe no QDateEdit (fluxo mais comum).
    void insertDividaSemVencimentoPersiste()
    {
        Divida d;
        d.tipo         = QStringLiteral("divida");
        d.contraparte  = QStringLiteral("Banco XYZ");
        d.descricao    = QStringLiteral("Emprestimo pessoal");
        d.valorTotal   = 300000; // R$ 3.000,00
        d.valorPago    = 0;
        d.jurosMensal  = 15000; // 1,5% a.m.
        d.dataInicio   = QDate::currentDate().toString(Qt::ISODate);
        d.status       = QStringLiteral("em_andamento");
        // dataVenc e obs ficam no default de DividaDialog::divida() quando o
        // QDateEdit continua em minimumDate() ("Sem vencimento"): QString() null.
        QVERIFY(d.dataVenc.isNull());
        QVERIFY(d.obs.isNull());

        const bool ok = DividaRepository().insert(d);

        QVERIFY2(ok, "insert deveria persistir a divida mesmo com dataVenc/obs nulos (FIN-01/#146)");
        QVERIFY(d.id > 0);

        const auto lido = DividaRepository().findById(d.id);
        QVERIFY(lido.has_value());
        QCOMPARE(lido->contraparte, QStringLiteral("Banco XYZ"));
        QCOMPARE(lido->valorTotal, qint64(300000));
        QCOMPARE(lido->dataVenc, QString::fromLatin1("")); // null -> "" persistido
        QCOMPARE(lido->obs,      QString::fromLatin1(""));
    }

    // ESPEC: update de Divida ja persistida, voltando o vencimento para "sem
    // vencimento" (dataVenc null de novo), tambem nao pode falhar.
    void updateDividaVoltandoParaSemVencimentoPersiste()
    {
        Divida d;
        d.tipo        = QStringLiteral("emprestimo");
        d.contraparte = QStringLiteral("Fulano de Tal");
        d.descricao   = QStringLiteral("Emprestimo para amigo");
        d.valorTotal  = 50000;
        d.dataInicio  = QDate::currentDate().toString(Qt::ISODate);
        d.dataVenc    = QDate::currentDate().addMonths(3).toString(Qt::ISODate);
        d.status      = QStringLiteral("em_andamento");
        d.obs         = QStringLiteral("vencimento inicial definido");
        QVERIFY(DividaRepository().insert(d));
        QVERIFY(d.id > 0);

        // Usuario reabre a divida e limpa o vencimento -> volta a ser null.
        d.dataVenc = QString();
        d.obs      = QString();

        const bool ok = DividaRepository().update(d);
        QVERIFY2(ok, "update deveria persistir mesmo voltando dataVenc/obs para null");

        const auto lido = DividaRepository().findById(d.id);
        QVERIFY(lido.has_value());
        QCOMPARE(lido->dataVenc, QString::fromLatin1(""));
        QCOMPARE(lido->obs,      QString::fromLatin1(""));
    }

    // ESPEC: insert com TODOS os campos TEXT deixados no default (QString()
    // null) persiste -- cobertura ampla dos campos TEXT NOT NULL de `dividas`.
    void insertComTodosCamposTextNulosPersiste()
    {
        Divida d;
        QVERIFY(d.tipo.isNull());
        QVERIFY(d.contraparte.isNull());
        QVERIFY(d.descricao.isNull());
        QVERIFY(d.dataInicio.isNull());
        QVERIFY(d.dataVenc.isNull());
        QVERIFY(d.status.isNull());
        QVERIFY(d.obs.isNull());

        const bool ok = DividaRepository().insert(d);
        QVERIFY2(ok, "insert deveria persistir com todos os campos TEXT nulos");
        QVERIFY(d.id > 0);
    }
};

QTEST_MAIN(TestDividaRepository)
#include "test_divida_repository.moc"
