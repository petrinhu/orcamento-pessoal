// PeuBud - ExternalProcessProvider TDD (#86)
// Wrapping QProcess para invocar scripts externos. Testes via binários POSIX
// padrão (/bin/echo, /bin/false, /bin/sh) — sem dependência de Python.

#include <QtTest/QtTest>
#include <QFileInfo>

#include "services/external/ExternalProcessProvider.h"

class TestExternalProcess : public QObject
{
    Q_OBJECT

private slots:
    void echoCapturaStdout();
    void falseRetornaExitCodeNaoZero();
    void programaInexistenteSinalizaErro();
    void timeoutMataProcesso();
    void stderrSeparadoDoStdout();

private:
    // Skip elegante se binário não disponível na distro do CI.
    bool ensureBinary(const QString &path) const
    {
        if (!QFileInfo::exists(path)) {
            QWARN(qPrintable(QStringLiteral("binário %1 ausente — pulando").arg(path)));
            return false;
        }
        return true;
    }
};

void TestExternalProcess::echoCapturaStdout()
{
    if (!ensureBinary(QStringLiteral("/bin/echo"))) QSKIP("echo missing");
    ExternalProcessProvider p;
    const auto r = p.run(QStringLiteral("/bin/echo"), {QStringLiteral("hello")});
    QVERIFY(r.isOk());
    QCOMPARE(r.exitCode, 0);
    QVERIFY(r.stdoutBytes.contains("hello"));
}

void TestExternalProcess::falseRetornaExitCodeNaoZero()
{
    if (!ensureBinary(QStringLiteral("/bin/false"))) QSKIP("false missing");
    ExternalProcessProvider p;
    const auto r = p.run(QStringLiteral("/bin/false"), {});
    // Exit code não-zero não é erro do provider — o processo rodou normal.
    QCOMPARE(r.exitCode, 1);
    QVERIFY(r.erro.isEmpty());
    QVERIFY(!r.isOk()); // isOk exige exitCode==0 E erro vazio
}

void TestExternalProcess::programaInexistenteSinalizaErro()
{
    ExternalProcessProvider p;
    const auto r = p.run(QStringLiteral("/nao/existe/binario/peubud_xyz"), {});
    QVERIFY(!r.isOk());
    QVERIFY(!r.erro.isEmpty());
}

void TestExternalProcess::timeoutMataProcesso()
{
    if (!ensureBinary(QStringLiteral("/bin/sh"))) QSKIP("sh missing");
    ExternalProcessProvider p;
    const auto r = p.run(QStringLiteral("/bin/sh"),
                          {QStringLiteral("-c"), QStringLiteral("sleep 5")},
                          200);
    QVERIFY(!r.isOk());
    QVERIFY2(r.erro.contains(QStringLiteral("Timeout")),
             qPrintable(QStringLiteral("erro real: %1").arg(r.erro)));
}

void TestExternalProcess::stderrSeparadoDoStdout()
{
    if (!ensureBinary(QStringLiteral("/bin/sh"))) QSKIP("sh missing");
    ExternalProcessProvider p;
    const auto r = p.run(QStringLiteral("/bin/sh"),
                          {QStringLiteral("-c"),
                           QStringLiteral("echo erro >&2; exit 0")});
    QVERIFY(r.isOk());
    QCOMPARE(r.exitCode, 0);
    QVERIFY(r.stderrBytes.contains("erro"));
    QVERIFY(!r.stdoutBytes.contains("erro"));
}

QTEST_GUILESS_MAIN(TestExternalProcess)
#include "test_external_process.moc"
