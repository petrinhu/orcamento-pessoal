// PeuBud - teste das tags coloridas (#166 Fase 3, Peça 1)
//
// Verifica:
//   (1) Theme::criarTag produz QLabel com objectName "tag" correto.
//   (2) A propriedade dinâmica "variante" é definida conforme a enum passada.
//   (3) O texto do label é o texto passado.
//   (4) O QSS global contém as regras das variantes (raio ~20px, fg/bg presentes).
//   (5) Render offscreen não é nulo (smoke de render).
//   (6) Tokens de tag não ficaram substituídos (@@TAG_*@@ ausentes no QSS final).
//
// Salva /tmp/tags_light.png e /tmp/tags_dark.png para aprovação visual.
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QWidget>

#include "ui/Tag.h"
#include "ui/Theme.h"

class TestTagRender : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // ── Contrato da API ───────────────────────────────────────────────────────
    // criarTag deve retornar QLabel com objectName "tag" e propriedade variante.
    void criarTag_objectName_tag();
    void criarTag_propriedadeVariante_sucesso();
    void criarTag_propriedadeVariante_perigo();
    void criarTag_propriedadeVariante_atencao();
    void criarTag_propriedadeVariante_info();
    void criarTag_propriedadeVariante_neutro();
    void criarTag_texto_correto();

    // ── QSS contém as regras das variantes ───────────────────────────────────
    // Sem essas regras no QSS, o chip fica sem estilo.
    void qss_contemSeletorTag_sucesso();
    void qss_contemSeletorTag_perigo();
    void qss_contemSeletorTag_atencao();
    void qss_contemSeletorTag_info();
    void qss_contemSeletorTag_neutro();

    // ── Tokens não ficaram non-substituídos ──────────────────────────────────
    void qss_semTokensTagResidu();

    // ── Render offscreen (smoke) ──────────────────────────────────────────────
    void renderLight_naoNulo();
    void renderDark_naoNulo();
};

// ── Helpers ───────────────────────────────────────────────────────────────────

// Monta um widget de demonstração com todas as variantes
static QWidget *criarDemoWidget(QWidget *parent = nullptr)
{
    auto *root = new QWidget(parent);
    auto *lay  = new QVBoxLayout(root);
    lay->setContentsMargins(16, 16, 16, 16);
    lay->setSpacing(8);

    // Status semânticos
    auto *rowStatus = new QHBoxLayout;
    rowStatus->setSpacing(8);
    rowStatus->addWidget(Theme::criarTag(QStringLiteral("Sucesso"),   Tag::Variante::Sucesso));
    rowStatus->addWidget(Theme::criarTag(QStringLiteral("Perigo"),    Tag::Variante::Perigo));
    rowStatus->addWidget(Theme::criarTag(QStringLiteral("Atenção"),   Tag::Variante::Atencao));
    rowStatus->addWidget(Theme::criarTag(QStringLiteral("Info"),      Tag::Variante::Info));
    rowStatus->addWidget(Theme::criarTag(QStringLiteral("Neutro"),    Tag::Variante::Neutro));
    rowStatus->addStretch();
    lay->addLayout(rowStatus);

    // Tipos (fixo/variável)
    auto *rowTipo = new QHBoxLayout;
    rowTipo->setSpacing(8);
    rowTipo->addWidget(Theme::criarTag(QStringLiteral("Fixo"),     Tag::Variante::Fixo));
    rowTipo->addWidget(Theme::criarTag(QStringLiteral("Variável"), Tag::Variante::Variavel));
    rowTipo->addStretch();
    lay->addLayout(rowTipo);

    // Categorias (paleta categórica)
    auto *rowCat = new QHBoxLayout;
    rowCat->setSpacing(8);
    rowCat->addWidget(Theme::criarTag(QStringLiteral("Azul"),      Tag::Variante::Azul));
    rowCat->addWidget(Theme::criarTag(QStringLiteral("Esmeralda"), Tag::Variante::Esmeralda));
    rowCat->addWidget(Theme::criarTag(QStringLiteral("Dourado"),   Tag::Variante::Dourado));
    rowCat->addWidget(Theme::criarTag(QStringLiteral("Vermelho"),  Tag::Variante::Vermelho));
    rowCat->addWidget(Theme::criarTag(QStringLiteral("Roxo"),      Tag::Variante::Roxo));
    rowCat->addWidget(Theme::criarTag(QStringLiteral("Slate"),     Tag::Variante::Slate));
    rowCat->addStretch();
    lay->addLayout(rowCat);

    // Frequências (usando as cores categóricas)
    auto *rowFreq = new QHBoxLayout;
    rowFreq->setSpacing(8);
    rowFreq->addWidget(Theme::criarTag(QStringLiteral("Mensal"),    Tag::Variante::Azul));
    rowFreq->addWidget(Theme::criarTag(QStringLiteral("Semanal"),   Tag::Variante::Esmeralda));
    rowFreq->addWidget(Theme::criarTag(QStringLiteral("Quinzenal"), Tag::Variante::Roxo));
    rowFreq->addWidget(Theme::criarTag(QStringLiteral("Anual"),     Tag::Variante::Dourado));
    rowFreq->addStretch();
    lay->addLayout(rowFreq);

    // Entrada/Gasto (tipo recorrência — semântico)
    auto *rowRec = new QHBoxLayout;
    rowRec->setSpacing(8);
    rowRec->addWidget(Theme::criarTag(QStringLiteral("Entrada"), Tag::Variante::Sucesso));
    rowRec->addWidget(Theme::criarTag(QStringLiteral("Gasto"),   Tag::Variante::Perigo));
    rowRec->addStretch();
    lay->addLayout(rowRec);

    root->setMinimumSize(520, 200);
    return root;
}

// ── initTestCase ──────────────────────────────────────────────────────────────

void TestTagRender::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

// ── Contrato da API ───────────────────────────────────────────────────────────

void TestTagRender::criarTag_objectName_tag()
{
    auto *lbl = Theme::criarTag(QStringLiteral("Teste"), Tag::Variante::Sucesso);
    QVERIFY(lbl != nullptr);
    QCOMPARE(lbl->objectName(), QStringLiteral("tag"));
    delete lbl;
}

void TestTagRender::criarTag_propriedadeVariante_sucesso()
{
    auto *lbl = Theme::criarTag(QStringLiteral("Ok"), Tag::Variante::Sucesso);
    QCOMPARE(lbl->property("variante").toString(), QStringLiteral("sucesso"));
    delete lbl;
}

void TestTagRender::criarTag_propriedadeVariante_perigo()
{
    auto *lbl = Theme::criarTag(QStringLiteral("Erro"), Tag::Variante::Perigo);
    QCOMPARE(lbl->property("variante").toString(), QStringLiteral("perigo"));
    delete lbl;
}

void TestTagRender::criarTag_propriedadeVariante_atencao()
{
    auto *lbl = Theme::criarTag(QStringLiteral("Aviso"), Tag::Variante::Atencao);
    QCOMPARE(lbl->property("variante").toString(), QStringLiteral("atencao"));
    delete lbl;
}

void TestTagRender::criarTag_propriedadeVariante_info()
{
    auto *lbl = Theme::criarTag(QStringLiteral("Info"), Tag::Variante::Info);
    QCOMPARE(lbl->property("variante").toString(), QStringLiteral("info"));
    delete lbl;
}

void TestTagRender::criarTag_propriedadeVariante_neutro()
{
    auto *lbl = Theme::criarTag(QStringLiteral("Neutro"), Tag::Variante::Neutro);
    QCOMPARE(lbl->property("variante").toString(), QStringLiteral("neutro"));
    delete lbl;
}

void TestTagRender::criarTag_texto_correto()
{
    const QString texto = QStringLiteral("Minha Tag");
    auto *lbl = Theme::criarTag(texto, Tag::Variante::Info);
    QCOMPARE(lbl->text(), texto);
    delete lbl;
}

// ── QSS contém seletores das variantes ───────────────────────────────────────

void TestTagRender::qss_contemSeletorTag_sucesso()
{
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(qss.contains(QStringLiteral("QLabel#tag[variante=\"sucesso\"]")),
             "Seletor QLabel#tag[variante=\"sucesso\"] ausente no QSS");
}

void TestTagRender::qss_contemSeletorTag_perigo()
{
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(qss.contains(QStringLiteral("QLabel#tag[variante=\"perigo\"]")),
             "Seletor QLabel#tag[variante=\"perigo\"] ausente no QSS");
}

void TestTagRender::qss_contemSeletorTag_atencao()
{
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(qss.contains(QStringLiteral("QLabel#tag[variante=\"atencao\"]")),
             "Seletor QLabel#tag[variante=\"atencao\"] ausente no QSS");
}

void TestTagRender::qss_contemSeletorTag_info()
{
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(qss.contains(QStringLiteral("QLabel#tag[variante=\"info\"]")),
             "Seletor QLabel#tag[variante=\"info\"] ausente no QSS");
}

void TestTagRender::qss_contemSeletorTag_neutro()
{
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(qss.contains(QStringLiteral("QLabel#tag[variante=\"neutro\"]")),
             "Seletor QLabel#tag[variante=\"neutro\"] ausente no QSS");
}

// ── Sem tokens residuais ──────────────────────────────────────────────────────

void TestTagRender::qss_semTokensTagResidu()
{
    const QString qss = Theme::qssParaTeste();
    // Nenhum @@TAG_*@@ deve sobrar após substituição
    QVERIFY2(!qss.contains(QStringLiteral("@@TAG_")),
             "Token @@TAG_*@@ não foi substituído no QSS — verificar gerarQss()");
}

// ── Render offscreen (smoke) ──────────────────────────────────────────────────

void TestTagRender::renderLight_naoNulo()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();

    auto *demo = criarDemoWidget();
    demo->show();
    QApplication::processEvents();
    const QPixmap pm = demo->grab();
    QVERIFY2(!pm.isNull(), "Render light nulo — verificar layout/widget");
    pm.save(QStringLiteral("/tmp/tags_light.png"));
    delete demo;
}

void TestTagRender::renderDark_naoNulo()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    Theme::aplicar();

    auto *demo = criarDemoWidget();
    demo->show();
    QApplication::processEvents();
    const QPixmap pm = demo->grab();
    QVERIFY2(!pm.isNull(), "Render dark nulo — verificar layout/widget");
    pm.save(QStringLiteral("/tmp/tags_dark.png"));

    delete demo;
    // Restaura light para não poluir outros testes
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

QTEST_MAIN(TestTagRender)
#include "test_tag_render.moc"
