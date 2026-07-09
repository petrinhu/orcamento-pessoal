#include "UnixSignalHandler.h"

#include <QDebug>
#include <QSocketNotifier>

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

int UnixSignalHandler::s_pipeFd[2] = {-1, -1};

UnixSignalHandler &UnixSignalHandler::instance()
{
    static UnixSignalHandler inst;
    return inst;
}

UnixSignalHandler::UnixSignalHandler(QObject *parent)
    : QObject(parent)
{
}

UnixSignalHandler::~UnixSignalHandler()
{
    if (s_pipeFd[0] != -1) ::close(s_pipeFd[0]);
    if (s_pipeFd[1] != -1) ::close(s_pipeFd[1]);
    s_pipeFd[0] = s_pipeFd[1] = -1;
}

// async-signal-safe: write() é permitido por POSIX signal-safety(7).
// Qualquer outra coisa (malloc, Qt, close() do fd real de dados, I/O
// bufferizado) NÃO é seguro dentro de um handler de sinal — daí o self-pipe:
// o trabalho de verdade acontece fora do handler, no event loop.
void UnixSignalHandler::posixHandler(int /*sig*/)
{
    const char byte = 1;
    // Resultado intencionalmente ignorado: em handler de sinal não há como
    // reagir a uma falha de write() de forma segura (nem qWarning é seguro
    // aqui). Pior caso: o self-pipe não acorda o event loop desta vez — a
    // aplicação continua rodando normalmente (degrada, não corrompe nada).
    const ssize_t ignored = ::write(s_pipeFd[1], &byte, sizeof(byte));
    (void)ignored;
}

bool UnixSignalHandler::install()
{
    if (m_installed)
        return true; // idempotente

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, s_pipeFd) != 0) {
        qWarning() << "UnixSignalHandler::install: socketpair() falhou —"
                      " sem graceful shutdown por sinal (SIGTERM/SIGINT matam"
                      " o processo sem cifrar o .db de volta pro .pbd)";
        return false;
    }

    struct sigaction sa{};
    sa.sa_handler = &UnixSignalHandler::posixHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGTERM, &sa, nullptr) != 0 || sigaction(SIGINT, &sa, nullptr) != 0) {
        qWarning() << "UnixSignalHandler::install: sigaction() falhou —"
                      " sem graceful shutdown por sinal";
        ::close(s_pipeFd[0]);
        ::close(s_pipeFd[1]);
        s_pipeFd[0] = s_pipeFd[1] = -1;
        return false;
    }

    m_notifier = new QSocketNotifier(s_pipeFd[0], QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, [this]() {
        char tmp;
        // Drena o byte do self-pipe (evita reativação espúria imediata do
        // notifier); o valor em si não carrega informação além de "acordou".
        const ssize_t ignored = ::read(s_pipeFd[0], &tmp, sizeof(tmp));
        (void)ignored;
        emit terminationRequested();
    });

    m_installed = true;
    return true;
}
