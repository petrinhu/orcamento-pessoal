# PeuBud — Personal Budget / Orçamento Pessoal

**[English](#english) · [Português](#português)**

[![Build](https://codeberg.org/petrinhu/orcamento_pessoal/actions/workflows/build.yml/badge.svg)](https://codeberg.org/petrinhu/orcamento_pessoal/actions)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue?logo=cplusplus)
![Qt6](https://img.shields.io/badge/Qt-6-41CD52?logo=qt)
![SQLite](https://img.shields.io/badge/SQLite-3-003B57?logo=sqlite)
![Platform](https://img.shields.io/badge/platform-Linux-FCC624?logo=linux)

---

## English

**PeuBud** is a desktop personal finance application built with C++20 and Qt6.
It tracks credit-card installments, income entries, and day-to-day expenses — all
stored locally in an AES-256-CBC-encrypted SQLite database.

### Features

| Module | Status |
|--------|--------|
| Dashboard — monthly/annual overview | 🚧 In development |
| Income — entries by source, taxes (IRPF / INSS), recurrence | ✅ |
| Expenses — one-off payments: PIX, debit, cash, fixed | ✅ |
| Cards — installment purchases with Price amortisation table | ✅ |
| Calculator — floating, field-injection, keyboard input | ✅ |
| Import — CSV, OFX, QIF, XLSX, JSON, TXT, PDF, HTML, Markdown | ✅ |
| Export — same formats | ✅ |
| Theme — Auto / Light / Dark (Forest Neutral palette) | ✅ |
| Investments | ❌ Planned |
| Planning & budgeting | ❌ Planned |
| AI financial consultant | ❌ Planned |

### Tech stack

- **Language:** C++20
- **UI:** Qt 6 — Widgets, Charts, Sql, Network
- **Database:** SQLite via QSQLITE (built-in)
- **Encryption:** OpenSSL — AES-256-CBC, PBKDF2-SHA256 (600 000 iterations)
- **Secrets:** libsecret → KWallet / GNOME Keyring
- **Build:** CMake 3.16+
- **Font:** Inter (Regular, Medium, SemiBold — embedded)
- **Primary platform:** Fedora / KDE Plasma (Wayland)

### Build

```bash
# Fedora
sudo dnf install qt6-qtbase-devel qt6-qtcharts-devel \
  openssl-devel libsecret-devel gcc-c++ cmake make -y

# Debian / Ubuntu
sudo apt install qt6-base-dev libqt6charts6-dev \
  libssl-dev libsecret-1-dev g++ cmake make -y

# Arch
sudo pacman -S qt6-base qt6-charts openssl libsecret gcc cmake make

# All distros
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/orcamento-pessoal
```

### Architecture

```
src/
  core/        interfaces, crypto (AES), secret store, DB manager, session, calculator hub
  models/      POD structs — Compra, Parcela, Cartao, FonteRenda, LancamentoEntrada, GastoAvulso
  repositories CRUD per entity (SQLite, bind-value only)
  services/    business logic — installments, currency, import, export, calculator
  ui/          MainWindow, theme, login, per-tab widgets, dialogs
resources/     Inter font, translations (.ts / .qm)
```

---

## Português

**PeuBud** é uma aplicação desktop de finanças pessoais escrita em C++20 e Qt6.
Controla parcelas de cartão de crédito, lançamentos de receitas e gastos do dia-a-dia —
tudo armazenado localmente em banco SQLite criptografado com AES-256-CBC.

### Funcionalidades

| Módulo | Status |
|--------|--------|
| Dashboard — resumo mensal/anual | 🚧 Em desenvolvimento |
| Entradas — por fonte de renda, impostos (IRPF / INSS), recorrência | ✅ |
| Gastos — avulsos: PIX, débito, dinheiro, fixos | ✅ |
| Cartões — compras parceladas com tabela Price de amortização | ✅ |
| Calculadora — flutuante, injeção em campos, entrada por teclado | ✅ |
| Importar — CSV, OFX, QIF, XLSX, JSON, TXT, PDF, HTML, Markdown | ✅ |
| Exportar — mesmos formatos | ✅ |
| Tema — Auto / Claro / Escuro (paleta Forest Neutral) | ✅ |
| Investimentos | ❌ Planejado |
| Planejamento e orçamento | ❌ Planejado |
| Consultor financeiro IA | ❌ Planejado |

### Stack

- **Linguagem:** C++20
- **UI:** Qt 6 — Widgets, Charts, Sql, Network
- **Banco de dados:** SQLite via QSQLITE (embutido)
- **Criptografia:** OpenSSL — AES-256-CBC, PBKDF2-SHA256 (600 000 iterações)
- **Segredos:** libsecret → KWallet / GNOME Keyring
- **Build:** CMake 3.16+
- **Fonte:** Inter (Regular, Medium, SemiBold — embutida)
- **Plataforma primária:** Fedora / KDE Plasma (Wayland)

### Compilar

```bash
# Fedora
sudo dnf install qt6-qtbase-devel qt6-qtcharts-devel \
  openssl-devel libsecret-devel gcc-c++ cmake make -y

# Debian / Ubuntu
sudo apt install qt6-base-dev libqt6charts6-dev \
  libssl-dev libsecret-1-dev g++ cmake make -y

# Arch
sudo pacman -S qt6-base qt6-charts openssl libsecret gcc cmake make

# Todas as distros
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/orcamento-pessoal
```

### Arquitetura

```
src/
  core/        interfaces, crypto (AES), secret store, gerenciador de BD, sessão, hub da calculadora
  models/      structs POD — Compra, Parcela, Cartao, FonteRenda, LancamentoEntrada, GastoAvulso
  repositories CRUD por entidade (SQLite, somente bind-value)
  services/    lógica de negócio — parcelas, moeda, importação, exportação, calculadora
  ui/          MainWindow, tema, login, widgets por aba, diálogos
resources/     fonte Inter, traduções (.ts / .qm)
```

### Convenções

- Valores monetários em **centavos (`qint64`)** — nunca `float`/`double`
- Juros armazenados como `% × 10000` (ex: 1,5% = 15 000)
- Queries SQL sempre com `bindValue` (sem concatenação de strings)
- `PRAGMA foreign_keys = ON` + `ON DELETE CASCADE/SET NULL`
- Prioridade: **Precisão financeira > Robustez > Segurança > UX**
