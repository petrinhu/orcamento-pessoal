// PeuBud - teste de REGRESSAO dos triggers de auditoria (#142, fase RED).
//
// BUG (auditoria C-level): DatabaseManager::createSchema() chama
// AuditLogService::ensureSchema(), que cria triggers AFTER INSERT/UPDATE/DELETE
// (audit_*) em 7 tabelas financeiras. Esses triggers referenciam colunas que
// NAO existem no schema real (ex.: NEW.valor / NEW.data / NEW.descricao em
// lancamentos_entradas, cuja coluna real e valor_bruto e cujo periodo e
// dia/mes/ano). SQLite cria o trigger sem validar as colunas, mas FALHA no
// DISPARO -> o INSERT na tabela aborta com "no such column: NEW.<x>".
//
// CONSEQUENCIA: num banco com schema completo (o que todo usuario tem em
// producao), QUALQUER insert nessas 7 tabelas falha. A feature de auditoria
// (#92) inutiliza a escrita das tabelas que ela pretende auditar.
//
// Este teste prova o bug: abre o DatabaseManager num diretorio temporario,
// chama createSchema() (que cria tabelas E triggers audit_*), e -- SEM dropar
// os triggers -- tenta um INSERT minimo valido em cada uma das 7 tabelas.
//
// ESPEC (comportamento correto esperado, fase GREEN futura):
//   1. o INSERT PERSISTE (count da tabela = 1, sem erro SQL);
//   2. o audit_log ganha >= 1 registro correspondente (a auditoria funciona).
//
// FASE RED: espera-se que TODOS os 7 casos FALHEM com "no such column"
// no INSERT. Isso e a confirmacao do bug -- NAO se conserta nada aqui e
// NAO se dropam os triggers (ao contrario de test_resumo.cpp, que os remove
// como workaround porque so LE dados).

#include <QtTest/QtTest>

#include <QByteArray>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>

#include "core/database/DatabaseManager.h"

class TestAuditTriggers : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tmp;       // diretorio de trabalho isolado
    QString       m_prevCwd;   // cwd original (restaurado no fim)
    QByteArray    m_prevHome;
    bool          m_prevHomeSet = false;

    QSqlDatabase db() const { return DatabaseManager::instance().db(); }

    // Conta linhas de uma tabela.
    int contar(const QString &tabela)
    {
        QSqlQuery q(db());
        if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(tabela)))
            return -1;
        if (!q.next()) return -1;
        return q.value(0).toInt();
    }

    // Conta registros de auditoria de uma tabela.
    int contarAudit(const QString &tabela)
    {
        QSqlQuery q(db());
        q.prepare(QStringLiteral(
            "SELECT COUNT(*) FROM audit_log WHERE tabela = :t"));
        q.bindValue(QStringLiteral(":t"), tabela);
        if (!q.exec()) return -1;
        if (!q.next()) return -1;
        return q.value(0).toInt();
    }

    // Executa um INSERT minimo valido e verifica a ESPEC (persistencia +
    // auditoria). Reporta a mensagem de erro SQL exata quando o INSERT falha
    // -- e o que prova o bug do trigger na fase RED.
    void verificaInsert(const QString &tabela, const QString &sqlInsert)
    {
        const int antesTab   = contar(tabela);
        const int antesAudit = contarAudit(tabela);
        QVERIFY2(antesTab == 0, qPrintable(QStringLiteral(
            "pre-condicao: %1 deveria comecar vazia").arg(tabela)));

        QSqlQuery ins(db());
        const bool ok = ins.exec(sqlInsert);

        // ESPEC 1: o INSERT persiste sem erro. Em RED, falha com "no such
        // column: NEW.<x>" disparado pelo trigger audit_<tabela>_ins.
        QVERIFY2(ok, qPrintable(QStringLiteral(
            "INSERT em %1 FALHOU (bug do trigger de auditoria): %2")
            .arg(tabela, ins.lastError().text())));

        // ESPEC 1 (cont.): a linha realmente ficou na tabela.
        QCOMPARE(contar(tabela), antesTab + 1);

        // ESPEC 2: a auditoria (#92) gravou o evento correspondente.
        QVERIFY2(contarAudit(tabela) >= antesAudit + 1, qPrintable(
            QStringLiteral("audit_log nao registrou INSERT em %1").arg(tabela)));
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
            QStringLiteral("test-audit-triggers"));
        QVERIFY2(ok, "DatabaseManager::open falhou");
        QVERIFY(DatabaseManager::instance().isOpen());

        // NAO dropar os triggers audit_*: o objetivo desta suite e exatamente
        // exercita-los. createSchema() (chamado por open()) ja os criou.
    }

    void cleanupTestCase()
    {
        // Nao chamamos close() para evitar criptografia em build release;
        // apenas restaura o cwd. O QTemporaryDir e removido no destrutor.
        if (m_prevHomeSet)
            qputenv("HOME", m_prevHome);
        else
            qunsetenv("HOME");
        if (!m_prevCwd.isEmpty())
            QDir::setCurrent(m_prevCwd);
    }

    void init()
    {
        // Isolamento real: cada caso comeca com as 7 tabelas + audit_log limpas.
        const QStringList tabelas = {
            QStringLiteral("lancamentos_entradas"),
            QStringLiteral("lancamentos_gastos"),
            QStringLiteral("lancamentos_investimentos"),
            QStringLiteral("compras"),
            QStringLiteral("parcelas"),
            QStringLiteral("dividas"),
            QStringLiteral("metas"),
            QStringLiteral("audit_log"),
        };
        for (const QString &t : tabelas) {
            QSqlQuery q(db());
            q.exec(QStringLiteral("DELETE FROM %1").arg(t));
        }
    }

    // ── 1/7: lancamentos_entradas ───────────────────────────────────────────
    // Trigger refere NEW.valor/NEW.data/NEW.descricao; real: valor_bruto + dia/mes/ano.
    void entradaInsertNaoFalhaPorTriggerAudit()
    {
        verificaInsert(QStringLiteral("lancamentos_entradas"), QStringLiteral(
            "INSERT INTO lancamentos_entradas"
            "(mes, ano, dia, fonte, valor_bruto, irpf, inss, obs, criado_em) "
            "VALUES (6, 2025, 1, 'Salario', 500000, 0, 0, '', '2025-06-01')"));
    }

    // ── 2/7: lancamentos_gastos ─────────────────────────────────────────────
    // Trigger refere NEW.categoria_id/NEW.data; real: categoria(TEXT) + dia/mes/ano.
    void gastoInsertNaoFalhaPorTriggerAudit()
    {
        verificaInsert(QStringLiteral("lancamentos_gastos"), QStringLiteral(
            "INSERT INTO lancamentos_gastos"
            "(mes, ano, dia, descricao, categoria, tipo_pgto, valor, obs, criado_em) "
            "VALUES (6, 2025, 1, 'gasto', 'mercado', 'pix', 120000, '', '2025-06-01')"));
    }

    // ── 3/7: lancamentos_investimentos ──────────────────────────────────────
    // Trigger refere NEW.ticker/quantidade/preco_unitario/data; real: nome/valor + dia/mes/ano.
    void investimentoInsertNaoFalhaPorTriggerAudit()
    {
        verificaInsert(QStringLiteral("lancamentos_investimentos"), QStringLiteral(
            "INSERT INTO lancamentos_investimentos"
            "(mes, ano, dia, nome, tipo_ativo, tipo_operacao, valor, obs, criado_em) "
            "VALUES (6, 2025, 1, 'PETR4', 'acao', 'aporte', 100000, '', '2025-06-01')"));
    }

    // ── 4/7: compras ────────────────────────────────────────────────────────
    // Trigger refere NEW.data/NEW.n_parcelas; real: mes_inicio/ano_inicio + num_parcelas.
    void compraInsertNaoFalhaPorTriggerAudit()
    {
        verificaInsert(QStringLiteral("compras"), QStringLiteral(
            "INSERT INTO compras"
            "(nome, valor_total, num_parcelas, parcela_inicial, mes_inicio, "
            " ano_inicio, moeda, criado_em) "
            "VALUES ('Compra', 90000, 1, 1, 6, 2025, 'BRL', '2025-06-01')"));
    }

    // ── 5/7: parcelas ───────────────────────────────────────────────────────
    // Trigger refere NEW.vencimento/NEW.pago; real: data_fatura/modificada.
    void parcelaInsertNaoFalhaPorTriggerAudit()
    {
        // FK compra_id: cria uma compra primeiro (sem assumir que o trigger
        // de compras passou -- inserimos direto e, se a compra falhar, o
        // QVERIFY abaixo expoe a causa).
        QSqlQuery c(db());
        const bool compraOk = c.exec(QStringLiteral(
            "INSERT INTO compras"
            "(nome, valor_total, num_parcelas, parcela_inicial, mes_inicio, "
            " ano_inicio, moeda, criado_em) "
            "VALUES ('Compra', 90000, 1, 1, 6, 2025, 'BRL', '2025-06-01')"));
        QVERIFY2(compraOk, qPrintable(QStringLiteral(
            "setup de parcelas: INSERT em compras falhou: %1")
            .arg(c.lastError().text())));
        const qint64 compraId = c.lastInsertId().toLongLong();

        verificaInsert(QStringLiteral("parcelas"), QStringLiteral(
            "INSERT INTO parcelas"
            "(compra_id, numero, valor, data_fatura, modificada) "
            "VALUES (%1, 1, 90000, '2025-06-01', 0)").arg(compraId));
    }

    // ── 6/7: dividas ────────────────────────────────────────────────────────
    // Trigger refere NEW.parcelas_pagas; real: valor_pago/status (sem parcelas_pagas).
    void dividaInsertNaoFalhaPorTriggerAudit()
    {
        verificaInsert(QStringLiteral("dividas"), QStringLiteral(
            "INSERT INTO dividas"
            "(tipo, contraparte, descricao, valor_total, valor_pago, "
            " status, criado_em) "
            "VALUES ('divida', 'Banco', 'Emprestimo', 500000, 0, "
            " 'em_andamento', '2025-06-01')"));
    }

    // ── 7/7: metas ──────────────────────────────────────────────────────────
    // Trigger refere NEW.valor_alvo/NEW.prazo; real: valor_meta/data_prazo.
    void metaInsertNaoFalhaPorTriggerAudit()
    {
        verificaInsert(QStringLiteral("metas"), QStringLiteral(
            "INSERT INTO metas"
            "(nome, categoria, valor_meta, valor_atual, data_prazo, "
            " status, criado_em) "
            "VALUES ('Reserva', 'reserva', 1000000, 0, '2025-12-31', "
            " 'em_andamento', '2025-06-01')"));
    }
};

QTEST_MAIN(TestAuditTriggers)
#include "test_audit_triggers.moc"
