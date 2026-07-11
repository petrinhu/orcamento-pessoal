#pragma once

#include <QList>
#include <QString>
#include <QtTypes>

// BuscaService (#139, Onda 7 — arquitetura) — camada Mid.
//
// Encapsula as 5 queries SQL da busca global de transações (antes cruas
// dentro de BuscaGlobalDialog, violando "UI não fala SQL"). A UI passa a
// consumir apenas Resultado; formatação de data/valor e tradução de rótulo +
// cor de destaque continuam na UI (presentation concern, fora do escopo do
// Service).
//
// Stateless: cada chamada abre QSqlQuery contra
// DatabaseManager::instance().db(); todas com bindValue (zero concat de SQL).
class BuscaService {
public:
    // Identificador ESTÁVEL (não traduzido) da categoria do resultado —
    // a UI mapeia para rótulo traduzido + cor.
    static constexpr const char *kCategoriaEntrada      = "entrada";
    static constexpr const char *kCategoriaGasto        = "gasto";
    static constexpr const char *kCategoriaInvestimento = "investimento";
    static constexpr const char *kCategoriaDivida       = "divida";
    static constexpr const char *kCategoriaCartao       = "cartao";

    // ── Tab indices (em sincronia com a ordem default de MainWindow) ───────
    // 0=Dashboard 1=Entradas 2=Gastos 3=Cartões 4=Recorrências 5=Dívidas
    // 6=Metas 7=Planejamento 8=Tendências 9=Investimentos 10=Proventos
    // 11=Bolsa 12=Lembretes.
    // ATENCAO: indices fixos sao fragueis se o usuario reordenar as abas
    // (drag & drop). Refactor recomendado: navegar por chave estavel
    // (objectName). Ver nota de follow-up no retorno da tarefa de abas
    // reordenaveis (herdada de BuscaGlobalDialog, #139 só moveu o dono).
    static constexpr int kTabEntradas      = 1;
    static constexpr int kTabGastos        = 2;
    static constexpr int kTabInvestimentos = 9;
    static constexpr int kTabDividas       = 5;
    static constexpr int kTabCartoes       = 3;

    struct Resultado {
        QString categoria;      // uma de kCategoria*
        QString data;           // ISO yyyy-MM-dd
        QString descricao;
        qint64  valor = 0;      // centavos; sinal conforme natureza (gasto negativo)
        QString categoriaObs;   // categoria/observação (texto livre) p/ coluna extra
        int     tabIndex = 0;   // aba destino no QTabWidget (ver nota acima)
    };

    // termo:   buscado via LIKE %termo% nos campos de texto de cada tabela.
    // periodo: "mes" | "3meses" | "6meses" | "ano" | "tudo" (qualquer outro
    //          valor, incl. vazio, é tratado como "tudo" — sem filtro de
    //          data). Filtra por data em entradas/gastos/investimentos;
    //          dívidas e cartões não têm filtro de período (igual ao
    //          comportamento original de BuscaGlobalDialog).
    // tipo:    "todos" | "entradas" | "gastos" | "investimentos" |
    //          "dividas" | "cartoes". Restringe quais categorias são
    //          consultadas.
    // Ordem do resultado: concatenação por categoria, na ordem entradas →
    // gastos → investimentos → dívidas → cartões; dentro de cada categoria,
    // ORDER BY data DESC (mesma ordem/limits da query original: LIMIT 500
    // para entradas/gastos/investimentos, LIMIT 200 para dívidas/cartões).
    [[nodiscard]] static QList<Resultado> buscar(const QString &termo,
                                                  const QString &periodo,
                                                  const QString &tipo);
};
