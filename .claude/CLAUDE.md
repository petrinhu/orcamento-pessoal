# CLAUDE.md — Orçamento Pessoal

## Visão geral do projeto

App de finanças pessoais em C++20/Qt6, construído em módulos incrementais.
**Módulo 1:** controle de parcelas do cartão de crédito.
**Módulo 2 (atual):** aba de Entradas — lançamentos de receitas por fonte de renda.

**Repositório Codeberg:** ssh://git@codeberg.org/petrinhu/orcamento_pessoal.git

## Papel e contexto

Engenheiro sênior C++20/Qt6, foco em Linux (Fedora/KDE Plasma primário,
Debian e Arch como alvos secundários de distribuição).

## Stack

- **Linguagem:** C++20
- **UI:** Qt6 (Widgets, Charts)
- **Banco de dados:** SQLite via QSQLITE (built-in no Qt6)
- **Criptografia:** OpenSSL — AES-256-CBC, PBKDF2-SHA256 (600.000 iterações)
- **Segredos do sistema:** libsecret (Secret Service API → KWallet/GNOME Keyring)
- **Build:** CMake 3.16+
- **Plataforma:** Fedora 43 / KDE Plasma (primário)
- **Fonte:** Inter (Regular, Medium, SemiBold) — embutida via .qrc
- **Tema:** Forest Neutral, light/dark automático via QStyleHints (Qt6)

## Dependências

```bash
# Fedora (dnf)
sudo dnf install \
  qt6-qtbase-devel qt6-qtcharts-devel \
  openssl-devel libsecret-devel \
  gcc-c++ cmake make -y

# Debian/Ubuntu (apt)
sudo apt install \
  qt6-base-dev libqt6charts6-dev \
  libssl-dev libsecret-1-dev \
  g++ cmake make -y

# Arch (pacman)
sudo pacman -S qt6-base qt6-charts \
  openssl libsecret \
  gcc cmake make
```

## Estrutura modular

```
src/
  main.cpp
  core/
    interfaces/
      ICryptoService.h       # SRP: só criptografia
      ISecretStore.h         # SRP: só armazenamento de segredos
      IRepository.h          # Repositório genérico (template)
    crypto/
      AesCryptoService.h/cpp # AES-256-CBC (debug: sem crypto)
      KeyDerivation.h/cpp    # PBKDF2-SHA256, 600k iter
    secret/
      SecretStoreService.h/cpp # libsecret → KWallet/GNOME Keyring
    database/
      DatabaseManager.h/cpp  # SQLite lifecycle, pragma setup
    session/
      UserSession.h/cpp      # Nome logado, último nome (QSettings)
    currency/
      ICurrencyProvider.h    # Interface para cotação
      FrankfurterProvider.h/cpp # frankfurter.app (ECB, sem API key)
  models/
    Compra.h                 # struct POD
    Parcela.h                # struct POD
    FonteRenda.h             # struct POD — fonte de renda (id, nome)
    LancamentoEntrada.h      # struct POD — recebimento mensal
  repositories/
    CompraRepository.h/cpp
    ParcelaRepository.h/cpp
    FonteRendaRepository.h/cpp          # CRUD + findOrCreate
    LancamentoEntradaRepository.h/cpp   # CRUD + findAll + findByMesAno
  services/
    ParcelaService.h/cpp     # CET, juros, Newton-Raphson, datas retroativas
    CurrencyService.h/cpp    # Conversão + cache de cotação
    ImportService.h/cpp      # Importação CSV/OFX/QIF/XLSX/JSON/…
    ExportService.h/cpp      # Exportação CSV/OFX/QIF/XLSX/JSON/…
  ui/
    Theme.h/cpp
    LoginDialog.h/cpp
    MainWindow.h/cpp         # QTabWidget: abas Parcelas + Entradas
    IconGenerator.h/cpp
    widgets/
      ParcelasWidget.h/cpp            # Tabela editável de parcelas
      AddCompraDialog.h/cpp           # Diálogo de entrada (modos 2.1–2.5)
      ConfigWidget.h/cpp              # CRUD cartões
      EntradasWidget.h/cpp            # Tabela de entradas (4 sub-cols/mês)
      LancamentoEntradaDialog.h/cpp   # Dialog: fonte, valor, impostos, recorrência
      ConfigFontesWidget.h/cpp        # CRUD fontes de renda (nas Preferências)
      PrefsWidget.h/cpp               # Painel de preferências completo
      AboutDialog.h/cpp
      HelpDialog.h/cpp
resources/
  fonts/Inter-*.ttf
  resources.qrc
data/                        # runtime — NÃO versionar
  <slug>.enc                 # banco criptografado
  .<slug>.db                 # temporário (debug: sem crypto)
```

## Princípios de código (SOLID)

- **SRP:** cada classe tem responsabilidade única — nada monolítico
- **OCP:** extensível para novos módulos sem modificar os existentes
- **LSP:** implementações substituem interfaces sem quebrar comportamento
- **ISP:** interfaces pequenas e específicas (ICryptoService, ISecretStore, IRepository)
- **DIP:** depender de abstrações, não de implementações concretas

## Convenções

- Valores monetários sempre em **centavos (qint64)** — nunca float/double
- Juros armazenados como inteiro: `% × 10000` (ex: 1,5% = 15000)
- Datas: `QDate` internamente; `dd/MM/yyyy` na UI
- Formatação: `centavosParaTexto()` → "R$ 0,00" (símbolo varia por moeda)
- IDs: 0 = não persistido, -1 = linha TOTAL nas tabelas
- Queries SQL: sempre com `bindValue` (sem concatenação de strings)
- `PRAGMA foreign_keys = ON` + `ON DELETE CASCADE/SET NULL` nas FKs
- `PRAGMA journal_mode = MEMORY` — sem journal files em disco
- Commits sempre em português
- **Não criar código C++ sem perguntar ao usuário antes**

## Login / sessão

- `#ifdef QT_DEBUG`: só nome, sem senha, último nome salvo via QSettings
- Release: nome + senha + checklist (≥8 chars, maiúscula, número)
- Chave AES armazenada via libsecret (KWallet/GNOME Keyring)

## Paleta de cores (Forest Neutral)

| Token      | Light   | Dark    |
|------------|---------|---------|
| Background | #F5F4F0 | #18181A |
| Surface    | #FFFFFF | #222225 |
| Acento     | #166F4A | #22A367 |
| Positivo   | #166F4A | #22A367 |
| Negativo   | #C94040 | #E85555 |

## Ferramentas permitidas

- **APENAS** ferramentas gratuitas, open-source, disponíveis via dnf/apt/pacman
- NÃO sugerir: PVS-Studio, Coverity, SonarCloud pago, Squish, ou qualquer tool com login/chave paga
- Ao recomendar tools, sempre fornecer equivalentes para as 3 distros (dnf/apt/pacman)

## Metodologia pré-distribuição

Antes de qualquer release, checklist cobrindo:
1. Análise estática: `clang-tidy`, `cppcheck`
2. Sanitizers: ASan, UBSan + `valgrind`
3. Segurança/CVE/hardening: `checksec`, `grype` ou `trivy`
4. QA funcional: `ctest`
5. UX/UI: GammaRay, revisão manual

Para cada item: comando exato + justificativa de por que é crítico.

## Prioridade

**Precisão financeira > Robustez > Segurança > UX**
