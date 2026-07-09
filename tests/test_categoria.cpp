// PeuBud - testes de Categoria (#118).
// Cobre a classificacao fixo/variavel da categoria (TipoDespesa):
//   (a) coluna `fixo` existe apos createSchema() (schema real);
//   (b) a migration ALTER e idempotente (rodar 2x nao quebra nem duplica);
//   (c) round-trip: salvar categoria Fixo, ler de volta, continua Fixo;
//   (d) categoria salva sem setar tipoDespesa le como Variavel (default).
//
// Usa o DatabaseManager real (mesmo padrao de test_audit_triggers): open()
// chama createSchema(), que cria TODAS as tabelas + triggers de auditoria.
// `categorias` NAO esta entre as tabelas auditadas, mas o teste roda com os
// triggers ativos -- prova que INSERT em categorias convive com eles.

#include <QtTest/QtTest>
#include <QByteArray>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>

#include "core/database/DatabaseManager.h"
#include "models/Categoria.h"
#include "repositories/CategoriaRepository.h"

class TestCategoria : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tmp;       // diretorio de trabalho isolado
    QString       m_prevCwd;   // cwd original (restaurado no fim)
    QByteArray    m_prevHome;
    bool          m_prevHomeSet = false;

    QSqlDatabase db() const { return DatabaseManager::instance().db(); }

    // Lista as colunas de `categorias` via PRAGMA table_info.
    QStringList colunasCategorias() const
    {
        QSqlQuery info(db());
        info.exec(QStringLiteral("PRAGMA table_info(categorias)"));
        QStringList cols;
        while (info.next())
            cols.append(info.value(1).toString());
        return cols;
    }

    // Aplica o mesmo ALTER idempotente da migration #118. Retorna true se
    // executou sem erro (ou foi pulado por ja existir = idempotente).
    bool addFixoIfMissing()
    {
        QStringList cols = colunasCategorias();
        if (cols.contains(QStringLiteral("fixo")))
            return true; // ja existe -> idempotente
        QSqlQuery alt(db());
        return alt.exec(QStringLiteral(
            "ALTER TABLE categorias ADD COLUMN fixo INTEGER NOT NULL DEFAULT 0"));
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_tmp.isValid());
        m_prevCwd = QDir::currentPath();
        // DatabaseManager usa caminhos relativos ("data/<slug>.db"); isolar cwd.
        QVERIFY(QDir::setCurrent(m_tmp.path()));
        // (#192) AppSettings::workingDbPath() usa QStandardPaths::AppDataLocation
        // (HOME/XDG_DATA_HOME), nao mais CWD-relativo -- isolar HOME tambem,
        // senao o .db em claro deste teste vaza pro ambiente real do dev.
        m_prevHome    = qgetenv("HOME");
        m_prevHomeSet = qEnvironmentVariableIsSet("HOME");
        qputenv("HOME", m_tmp.path().toUtf8());

        const bool ok = DatabaseManager::instance().open(
            QStringLiteral("test-categoria"));
        QVERIFY2(ok, "DatabaseManager::open falhou");
        QVERIFY(DatabaseManager::instance().isOpen());
    }

    void cleanupTestCase()
    {
        // Nao chamamos close() (evita criptografia em build release);
        // apenas restaura o cwd. O QTemporaryDir e removido no destrutor.
        if (m_prevHomeSet)
            qputenv("HOME", m_prevHome);
        else
            qunsetenv("HOME");
        if (!m_prevCwd.isEmpty())
            QDir::setCurrent(m_prevCwd);
    }

    // (a) A migration #118 ja rodou dentro de createSchema(): a coluna existe.
    void colunaFixoExisteAposSchema()
    {
        const QStringList cols = colunasCategorias();
        QVERIFY2(cols.contains(QStringLiteral("fixo")),
                 qPrintable(QStringLiteral("colunas: %1")
                                .arg(cols.join(QStringLiteral(", ")))));
    }

    // (b) Rodar o ALTER idempotente de novo nao quebra nem duplica a coluna.
    void alterFixoIdempotente()
    {
        QVERIFY(addFixoIfMissing()); // 1a chamada (provavelmente skip)
        QVERIFY(addFixoIfMissing()); // 2a chamada deve continuar ok

        int count = 0;
        for (const QString &c : colunasCategorias())
            if (c == QLatin1String("fixo"))
                ++count;
        QCOMPARE(count, 1);
    }

    // (c) Round-trip: categoria marcada Fixo persiste e le como Fixo.
    void roundTripFixo()
    {
        CategoriaRepository repo;
        Categoria c;
        c.nome        = QStringLiteral("Aluguel");
        c.tipo        = QStringLiteral("gasto");
        c.tipoDespesa = TipoDespesa::Fixo;

        const int id = repo.insert(c);
        QVERIFY2(id > 0, "insert de categoria Fixo falhou");

        const Categoria lida = repo.findById(id);
        QCOMPARE(lida.id, id);
        QCOMPARE(lida.nome, QStringLiteral("Aluguel"));
        QVERIFY(lida.tipoDespesa == TipoDespesa::Fixo);

        // E aparece corretamente no findByTipo tambem.
        const auto gastos = repo.findByTipo(QStringLiteral("gasto"));
        bool achou = false;
        for (const auto &g : gastos)
            if (g.id == id) {
                QVERIFY(g.tipoDespesa == TipoDespesa::Fixo);
                achou = true;
            }
        QVERIFY(achou);
    }

    // (d) Categoria salva sem setar tipoDespesa le como Variavel (default).
    void defaultVariavel()
    {
        CategoriaRepository repo;
        Categoria c;
        c.nome = QStringLiteral("Mercado Avulso");
        c.tipo = QStringLiteral("gasto");
        // tipoDespesa nao setado -> usa default do struct (Variavel).

        const int id = repo.insert(c);
        QVERIFY(id > 0);

        const Categoria lida = repo.findById(id);
        QVERIFY(lida.tipoDespesa == TipoDespesa::Variavel);
    }

    // (d.bis) Linha inserida via SQL cru sem a coluna fixo (banco "antigo"
    // simulado por DEFAULT 0) tambem le como Variavel pelo repo.
    void linhaSqlCruDefaultVariavel()
    {
        QSqlQuery ins(db());
        ins.prepare(QStringLiteral(
            "INSERT INTO categorias (nome, tipo) VALUES (:n, :t)"));
        ins.bindValue(QStringLiteral(":n"), QStringLiteral("Lazer"));
        ins.bindValue(QStringLiteral(":t"), QStringLiteral("gasto"));
        QVERIFY2(ins.exec(), qPrintable(ins.lastError().text()));
        const int id = ins.lastInsertId().toInt();
        QVERIFY(id > 0);

        const Categoria lida = CategoriaRepository().findById(id);
        QVERIFY(lida.tipoDespesa == TipoDespesa::Variavel);
    }
};

QTEST_MAIN(TestCategoria)
#include "test_categoria.moc"
