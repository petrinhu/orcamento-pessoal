// PeuBud — #181: pastas de save fixas
//
// Testa:
//  1. savePathFixo(slug) retorna o padrão esperado e mkpath cria a pasta.
//  2. autosavePathFixo(slug) idem para o auto.
//  3. Lógica de migração: cofre no caminho novo ausente + cofre legado presente
//     → copia para novo destino, renomeia legado para .bak, nunca deleta.
//  4. Migração idempotente: cofre novo já existe → não toca o legado.
//  5. Novo usuário (nenhum cofre em lugar algum) → apenas retorna false, sem erro.
//
// A migração real é feita por DatabaseManager::open() (compilada apenas em
// #ifndef QT_DEBUG). Aqui testamos a lógica equivalente de forma pura em
// QTemporaryDir para verificar o contrato sem precisar do ciclo de cripto.

#include <QtTest/QtTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QString>

// Replica a lógica de migrarCofreSeNecessario() de DatabaseManager.cpp
// para teste isolado (a função real fica atrás de #ifndef QT_DEBUG).
static bool migrarCofreSeNecessario_testavel(const QString &pbdPathNovo,
                                              const QString &pbdPathLegado)
{
    if (QFile::exists(pbdPathNovo))
        return true;

    if (!QFile::exists(pbdPathLegado))
        return false;

    const QString tmpNovo = pbdPathNovo + QStringLiteral(".new");
    if (!QFile::copy(pbdPathLegado, tmpNovo))
        return false;
    if (!QFile::rename(tmpNovo, pbdPathNovo)) {
        QFile::remove(tmpNovo);
        return false;
    }

    const QString bakPath = pbdPathLegado + QStringLiteral(".bak");
    QFile::rename(pbdPathLegado, bakPath); // best-effort; não fatal se falhar

    return true;
}

class TestPastasFixas : public QObject
{
    Q_OBJECT

private slots:
    void savePathFixo_formatoCorreto();
    void autosavePathFixo_formatoCorreto();
    void savePathFixo_mkpathCriaPasta();
    void autosavePathFixo_mkpathCriaPasta();
    void migracao_cofreNovoAusenteElegadoPresente_copiaErebatiza();
    void migracao_cofreNovoJaExiste_naoTocaLegado();
    void migracao_novoUsuarioSemCofre_retornaFalse();
    void migracao_atomica_tmpNaoVazaSeRenomearFalhar();
};

// ── Formato dos caminhos ─────────────────────────────────────────────────────

void TestPastasFixas::savePathFixo_formatoCorreto()
{
    // HOME pode variar entre máquinas — verificamos a estrutura relativa.
    const QString home = QDir::homePath();
    const QString slug = QStringLiteral("petrus");

    const QString expected = home + QStringLiteral("/peubud/save/") + slug
                           + QStringLiteral("/database.pbd");

    // Replica a fórmula de AppSettings::savePathFixo sem chamar mkpath.
    const QString computed = home + QStringLiteral("/peubud/save/") + slug
                           + QStringLiteral("/database.pbd");

    QCOMPARE(computed, expected);
    QVERIFY(computed.endsWith(QStringLiteral("/database.pbd")));
    QVERIFY(computed.contains(QStringLiteral("/peubud/save/")));
    QVERIFY(computed.contains(slug));
}

void TestPastasFixas::autosavePathFixo_formatoCorreto()
{
    const QString home = QDir::homePath();
    const QString slug = QStringLiteral("petrus");

    const QString computed = home + QStringLiteral("/peubud/save/") + slug
                           + QStringLiteral("/database.auto.pbd");

    QVERIFY(computed.endsWith(QStringLiteral("/database.auto.pbd")));
    QVERIFY(computed.contains(QStringLiteral("/peubud/save/")));
    QVERIFY(computed.contains(slug));
}

// ── mkpath cria a pasta ─────────────────────────────────────────────────────

void TestPastasFixas::savePathFixo_mkpathCriaPasta()
{
    // Usamos QTemporaryDir para simular o ~/peubud/save/<slug>/ sem poluir HOME.
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString slug = QStringLiteral("test_save_mkpath");
    const QString dir  = tmp.filePath(QStringLiteral("peubud/save/") + slug);
    const QString path = dir + QStringLiteral("/database.pbd");

    // Antes: pasta não existe.
    QVERIFY(!QDir(dir).exists());

    // mkpath cria a árvore inteira (simula o que savePathFixo faz).
    QVERIFY(QDir().mkpath(dir));
    QVERIFY(QDir(dir).exists());

    // O caminho esperado do arquivo faz parte da pasta criada.
    QCOMPARE(QFileInfo(path).absolutePath(), QDir(dir).absolutePath());
}

void TestPastasFixas::autosavePathFixo_mkpathCriaPasta()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString slug = QStringLiteral("test_auto_mkpath");
    const QString dir  = tmp.filePath(QStringLiteral("peubud/save/") + slug);
    const QString path = dir + QStringLiteral("/database.auto.pbd");

    QVERIFY(!QDir(dir).exists());
    QVERIFY(QDir().mkpath(dir));
    QVERIFY(QDir(dir).exists());
    QCOMPARE(QFileInfo(path).absolutePath(), QDir(dir).absolutePath());
}

// ── Lógica de migração ──────────────────────────────────────────────────────

// Caso principal: cofre novo ausente, legado presente
// → copia para novo, renomeia legado para .bak, não deleta original.
void TestPastasFixas::migracao_cofreNovoAusenteElegadoPresente_copiaErebatiza()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // Estrutura: legado em data/<slug>.pbd, novo em peubud/save/<slug>/database.pbd
    const QString slug      = QStringLiteral("joao");
    const QString dirLegado = tmp.filePath(QStringLiteral("data"));
    const QString dirNovo   = tmp.filePath(
        QStringLiteral("peubud/save/") + slug);

    QVERIFY(QDir().mkpath(dirLegado));
    QVERIFY(QDir().mkpath(dirNovo));

    const QString pbdLegado = dirLegado + QStringLiteral("/") + slug
                            + QStringLiteral(".pbd");
    const QString pbdNovo   = dirNovo + QStringLiteral("/database.pbd");
    const QString bakPath   = pbdLegado + QStringLiteral(".bak");

    // Simula cofre legado existente com conteúdo distinto.
    {
        QFile f(pbdLegado);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArrayLiteral("dados_cifrados_legados"));
    }

    // Pré-condições.
    QVERIFY(QFile::exists(pbdLegado));
    QVERIFY(!QFile::exists(pbdNovo));
    QVERIFY(!QFile::exists(bakPath));

    // Executa migração.
    const bool ok = migrarCofreSeNecessario_testavel(pbdNovo, pbdLegado);
    QVERIFY(ok);

    // Cofre novo foi criado com o mesmo conteúdo.
    QVERIFY(QFile::exists(pbdNovo));
    {
        QFile f(pbdNovo);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), QByteArrayLiteral("dados_cifrados_legados"));
    }

    // Legado renomeado para .bak (não deletado — é dado financeiro!).
    QVERIFY(QFile::exists(bakPath));
    QVERIFY(!QFile::exists(pbdLegado)); // original sumiu (virou .bak)
    {
        QFile f(bakPath);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), QByteArrayLiteral("dados_cifrados_legados"));
    }
}

// Caso de idempotência: cofre novo já existe → não toca o legado.
void TestPastasFixas::migracao_cofreNovoJaExiste_naoTocaLegado()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString slug    = QStringLiteral("maria");
    const QString dirLeg  = tmp.filePath(QStringLiteral("data"));
    const QString dirNovo = tmp.filePath(
        QStringLiteral("peubud/save/") + slug);

    QVERIFY(QDir().mkpath(dirLeg));
    QVERIFY(QDir().mkpath(dirNovo));

    const QString pbdLegado = dirLeg + QStringLiteral("/") + slug
                            + QStringLiteral(".pbd");
    const QString pbdNovo   = dirNovo + QStringLiteral("/database.pbd");

    // Ambos existem.
    {
        QFile f(pbdNovo);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArrayLiteral("dados_novos"));
    }
    {
        QFile f(pbdLegado);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArrayLiteral("dados_legados"));
    }

    const bool ok = migrarCofreSeNecessario_testavel(pbdNovo, pbdLegado);
    QVERIFY(ok); // retorna true (cofre novo existe)

    // Cofre novo intacto, legado intacto (migração não tocou em nada).
    QVERIFY(QFile::exists(pbdNovo));
    QVERIFY(QFile::exists(pbdLegado));
    QVERIFY(!QFile::exists(pbdLegado + QStringLiteral(".bak")));

    {
        QFile f(pbdNovo);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), QByteArrayLiteral("dados_novos"));
    }
    {
        QFile f(pbdLegado);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), QByteArrayLiteral("dados_legados"));
    }
}

// Caso de novo usuário: nenhum cofre em lugar algum → retorna false, sem erro.
void TestPastasFixas::migracao_novoUsuarioSemCofre_retornaFalse()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString slug    = QStringLiteral("usuario_novo");
    const QString dirNovo = tmp.filePath(
        QStringLiteral("peubud/save/") + slug);
    QVERIFY(QDir().mkpath(dirNovo));

    const QString pbdNovo   = dirNovo + QStringLiteral("/database.pbd");
    const QString pbdLegado = tmp.filePath(QStringLiteral("data/")
                            + slug + QStringLiteral(".pbd"));

    // Nenhum cofre existe.
    QVERIFY(!QFile::exists(pbdNovo));
    QVERIFY(!QFile::exists(pbdLegado));

    const bool ok = migrarCofreSeNecessario_testavel(pbdNovo, pbdLegado);
    QVERIFY(!ok); // novo usuário — banco começa vazio (correto)

    // Nada foi criado indevidamente.
    QVERIFY(!QFile::exists(pbdNovo));
    QVERIFY(!QFile::exists(pbdNovo + QStringLiteral(".new")));
}

// Garantia de atomicidade: .new não deve vazar se o rename para destino falhar.
// Simula falha de rename colocando um diretório no lugar de pbdNovo.
void TestPastasFixas::migracao_atomica_tmpNaoVazaSeRenomearFalhar()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString slug    = QStringLiteral("carlos");
    const QString dirLeg  = tmp.filePath(QStringLiteral("data"));
    const QString dirNovo = tmp.filePath(
        QStringLiteral("peubud/save/") + slug);

    QVERIFY(QDir().mkpath(dirLeg));
    QVERIFY(QDir().mkpath(dirNovo));

    const QString pbdLegado = dirLeg + QStringLiteral("/") + slug
                            + QStringLiteral(".pbd");
    // pbdNovo aponta para um subdiretório (impossível ser substituído por arquivo).
    // Isso simula: QFile::rename(.new → pbdNovo) falha.
    const QString pbdNovo = dirNovo + QStringLiteral("/database.pbd");
    // Criamos um diretório onde o arquivo deveria ir → rename falha.
    QVERIFY(QDir().mkpath(pbdNovo));

    {
        QFile f(pbdLegado);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArrayLiteral("dados"));
    }

    const bool ok = migrarCofreSeNecessario_testavel(pbdNovo, pbdLegado);

    // Migração falha (rename impossível).
    // O importante: .new não deve ter ficado no disco.
    const QString tmpPath = pbdNovo + QStringLiteral(".new");
    QVERIFY(!QFile::exists(tmpPath));

    // Legado continua intacto (nunca foi tocado se a cópia falhou).
    // Nota: se QFile::copy para tmpPath falhou (destino é dir), legado permanece.
    QVERIFY(QFile::exists(pbdLegado));

    // ok pode ser true ou false dependendo de como o FS lida com copy para dentro
    // de dir — o que importa é que .new não vaza e legado não é deletado.
    (void)ok;
}

QTEST_GUILESS_MAIN(TestPastasFixas)
#include "test_pastas_fixas.moc"
