// PeuBud - #193 (Onda U, DIST-02): AboutDialog legalmente/tecnicamente incorreto.
//
// Antes do fix: `lblLic` afirmava que a AGPL-3.0 proibe uso comercial (FALSO
// — a AGPL PERMITE uso comercial, so exige disponibilizar o codigo-fonte,
// inclusive em uso via rede) e `lblDesc` descrevia a cripto ATIVA como
// AES-256-CBC (esse e o formato LEGADO, so leitura; a cripto ativa e AEAD
// AES-256-GCM/XChaCha20-Poly1305 + Argon2id — ver SodiumCryptoService).
//
// Contrato apos o fix:
//   (1) Nenhum QLabel do dialogo afirma "uso comercial ... proibido"
//       (nem a variante em ingles "commercial ... forbidden").
//   (2) A cripto ATIVA (AEAD + Argon2id) e mencionada em algum QLabel.
//   (3) Toda ocorrencia de "AES-256-CBC" no texto vem qualificada como
//       formato legado (nao pode aparecer como se fosse a cripto ativa).
//   (4) Gate regex ESTATICO: varre AboutDialog.cpp e peubud_en.ts (fonte E
//       traducao, para nao divergir de novo como no #154) atras dos mesmos
//       padroes falsos — barra regressao mesmo que o texto real da UI mude
//       de widget/objectName no futuro.
//
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QRegularExpression>
#include <QString>

#include "ui/widgets/AboutDialog.h"

#ifndef PEUBUD_SOURCE_DIR
#define PEUBUD_SOURCE_DIR "."
#endif

namespace {

// "comercial"/"commercial" a menos de 80 chars de "proibid"/"forbidden"
// (em qualquer ordem) = a alegacao falsa que a AGPL proibiria uso comercial.
bool contemAlegacaoAgplFalsa(const QString &texto)
{
    static const QRegularExpression rxPt(
        QStringLiteral("comercial[\\s\\S]{0,80}proibid|proibid[\\s\\S]{0,80}comercial"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression rxEn(
        QStringLiteral("commercial[\\s\\S]{0,80}forbidden|forbidden[\\s\\S]{0,80}commercial"),
        QRegularExpression::CaseInsensitiveOption);
    return texto.contains(rxPt) || texto.contains(rxEn);
}

// Toda ocorrencia de AES-256-CBC precisa ter "legad"/"legacy" a menos de
// 60 chars — senao esta sendo apresentada como se fosse a cripto ativa.
bool aesCbcSemQualificadorDeLegado(const QString &texto)
{
    static const QRegularExpression rxAes(
        QStringLiteral("AES-256-CBC"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression rxQualificador(
        QStringLiteral("legad|legacy"), QRegularExpression::CaseInsensitiveOption);

    auto it = rxAes.globalMatch(texto);
    while (it.hasNext()) {
        const auto m = it.next();
        const int ini = qMax(0, m.capturedStart() - 60);
        const int fim = qMin(texto.size(), m.capturedEnd() + 60);
        const QString contexto = texto.mid(ini, fim - ini);
        if (!contexto.contains(rxQualificador)) {
            return true; // achou AES-256-CBC "solto" — falha
        }
    }
    return false;
}

QString lerArquivo(const QString &caminho)
{
    QFile f(caminho);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromUtf8(f.readAll());
}

} // namespace

class TestAboutDialogRender : public QObject {
    Q_OBJECT

private slots:
    void renderNaoNulo();
    void semAlegacaoAgplFalsaNosLabels();
    void criptoAtivaAeadArgon2idDescrita();
    void aesCbcSoAparecemQualificadaComoLegado();
    void gateRegexFonteCppSemTextoFalso();
    void gateRegexTraducaoEnSemTextoFalso();
};

void TestAboutDialogRender::renderNaoNulo()
{
    AboutDialog dlg;
    QVERIFY2(&dlg != nullptr, "AboutDialog nao construiu");
}

void TestAboutDialogRender::semAlegacaoAgplFalsaNosLabels()
{
    AboutDialog dlg;
    const auto labels = dlg.findChildren<QLabel *>();
    for (auto *lbl : labels) {
        QVERIFY2(!contemAlegacaoAgplFalsa(lbl->text()),
                 qPrintable(QStringLiteral(
                     "QLabel afirma que AGPL proibe uso comercial (FALSO — #193): '%1'")
                     .arg(lbl->text())));
    }
}

void TestAboutDialogRender::criptoAtivaAeadArgon2idDescrita()
{
    AboutDialog dlg;
    const auto labels = dlg.findChildren<QLabel *>();
    bool mencionaAead = false;
    bool mencionaArgon2id = false;
    for (auto *lbl : labels) {
        if (lbl->text().contains(QStringLiteral("AEAD"), Qt::CaseInsensitive)) {
            mencionaAead = true;
        }
        if (lbl->text().contains(QStringLiteral("Argon2id"), Qt::CaseInsensitive)) {
            mencionaArgon2id = true;
        }
    }
    QVERIFY2(mencionaAead, "Nenhum label menciona AEAD — cripto ativa nao descrita (#193)");
    QVERIFY2(mencionaArgon2id, "Nenhum label menciona Argon2id — derivacao de chave ativa nao descrita (#193)");
}

void TestAboutDialogRender::aesCbcSoAparecemQualificadaComoLegado()
{
    AboutDialog dlg;
    const auto labels = dlg.findChildren<QLabel *>();
    for (auto *lbl : labels) {
        QVERIFY2(!aesCbcSemQualificadorDeLegado(lbl->text()),
                 qPrintable(QStringLiteral(
                     "AES-256-CBC aparece sem qualificador 'legado' — parece a cripto ativa (#193): '%1'")
                     .arg(lbl->text())));
    }
}

void TestAboutDialogRender::gateRegexFonteCppSemTextoFalso()
{
    const QString caminho = QStringLiteral(PEUBUD_SOURCE_DIR)
        + QStringLiteral("/src/ui/widgets/AboutDialog.cpp");
    const QString conteudo = lerArquivo(caminho);
    QVERIFY2(!conteudo.isEmpty(), qPrintable(QStringLiteral("Nao consegui ler %1").arg(caminho)));

    QVERIFY2(!contemAlegacaoAgplFalsa(conteudo),
             "AboutDialog.cpp contem a alegacao falsa 'comercial...proibido' (#193)");
    QVERIFY2(!aesCbcSemQualificadorDeLegado(conteudo),
             "AboutDialog.cpp contem 'AES-256-CBC' sem qualificar como legado (#193)");
}

void TestAboutDialogRender::gateRegexTraducaoEnSemTextoFalso()
{
    const QString caminho = QStringLiteral(PEUBUD_SOURCE_DIR)
        + QStringLiteral("/resources/translations/peubud_en.ts");
    const QString conteudo = lerArquivo(caminho);
    QVERIFY2(!conteudo.isEmpty(), qPrintable(QStringLiteral("Nao consegui ler %1").arg(caminho)));

    QVERIFY2(!contemAlegacaoAgplFalsa(conteudo),
             "peubud_en.ts contem a alegacao falsa 'commercial...forbidden' (#193, sincronizar com #154)");
    QVERIFY2(!aesCbcSemQualificadorDeLegado(conteudo),
             "peubud_en.ts contem 'AES-256-CBC' sem qualificar como legado (#193, sincronizar com #154)");
}

QTEST_MAIN(TestAboutDialogRender)
#include "test_about_dialog_render.moc"
