#!/usr/bin/env bash
# register.sh — registra o forgejo-runner self-hosted do PeuBud no Codeberg.
#
# Uso:
#   ./register.sh <TOKEN_DE_REGISTRO>
#
# Onde pegar o TOKEN (só o líder consegue, não é automatizável):
#   https://codeberg.org -> repo petrinhu/orcamento_pessoal
#     -> Settings -> Actions -> Runners -> "Create new runner"
#   Copiar o token mostrado (só aparece 1x). Escopo repo (não precisa de org).
#
# O que este script faz:
#   1. Cria ~/.config/forgejo-runner/peubud/ (FORA do repo git — nunca vira
#      commit por acidente).
#   2. Copia o config.yaml (template deste diretório) pra lá, se ainda não
#      existir um (não sobrescreve config já ajustado manualmente).
#   3. Roda `forgejo-runner register`, que grava o `.runner` (token de
#      sessão) dentro de ~/.config/forgejo-runner/peubud/ — esse arquivo é
#      o segredo real; fica fora do repo por construção.
#
# Depois de rodar este script, o próximo passo é instalar e subir o
# systemd --user unit (ver docs/ci/self-hosted-runner.md).

set -euo pipefail

TOKEN="${1:?Uso: $0 <TOKEN_DE_REGISTRO>  (pego em Settings -> Actions -> Runners -> Create new runner)}"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER_DIR="$HOME/.config/forgejo-runner/peubud"
LABEL="self-hosted-fedora:docker://fedora:42"
RUNNER_NAME="peubud-fedora-laptop"
INSTANCE_URL="https://codeberg.org"

# Binário nativo (não a imagem/container do runner — ver docs/ci/self-hosted-runner.md
# seção "binário vs container" pra justificativa). Prioriza /usr/local/bin
# (versão mais nova instalada manualmente) e cai pro pacote dnf se ausente.
RUNNER_BIN="$(command -v forgejo-runner || true)"
if [ -z "$RUNNER_BIN" ]; then
  echo "ERRO: forgejo-runner não encontrado no PATH. Instale: sudo dnf install forgejo-runner" >&2
  exit 1
fi

mkdir -p "$RUNNER_DIR"

if [ -f "$RUNNER_DIR/config.yaml" ]; then
  echo "Config já existe em $RUNNER_DIR/config.yaml — não sobrescrevendo (edite manualmente se precisar)."
else
  cp "$REPO_ROOT/config.yaml" "$RUNNER_DIR/config.yaml"
  echo "Config copiado pra $RUNNER_DIR/config.yaml"
fi

cd "$RUNNER_DIR"

"$RUNNER_BIN" register \
  --no-interactive \
  --instance "$INSTANCE_URL" \
  --token "$TOKEN" \
  --name "$RUNNER_NAME" \
  --labels "$LABEL" \
  --config "$RUNNER_DIR/config.yaml"

echo ""
echo "Registrado com sucesso."
echo "Estado (segredo, NÃO commitar): $RUNNER_DIR/.runner"
echo ""
echo "Próximo passo (systemd --user):"
echo "  mkdir -p ~/.config/systemd/user"
echo "  cp $REPO_ROOT/forgejo-runner.service ~/.config/systemd/user/"
echo "  systemctl --user daemon-reload"
echo "  systemctl --user enable --now forgejo-runner.service"
echo "  systemctl --user status forgejo-runner.service"
