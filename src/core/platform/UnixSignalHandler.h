#pragma once

#include <QObject>

class QSocketNotifier;

// (#192/SEC-02) Handler de SIGTERM/SIGINT via self-pipe trick.
//
// PROBLEMA: sem handler algum, um `kill <pid>` (SIGTERM, default de `kill`),
// `systemctl stop`, ou logout do DE mata o processo sem rodar
// DatabaseManager::close() — o .db em claro (dado financeiro
// descriptografado) fica órfão em disco, nunca cifrado de volta pro .pbd.
//
// INVARIANTE (signal-safety, POSIX signal-safety(7)): o handler de sinal em
// si (posixHandler) só chama write() — a ÚNICA operação async-signal-safe
// necessária aqui. NUNCA chama close()/malloc/emit Qt/qualquer coisa que não
// seja async-signal-safe dentro do handler. A limpeza real (fechar/cifrar o
// banco) roda depois, no event loop do Qt, quando o QSocketNotifier lê o
// byte do self-pipe e emite terminationRequested() — o slot conectado
// (main.cpp) é quem chama DatabaseManager::close().
class UnixSignalHandler : public QObject {
    Q_OBJECT
public:
    static UnixSignalHandler &instance();

    // Instala os handlers de SIGTERM/SIGINT (idempotente — chamadas
    // subsequentes são no-op e retornam true). Retorna false se
    // socketpair()/sigaction() falhar (log via qWarning; a app segue sem
    // graceful shutdown por sinal — degrada, não crasha).
    bool install();

signals:
    // Emitido no event loop do Qt (thread principal) quando SIGTERM ou
    // SIGINT chega. O slot conectado deve fazer o cleanup real (fechar DB).
    void terminationRequested();

private:
    explicit UnixSignalHandler(QObject *parent = nullptr);
    ~UnixSignalHandler() override;

    // async-signal-safe: SÓ write() no self-pipe. Nada de Qt/malloc/close()
    // de verdade aqui — ver invariante na doc da classe.
    static void posixHandler(int sig);

    static int s_pipeFd[2];

    QSocketNotifier *m_notifier   = nullptr;
    bool             m_installed  = false;
};
