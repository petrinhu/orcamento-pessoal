#include "services/CotahistProvider.h"
#include "services/CotahistParser.h"
#include "repositories/PriceRepository.h"
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QStringConverter>

namespace {
// (#203a/SEC-04) teto de tamanho da resposta do download COTAHIST. O ZIP
// anual real da B3 fica na casa de 40-60MB; 200MB dá margem generosa sem
// deixar o processo vulnerável a um endpoint comprometido/redirecionado
// devolvendo um payload sem limite (memory exhaustion via
// QNetworkReply::readAll() sem teto).
constexpr qint64 kCotahistMaxBytes = 200LL * 1024 * 1024;
}

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
    // (#203a/SEC-04) teto de redirecionamentos — a política padrão do Qt6
    // (NoLessSafeRedirectPolicy) já segue redirect automaticamente; sem
    // teto explícito o máximo default é 50 hops. `FollowRedirectsAttribute`
    // (usado aqui antes) foi removido no Qt6 — API atual é
    // setMaximumRedirectsAllowed().
    request.setMaximumRedirectsAllowed(5);
    QNetworkReply *reply = m_networkManager->get(request);
    m_activeDownloads.insert(destPath, reply);

    // (#203a/SEC-04) aborta o download se a resposta ultrapassar o teto de
    // bytes — protege contra memory exhaustion se o endpoint (ou um
    // redirect malicioso dentro do teto acima) responder com um payload
    // sem limite; `reply->readAll()` abaixo não teria teto próprio.
    connect(reply, &QNetworkReply::downloadProgress, this,
            [reply](qint64 bytesReceived, qint64 /*bytesTotal*/) {
        if (bytesReceived > kCotahistMaxBytes) {
            qWarning() << "CotahistProvider: download excedeu o teto de"
                       << kCotahistMaxBytes << "bytes — abortando";
            reply->abort();
        }
    });

    // (#203a) bug pré-existente adjacente: capture list não incluía `url`
    // (usado no qWarning abaixo) — erro de compilação real, não coberto
    // pelo escopo original de QA-01 ("erros de compilação Qt6 não
    // relacionados... fora de escopo"); corrigido aqui por estar dentro do
    // mesmo bloco tocado pelo fix de SEC-04.
    connect(reply, &QNetworkReply::finished, this, [this, reply, destPath, url]() {
        m_activeDownloads.remove(destPath);
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Erro no download de" << url << ":" << reply->errorString();
            emit onError(reply->error());
            return;
        }

        QByteArray zipData = reply->readAll();
        if (zipData.size() > kCotahistMaxBytes) {
            qWarning() << "CotahistProvider: resposta" << zipData.size()
                       << "bytes excede o teto de" << kCotahistMaxBytes
                       << "— descartando (não grava em disco)";
            return;
        }
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
    // COTAHIST usa codificação Windows-1252 (ISO-8859-1). Qt6 removeu
    // QTextCodec (Qt5) -- QStringConverter::Latin1 é a aproximação mais
    // próxima disponível sem depender de Qt6Core5Compat (não linkado neste
    // projeto); ticker/nome de ativo são ASCII na prática, sem perda real.
    in.setEncoding(QStringConverter::Latin1);

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

// #199 (QA-01): logica real movida para cotahist::parseLine (function livre,
// sem QObject/rede/DB -- ver services/CotahistParser.h/.cpp). Inclui o fix
// do campo `fechamento` (lia PREMED mid(95,13); correto e PREULT mid(108,13)).
PrecoAtivo CotahistProvider::parseCotahistLine(const QString &line)
{
    return cotahist::parseLine(line);
}

bool CotahistProvider::isValidRecord(const QString &line)
{
    return cotahist::isValidRecord(line);
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
