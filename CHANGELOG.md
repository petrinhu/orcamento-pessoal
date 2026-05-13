# Changelog

Todas as mudanças notáveis do PeuBud (Personal Budget) são registradas neste arquivo.

Formato: [Keep a Changelog 1.1.0](https://keepachangelog.com/pt-BR/1.1.0/).
Versionamento: [SemVer 2.0.0](https://semver.org/lang/pt-BR/).

---

## [Não publicado]

### Added
- Bloco 6 do backlog (#96 a #101): empacotamento DEB, RPM, AUR, tar.gz; assinatura GPG; Codeberg Releases automatizado; repo APT+RPM auto-hostado via Codeberg Pages.
- Bloco 5 do backlog (#81 a #95): aprendizados extraídos da auditoria de `models/` (kmymoney, skrooge, gnucash, grisbi, bot-tesouro-direto, openclaw, Denaro, web-budget).
- Bloco 4 do backlog (#70 a #80): conformidade, infra e auditoria (sync CONTRACT, TESTES.md local, Codeberg Actions, clang-tidy/cppcheck, ASan CI, backup cifrado, atalhos globais).
- `CONTRACT.md` versionado no repo (fork local do manual canônico `[[CONTRACT]]` do vault segundo cérebro, 1048 linhas).
- `TODO.md` no formato canônico da skill `tab_pendencias` (colunas ID, Grupo, Descrição, Prioridade, Pré-requisito, Dificuldade, Status, Estado Auditado).
- Calculadora, 5 abas novas: Bolsa, Imóveis, Renda Fixa, IR e Tributos, Inflação e Câmbio (esqueletos, wizards internos pendentes para #43, #54, #55, #56, #57).
- Calculadora v3, wizards #12 a #20: Comparador RF (CDB vs LCI/LCA), FIRE Number, variantes Lean/Fat/Coast/Barista/Geographic, Reserva de Emergência, CLT vs PJ.
- `MarketDataService` com interface `IMarketDataProvider`; providers iniciais `B3DirectQuoteProvider` (cotacao.b3.com.br) e `CotahistProvider` (COTAHIST B3) para #45 e #59.
- `AppSettings` seção Mercado: `marketDefaultProvider`, `brapiToken`, `brapiPlan`, `b3OfficialKeyId`, `marketPollingInterval`, `cotahistAnosHistorico`.
- `ExportDialog`, escolha de seções (checkboxes pré-marcados) e formato (9 formatos), antes do save file dialog (#11).
- `ExportService` ampliado: serialização de todas as seções (Entradas, Gastos, Investimentos, Planejamento, Recorrências, Dívidas, Metas, Lembretes, Cartões); XLSX uma aba por seção; OFX e QIF apenas transações financeiras.
- Modelo `PrecoAtivo` com `PriceRepository` e tabelas SQLite `precos_ativos` e `cotacoes_tempo_real` (#44).
- Aba Dashboard com resumo mensal/anual (#6).
- Aba Gastos, lançamentos avulsos PIX/débito/dinheiro/fixos (#14).
- Aba Investimentos (#20), Planejamento (#22), Tendências (#23), Dívidas (#33), Metas (#28), Lembretes (#31).
- Reconciliação bancária extrato vs app (#27).
- Busca/filtro global de transações (#32).
- Transações recorrentes agendadas (#26).
- Undo/Redo via `QUndoStack` único em `MainWindow` com `LambdaCommand` genérico (#30).
- Calculadora v1, v2, v3: aba Comum, aba Financeira HP-style (TVM, BEGIN/END, "Enviar para Cartões"), aba Wizards (CET, juros, FV, renda passiva, ROI, comprometimento, saldo devedor, meta, amortização parcial) para #17, #36 e #37.
- Cartões: painel inferior editável e tabela de amortização Price (#35).
- Categorias hierárquicas (gastos, entradas, investimentos) para #21.
- Importação CSV, OFX, QIF, XLSX, JSON, TXT, PDF, HTML, MD.

### Changed
- `PENDENCIAS.md` migrado para `TODO.md` no formato canônico `tab_pendencias`; conformidade com regra global `~/.claude/docs/estilo-trabalho.md`.
- Paleta de cores migrada de Forest Neutral para KDE Breeze (Light/Dark via `QStyleHints`).
- `objectName` de botões primários: `primaryBtn` para `primary` (alinha com seletor QSS central).
- Cartões: aba renomeada de "Parcelas" para "Cartões" (#3); botão "Nova compra" para "Novo item" (#4).
- Migração de criptografia: AES-256-CBC com PBKDF2-SHA256 para AES-256-GCM e XChaCha20-Poly1305 via libsodium; Argon2id; formato versionado; backward-compat CBC (#42).

### Removed
- `PENDENCIAS.md` (substituído por `TODO.md` canônico).
- `SECURITY.md` (conteúdo virou #42 ✅, migração libsodium completada).
- Dir órfão `.claude/projects/-...-projetos-dev-...-memory/` (path obsoleto; sistema lê memórias de `~/.claude/projects/<slug>/`).
- Item #77 (Flatpak/AppImage) cancelado por decisão do usuário.

### Security
- AES-256-GCM e XChaCha20-Poly1305 (libsodium) substituem AES-256-CBC para #42.
- Argon2id substitui PBKDF2-SHA256.
- Formato de arquivo cifrado versionado: 1 byte versão, 1 byte algoritmo, 24 bytes nonce, ciphertext, tag de 16 bytes.
- Backward-compat com arquivos antigos AES-256-CBC.

---

## Histórico pré-changelog

> Mantido para referência. Antes da introdução do CHANGELOG (#78), versões eram informais e o histórico cabe em `git log --oneline`.

### Marcos relevantes
- **2026-05-13**: migração `PENDENCIAS.md` para `TODO.md` canônico; CONTRACT.md versionado; introdução do CHANGELOG.
- **2026-04**: aba Dashboard, refactor Services, Cartões com tabela de amortização.
- **2026-03**: Reconciliação, Metas, Lembretes, Busca Global; pesquisa de APIs B3.
- **2026-02**: aba Gastos, Calculadora v1.
- **2026-01**: migração MySQL para SQLite com AES-256-CBC; redesign do login; CI inicial; renomeação para PeuBud.
- **2026-pre**: estrutura inicial modular (`core/`, `models/`, `repositories/`, `services/`, `ui/widgets/`).
