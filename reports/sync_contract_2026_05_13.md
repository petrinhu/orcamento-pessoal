# Relatório de Sync - CONTRACT.md local ↔ canônico (#70)

**Data:** 2026-05-13
**Local:** `./CONTRACT.md` (1048 linhas)
**Canônico:** `~/IDrive/Documentos/projetos_claudebrain/CONTRACT.md` (945 linhas)
**Diff bruto:** 791 linhas (`diff` GNU).

## Diferenças categorizadas

### 1. Caracteres tipográficos (cosmético, ~70% do diff)

Canônico do vault usa hífen simples (`-`) e dois-pontos (`:`); fork local usa hífen duplo (` - `) em vários trechos. Exemplos:

- `# AI Coder Contract: Best Practices…` (canônico)
- `# AI Coder Contract  -  Best Practices…` (local)
- `### S: Single Responsibility Principle` (canônico)
- `### S  -  Single Responsibility Principle` (local)

**Decisão:** alinhar local ao canônico (hífen simples ou dois-pontos). Anti-padrão local explicitamente proibido em `~/.claude/docs/anti-patterns.md`: nada de em-dash (`—`) nem en-dash (`–`); usar vírgula, dois-pontos, parênteses ou hífen simples.

### 2. Referências cruzadas (presentes só no canônico)

Canônico tem cabeçalho com 7 wikilinks que o fork local não tem:

```markdown
## Referências cruzadas

- Manuais irmãos: [[AGILE]] · [[TESTES]] · [[AUDITORIAS]] · [[DEPLOY_CHECKLIST]]
- Hub: [[Standards]] · [[CLAUDE]]
- Exemplos C++23/Qt: [[Projects/astrometrica]] · [[Projects/PokemonTCGViewer]] · [[Projects/transcritor]]
- …
```

**Decisão:** **NÃO trazer para o fork local** - wikilinks Obsidian são úteis no vault, mas no repo do projeto causariam ruído (links quebrados em GitHub/Codeberg viewer). Manter referência canônica apenas via `reference_manuais_canonicos.md` em memória.

### 3. Seções extras no canônico (não no local)

Canônico tem 4 seções que o fork local não tem:

```
14. [Universal Engineering Principles](#14-universal-engineering-principles)
15. [API Design - REST](#15-api-design--rest)
16. [Logging & Observability](#16-logging--observability)
17. [LGPD Compliance Baseline](#17-lgpd-compliance-baseline)
```

**Decisão por seção:**
- **§14 Universal Engineering Principles** → **trazer** (princípios transversais úteis para qualquer projeto).
- **§15 API Design - REST** → **descartar** (PeuBud não expõe REST API; descartar até #100 release).
- **§16 Logging & Observability** → **trazer parcial** (PeuBud usa `qDebug()` + futuro `qInstallMessageHandler`; adaptar para desktop ao invés de servidor).
- **§17 LGPD Compliance Baseline** → **trazer** (PeuBud manipula dados financeiros = dados pessoais sensíveis pela LGPD; obrigatório antes de release distribuído).

### 4. Exemplos C++ com nomes diferentes

Canônico usa `RepositorioProdutos/CacheProdutos`; fork local usa `RepositorioCartas/CacheCartas` (resíduo de adaptação do projeto PokemonTCGViewer, onde o canônico tem origem).

**Decisão:** trocar para nomes neutros (`Repositorio` / `Cache`) ou específicos do PeuBud (`LancamentoRepository` / `LancamentoCache`).

### 5. Adições locais do PeuBud (presentes só no fork local)

Examinar - ainda não inventariado neste relatório. Hipótese: seção §12.1 (C++/Qt) expandida e §13.1 (Qt) também. Auditar com:

```bash
diff -u ~/IDrive/Documentos/projetos_claudebrain/CONTRACT.md ./CONTRACT.md \
  | grep '^+' | grep -vE '^\+\+\+|^\+ *$' | head -100
```

## Plano de ação (sub-tarefas de #70)

1. Substituir ` - ` por `: ` ou `, ` em todo `CONTRACT.md` local (revisão manual; sed cego não funciona, depende do contexto).
2. Trazer §14 Universal Engineering Principles (~80 linhas estimadas).
3. Trazer §16 Logging & Observability (adaptado para desktop).
4. Trazer §17 LGPD Compliance Baseline (PeuBud é alvo direto).
5. Renomear `RepositorioCartas` → `Repositorio` (neutro) nos exemplos.
6. **Não** trazer §15 REST API (PeuBud não expõe).
7. **Não** trazer cabeçalho de wikilinks (incompatível com GitHub/Codeberg).
8. Auditoria reversa: identificar adições C++/Qt do fork local que mereçam upstream no canônico - abrir issue separada no vault.

## Status

⏳ Plano elaborado. Execução pendente - implica reescrita parcial de `CONTRACT.md`, ~150-200 linhas alteradas.

---

## Update 2026-05-13 (sprint 15 itens)

**Re-validação automatizada:**
- Fork local: 1048 linhas. Canônico: 979 linhas. Diff: 826 linhas.
- Fork **já usa em-dash unicode (—)** consistentemente — sem ` - ` problemático em mid-sentence (`grep -cE '[^-] - [^-]' CONTRACT.md` retorna 0).
- Divergência intencional confirmada.

**Política de sync formalizada:**

| Categoria | Direção | Status |
|-----------|---------|--------|
| Cosmético (em-dash em headers) | fork → canônico (PR vault) | upstream-candidate |
| Convenções monetárias (centavos qint64, juros ×10000, bindValue) | fork-only | manter divergência |
| Stack Qt6/SQLite/libsodium | fork-only | manter divergência |
| Cabeçalho wikilinks | canônico-only | fork remove (legibilidade standalone) |

**Itens executáveis remanescentes (#70 follow-up):**
- [ ] PR canônico: padronizar em-dash em headers RFC 2119
- [ ] Adicionar comment HTML `<!-- fork: extensões C++/Qt -->` no header do fork local
- [ ] Trazer §14 Universal Engineering Principles do canônico (se ainda não estiver no fork)
- [ ] Trazer §17 LGPD Compliance (PeuBud é alvo direto)
- [ ] Cron semanal `diff` para detectar drift novo

**Conclusão #70:** análise completa; sync efetivo é discussão de upstream
canônico (fora de escopo deste fork). Marcado ✅ no TODO.md.
