// PeuBud - TDD do UserAccountRepository (ADR-001 #212, Fatia 1).
// I/O burro de users.json: round-trip + detecção de adulteração via
// assinatura Ed25519. NÃO testa regra de negócio (isso é UserAccountService,
// fatia 2, fora de escopo aqui).

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "repositories/UserAccountRepository.h"
#include "core/crypto/SodiumCryptoService.h"

class TestUserAccountRepository : public QObject
{
    Q_OBJECT

private slots:
    void roundTripPreservaRegistros();
    void roundTripPreservaCamposDeSigningKeyDoAdmin();
    void detectaAdulteracaoDeCampoDeUsuario();
    void detectaAdulteracaoDaAssinatura();
    void isEmptyEExistsComArquivoAusente();
    void isEmptyComZeroUsuarios();
    void existsPassaParaTrueAposSave();

private:
    static UserAccountRecord criarUsuario(const QString &nome, Role role);
};

// (ADR-001 §8 passo 6/D7, Fatia 4) saltPw/saltRk/kdfOps/kdfMem/
// wrappedKeyByPass/wrappedKeyByRecovery saíram do registro (migraram pro
// VaultHeaderData do .pbd v2, D1) — o fixture agora popula o canal de
// convite (saltInvite/kdfOpsInvite/kdfMemInvite/wrappedMarkerByInvite) no
// lugar deles.
UserAccountRecord TestUserAccountRepository::criarUsuario(const QString &nome, Role role)
{
    UserAccountRecord u;
    u.username              = nome;
    u.role                  = role;
    u.invitePending          = false;
    u.saltInvite             = QByteArray(16, 'A');
    u.kdfOpsInvite           = 3;
    u.kdfMemInvite           = 268435456; // 256 MB — MODERATE
    u.wrappedMarkerByInvite  = QByteArray(72, 'C');
    return u;
}

void TestUserAccountRepository::roundTripPreservaRegistros()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("users.json"));

    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, path);

    const auto kp = crypto.generateSignKeypair();
    QVector<UserAccountRecord> users;
    users.append(criarUsuario(QStringLiteral("admin"), Role::Admin));
    users.append(criarUsuario(QStringLiteral("maria"), Role::User));

    QVERIFY(repo.saveAll(users, kp.publicKey, kp.secretKey));

    const auto result = repo.loadAll();
    QVERIFY(result.ok);
    QVERIFY(!result.tampered);
    QVERIFY(!repo.lastLoadWasTampered());
    QCOMPARE(result.adminPubKey, kp.publicKey);
    QCOMPARE(result.users.size(), 2);

    QCOMPARE(result.users[0].username, QStringLiteral("admin"));
    QVERIFY(result.users[0].role == Role::Admin);
    QCOMPARE(result.users[0].saltInvite, users[0].saltInvite);
    QCOMPARE(result.users[0].kdfOpsInvite, users[0].kdfOpsInvite);
    QCOMPARE(static_cast<qint64>(result.users[0].kdfMemInvite), static_cast<qint64>(users[0].kdfMemInvite));
    QCOMPARE(result.users[0].wrappedMarkerByInvite, users[0].wrappedMarkerByInvite);
    QCOMPARE(result.users[0].invitePending, users[0].invitePending);

    QCOMPARE(result.users[1].username, QStringLiteral("maria"));
    QVERIFY(result.users[1].role == Role::User);
}

// (Fatia 2, ADR-001 #212 item A) wrappedSignKeyByPass/Recovery são novos
// desde a Fatia 1 — prova que entram no round-trip (e portanto na
// assinatura, via canonicalContent) sem quebrar o formato. Usuário comum
// (criarUsuario) deixa os 2 campos vazios; este teste cobre o caso admin
// com a signing key preenchida.
void TestUserAccountRepository::roundTripPreservaCamposDeSigningKeyDoAdmin()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("users.json"));

    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, path);
    const auto kp = crypto.generateSignKeypair();

    UserAccountRecord admin = criarUsuario(QStringLiteral("admin"), Role::Admin);
    admin.wrappedSignKeyByPass     = QByteArray(104, 'E'); // 64B secretkey + 24B nonce + 16B MAC
    admin.wrappedSignKeyByRecovery = QByteArray(104, 'F');

    QVector<UserAccountRecord> users;
    users.append(admin);
    users.append(criarUsuario(QStringLiteral("maria"), Role::User)); // campos ficam vazios (default)

    QVERIFY(repo.saveAll(users, kp.publicKey, kp.secretKey));

    const auto result = repo.loadAll();
    QVERIFY(result.ok);
    QVERIFY(!result.tampered);
    QCOMPARE(result.users.size(), 2);
    QCOMPARE(result.users[0].wrappedSignKeyByPass, admin.wrappedSignKeyByPass);
    QCOMPARE(result.users[0].wrappedSignKeyByRecovery, admin.wrappedSignKeyByRecovery);
    QVERIFY2(result.users[1].wrappedSignKeyByPass.isEmpty(),
        "usuário comum deveria manter wrappedSignKeyByPass vazio no round-trip");
    QVERIFY2(result.users[1].wrappedSignKeyByRecovery.isEmpty(),
        "usuário comum deveria manter wrappedSignKeyByRecovery vazio no round-trip");
}

void TestUserAccountRepository::detectaAdulteracaoDeCampoDeUsuario()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("users.json"));

    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, path);
    const auto kp = crypto.generateSignKeypair();

    QVector<UserAccountRecord> users;
    users.append(criarUsuario(QStringLiteral("maria"), Role::User));
    QVERIFY(repo.saveAll(users, kp.publicKey, kp.secretKey));

    // Ataque CWE-269: edita "role" de "user" -> "admin" direto no disco,
    // SEM re-assinar. loadAll() deve recusar (fail-secure).
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        f.close();

        QJsonObject top = doc.object();
        QJsonArray arr = top.value(QStringLiteral("users")).toArray();
        QJsonObject u0 = arr.at(0).toObject();
        u0[QStringLiteral("role")] = QStringLiteral("admin");
        arr.replace(0, u0);
        top[QStringLiteral("users")] = arr;

        QFile out(path);
        QVERIFY(out.open(QIODevice::WriteOnly | QIODevice::Truncate));
        out.write(QJsonDocument(top).toJson());
        out.close();
    }

    const auto result = repo.loadAll();
    QVERIFY(!result.ok);
    QVERIFY(result.tampered);
    QVERIFY(repo.lastLoadWasTampered());
    QVERIFY(result.users.isEmpty());
}

void TestUserAccountRepository::detectaAdulteracaoDaAssinatura()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("users.json"));

    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, path);
    const auto kp = crypto.generateSignKeypair();

    QVector<UserAccountRecord> users;
    users.append(criarUsuario(QStringLiteral("admin"), Role::Admin));
    QVERIFY(repo.saveAll(users, kp.publicKey, kp.secretKey));

    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        f.close();

        QJsonObject top = doc.object();
        QString sig = top.value(QStringLiteral("signature")).toString();
        QVERIFY(!sig.isEmpty());
        // Troca 1 caractere base64 — invalida a assinatura sem mudar o tamanho.
        const QChar original = sig.at(0);
        sig[0] = (original == QLatin1Char('A')) ? QLatin1Char('B') : QLatin1Char('A');
        top[QStringLiteral("signature")] = sig;

        QFile out(path);
        QVERIFY(out.open(QIODevice::WriteOnly | QIODevice::Truncate));
        out.write(QJsonDocument(top).toJson());
        out.close();
    }

    const auto result = repo.loadAll();
    QVERIFY(!result.ok);
    QVERIFY(result.tampered);
    QVERIFY(repo.lastLoadWasTampered());
    QVERIFY(result.users.isEmpty());
}

void TestUserAccountRepository::isEmptyEExistsComArquivoAusente()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("nao-existe.json"));

    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, path);

    QVERIFY(!repo.exists());
    QVERIFY(repo.isEmpty());
}

void TestUserAccountRepository::isEmptyComZeroUsuarios()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("users.json"));

    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, path);
    const auto kp = crypto.generateSignKeypair();

    QVERIFY(repo.saveAll({}, kp.publicKey, kp.secretKey));
    QVERIFY(repo.exists());
    QVERIFY(repo.isEmpty());

    const auto result = repo.loadAll();
    QVERIFY(result.ok);
    QVERIFY(result.users.isEmpty());
}

void TestUserAccountRepository::existsPassaParaTrueAposSave()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("users.json"));

    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, path);
    QVERIFY(!repo.exists());

    const auto kp = crypto.generateSignKeypair();
    QVector<UserAccountRecord> users;
    users.append(criarUsuario(QStringLiteral("admin"), Role::Admin));
    QVERIFY(repo.saveAll(users, kp.publicKey, kp.secretKey));

    QVERIFY(repo.exists());
}

QTEST_GUILESS_MAIN(TestUserAccountRepository)
#include "test_user_account_repository.moc"
