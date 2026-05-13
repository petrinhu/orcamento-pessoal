#include "BackupService.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QSettings>
#include <QStandardPaths>
#include <algorithm>

namespace {
constexpr auto KEY_DESTINO    = "backup/destino";
constexpr auto KEY_FREQUENCIA = "backup/frequencia";
constexpr auto KEY_RETENCAO   = "backup/retencao";
constexpr auto KEY_ULTIMO     = "backup/ultimo";

QString freqToString(BackupService::Frequencia f)
{
    switch (f) {
    case BackupService::Frequencia::Diaria:    return QStringLiteral("diaria");
    case BackupService::Frequencia::Semanal:   return QStringLiteral("semanal");
    case BackupService::Frequencia::Desativado:
    default:                                    return QStringLiteral("desativado");
    }
}

BackupService::Frequencia freqFromString(const QString &s)
{
    if (s == QLatin1String("diaria"))  return BackupService::Frequencia::Diaria;
    if (s == QLatin1String("semanal")) return BackupService::Frequencia::Semanal;
    return BackupService::Frequencia::Desativado;
}
}

BackupService::Config BackupService::load()
{
    QSettings s;
    Config c;
    c.destino    = s.value(KEY_DESTINO).toString();
    c.frequencia = freqFromString(s.value(KEY_FREQUENCIA, QStringLiteral("desativado")).toString());
    c.retencao   = s.value(KEY_RETENCAO, 7).toInt();
    c.ultimo     = s.value(KEY_ULTIMO).toDateTime();
    if (c.destino.isEmpty()) c.destino = diretorioPadrao();
    if (c.retencao < 1) c.retencao = 1;
    return c;
}

void BackupService::save(const Config &c)
{
    QSettings s;
    s.setValue(KEY_DESTINO,    c.destino);
    s.setValue(KEY_FREQUENCIA, freqToString(c.frequencia));
    s.setValue(KEY_RETENCAO,   c.retencao);
    if (c.ultimo.isValid()) {
        s.setValue(KEY_ULTIMO, c.ultimo);
    }
}

QString BackupService::diretorioPadrao()
{
    const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QString dir  = home + QStringLiteral("/Backups/peubud");
    QDir().mkpath(dir);
    return dir;
}

QString BackupService::runBackup(const Config &c, const QString &slug,
                                  const ExportFn &exporter)
{
    if (slug.isEmpty() || !exporter) return {};
    QString destino = c.destino;
    if (destino.isEmpty()) destino = diretorioPadrao();
    QDir().mkpath(destino);

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmm"));
    const QString nome  = QStringLiteral("peubud-%1-%2.pbd").arg(slug, stamp);
    const QString full  = QDir(destino).filePath(nome);

    if (!exporter(full)) return {};

    Config updated = c;
    updated.ultimo = QDateTime::currentDateTime();
    save(updated);
    pruneRetention(destino, slug, c.retencao);

    return full;
}

bool BackupService::shouldRunNow(const Config &c, const QDateTime &agora)
{
    if (c.frequencia == Frequencia::Desativado) return false;
    if (!c.ultimo.isValid()) return true; // nunca rodou
    const qint64 segundos = c.ultimo.secsTo(agora);
    if (segundos < 0) return false; // clock skew

    const qint64 dia    = 24 * 60 * 60;
    const qint64 semana = 7 * dia;
    switch (c.frequencia) {
    case Frequencia::Diaria:  return segundos >= dia;
    case Frequencia::Semanal: return segundos >= semana;
    default:                   return false;
    }
}

QStringList BackupService::listarBackups(const QString &destino, const QString &slug)
{
    if (destino.isEmpty() || slug.isEmpty()) return {};
    QDir dir(destino);
    const QString filtro = QStringLiteral("peubud-%1-*.pbd").arg(slug);
    auto entries = dir.entryInfoList(QStringList{filtro},
                                      QDir::Files | QDir::NoDotAndDotDot,
                                      QDir::NoSort);
    // Ordenacao explicita por mtime (mais recente primeiro). Evita variacao
    // entre filesystems / Qt sort flags que podem cair em desempate por nome.
    std::sort(entries.begin(), entries.end(),
              [](const QFileInfo &a, const QFileInfo &b) {
                  return a.lastModified() > b.lastModified();
              });
    QStringList result;
    result.reserve(entries.size());
    for (const auto &fi : entries) result.append(fi.absoluteFilePath());
    return result;
}

int BackupService::pruneRetention(const QString &destino, const QString &slug, int manter)
{
    if (manter <= 0) manter = 1;
    const QStringList lista = listarBackups(destino, slug);
    if (lista.size() <= manter) return 0;
    int removidos = 0;
    for (int i = manter; i < lista.size(); ++i) {
        if (QFile::remove(lista[i])) ++removidos;
    }
    return removidos;
}
