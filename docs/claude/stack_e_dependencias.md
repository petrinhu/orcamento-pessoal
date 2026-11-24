# Stack e dependências do Orçamento Pessoal

## Stack

- **Linguagem:** C++20
- **UI:** Qt6 (Widgets, Charts)
- **Banco de dados:** SQLite via QSQLITE (built-in no Qt6)
- **Criptografia:** libsodium — AEAD AES-256-GCM (com AES-NI) ou XChaCha20-Poly1305 (fallback), chave derivada por Argon2id (MODERATE: ops=3, mem=256 MB). Formato `.pbd` v1 (magic `PBD\x01`). Legado AES-256-CBC + PBKDF2-SHA256 (600k iter, OpenSSL) só para **leitura** de arquivos antigos — migrado de forma transparente para o formato novo no próximo save/close.
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
