#include "services/CotahistProvider.h"
#include "repositories/PriceRepository.h"
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QTextCodec>

CotahistProvider::CotahistProvider(QObject *parent)
    : IMarketDataProvider(parent)
    , m_networkManager(nullptr)
{
    // Cache em subpasta dentro do diretório de dados da aplicação
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_downloadPath = dataDir + "/marketdata/cotahist";
    QDir dir(m_downloadPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

CotahistProvider::~CotahistProvider()
{
    // Cancelar downloads ativos
    for (auto *reply : m_activeDownloads) {
        if (reply && !reply->isFinished()) {
            reply->abort();
        }
    }
    if (m_networkManager) {
        delete m_networkManager;
    }
}

QString CotahistProvider::name() const
{
    return "B3 COTAHIST Histórico";
}

bool CotahistProvider::requiresAuth() const
{
    return false; // gratuito, sem autenticação
}

int CotahistProvider::typicalDelayMinutes() const
{
    return 0; // dados históricos, sem delay
}

// ====================================================================
// IMarketDataProvider interface
// ====================================================================

void CotahistProvider::fetchQuote(const QString &ticker,
                                  std::function<void(const CotacaoRT&)> callback)
{
    Q_UNUSED(ticker);
    qWarning() << "CotahistProvider não implementa fetchQuote (use B3DirectQuoteProvider para tempo real)";
    callback(CotacaoRT{});
}

void CotahistProvider::fetchOHLCV(const QString &ticker,
                                  const QDate &inicio,
                                  const QDate &fim,
                                  const QString &intervalo,
                                  std::function<void(const QList<PrecoAtivo>&)> callback)
{
    Q_UNUSED(intervalo);
    // 1) Verifica se já temos os dados no banco local
    auto dadosBanco = m_repo.findByTickerRange(ticker, inicio, fim);
    if (!dadosBanco.isEmpty()) {
        qDebug() << "Dados históricos encontrados no banco para" << ticker
                 << "(" << dadosBanco.size() << "registros)";
        callback(dadosBanco);
        return;
    }

    // 2) Determina anos necessários para download
    QList<int> anosNecessarios;
    for (int ano = inicio.year(); ano <= fim.year(); ++ano) {
        QDate anoInicio(ano, 1, 1);
        QDate anoFim(ano, 12, 31);
        if (anoFim < inicio || anoInicio > fim) continue;
        QString zipPath = QString("%1/COTAHIST_A%2.ZIP").arg(m_downloadPath).arg(ano);
        if (!QFile::exists(zipPath)) {
            anosNecessarios.append(ano);
        }
    }

    // 3) Baixar anos faltantes
    for (int ano : anosNecessarios) {
        QUrl url(QString("https://bvmf.bmfbovespa.com.br/InstDados/SerHist/COTAHIST_A%1.ZIP").arg(ano));
        QString zipPath = QString("%1/COTAHIST_A%2.ZIP").arg(m_downloadPath).arg(ano);
        if (!downloadAndExtractZip(url, zipPath)) {
            qWarning() << "Falha ao baixar COTAHIST" << ano;
        }
    }

    // 4) Processar arquivos locais (descomprimidos ou ZIP?)
    // Vamos descomprimir inline usando miniz ou QProcess unzip
    // Por enquanto, vamos processar apenas se o arquivo descomprimido .TXT existir
    QList<PrecoAtivo> dados;
    for (int ano = inicio.year(); ano <= fim.year(); ++ano) {
        QString txtPath = QString("%1/COTAHIST_A%2.TXT").arg(m_downloadPath).arg(ano);
        QString zipPath = QString("%1/COTAHIST_A%2.ZIP").arg(m_downloadPath).arg(ano);

        // Se existe ZIP mas não existe TXT, descompactar
        if (QFile::exists(zipPath) && !QFile::exists(txtPath)) {
            // Usar QProcess com unzip
            QProcess unzip;
            unzip.setWorkingDirectory(m_downloadPath);
            QStringList args;
            args << "-p" << zipPath; // extrair para stdout
            unzip.start("unzip", args);
            if (!unzip.waitForFinished()) {
                qWarning() << "unzip falhou para" << zipPath << ":" << unzip.errorString();
                continue;
            }
            QByteArray out = unzip.readAllStandardOutput();
            QFile outFile(txtPath);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(out);
                outFile.close();
            } else {
                qWarning() << "Não foi possível escrever" << txtPath;
                continue;
            }
        }

        if (QFile::exists(txtPath)) {
            processCotahistFile(txtPath, [&dados](const QList<PrecoAtivo> &lote) {
                dados.append(lote);
            });
        }
    }

    // 5) Upsert batch no banco
    if (!dados.isEmpty()) {
        m_repo.upsertBatch(dados);
    }

    // 6) Retornar dados do banco (agora devem estar lá)
    auto resultado = m_repo.findByTickerRange(ticker, inicio, fim);
    callback(resultado);
}

void CotahistProvider::search(const QString &query,
                              std::function<void(const QStringList&)> callback)
{
    auto todosTickers = m_repo.tickersComHistorico();
    QStringList resultado;
    for (const QString &ticker : todosTickers) {
        if (ticker.contains(query, Qt::CaseInsensitive)) {
            resultado.append(ticker);
        }
    }
    callback(resultado);
}

// ====================================================================
// Download & extração
// ====================================================================

void CotahistProvider::downloadHistoricalData(int years)
{
    QDate current = QDate::currentDate();
    int thisYear = current.year();
    for (int i = 0; i < years; ++i) {
        int ano = thisYear - i;
        QUrl url(QString("https://bvmf.bmfbovespa.com.br/InstDados/SerHist/COTAHIST_A%1.ZIP").arg(ano));
        QString zipPath = QString("%1/COTAHIST_A%2.ZIP").arg(m_downloadPath).arg(ano);
        if (!QFile::exists(zipPath)) {
            downloadAndExtractZip(url, zipPath);
        } else {
            qDebug() << "Arquivo já existe:" << zipPath;
        }
    }
}

bool CotahistProvider::downloadAndExtractZip(const QUrl &url, const QString &destPath)
{
    if (!m_networkManager) {
        m_networkManager = new QNetworkAccessManager(this);
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QNetworkReply *reply = m_networkManager->get(request);
    m_activeDownloads.insert(destPath, reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply, destPath]() {
        m_activeDownloads.remove(destPath);
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Erro no download de" << url << ":" << reply->errorString();
            emit onError(reply->error());
            return;
        }

        QByteArray zipData = reply->readAll();
        QFile outFile(destPath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            qWarning() << "Não foi possível salvar" << destPath;
            return;
        }
        outFile.write(zipData);
        outFile.close();

        qDebug() << "Download concluído:" << destPath;

        // Extrair o ZIP imediatamente
        QProcess unzip;
        unzip.setWorkingDirectory(m_downloadPath);
        QStringList args;
        args << "-p" << destPath; // extrair para stdout
        unzip.start("unzip", args);
        if (!unzip.waitForFinished()) {
            qWarning() << "Falha ao extrair" << destPath << ":" << unzip.errorString();
        } else {
            qDebug() << "Arquivo extraído em" << m_downloadPath;
        }

        emit onHistoricalDataReady();
    });

    return true;
}

void CotahistProvider::processCotahistFile(const QString &filePath,
                                           std::function<void(const QList<PrecoAtivo>&)> callback)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Não foi possível abrir" << filePath;
        return;
    }

    QTextStream in(&file);
    // COTAHIST usa codificação Windows-1252 (ISO-8859-1)
    in.setCodec(QTextCodec::codecForName("Windows-1252"));

    QList<PrecoAtivo> lote;
    int linhaNum = 0;

    while (!in.atEnd()) {
        QString line = in.readLine();
        linhaNum++;
        if (!isValidRecord(line)) {
            continue;
        }

        PrecoAtivo p = parseCotahistLine(line);
        if (p.ticker.isEmpty()) {
            continue;
        }

        lote.append(p);

        if (lote.size() >= 10000) {
            if (callback) callback(lote);
            lote.clear();
        }
    }

    if (!lote.isEmpty()) {
        if (callback) callback(lote);
    }

    file.close();

    qDebug() << "Processado" << linhaNum << "linhas em" << filePath
             << "→" << lote.size() << "registros válidos";
}

PrecoAtivo CotahistProvider::parseCotahistLine(const QString &line)
{
    PrecoAtivo p;
    if (line.length() < 200) return p;

    // Tipo (1-indexed: 1-2)
    QString tipo = line.mid(0, 2);
    if (tipo != "01") return p;

    // Data do pregão (positions 3-10, 0-indexed mid(2,8))
    QString dataStr = line.mid(2, 8);
    QDate data = QDate::fromString(dataStr, "yyyyMMdd");
    if (!data.isValid()) return p;

    // Ticker (positions 13-24, 0-indexed mid(12,12))
    QString ticker = line.mid(12, 12).trimmed();
    if (ticker.isEmpty()) return p;

    // Helper para converter string monetária (ex: "  12345,67") para centavos
    auto monetToCents = [](const QString &s) -> qint64 {
        if (s.isEmpty()) return 0;
        QString limpo = s.trimmed();
        if (limpo.length() < 3) return 0;
        // Separar parte inteira e decimal (2 dígitos)
        QString parteInt = limpo.left(limpo.length() - 2);
        QString parteDec = limpo.mid(limpo.length() - 2);
        bool ok1, ok2;
        int i = parteInt.toInt(&ok1);
        int d = parteDec.toInt(&ok2);
        if (!ok1 || !ok2) return 0;
        return static_cast<qint64>(i) * 100 + d;
    };

    // Helper para volume (inteiro longo)
    auto strToLongLong = [](const QString &s) -> qint64 {
        if (s.isEmpty()) return 0;
        QString limpo = s.trimmed();
        bool ok;
        qint64 v = limpo.toLongLong(&ok);
        return ok ? v : 0;
    };

    // Preços: abertura (57-69), máxima (70-82), mínima (83-95), último (96-108)
    // 0-indexed: abertura mid(56,13), maxima mid(69,13), minimas mid(82,13), ultimo mid(95,13)
    QString aberturaStr = line.mid(56, 13);
    QString maximaStr   = line.mid(69, 13);
    QString minimaStr   = line.mid(82, 13);
    QString ultimoStr   = line.mid(95, 13);
    // Volume: positions 171-188, 0-indexed mid(170, 18)
    QString volumeStr   = line.mid(170, 18);

    p.ticker = ticker;
    p.data = data;
    p.abertura = monetToCents(aberturaStr);
    p.maxima = monetToCents(maximaStr);
    p.minima = monetToCents(minimaStr);
    p.fechamento = monetToCents(ultimoStr);
    p.volume = strToLongLong(volumeStr);
    p.fonte = "COTAHIST_B3";
    p.atualizado = QDateTime::currentDateTime();

    return p;
}

bool CotahistProvider::isValidRecord(const QString &line) const
{
    if (line.length() < 200) {
        return false;
    }
    QString tipo = line.mid(0, 2);
    return (tipo == "01"); // apenas registros de preço
}

// ====================================================================
// Signal slots (simulated)
// ====================================================================
void CotahistProvider::onDownloadFinished()
{
    // Notificação interna; não usado
}

void CotahistProvider::onHistoricalDataReady()
{
    qDebug() << "Dados históricos COTAHIST disponíveis.";
}

void CotahistProvider::onError(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    qWarning() << "Erro no provider COTAHIST";
}

#include "services/CotahistProvider.moc"
