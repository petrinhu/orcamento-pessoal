// PeuBud - regressao #196 (Onda U, UX-01)
//
// Bug: DashboardWidget::atualizarDados() seta m_subParcelas com o texto real
// ("N parcela(s)"). MainWindow chama GlossarioDecorator::aplicarLabelsColoquiais
// (modo iniciante = default) DEPOIS que o card ja exibe o dado real; o
// mecanismo casa o termo "PARCELA" em "N parcela(s)" e reescreve com o rotulo
// coloquial fixo "Parcela mensal" — destruindo a contagem na 1a tela de todo
// usuario novo. Fix: makeCard() marca glossarioIgnore=true nos labels que
// cria (o mecanismo ja respeita a property).
//
// Sem abrir DB: DashboardWidget::atualizarDados() so agrega via repositorios
// que toleram QSqlDatabase invalida (q.exec() falha, listas ficam vazias —
// mesmo contrato usado pelos demais testes desta suite). O card de Parcelas
// fica com contagem 0, mas o texto "0 parcela(s)" ja e suficiente para
// reproduzir o bug (o termo "PARCELA" casa independente do numero).
//
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest>
#include <QApplication>
#include <QLabel>
#include <QWidget>

#include "core/settings/AppSettings.h"
#include "ui/GlossarioDecorator.h"
#include "ui/Theme.h"
#include "ui/widgets/DashboardWidget.h"

class TestDashboardGlossario : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void subLabelParcelasSobreviveAoGlossario();
};

void TestDashboardGlossario::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
    // modoIniciante=true e o default do app — e o cenario que dispara
    // aplicarLabelsColoquiais em MainWindow::MainWindow (linha 258).
    AppSettings::instance().setModoIniciante(true);
}

void TestDashboardGlossario::subLabelParcelasSobreviveAoGlossario()
{
    DashboardWidget dash;

    // Localiza o sub-label do card "Parcelas" pelo texto ("N parcela(s)"),
    // ja que m_subParcelas e privado — mesmo padrao de teste "por contrato
    // visivel" usado em test_cards_render.cpp/test_tag_render.cpp.
    QLabel *subParcelas = nullptr;
    const auto labels = dash.findChildren<QLabel *>();
    for (auto *l : labels) {
        if (l->text().contains(QStringLiteral("parcela(s)"), Qt::CaseInsensitive)) {
            subParcelas = l;
            break;
        }
    }
    QVERIFY2(subParcelas,
        "sub-label 'N parcela(s)' nao encontrado no DashboardWidget — "
        "contrato de texto do card Parcelas mudou?");

    const QString textoAntes = subParcelas->text();
    QVERIFY2(!textoAntes.isEmpty(), "sub-label de parcelas vazio antes do glossario");

    // Reproduz exatamente o passo que MainWindow::MainWindow faz apos criar
    // as abas (linha 258): aplicarLabelsColoquiais no widget inteiro.
    GlossarioDecorator::aplicarLabelsColoquiais(&dash);

    QCOMPARE(subParcelas->text(), textoAntes);
    QVERIFY2(!subParcelas->text().contains(QStringLiteral("Parcela mensal")),
        "#196 regressao: glossario sobrescreveu a contagem real do card "
        "Parcelas com o rotulo coloquial fixo 'Parcela mensal'");
}

QTEST_MAIN(TestDashboardGlossario)
#include "test_dashboard_glossario.moc"
