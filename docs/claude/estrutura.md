# Estrutura modular do Orçamento Pessoal

Os princípios que regem esta estrutura estão na OP-03 do `GODS_LAWS.md`.

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
      SodiumCryptoService.h/cpp # ATIVO: AEAD AES-GCM/XChaCha20 + Argon2id (debug: sem crypto)
      AesCryptoService.h/cpp # LEGACY (só leitura): AES-256-CBC
      KeyDerivation.h/cpp    # LEGACY (só leitura): PBKDF2-SHA256, 600k iter
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
