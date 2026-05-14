#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

// Interface para invocar scripts externos (Python, shell, etc.) que
// retornam JSON em stdout. Isola HTTP/parsing de libs externas do binário Qt.
//
// Caso de uso #58 alternativo (#86): script Python `cotacao_brapi.py PETR4`
// imprime `{"price": 28.50, "var": 1.2}` em stdout. App parsa via QJson.
//
// Padrão de skrooge (skgbankmodeler/skgunitobject.cpp:2292+, Python) e
// gnucash (libgnucash/app-utils/gnc-quotes.cpp, Perl Finance::Quote).
class IExternalProcessProvider {
public:
    struct Resultado {
        int        exitCode    = -1;
        QByteArray stdoutBytes;
        QByteArray stderrBytes;
        QString    erro;            // mensagem se falhou
        bool isOk() const { return exitCode == 0 && erro.isEmpty(); }
    };

    virtual ~IExternalProcessProvider() = default;

    // Executa programa com args. timeoutMs: kill após N ms (default 10s).
    [[nodiscard]] virtual Resultado run(const QString &program,
                                         const QStringList &args,
                                         int timeoutMs = 10000) const = 0;
};
