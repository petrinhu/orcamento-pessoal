#pragma once

class QWidget;

// GlossarioDecorator (#125) - camada Front (UI).
//
// Move para a camada de UI as funcoes que percorrem a arvore de QWidget e
// aplicam tooltips/labels/microcopy a partir do GlossarioService. O servico
// (camada Mid) so expoe DADOS puros (definicao/microcopy/labelColoquial/
// primeiroTermoEncontrado); a manipulacao de widgets vive aqui, eliminando o
// furo de camada Mid->Front.
//
// Stateless: funcoes livres. Chamar uma vez apos construir o widget root.
namespace GlossarioDecorator {

// aplicarTooltips (#125, atualizado #166 Peca 3 §9.6):
// - Nao-destrutivo: nunca altera text() do widget.
// - Seta toolTip() com definicao do glossario (tooltip escuro via QSS global).
// - Affordanca: cursor WhatsThisCursor (?) nos QLabel com tooltip — indica
//   "passe o mouse para saber mais".
// - Cue estatico (§9.6): nos QLabel-chave curtos (texto ~= termo), instala o
//   sublinhado pontilhado via GlossaryUnderlineFilter (event filter que pinta
//   por cima, sem trocar a classe nem mexer no layout). Cobertura AMPLA: mesma
//   varredura dos tooltips. Labels de prosa longa ficam so com tooltip (nao
//   risca a frase inteira). Botoes nao recebem sublinhado (ja tem chrome).
// - Idempotente: tooltip so quando vazio; sublinhado marcado por propriedade.
void aplicarTooltips(QWidget *root);

// aplicarLabelsColoquiais (#104):
// - Altera text() (DESTRUTIVO): troca o texto do widget pelo labelColoquial.
// - Usado apenas no modo Iniciante para renomear siglas para versao amigavel.
// - NAO substitui aplicarTooltips (§9.6): o tooltip escuro continua como
//   mecanismo principal de educacao contextual; este e complementar.
// - Mantido para compatibilidade. Callers (MainWindow) continuam chamando ambos.
// Ex.: "FIRE" -> "Aposentadoria Antecipada (FIRE)". No-op em modo Avancado. (#104)
void aplicarLabelsColoquiais(QWidget *root);

// Inspeciona QFormLayout em todos descendentes; para cada linha cujo label
// contem sigla cadastrada e que tem microcopy nao-vazio, insere uma linha
// logo abaixo com QLabel cinza pequeno (11px) em pt-BR plain. Idempotente
// (marca o QLabel inserido com property "microcopy" = true). (#105)
void aplicarMicrocopies(QWidget *root);

} // namespace GlossarioDecorator
