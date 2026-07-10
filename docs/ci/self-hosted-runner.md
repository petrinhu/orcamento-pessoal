# Runner self-hosted do Forgejo Actions (Codeberg) — PeuBud

Registro didático, pt-BR, sem assumir conhecimento prévio de CI/CD ou containers.

## 1. Por que isso existe

O CI do PeuBud (`.forgejo/workflows/build.yml`) hoje roda em **runners voluntários
compartilhados** do Codeberg (`runs-on: codeberg-medium`): máquinas mantidas de
graça pela Codeberg e.V. e por voluntários da comunidade, sem custo pro usuário,
mas com **capacidade finita e fair-use explícito** ("minimize resource-heavy CI",
ver `docs.codeberg.org/ci/actions/` e a memória `codeberg-runners`).

Os jobs `build` (compila Qt6/C++20 do zero) e `sanitizers` (recompila tudo de
novo com ASan+UBSan) levam **~30min ao todo** por causa da imagem `fedora:42`
crua (sem cache de pacote entre runs) + build C++ sempre do zero (sem cache de
objeto entre runs). Isso é carga pesada num pool que outros projetos FOSS
também usam.

Um **runner self-hosted** — a própria máquina do líder, que já tem `podman` e
Fedora 44 — resolve isso: **carga zero pro Codeberg**, build mais rápido (SSD
local + sem fila de pool compartilhado), e alinhado com a preferência geral do
projeto por **execução local / soberania de dado** — o código-fonte e o build
não saem da máquina do dono.

`lint` continua no `codeberg-medium`: é curto (cppcheck só, informativo pra
clang-tidy) e é exatamente o tipo de job leve que a Codeberg pede pra manter
no pool compartilhado.

## 2. Decisão: binário nativo vs runner em container

Duas formas de rodar o `forgejo-runner`:

| Abordagem | Como funciona |
|---|---|
| **A. Binário nativo + engine externa (podman rootless)** | `forgejo-runner` roda direto no host como processo do usuário; fala com o `podman.sock` rootless pra criar um container `fedora:42` por job |
| **B. O próprio runner dentro de um container** | `forgejo-runner` roda dentro de um container Docker/Podman (imagem `code.forgejo.org/forgejo/runner`), que por sua vez precisa acesso ao socket do host pra criar OUTROS containers (job containers) — um caso de Docker-in-Docker/podman-in-podman |

**Decisão: Opção A (binário nativo).** Motivos:

1. **Já está instalado** nesta máquina via `dnf` (`forgejo-runner-12.9.0-1.fc44`,
   `/usr/bin/forgejo-runner`; há também um binário mais novo em
   `/usr/local/bin/forgejo-runner`, v12.10.0, à frente no `PATH` — os scripts
   aqui usam esse). Zero imagem extra pra baixar/manter atualizada.
2. **Menos uma camada de isolamento pra depurar.** B exige repassar o socket
   podman PRA DENTRO do container do runner (bind mount de socket), que por
   sua vez cria containers de job — dois saltos de "isolamento vazando pra
   fora do isolamento". A rota rootless nativa é: **processo do usuário →
   socket do podman.sock do próprio usuário → container do job.** Um salto
   só, mais fácil de raciocinar sobre segurança.
3. **systemd `--user` unit é natural pra binário nativo.** Empacotar B como
   serviço systemd que gerencia um container que gerencia outros containers é
   mais frágil (restart, cleanup de containers órfãos, ordem de start do
   socket) do que um processo simples com `Restart=on-failure`.
4. **Sem privilégio extra.** B tipicamente pede `--privileged` ou montar o
   socket do host no container do runner — superfície maior. A opção
   escolhida nunca roda nada como root nem `--privileged` (ver `config.yaml`,
   `privileged: false`).

Isso é o padrão recomendado pela própria doc do Forgejo Runner pra hosts
Linux com engine de container disponível (ver
`~/.claude/skills/forgejo/references/runner-security.md`, tabela "Docker
access em jobs").

## 3. Pré-requisitos (já confirmados nesta máquina)

```bash
which forgejo-runner   # /usr/local/bin/forgejo-runner (v12.10.0) — OK, já instalado
which podman           # /usr/bin/podman (v5.8.4) — OK, já instalado
loginctl show-user "$USER" | grep Linger   # Linger=yes — OK, serviços --user sobrevivem a logout
```

Se `forgejo-runner` não estiver instalado numa máquina nova:

```bash
# Fedora (dnf)
sudo dnf install forgejo-runner

# Debian/Ubuntu (apt) — pacote pode não existir em repos estáveis; baixar
# binário estático da release oficial:
# https://code.forgejo.org/forgejo/runner/releases

# Arch (pacman/AUR)
yay -S forgejo-runner-bin   # AUR, não é core/extra
```

## 4. Arquivos criados (neste PR/working tree, ainda não commitados/pushados)

```
ci/forgejo-runner/config.yaml           # template de config (SEM segredos)
ci/forgejo-runner/register.sh           # script de registro, parametrizado por TOKEN
ci/forgejo-runner/forgejo-runner.service # systemd --user unit template
docs/ci/self-hosted-runner.md           # este arquivo
```

**Nada disso contém o token.** O estado real pós-registro
(`~/.config/forgejo-runner/peubud/.runner`, que contém um segredo de sessão)
fica **fora do repo git**, em `~/.config/forgejo-runner/peubud/` — por
construção, nunca pode virar commit por acidente.

## 5. Passo-a-passo (o que o LÍDER precisa rodar)

### Passo 1 — Gerar o token de registro (só o líder consegue; é ação manual na UI)

Caminho exato:

1. Abrir `https://codeberg.org/petrinhu/orcamento_pessoal`
2. **Settings** (aba do repo) → **Actions** (menu lateral) → **Runners**
3. Botão **"Create new runner"**
4. Copiar o token mostrado (só aparece **uma vez**; se perder, gera outro)

(Alternativa: registrar no nível da CONTA em vez do repo — `Settings` da
conta → `Actions` → `Runners` → mesmo botão — deixa o runner disponível pra
todos os repos do usuário. Recomendo começar por **escopo de repo** — mais
restrito, e é só o PeuBud que precisa disso agora.)

### Passo 2 — Registrar o runner

```bash
cd /home/petrus/IDrive/Documentos/projetos_claudebrain/Projects/orcamento-pessoal
./ci/forgejo-runner/register.sh <COLE_O_TOKEN_AQUI>
```

Isso cria `~/.config/forgejo-runner/peubud/config.yaml` (copiado do template)
e `~/.config/forgejo-runner/peubud/.runner` (registro + token de sessão).

### Passo 3 — Habilitar o socket rootless do podman + subir o serviço

```bash
mkdir -p ~/.config/systemd/user
cp ci/forgejo-runner/forgejo-runner.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now podman.socket
systemctl --user enable --now forgejo-runner.service
```

### Validar

```bash
systemctl --user status podman.socket forgejo-runner.service
journalctl --user -u forgejo-runner.service -f   # logs em tempo real
```

Na UI: `Settings → Actions → Runners` do repo deve mostrar o runner
`peubud-fedora-laptop` com status **Idle** (verde) e label
`self-hosted-fedora`.

**Neste ponto o runner está vivo mas NENHUM job usa ele ainda** — o
`build.yml` continua com `runs-on: codeberg-medium` em tudo. A troca de
`runs-on:` é o próximo passo, gated nisso (ver seção 6).

### Comandos de manutenção

```bash
systemctl --user restart forgejo-runner.service   # após editar config.yaml
systemctl --user stop forgejo-runner.service       # parar temporariamente
systemctl --user disable --now forgejo-runner.service  # desligar de vez
```

Pra desregistrar do lado do Codeberg (se for descomissionar de vez): apagar
o runner na UI (`Settings → Actions → Runners → 🗑`) — o `.runner` local fica
inválido automaticamente.

## 6. Proposta de diff do `build.yml` (NÃO aplicada — só documentada aqui)

**Pré-requisito: só aplicar depois que o runner aparecer "Idle" na UI.** Se o
`runs-on:` for trocado antes disso, os jobs `build`/`sanitizers` ficam presos
em `pending`/`waiting` pra sempre (não existe runner nenhum atendendo o
label `self-hosted-fedora` ainda).

### Mudança 1 — `runs-on` de `build` e `sanitizers`

```diff
   build:
     name: Build + Test (Fedora)
-    runs-on: codeberg-medium
+    runs-on: self-hosted-fedora
     container:
       image: fedora:42
```

```diff
   sanitizers:
     name: ASan + UBSan (Fedora)
-    runs-on: codeberg-medium
+    runs-on: self-hosted-fedora
     container:
       image: fedora:42
```

`lint` **fica como está** (`runs-on: codeberg-medium`) — é o job leve que o
Codeberg aceita bem, não há motivo pra tirar do pool compartilhado.

### Mudança 2 — escopo de trigger: heavy CI só em PR→main e em release, não em todo push (política do líder)

Hoje o `on:` do workflow dispara `build`/`sanitizers`/`lint` em **todo push
em `main`** + **toda atualização de PR pra `main`**. Como o pesado vai
migrar pro self-hosted (custo zero pro Codeberg), rodar a cada push de PR
deixa de ser problema de fair-use — mas o líder pediu escopo mais restrito
mesmo assim: **PR→main + push de tag de release**, não em todo push de
branch comum.

Duas formas de fazer isso. **Recomendo a Opção A** (2 arquivos) por ser mais
explícita — cada workflow mostra na UI exatamente quando dispara, sem
depender de leitura de `if:` condicional.

#### Opção A (recomendada): separar em 2 arquivos de workflow

`.forgejo/workflows/build.yml` fica só com `lint` (like hoje, sem mudança de
trigger):

```yaml
name: build
on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  lint:
    name: Lint (cppcheck + clang-tidy)
    runs-on: codeberg-medium
    container:
      image: fedora:42
    steps:
      # ... (conteúdo do job lint, sem mudança)
```

Novo arquivo `.forgejo/workflows/build-heavy.yml` com `build` + `sanitizers`,
trigger restrito:

```yaml
name: build-heavy
on:
  pull_request:
    branches: [main]
  push:
    tags: ['v*']

jobs:
  build:
    name: Build + Test (Fedora)
    runs-on: self-hosted-fedora
    container:
      image: fedora:42
    steps:
      # ... (conteúdo idêntico ao job build atual)

  sanitizers:
    name: ASan + UBSan (Fedora)
    runs-on: self-hosted-fedora
    container:
      image: fedora:42
    steps:
      # ... (conteúdo idêntico ao job sanitizers atual)
```

Trade-off explícito: um **push direto em `main` fora de PR** (ex.: merge
manual sem PR, ou push administrativo) **não** dispara `build`/`sanitizers`
— só `lint`. Na prática, se o fluxo do projeto é sempre PR→main, isso não é
problema (a PR já validou antes do merge). Se o líder quiser um "cinto de
segurança" pra push direto em main também, é só adicionar
`push: {branches: [main]}` ao `on:` do `build-heavy.yml` (variante C abaixo).

#### Opção B (alternativa): 1 arquivo só, com `if:` por job

```diff
 on:
   push:
     branches: [main]
+    tags: ['v*']
   pull_request:
     branches: [main]

 jobs:
   build:
     name: Build + Test (Fedora)
-    runs-on: codeberg-medium
+    runs-on: self-hosted-fedora
+    if: forgejo.event_name == 'pull_request' || startsWith(forgejo.ref, 'refs/tags/v')
     container:
       image: fedora:42
```

(mesmo `if:` no job `sanitizers`). Um push comum em `main` ainda dispara o
*workflow run*, mas os jobs `build`/`sanitizers` aparecem como **skipped**
(cinza) na UI — custo zero de runner (job pulado não é despachado pra
nenhum runner), só um pouco de ruído visual. `lint` roda sempre, sem `if:`.

#### Variante C (opcional): cinto de segurança em push direto pra main

Se quiser manter validação pesada também em push direto (sem PR) pra `main`,
adicionar em qualquer uma das opções acima:

```yaml
  push:
    branches: [main]     # + isto, além de tags: ['v*']
    tags: ['v*']
```

(Opção B: sem precisar do `if:` extra, já que `forgejo.event_name ==
'push'` sobre `branches: [main]` cai fora da condição do `if:` proposto —
seria preciso reescrever o `if:` pra incluir esse caso. Avaliar se vale a
pena antes de adicionar; aumenta de novo a frequência de heavy build.)

#### Refinamentos opcionais de ruído (não recomendado agora, só registro)

Como o pesado sai do Codeberg, reduzir ainda mais os disparos de PR deixa
de ser sobre fair-use e vira só sobre "quero ver menos runs na UI". Se
algum dia isso incomodar:

- **Gate por draft**: `if: github.event.pull_request.draft == false` — só
  roda quando a PR sai de rascunho.
- **`paths-ignore`**: pular o trigger quando só `.md`/`docs/` mudam (ex.:
  `paths-ignore: ['docs/**', '*.md']`).

Nenhum dos dois é necessário agora — deixando registrado caso o volume de
PRs cresça.

## 7. Cuidados

### Segurança

- O runner **executa código do repo** (incluindo de PRs, se aprovado — ver
  regras de aprovação manual em `~/.claude/skills/forgejo/references/runner-security.md`).
  Rootless + container isola: o processo roda como usuário comum (não root),
  `privileged: false`, sem socket docker montado dentro do job
  (`docker_host: "-"`), sem volumes custom (`valid_volumes: []`).
- **PR de fork**: por padrão o Codeberg pede aprovação manual pra rodar
  Actions de contribuidores não-confiáveis. Não mudar isso pra "auto-approve"
  só porque o runner é local — a régua de confiança é a mesma.
- **Nunca** commitar `~/.config/forgejo-runner/peubud/.runner` (fica fora do
  repo por construção, mas reforçando: se algum dia esse diretório for
  movido pra dentro do repo por engano, é o primeiro arquivo a checar antes
  de `git add`).
- `NoNewPrivileges=true` + `ProtectSystem=strict` + `ProtectHome=read-only`
  no systemd unit — defesa em profundidade adicional, mesmo já não sendo
  root.

### Recursos / bateria (é notebook)

- Build Fedora+ASan é pesado de CPU (compila tudo do zero 2x: Release +
  Debug com sanitizers). Isso **suga bateria rápido**.
- **Recomendação:** só deixar o `forgejo-runner.service` ativo (`enable`)
  quando o notebook estiver **na tomada**. Não precisa ficar sempre ligado —
  é seu próprio runner, não um serviço 24/7 crítico. Pra rodar sob demanda:
  ```bash
  systemctl --user start forgejo-runner.service   # antes de abrir/atualizar um PR
  systemctl --user stop forgejo-runner.service    # depois, ou ao sair de casa
  ```
- `container.options: "--memory=6g --cpus=3"` no `config.yaml` limita o
  quanto um job pode consumir — ajustar conforme a RAM/CPU real livre da
  máquina (checar com `free -h` / `nproc`).
- Enquanto o job não roda (`capacity: 1`, só 1 de cada vez), o runner ocioso
  consome CPU/bateria desprezível — só o processo esperando fetch da fila.

### Limpeza de disco

Builds C++ com `FetchContent` (QXlsx etc.) passam de GB facilmente, e cada
job cria um container novo + imagem `fedora:42` cacheada localmente.

```bash
podman ps -a                    # containers parados acumulados
podman system df                # quanto cada categoria (imagens/containers/volumes) usa
podman system prune             # limpa containers parados + imagens dangling (seguro)
podman system prune -a --volumes  # mais agressivo: remove TODAS imagens não usadas + volumes órfãos (rodar só se precisar liberar espaço de verdade — vai re-baixar fedora:42 no próximo job)
```

Sugestão: rodar `podman system prune` manualmente de vez em quando (ou um
timer `systemd --user` mensal, fora do escopo deste PR) em vez de
automatizar dentro do próprio job de CI (evita apagar cache do job seguinte
por engano).

## 8. Cross-platform (equivalentes, caso a máquina mude)

A regra do líder é não assumir Linux-only. Se este runner precisar migrar
pra outra máquina/SO no futuro:

| Linux (aqui) | macOS | Windows |
|---|---|---|
| `systemd --user` unit | `launchd` (`~/Library/LaunchAgents/*.plist`, `KeepAlive`) | Task Scheduler (`schtasks /create`, trigger "At log on" ou "At startup") |
| `podman` rootless | `podman machine` (VM leve) ou Docker Desktop | Docker Desktop (WSL2 backend) ou Podman Desktop |
| `loginctl enable-linger` | N/A (launchd Agent roda por sessão; LaunchDaemon roda sem login) | serviço rodando como "Run whether user is logged on or not" |

O binário `forgejo-runner` é multiplataforma (Go, builds oficiais pra
linux/darwin/windows) — a lógica de `register`/`daemon` é idêntica; só o
"como iniciar automaticamente" muda.
