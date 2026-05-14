#pragma once

#include "IExternalProcessProvider.h"

// Implementação default via QProcess.
// Bloqueante (síncrona): start + waitForStarted + waitForFinished com timeout.
// Mata o processo se exceder timeoutMs. Captura stdout/stderr separados.
class ExternalProcessProvider final : public IExternalProcessProvider {
public:
    [[nodiscard]] Resultado run(const QString &program,
                                const QStringList &args,
                                int timeoutMs = 10000) const override;
};
