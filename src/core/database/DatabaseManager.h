#pragma once

#include "core/crypto/VaultFile.h"

#include <QSqlDatabase>
#include <QString>
#include <QByteArray>
#include <functional>
#include <vector>

// Manages the SQLite connection lifecycle:
//   open (decrypt .pbd → .db) / close (encrypt .db → .pbd)
// In QT_DEBUG: uses plain .db with no encryption.
class DatabaseManager {
public:
    static DatabaseManager &instance();

    // Opens (or creates) the database for the given user slug.
    // In release: decrypts data/<slug>.pbd.
    //   keyNew    — Argon2id-derived key (new format: AES-GCM / XChaCha20).
    //   keyLegacy — PBKDF2-derived key   (legacy format: AES-256-CBC).
    //               Pass empty if no legacy file can exist (new user).
    // On first open after a format upgrade, the file is re-encrypted automatically
    // with keyNew when the database is closed (transparent migration).
    // In debug: opens data/<slug>.db directly (no encryption).
    bool open(const QString &slug,
              const QByteArray &keyNew    = {},
              const QByteArray &keyLegacy = {});
    bool save();  // flush DB → .pbd sem fechar (release); no-op em debug
    // Fecha a conexão e persiste (release: cifra .db → .pbd). Retorna false
    // se a cifragem foi ABORTADA por falha de integrity_check (#191/FIN-02):
    // nesse caso o .pbd anterior (último bom) é preservado intacto — o
    // chamador deve alertar o usuário (dado da sessão pode ter sido perdido,
    // mas o cofre em disco não foi corrompido).
    bool close();
    bool isOpen() const;

    // File-level operations (release mode only; debug: plain copy)
    bool exportToFile(const QString &destPath);   // encrypt current DB → destPath
    bool importFromFile(const QString &srcPath);  // decrypt srcPath → replace session

    QSqlDatabase db() const { return m_db; }

    // (#191/FIN-02) Verifica a integridade de um arquivo SQLite via
    // `PRAGMA integrity_check`, usando uma conexão própria e somente-leitura
    // (não interfere com m_db). Retorna true SOMENTE quando o resultado é
    // exatamente "ok". Falha ao abrir, arquivo ausente, ou qualquer resultado
    // diferente de "ok" retornam false (fail-safe: na dúvida, não persiste).
    // Público e static para ser testável isoladamente (sem depender do
    // singleton nem de uma sessão aberta) e para servir de gate único dentro
    // de encryptAndSave() (chamado por save()/close()/exportToFile()).
    static bool checkFileIntegrity(const QString &dbFilePath);

    // (#200/ARQ-01) Injeção de schema Mid/Backend — rompe a inversão de
    // camada em que o Foundation (`DatabaseManager`) conhecia 4 classes
    // concretas (`FechamentoMensalService`, `AuditLogService`,
    // `ProventoRepository`, `AjusteCorporativoService`) para criar as
    // próprias tabelas. Mesmo padrão de DI via std::function já usado em
    // `BackupService::ExportFn` (#79).
    //
    // O ponto de composição registra os providers ANTES do primeiro
    // open() — em produção isso é `AppBootstrap::registerCoreSchemaProviders()`
    // (chamado em main.cpp); suítes de teste que abrem o DB diretamente
    // (sem passar por main()) chamam o mesmo método em initTestCase().
    // createSchema() invoca os providers registrados, na ordem de registro,
    // depois de criar as 16 tabelas inline do Foundation.
    using SchemaProvider = std::function<void(QSqlDatabase &)>;
    static void registerSchemaProvider(SchemaProvider provider);

    // (ADR-001 §8 passo 6/D4, Fatia 4) Injeta o header `.pbd` v2 que a
    // PRÓXIMA save()/close() vai gravar (usado no bootstrap de um cofre
    // NOVO — chamar ANTES do open() correspondente, ver
    // UserAccountService::bootstrapVault). `header.username` vazio força v1
    // (reservado à migração de cofres legados — Fatia 5, não usado ainda,
    // só documentado aqui).
    //
    // Ordering em open() (documentado aqui por ser sutil — ver
    // decryptAndLoad()): (1) `.pbd` detectado como V2 → open() RESTAURA
    // m_vaultHeader/m_vaultIsV2 a partir do PRÓPRIO ARQUIVO, sobrescrevendo
    // o que este setter tiver injetado; (2) V1/legado → open() RESETA os
    // dois pra vazio/false (singleton reusado entre logins — sem isso, um
    // header v2 de uma sessão anterior vazaria); (3) arquivo NÃO EXISTE
    // (bootstrap de cofre novo) → open() NÃO TOCA em m_vaultHeader/
    // m_vaultIsV2, preservando exatamente o que este setter injetou — o
    // 1º close()/save() subsequente materializa o header no `.pbd` novo via
    // encryptAndSave()/VaultFile::frameV2().
    void setVaultHeader(const VaultHeaderData &header);
    [[nodiscard]] VaultHeaderData vaultHeader() const { return m_vaultHeader; }
    [[nodiscard]] bool vaultIsV2() const { return m_vaultIsV2; }

    // (ADR-001 §8 passos 6-7, Fatia 5) Troca a chave `K` do PRÓXIMO
    // save()/close() SEM reabrir o arquivo — usada exclusivamente pela
    // migração de cofres legados (LegacyVaultMigrationService): o `.db` de
    // trabalho já está aberto/decifrado com a chave ANTIGA (derivada pelo
    // caller, mesmo caminho de LoginDialog::onEntrar, e passada ao open()
    // que antecede este rekey()); rekey() só troca a chave que o PRÓXIMO
    // save()/close() vai usar para CIFRAR — o payload plaintext do `.db` de
    // trabalho nunca é relido nem re-derivado, só a chave de saída muda.
    // Combinado com setVaultHeader(header) (chamar ANTES do PRÓXIMO
    // save()/close()), materializa o `.pbd` v2 com o `K` novo sem qualquer
    // perda de dado.
    //
    // Exige DB aberto (isOpen()) — retorna false e não faz nada se não
    // houver sessão (a chamada é sempre logicamente posterior a um open()
    // bem-sucedido). Zera (sodium_memzero) a chave antiga antes de
    // substituí-la — mesma disciplina de close(). Não confundir com
    // resetPasswordViaRecovery/completeFirstLogin (Back/UserAccountService):
    // nenhum dos dois muda `K` (só re-embrulham `K` sob uma senha/params
    // novos) — rekey() é o ÚNICO caminho que troca `K` sem reabrir o
    // arquivo.
    [[nodiscard]] bool rekey(const QByteArray &newKey);

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    void createSchema();
    bool encryptAndSave(const QString &srcDbPath, const QByteArray &key,
                        const QString &destPath);
    bool decryptAndLoad(const QString &encPath, const QString &dbPath,
                        const QByteArray &keyNew, const QByteArray &keyLegacy);

    QSqlDatabase m_db;
    QString      m_dbPath;
    QString      m_slug;
    QByteArray   m_key;        // Argon2id key (new format)
    QByteArray   m_keyLegacy;  // PBKDF2 key (legacy format — cleared after migration)
    bool         m_needsReencrypt = false; // true when a legacy file was opened

    // (ADR-001 §8 passo 6/D4, Fatia 4) Header `.pbd` v2 da sessão ATUAL —
    // vazio/false por padrão (v1 puro). Ver setVaultHeader()/open()/
    // decryptAndLoad()/close() para o ciclo de vida completo.
    VaultHeaderData m_vaultHeader;
    bool            m_vaultIsV2 = false;

    static std::vector<SchemaProvider> &schemaProviders();
};
