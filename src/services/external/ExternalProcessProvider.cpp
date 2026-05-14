#include "ExternalProcessProvider.h"

#include <QProcess>

IExternalProcessProvider::Resultado
ExternalProcessProvider::run(const QString &program,
                              const QStringList &args,
                              int timeoutMs) const
{
    Resultado r;
    QProcess proc;
    proc.setProgram(program);
    proc.setArguments(args);
    proc.start();
    if (!proc.waitForStarted(timeoutMs)) {
        r.erro = QStringLiteral("Falha ao iniciar: %1").arg(proc.errorString());
        return r;
    }
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(1000);
        r.stdoutBytes = proc.readAllStandardOutput();
        r.stderrBytes = proc.readAllStandardError();
        r.erro = QStringLiteral("Timeout (>%1ms)").arg(timeoutMs);
        return r;
    }
    r.exitCode    = proc.exitCode();
    r.stdoutBytes = proc.readAllStandardOutput();
    r.stderrBytes = proc.readAllStandardError();
    if (proc.exitStatus() != QProcess::NormalExit) {
        r.erro = QStringLiteral("Processo encerrou anormalmente");
    }
    return r;
}
