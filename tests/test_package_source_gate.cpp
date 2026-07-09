// PeuBud - #96 (Onda U, DIST-01): CPack source package vazando 670MB de
// terceiros. `CPACK_SOURCE_IGNORE_FILES` (CMakeLists.txt) não excluía
// `models/` (apps de referência de terceiros, 670MB), `build-rel/`,
// `build-verif/` nem `.superpowers/` (notas de sessão) — `cpack --config
// CPackSourceConfig.cmake` empacotava tudo isso no tarball fonte.
//
// Contrato após o fix:
//   (1) `cpack --config <build>/CPackSourceConfig.cmake` roda com sucesso
//       (exit 0) e gera exatamente 1 tarball `.tar.gz`.
//   (2) O tarball tem no máximo 5 MB (o repo versionado tem ~5,5 MB; tudo
//       além disso é sinal de vazamento de diretório pesado não-versionado).
//   (3) O tarball NÃO contém nenhuma entrada dentro de `models/`,
//       `build-rel/`, `build-verif/` ou `.superpowers/`.
//   (4) `src/models/` (structs POD de domínio, legítimos) CONTINUA no
//       tarball — o fix precisa ser cirúrgico (âncora em
//       `${CMAKE_SOURCE_DIR}/models/`), não um `/models/` genérico que
//       também apagaria `src/models/*.h` do pacote fonte.
//
// Gate real: invoca o `cpack` de verdade contra a config gerada pelo build
// atual (mesma `CMakeLists.txt` de produção) — não uma reimplementação da
// lista de regex. Se `cpack` não estiver instalado, QSKIP (ambiente sem
// cmake completo).
//
// Executar com QT_QPA_PLATFORM=offscreen (QTEST_GUILESS_MAIN não abre
// display, mas mantemos o padrão do projeto por consistência).

#include <QtTest>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "services/external/ExternalProcessProvider.h"

#ifndef PEUBUD_SOURCE_DIR
#define PEUBUD_SOURCE_DIR "."
#endif
#ifndef PEUBUD_BINARY_DIR
#define PEUBUD_BINARY_DIR "."
#endif

namespace {
constexpr qint64 kMaxTarballBytes = 5LL * 1024 * 1024; // 5 MB (work order DIST-01)
constexpr int    kCpackTimeoutMs  = 120000;             // empacotar ~5MB e' rapido; margem generosa p/ CI lento
}

class TestPackageSourceGate : public QObject
{
    Q_OBJECT

private slots:
    void tarballFonteExcluiDiretoriosPesadosDeTerceiros();
};

void TestPackageSourceGate::tarballFonteExcluiDiretoriosPesadosDeTerceiros()
{
    const QString cpackBin = QStandardPaths::findExecutable(QStringLiteral("cpack"));
    if (cpackBin.isEmpty()) {
        QSKIP("cpack ausente no PATH - instale cmake completo (dnf/apt/pacman install cmake)");
    }

    const QString configPath = QStringLiteral(PEUBUD_BINARY_DIR) + QStringLiteral("/CPackSourceConfig.cmake");
    if (!QFileInfo::exists(configPath)) {
        QSKIP(qPrintable(QStringLiteral("CPackSourceConfig.cmake ausente em %1 - build nao configurado p/ CPack")
                              .arg(configPath)));
    }

    QTemporaryDir outDir;
    QVERIFY2(outDir.isValid(), "falha ao criar diretorio temporario p/ saida do cpack");

    ExternalProcessProvider proc;
    const auto r = proc.run(cpackBin,
                             {QStringLiteral("--config"), configPath,
                              QStringLiteral("-B"), outDir.path()},
                             kCpackTimeoutMs);
    QVERIFY2(r.isOk(),
             qPrintable(QStringLiteral("cpack falhou: erro='%1' exitCode=%2 stderr=%3")
                            .arg(r.erro)
                            .arg(r.exitCode)
                            .arg(QString::fromUtf8(r.stderrBytes))));

    // Localiza o(s) tarball(s) gerado(s) na raiz do outDir.
    QDir dir(outDir.path());
    const QStringList tarballs = dir.entryList({QStringLiteral("*.tar.gz")}, QDir::Files);
    QVERIFY2(tarballs.size() == 1,
             qPrintable(QStringLiteral("esperava exatamente 1 tarball .tar.gz, achou %1: %2")
                            .arg(tarballs.size())
                            .arg(tarballs.join(QStringLiteral(", ")))));

    const QString tarballPath = dir.filePath(tarballs.first());
    const qint64 tarballBytes = QFileInfo(tarballPath).size();
    QVERIFY2(tarballBytes <= kMaxTarballBytes,
             qPrintable(QStringLiteral("tarball fonte com %1 bytes (> %2 MB) - CPACK_SOURCE_IGNORE_FILES "
                                       "nao esta excluindo diretorio(s) pesado(s) (models/build-rel/"
                                       "build-verif/.superpowers)")
                            .arg(tarballBytes)
                            .arg(kMaxTarballBytes / (1024 * 1024))));

    // Lista o conteudo do tarball (tar precisa estar no PATH - dependencia
    // padrao de qualquer distro Linux com toolchain de build).
    const QString tarBin = QStandardPaths::findExecutable(QStringLiteral("tar"));
    QVERIFY2(!tarBin.isEmpty(), "tar ausente no PATH");
    const auto listagem = proc.run(tarBin, {QStringLiteral("tzf"), tarballPath}, 15000);
    QVERIFY2(listagem.isOk(),
             qPrintable(QStringLiteral("tar tzf falhou: %1").arg(listagem.erro)));

    const QString conteudo = QString::fromUtf8(listagem.stdoutBytes);

    // Verificacao por segmento de path (nao substring solta): cada entrada
    // do tar vem prefixada com "<pkg>-<versao>-Source/" (ex.:
    // "peubud-0.1.0-Source/models/foo"). Comparamos o que vem DEPOIS desse
    // prefixo com os nomes proibidos na RAIZ do pacote - um '/models/'
    // generico tambem acertaria 'src/models/' (domain structs legitimos),
    // que a contra-prova abaixo exige que continue no tarball.
    const QStringList linhas = conteudo.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &linha : linhas) {
        const int primeiraBarra = linha.indexOf(QLatin1Char('/'));
        if (primeiraBarra < 0) continue;
        const QString relativo = linha.mid(primeiraBarra + 1); // remove "<pkg>-Source/"
        for (const QString &proibido : {QStringLiteral("models/"),
                                         QStringLiteral("build-rel/"),
                                         QStringLiteral("build-verif/"),
                                         QStringLiteral(".superpowers/")}) {
            QVERIFY2(!relativo.startsWith(proibido),
                     qPrintable(QStringLiteral("tarball contem entrada proibida na raiz '%1' (linha "
                                               "completa: '%2') - vazamento de diretorio pesado/"
                                               "nao-versionado (#96 DIST-01)")
                                    .arg(proibido, linha)));
        }
    }

    // Contra-prova: src/models/ (domain structs, POD legitimos) tem que
    // continuar no pacote - a exclusao de 'models/' precisa ser cirurgica
    // (ancorada na raiz do projeto), nao um '/models/' generico.
    QVERIFY2(conteudo.contains(QStringLiteral("/src/models/")),
             "src/models/ sumiu do tarball fonte - fix de DIST-01 excluiu "
             "domain structs legitimos junto com o models/ de terceiros "
             "(regex precisa ser ancorada em ${CMAKE_SOURCE_DIR}/models/)");
}

QTEST_GUILESS_MAIN(TestPackageSourceGate)
#include "test_package_source_gate.moc"
