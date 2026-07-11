#pragma once

#include <QByteArray>
#include <QString>

#include <cstddef>   // size_t

// (ADR-001 #212 §5.2, Fatia 3 — Foundation) Framing PURO do formato `.pbd`
// v2 auto-contido. No modelo multiusuário, os wraps por-senha/recovery de
// CADA usuário precisam morar no cofre do próprio usuário (não só no
// registro central `users.json`, que só o admin assina) — resiliência a
// perda do registro (ADR-001 §2 D-D).
//
// Formato `.pbd` v2 (estritamente aditivo sobre o v1 — payload cifrado não
// muda, mesmo AEAD, mesma K — ADR-001 §5.2):
//
//   [4]            magic 'P','B','D',0x02
//   [2]            header_len : uint16 BIG-ENDIAN (tamanho em bytes do
//                  bloco header que segue)
//   [header_len]   header : JSON compacto UTF-8 — ver campos abaixo
//   [resto]        innerBlob : EXATAMENTE a saída de
//                  SodiumCryptoService::encrypt(plaintext, K) — começa com
//                  o magic v1 'P','B','D',0x01 + algo + nonce + cipher+tag —
//                  copiado VERBATIM, nunca reinterpretado por esta classe.
//
// Header JSON (chaves curtas — o header vai duplicado em TODO cofre, custo
// de espaço importa mais que legibilidade; todos os binários em base64):
//   { "u": "<username>", "sp": "<base64 salt_pw>", "sr": "<base64 salt_rk>",
//     "ops": <int>, "mem": <int>,
//     "wp": "<base64 wrapped_key_by_pass>",
//     "wr": "<base64 wrapped_key_by_recovery>" }
//
// ── Por que o header NÃO tem MAC próprio e ainda assim é seguro ────────────
// O header é texto em claro NÃO autenticado — e isso é intencional e
// seguro: adulterar sp/ops/mem só faz o `unwrap` do secretbox (wp/wr)
// FALHAR (a wrapKey derivada com params alterados não bate com a que
// criou o wrap) → nega login (DoS), NUNCA vaza K nem barateia brute-force
// (o atacante precisaria dos params originais — que são justamente os
// caros). Substituir o header inteiro por um forjado também não ajuda: o
// innerBlob continua cifrado com o K original, que o atacante não tem.
// Logo, a autenticação vem do próprio secretbox dos wraps + do AEAD do
// innerBlob — não do header. Ver ADR-001 §5.2 para o raciocínio completo.
struct VaultHeaderData {
    QString    username;
    QByteArray saltPw;                  // "sp" — salt Argon2id da senha (16B)
    QByteArray saltRk;                  // "sr" — salt Argon2id da recovery key (16B)
    int        kdfOps = 0;              // "ops" — crypto_pwhash_OPSLIMIT_* gravado
    size_t     kdfMem = 0;              // "mem" — crypto_pwhash_MEMLIMIT_* gravado
    QByteArray wrappedKeyByPass;        // "wp" — 72B: nonce(24)+secretbox(K)+MAC
    QByteArray wrappedKeyByRecovery;    // "wr" — idem, via recovery key
};

// Classe SEM ESTADO — só métodos estáticos. Sem exceção (CONTRACT §6.4):
// entrada adversarial (arquivo truncado/malicioso) NUNCA lança — falha
// sempre por retorno (Format::Unknown / ParseResult::ok=false).
class VaultFile {
public:
    VaultFile() = delete;

    // v2 = magic(4) + header_len BE(2) + headerJson + innerBlobV1 (verbatim).
    [[nodiscard]] static QByteArray frameV2(const VaultHeaderData &hdr, const QByteArray &innerBlobV1);

    // True se `fileBytes` começa com o magic v2 ('P','B','D',0x02).
    [[nodiscard]] static bool isV2(const QByteArray &fileBytes);

    enum class Format { V1, V2, Unknown };
    // Detecta o formato pelo magic. V1 delega a
    // SodiumCryptoService::isNewFormat (magic 0x01) — não reimplementado
    // aqui, fonte única de verdade continua no Foundation de cripto.
    [[nodiscard]] static Format detect(const QByteArray &fileBytes);

    struct ParseResult {
        bool ok = false;
        VaultHeaderData header;
        QByteArray innerBlob;
    };
    // Separa o header (parseado + validado) do innerBlob (o resto do
    // arquivo, um blob v1 verbatim — não decodificado/decifrado aqui, é
    // trabalho de SodiumCryptoService/DatabaseManager numa fatia futura).
    //
    // ok=false (NUNCA lança) se: magic errado, header_len inconsistente com
    // o tamanho real do arquivo (arquivo truncado ou header_len forjado
    // maior que o buffer), JSON malformado, campo ausente/tipo errado, ou
    // base64 inválido em qualquer campo binário.
    [[nodiscard]] static ParseResult parseV2(const QByteArray &fileBytes);
};
