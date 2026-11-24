# GODS_LAWS.md: Orçamento Pessoal

> Ordens do líder para este projeto. Cada lei tem um gatilho, conferido no momento da ação.

## Protocolo de uso

1. Antes de agir, confira o índice de gatilhos do `.claude/CLAUDE.md` deste projeto; se um gatilho casa, leia a lei inteira antes do primeiro comando.
2. Siga o protocolo de uso do `~/.claude/GODS_LAWS.md`. No que este arquivo silencia, vale a lei global; `L-NN` sem prefixo é lei global e `OP-NN` é lei daqui.

---

## OP-01

**Data:** regra permanente.

Decida pela ordem de prioridade do projeto: **Precisão financeira > Robustez > Segurança > UX**.

## OP-02

**Data:** regra permanente.

**Não crie código C++ sem perguntar ao usuário antes.**

## OP-03

**Data:** regra permanente.

Siga SOLID. A responsabilidade única (SRP) é a da L-34 global. Além dela:

- **OCP:** deixe o app extensível para novos módulos sem modificar os existentes.
- **LSP:** faça cada implementação valer no lugar da sua interface sem quebrar comportamento.
- **ISP:** mantenha as interfaces pequenas e específicas (ICryptoService, ISecretStore, IRepository).
- **DIP:** dependa de abstrações, não de implementações concretas.

## OP-04

**Data:** regra permanente.

- Guarde valores monetários sempre em **centavos (qint64)**, nunca em float/double.
- Armazene juros como inteiro: `% × 10000` (ex: 1,5% = 15000).
- Use `QDate` internamente e `dd/MM/yyyy` na UI.
- Formate com `centavosParaTexto()` → "R$ 0,00" (o símbolo varia por moeda).
- Use os IDs assim: 0 = não persistido, -1 = linha TOTAL nas tabelas.

## OP-05

**Data:** regra permanente.

- Escreva toda query SQL com `bindValue`, sem concatenação de strings.
- Ligue `PRAGMA foreign_keys = ON`, com `ON DELETE CASCADE/SET NULL` nas FKs.
- Use `PRAGMA journal_mode = MEMORY`, sem journal files em disco.

## OP-06

**Data:** regra permanente.

Mantenha o login e a sessão assim:

- `#ifdef QT_DEBUG`: só nome, sem senha, último nome salvo via QSettings.
- Release: nome + senha + checklist (≥8 chars, maiúscula, número).
- Chave AES armazenada via libsecret (KWallet/GNOME Keyring).

## OP-07

**Data:** regra permanente.

Use **APENAS** ferramentas gratuitas, open-source, disponíveis via dnf/apt/pacman. Não sugira PVS-Studio, Coverity, SonarCloud pago, Squish nem qualquer tool com login ou chave paga. Ao recomendar tools, forneça sempre os equivalentes para as 3 distros (dnf/apt/pacman).

## OP-08

**Data:** regra permanente.

Antes de qualquer release, cubra este checklist dentro do portão único da L-35 (o CI do repo), e rode isolado, pela L-50, o que abre janela:

1. Análise estática: `clang-tidy`, `cppcheck`
2. Sanitizers: ASan, UBSan + `valgrind`
3. Segurança/CVE/hardening: `checksec`, `grype` ou `trivy`
4. QA funcional: `ctest`
5. UX/UI: GammaRay, revisão manual

Para cada item, dê o comando exato e a justificativa de por que é crítico.

## OP-09

**Data:** regra permanente.

Escreva os commits sempre em português.
