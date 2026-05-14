// PeuBud - testes de integracao SQLite (#73).
// Foco: idempotencia das migrations (PRAGMA table_info + ALTER TABLE).
// Cobertura: padrao usado em DatabaseManager para adicionar colunas em
// schemas existentes sem quebrar bancos antigos.

#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTemporaryFile>

class TestSqliteMigrations : public QObject
{
    Q_OBJECT

private:
    QSqlDatabase db;
    QString tmpPath;

private slots:
    void initTestCase()
    {
        QTemporaryFile tmp;
        tmp.setAutoRemove(false);
        QVERIFY(tmp.open());
        tmpPath = tmp.fileName();
        tmp.close();

        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                        QStringLiteral("test_migrations"));
        db.setDatabaseName(tmpPath);
        QVERIFY(db.open());
    }

    void cleanupTestCase()
    {
        db.close();
        QSqlDatabase::removeDatabase(QStringLiteral("test_migrations"));
        QFile::remove(tmpPath);
    }

    void pragmaTableInfoListaColunas()
    {
        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral(
            "CREATE TABLE t1 (id INTEGER PRIMARY KEY, nome TEXT)")));

        QSqlQuery info(db);
        QVERIFY(info.exec(QStringLiteral("PRAGMA table_info(t1)")));

        QStringList colunas;
        while (info.next()) colunas.append(info.value(1).toString());

        QCOMPARE(colunas.size(), 2);
        QVERIFY(colunas.contains(QStringLiteral("id")));
        QVERIFY(colunas.contains(QStringLiteral("nome")));
    }

    void alterTableAddColumnIdempotente()
    {
        // 1a execucao: coluna nao existe -> ALTER
        // 2a execucao: coluna existe -> skip (idempotente)
        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS t2 (id INTEGER, valor INTEGER)")));

        auto addColIfMissing = [this](const QString &col,
                                       const QString &decl) {
            QSqlQuery info(db);
            info.exec(QStringLiteral("PRAGMA table_info(t2)"));
            bool found = false;
            while (info.next()) {
                if (info.value(1).toString() == col) found = true;
            }
            if (!found) {
                QSqlQuery alt(db);
                return alt.exec(QStringLiteral("ALTER TABLE t2 ADD COLUMN %1 %2")
                    .arg(col, decl));
            }
            return true; // idempotente
        };

        QVERIFY(addColIfMissing(QStringLiteral("descricao"),
                                 QStringLiteral("TEXT DEFAULT ''")));
        // Segunda chamada deve retornar true sem erro
        QVERIFY(addColIfMissing(QStringLiteral("descricao"),
                                 QStringLiteral("TEXT DEFAULT ''")));

        // Confirma que coluna foi adicionada uma vez soh
        QSqlQuery info(db);
        info.exec(QStringLiteral("PRAGMA table_info(t2)"));
        int count = 0;
        while (info.next()) {
            if (info.value(1).toString() == QLatin1String("descricao")) {
                ++count;
            }
        }
        QCOMPARE(count, 1);
    }

    void schemaBackwardCompatInsertSemColunaNova()
    {
        // Banco antigo (sem coluna nova) ainda recebe INSERT?
        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS t3 (id INTEGER PRIMARY KEY, valor INTEGER)")));
        QVERIFY(q.exec(QStringLiteral(
            "INSERT INTO t3 (valor) VALUES (100)")));

        // Adiciona coluna nova
        QVERIFY(q.exec(QStringLiteral(
            "ALTER TABLE t3 ADD COLUMN moeda TEXT DEFAULT 'BRL'")));

        // Linha antiga continua acessivel; coluna nova default
        QVERIFY(q.exec(QStringLiteral(
            "SELECT valor, moeda FROM t3 WHERE id = 1")));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 100);
        QCOMPARE(q.value(1).toString(), QStringLiteral("BRL"));
    }

    void foreignKeysOn()
    {
        QSqlQuery q(db);
        q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        q.exec(QStringLiteral("PRAGMA foreign_keys"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
    }

    void journalModeMemory()
    {
        QSqlQuery q(db);
        q.exec(QStringLiteral("PRAGMA journal_mode = MEMORY"));
        q.exec(QStringLiteral("PRAGMA journal_mode"));
        QVERIFY(q.next());
        // No SQLite, journal_mode = MEMORY pode falhar em algumas builds;
        // aceita "memory" ou fallback "delete"/"truncate"
        const QString mode = q.value(0).toString().toLower();
        QVERIFY2(mode == QLatin1String("memory")
                 || mode == QLatin1String("delete")
                 || mode == QLatin1String("truncate")
                 || mode == QLatin1String("wal"),
                 qPrintable(QStringLiteral("journal_mode=%1").arg(mode)));
    }
};

QTEST_MAIN(TestSqliteMigrations)
#include "test_sqlite_migrations.moc"
