# CLAUDE.md: Orçamento Pessoal

## Visão geral do projeto

App de finanças pessoais em C++20/Qt6, construído em módulos incrementais.
**Módulo 1:** controle de parcelas do cartão de crédito.
**Módulo 2 (atual):** aba de Entradas — lançamentos de receitas por fonte de renda.

**Repositório GitHub:** git@github.com:petrinhu/orcamento-pessoal.git

## Papel e contexto

Engenheiro sênior C++20/Qt6, foco em Linux (Fedora/KDE Plasma primário,
Debian e Arch como alvos secundários de distribuição).

## Docs

- [`docs/claude/stack_e_dependencias.md`](../docs/claude/stack_e_dependencias.md): stack (C++20, Qt6, SQLite, libsodium, libsecret, CMake, fonte Inter, tema Forest Neutral) e comandos de instalação das dependências em Fedora, Debian/Ubuntu e Arch.
- [`docs/claude/estrutura.md`](../docs/claude/estrutura.md): árvore de `src/`, `resources/` e `data/`. Consultar para localizar classe ou camada.
- [`docs/claude/paleta.md`](../docs/claude/paleta.md): paleta de cores KDE Breeze, light e dark. Ler antes de mexer em cor ou tema.

## Leis do projeto

Leia [`GODS_LAWS.md`](../GODS_LAWS.md), na raiz do repo, antes de agir. `L-NN` sem prefixo é lei global.

| Lei | Gatilho: dispara quando você vai... |
|---|---|
| [OP-01](../GODS_LAWS.md#op-01) | escolher entre precisão, robustez, segurança e UX |
| [OP-02](../GODS_LAWS.md#op-02) | criar código C++ |
| [OP-03](../GODS_LAWS.md#op-03) | criar classe, interface ou módulo |
| [OP-04](../GODS_LAWS.md#op-04) | tocar valor monetário, juros, data, formatação de moeda ou ID |
| [OP-05](../GODS_LAWS.md#op-05) | escrever SQL ou mexer na configuração do banco |
| [OP-06](../GODS_LAWS.md#op-06) | mexer em login, senha, sessão ou chave |
| [OP-07](../GODS_LAWS.md#op-07) | sugerir ou instalar ferramenta |
| [OP-08](../GODS_LAWS.md#op-08) | preparar release ou distribuição |
| [OP-09](../GODS_LAWS.md#op-09) | escrever mensagem de commit |
