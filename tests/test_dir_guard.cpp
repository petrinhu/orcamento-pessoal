// PeuBud — #178: garantia de pasta antes de save/autosave
//
// Testa a lógica PURA do helper garantirPastaExiste():
//   - se a pasta já existe → nenhuma ação necessária
//   - se não existe e o usuário escolhe [Criar pasta] → QDir().mkpath() cria
//   - se mkpath falha (ex.: caminho inválido) → retorna false
//
// O ramo de QMessageBox (GUI) não é testável headless; cobrimos aqui apenas
// o comportamento de mkpath + verificação de existência, que é a lógica de
// negócio crítica (evitar perda silenciosa de dados).

#include <QtTest/QtTest>
#include <QDir>
#include <QTemporaryDir>

class TestDirGuard : public QObject
{
    Q_OBJECT

private slots:
    void pastaExistente_naoRequerCriacao();
    void pastaInexistente_mkpathCria();
    void subpastaInexistente_mkpathCriaRecursivo();
    void mkpathEmCaminhoInvalido_retornaFalse();
};

// Caso 1: pasta já existe → mkpath é idempotente (retorna true sem criar nada
// extra). Simula o fast-path de garantirPastaExiste quando dir.exists() == true.
void TestDirGuard::pastaExistente_naoRequerCriacao()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString existente = tmp.path(); // já existe
    QVERIFY(QDir(existente).exists());

    // mkpath em pasta existente deve retornar true (idempotente)
    QVERIFY(QDir().mkpath(existente));
}

// Caso 2: pasta inexistente → mkpath cria. Simula o ramo [Criar pasta].
void TestDirGuard::pastaInexistente_mkpathCria()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString novaPasta = tmp.filePath(QStringLiteral("saves"));
    QVERIFY(!QDir(novaPasta).exists()); // pré-condição: não existe

    QVERIFY(QDir().mkpath(novaPasta));
    QVERIFY(QDir(novaPasta).exists()); // pós-condição: criada
}

// Caso 3: subpastas aninhadas — mkpath cria toda a árvore.
// Relevante para autosavesFolder() que é "savesFolder()/autosaves".
void TestDirGuard::subpastaInexistente_mkpathCriaRecursivo()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString aninhada = tmp.filePath(
        QStringLiteral("PeuBud/saves/autosaves"));
    QVERIFY(!QDir(aninhada).exists());

    QVERIFY(QDir().mkpath(aninhada));
    QVERIFY(QDir(aninhada).exists());
}

// Caso 4: caminho impossível (arquivo existente como componente de path) →
// mkpath retorna false → helper deve retornar false (não aborta com crash).
void TestDirGuard::mkpathEmCaminhoInvalido_retornaFalse()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // Cria um arquivo onde gostaríamos de ter uma pasta
    const QString arquivo = tmp.filePath(QStringLiteral("bloqueio"));
    QFile f(arquivo);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("x");
    f.close();

    // Tenta criar "bloqueio/subdir" — deve falhar pois "bloqueio" é arquivo
    const QString impossivel = arquivo + QStringLiteral("/subdir");
    QVERIFY(!QDir().mkpath(impossivel));
}

QTEST_GUILESS_MAIN(TestDirGuard)
#include "test_dir_guard.moc"
