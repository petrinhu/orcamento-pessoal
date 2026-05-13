// PeuBud - BackupService TDD (#79)
// Foca na logica pura (retencao + scheduling). Backup real depende de
// DatabaseManager aberto e libsodium; coberto por testes manuais.

#include <QtTest/QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QDateTime>
#include <QThread>
#include "services/BackupService.h"

class TestBackup : public QObject
{
    Q_OBJECT

private slots:
    void shouldRunDesativadoSempreFalse();
    void shouldRunDiariaNuncaRodouTrue();
    void shouldRunDiariaMenosDeUmDiaFalse();
    void shouldRunDiariaMaisDeUmDiaTrue();
    void shouldRunSemanalAposSeteDiasTrue();

    void listarBackupsOrdenadoPorMtime();
    void pruneRetencaoMantemMaisRecentes();
    void pruneRetencaoIgnoraOutrosArquivos();
};

void TestBackup::shouldRunDesativadoSempreFalse()
{
    BackupService::Config c;
    c.frequencia = BackupService::Frequencia::Desativado;
    QVERIFY(!BackupService::shouldRunNow(c, QDateTime::currentDateTime()));
}

void TestBackup::shouldRunDiariaNuncaRodouTrue()
{
    BackupService::Config c;
    c.frequencia = BackupService::Frequencia::Diaria;
    // c.ultimo invalido (nunca rodou)
    QVERIFY(BackupService::shouldRunNow(c, QDateTime::currentDateTime()));
}

void TestBackup::shouldRunDiariaMenosDeUmDiaFalse()
{
    const QDateTime agora = QDateTime(QDate(2026, 5, 13), QTime(10, 0));
    BackupService::Config c;
    c.frequencia = BackupService::Frequencia::Diaria;
    c.ultimo     = agora.addSecs(-3600 * 12); // 12h atras
    QVERIFY(!BackupService::shouldRunNow(c, agora));
}

void TestBackup::shouldRunDiariaMaisDeUmDiaTrue()
{
    const QDateTime agora = QDateTime(QDate(2026, 5, 13), QTime(10, 0));
    BackupService::Config c;
    c.frequencia = BackupService::Frequencia::Diaria;
    c.ultimo     = agora.addSecs(-3600 * 25); // 25h atras
    QVERIFY(BackupService::shouldRunNow(c, agora));
}

void TestBackup::shouldRunSemanalAposSeteDiasTrue()
{
    const QDateTime agora = QDateTime(QDate(2026, 5, 13), QTime(10, 0));
    BackupService::Config c;
    c.frequencia = BackupService::Frequencia::Semanal;
    c.ultimo     = agora.addDays(-8);
    QVERIFY(BackupService::shouldRunNow(c, agora));
    c.ultimo = agora.addDays(-3);
    QVERIFY(!BackupService::shouldRunNow(c, agora));
}

void TestBackup::listarBackupsOrdenadoPorMtime()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // 3 arquivos com mtime crescente
    const QStringList nomes = {
        QStringLiteral("peubud-teste-20260510-0900.pbd"),
        QStringLiteral("peubud-teste-20260511-0900.pbd"),
        QStringLiteral("peubud-teste-20260512-0900.pbd"),
    };
    for (int i = 0; i < nomes.size(); ++i) {
        const QString full = tmp.filePath(nomes[i]);
        QFile f(full);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("payload"));
        f.close();
        // Forca mtime crescente via sleep curto (setFileTime nao funciona
        // de forma confiavel em todos os FS/Qt builds).
        QThread::msleep(15);
    }

    const QStringList lista = BackupService::listarBackups(tmp.path(),
                                                            QStringLiteral("teste"));
    QCOMPARE(lista.size(), 3);
    // Mais recente primeiro
    QVERIFY(lista.first().endsWith(nomes.last()));
    QVERIFY(lista.last().endsWith(nomes.first()));
}

void TestBackup::pruneRetencaoMantemMaisRecentes()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    for (int i = 0; i < 5; ++i) {
        const QString full = tmp.filePath(
            QStringLiteral("peubud-teste-202605%1-1200.pbd")
                .arg(10 + i, 2, 10, QChar('0')));
        QFile f(full);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("x"));
        f.close();
        QThread::msleep(15);
    }
    const int removidos = BackupService::pruneRetention(tmp.path(),
                                                         QStringLiteral("teste"), 3);
    QCOMPARE(removidos, 2);
    const QStringList restantes = BackupService::listarBackups(tmp.path(),
                                                                QStringLiteral("teste"));
    QCOMPARE(restantes.size(), 3);
}

void TestBackup::pruneRetencaoIgnoraOutrosArquivos()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // 2 arquivos do slug "teste" + 1 arquivo nao relacionado
    QFile a(tmp.filePath(QStringLiteral("peubud-teste-20260510-1200.pbd")));
    a.open(QIODevice::WriteOnly); a.write("x"); a.close();
    QFile b(tmp.filePath(QStringLiteral("peubud-teste-20260511-1200.pbd")));
    b.open(QIODevice::WriteOnly); b.write("x"); b.close();
    QFile c(tmp.filePath(QStringLiteral("outroapp-config.bin")));
    c.open(QIODevice::WriteOnly); c.write("x"); c.close();

    BackupService::pruneRetention(tmp.path(), QStringLiteral("teste"), 1);

    // outroapp-config.bin nao deve ter sido tocado
    QVERIFY(QFile::exists(tmp.filePath(QStringLiteral("outroapp-config.bin"))));
}

QTEST_GUILESS_MAIN(TestBackup)
#include "test_backup.moc"
