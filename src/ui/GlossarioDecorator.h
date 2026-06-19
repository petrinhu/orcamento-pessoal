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

// Percorre findChildren<QLabel*>/QPushButton* e aplica tooltip automatico
// quando o texto bate com algum termo do glossario. Idempotente (so escreve
// quando toolTip() esta vazio).
void aplicarTooltips(QWidget *root);

// Aplica labelColoquial em QLabel/QPushButton/QGroupBox/QCheckBox/QRadioButton
// quando o modo iniciante esta ativo. Ex.: "FIRE" -> "Aposentadoria
// Antecipada (FIRE)". No-op em modo Avancado. (#104)
void aplicarLabelsColoquiais(QWidget *root);

// Inspeciona QFormLayout em todos descendentes; para cada linha cujo label
// contem sigla cadastrada e que tem microcopy nao-vazio, insere uma linha
// logo abaixo com QLabel cinza pequeno (11px) em pt-BR plain. Idempotente
// (marca o QLabel inserido com property "microcopy" = true). (#105)
void aplicarMicrocopies(QWidget *root);

} // namespace GlossarioDecorator
