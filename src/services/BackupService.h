#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <functional>

// BackupService (#79) - backup automatico cifrado do banco.
//
// Reutiliza `DatabaseManager::exportToFile` para gerar copias cifradas
// (XChaCha20-Poly1305 + Argon2id) em diretorio configuravel. Aplica
// politica de retencao (mantem N copias mais recentes).
//
// Persiste estado via QSettings com prefixo "backup/":
//   backup/destino     (QString)  - diretorio dos arquivos
//   backup/frequencia  (QString)  - "diaria"|"semanal"|"desativado"
//   backup/retencao    (int)      - quantidade max de copias (default 7)
//   backup/ultimo      (QDateTime)- timestamp do ultimo backup bem-sucedido
//
// Nomeacao: "peubud-<slug>-YYYYMMDD-HHmm.pbd"
class BackupService
{
public:
    enum class Frequencia { Desativado, Diaria, Semanal };

    struct Config {
        QString     destino;       // diretorio (vazio = ~/Backups/peubud)
        Frequencia  frequencia = Frequencia::Desativado;
        int         retencao   = 7;
        QDateTime   ultimo;
    };

    // ── Persistencia (QSettings) ────────────────────────────────────────────
    [[nodiscard]] static Config load();
    static void save(const Config &c);

    // ── Operacoes ──────────────────────────────────────────────────────────

    // ExportFn: funcao que recebe caminho destino e cifra/grava o banco.
    // Default em runBackup() usa DatabaseManager::exportToFile; testes
    // podem injetar mock (lambda que cria arquivo dummy).
    using ExportFn = std::function<bool(const QString &)>;

    // Executa backup imediato via `exporter`. Retorna caminho final (vazio
    // em falha) e atualiza QSettings backup/ultimo. Faz prune apos export.
    [[nodiscard]] static QString runBackup(const Config &c,
                                            const QString &slug,
                                            const ExportFn &exporter);

    // True se ja passou o intervalo (diario/semanal) desde c.ultimo.
    // Frequencia=Desativado sempre retorna false.
    [[nodiscard]] static bool shouldRunNow(const Config &c,
                                            const QDateTime &agora = QDateTime::currentDateTime());

    // Lista arquivos `peubud-<slug>-*.pbd` em destino, ordenados por mtime
    // (mais recente primeiro).
    [[nodiscard]] static QStringList listarBackups(const QString &destino,
                                                    const QString &slug);

    // Apaga arquivos alem do limite de retencao. Retorna quantidade removida.
    static int pruneRetention(const QString &destino, const QString &slug, int manter);

    // Diretorio default (~/Backups/peubud), criado se nao existir.
    [[nodiscard]] static QString diretorioPadrao();
};
