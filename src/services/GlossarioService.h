#pragma once

#include <QString>
#include <QStringList>

class QWidget;

// GlossarioService (#103) - dicionario pt-BR de termos financeiros.
//
// Resolve o problema: app tem 50+ siglas que pessoa nao-tecnica nao entende.
// Mitigacao em 3 camadas (#103-#112):
//   1. Tooltips automaticos em QLabel/QPushButton (este servico)
//   2. Microcopy explicativo abaixo de campos (#105 usa `microcopy`)
//   3. Renomear labels para coloquial + sigla parenteses (#111)
//
// Stateless. Tabela compile-time linear (80+ entradas) - lookup O(n) ok
// para uso esporadico em construcao de widgets.
class GlossarioService
{
public:
    // Definicao completa do termo. Ex.: "CET" -> "Custo Efetivo Total:
    // taxa anual que inclui juros + tarifas + IOF". Vazio se desconhecido.
    [[nodiscard]] static QString definicao(const QString &termo);

    // Versao curta (1 linha, usada em microcopy abaixo de campo).
    // Ex.: "CET" -> "= juros total ao ano (com IOF e tarifas)".
    [[nodiscard]] static QString microcopy(const QString &termo);

    // Label coloquial: termo amigavel + sigla entre parenteses pequenos.
    // Ex.: "FIRE" -> "Aposentadoria Antecipada (FIRE)".
    // Usado em modo Iniciante (#111). Vazio se mesmo nome.
    [[nodiscard]] static QString labelColoquial(const QString &termo);

    // Termos cadastrados. Util para teste/diagnostico + #107 glossario dialog.
    [[nodiscard]] static QStringList termos();

    // Verifica se string contem alguma sigla cadastrada (procura case-insensitive
    // por word-boundaries). Util para varrer labels existentes.
    [[nodiscard]] static QString primeiroTermoEncontrado(const QString &texto);

    // Helper: percorre `findChildren<QLabel*>` e aplica tooltip automatico
    // quando o texto bate com algum termo do glossario. Chamar uma vez apos
    // construir o widget.
    static void aplicarTooltips(QWidget *root);
};
