# Security — PeuBud

================================================================================
PRIORIDADE ALTA - PENDÊNCIA DE SEGURANÇA - MIGRAÇÃO DE CRIPTOGRAFIA

Tarefa: Evoluir para modelo híbrido AES-256-GCM (principal) + XChaCha20-Poly1305 (fallback)

Contexto atual:
- O app já persiste dados financeiros cifrados com AES (detalhar modo atual: CBC? GCM? outro?).
- Hardware alvo tem AES-NI (Intel 12th Gen i5-12500H).

Objetivo da migração (2026):
1. Manter AES-256-GCM como algoritmo principal (preferencial quando AES-NI presente).
2. Adicionar suporte a XChaCha20-Poly1305 como fallback (mais tolerante a nonce reuse, melhor em software puro ou ARM futuro).
3. Usar exclusivamente libsodium para as duas implementações:
   - AES-256-GCM → crypto_aead_aes256gcm_encrypt / decrypt
   - XChaCha20-Poly1305 → crypto_aead_xchacha20poly1305_ietf_encrypt / decrypt
4. Formato de arquivo compatível:
   - Versão (1 byte)
   - Algoritmo ID (1 byte: 0=AES-GCM, 1=XChaCha20-Poly1305)
   - Nonce (24 bytes fixos; AES-GCM usa os 12 primeiros)
   - Ciphertext + tag (16 bytes)
5. Derivação de chave: Argon2id com parâmetros fortes (memória ≥64MiB, iterações ≥3, paralelismo 4).
6. Lógica de escolha: detectar AES-NI em runtime → preferir AES-GCM; fallback para XChaCha20.
7. Garantir backward compatibility: ler arquivos antigos (sem byte de algoritmo) assumindo AES atual.

Critérios de aceitação futuros:
- Leitura/escrita de arquivos antigos e novos sem perda de dados.
- Nonce gerado de forma segura e única (randombytes_buf).
- Nunca reutilizar nonce+chave.
- Testes unitários para ambos os caminhos.
- Opção futura de configuração para forçar um algoritmo.

Status: PENDENTE (aguardando decisão final e testes de desempenho com libsodium)
Prioridade: Alta (melhoria de segurança e resiliência)
Prazo sugerido: Antes de v1.0 com persistência de dados sensíveis
================================================================================
