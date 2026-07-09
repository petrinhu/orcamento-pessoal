// PeuBud - #192 (Onda U, SEC-02): handler de SIGTERM/SIGINT via self-pipe.
//
// BUG: o processo nao instalava handler de SIGTERM/SIGINT nenhum. Um
// `systemctl stop`, logout do DE, ou `kill <pid>` (SIGTERM, default de
// `kill`) matava o processo sem rodar DatabaseManager::close() -> o .db em
// claro (dado financeiro descriptografado) ficava orfao em disco, sem ser
// cifrado de volta pro .pbd.
//
// FIX: UnixSignalHandler instala sigaction(SIGTERM/SIGINT) cujo handler e
// async-signal-safe (POSIX signal-safety(7)): SO escreve 1 byte num
// self-pipe (write() e seguro; close()/malloc/emit Qt NAO sao). A limpeza
// real roda depois, no event loop do Qt, via QSocketNotifier -> emite
// terminationRequested() -- o slot conectado (main.cpp) e quem chama
// DatabaseManager::close().
//
// Este teste ENVIA sinais reais (::raise) pro proprio processo pra provar
// que o mecanismo funciona ponta-a-ponta, nao so a instalacao.

#include <QtTest/QtTest>
#include <QSignalSpy>

#include <csignal>

#include "core/platform/UnixSignalHandler.h"

class TestUnixSignalHandler : public QObject
{
    Q_OBJECT

private slots:
    void install_retornaTrue()
    {
        QVERIFY(UnixSignalHandler::instance().install());
    }

    void install_idempotente_segundaChamadaTambemRetornaTrue()
    {
        QVERIFY(UnixSignalHandler::instance().install());
        QVERIFY(UnixSignalHandler::instance().install());
    }

    // O CERNE do fix: SIGTERM real (o que `kill`/systemd/logout enviam)
    // acorda o event loop do Qt via self-pipe e emite terminationRequested().
    void sigterm_emiteTerminationRequestedViaEventLoop()
    {
        QVERIFY(UnixSignalHandler::instance().install());

        QSignalSpy spy(&UnixSignalHandler::instance(),
                       &UnixSignalHandler::terminationRequested);
        QVERIFY(spy.isValid());
        QCOMPARE(spy.count(), 0);

        ::raise(SIGTERM);

        // spy.wait() bombeia o event loop -- se o sinal fosse tratado de
        // forma sincrona/direta (violando signal-safety), o teste travaria
        // ou crasharia aqui em vez de simplesmente aguardar o QSocketNotifier.
        QVERIFY2(spy.wait(2000),
                 "terminationRequested() nao chegou via event loop apos SIGTERM");
        QCOMPARE(spy.count(), 1);
    }

    void sigint_emiteTerminationRequestedViaEventLoop()
    {
        QVERIFY(UnixSignalHandler::instance().install());

        QSignalSpy spy(&UnixSignalHandler::instance(),
                       &UnixSignalHandler::terminationRequested);

        ::raise(SIGINT);

        QVERIFY2(spy.wait(2000),
                 "terminationRequested() nao chegou via event loop apos SIGINT");
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_GUILESS_MAIN(TestUnixSignalHandler)
#include "test_unix_signal_handler.moc"
