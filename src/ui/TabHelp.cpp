#include "TabHelp.h"

#include <QCoreApplication>

// Registro central (DRY) do texto de ajuda de cada aba. Alimenta tanto o
// TabShell (descrição + instruções colapsáveis no topo) quanto o HelpDialog.
// Para adicionar uma aba: incluir a entrada no QMap abaixo e a tabKey em
// tabHelpOrder() na posição visual da aba.
const QMap<QString, TabHelpEntry> &tabHelp()
{
    static const QMap<QString, TabHelpEntry> mapa = [] {
        QMap<QString, TabHelpEntry> m;

        m.insert(QStringLiteral("tab_dashboard"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Resumo"),
            QCoreApplication::translate("TabHelp",
                "🏠 Aqui você vê o resumo do seu dinheiro no mês e no ano — quanto "
                "entrou, quanto saiu, quanto sobrou e como andam os "
                "investimentos. É o panorama geral, sem precisar abrir cada aba."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "Use as setas ◀️ ▶️ no topo pra trocar o mês."),
                QCoreApplication::translate("TabHelp",
                    "Veja os cartões de \"Este mês\" e os de \"Acumulado no "
                    "ano\"."),
                QCoreApplication::translate("TabHelp",
                    "Clique num cartão com a setinha ➡️ pra pular pra aquela aba."),
                QCoreApplication::translate("TabHelp",
                    "No gráfico 📊 \"Entradas × Gastos\", escolha o período (3/6/12/"
                    "24 meses)."),
                QCoreApplication::translate("TabHelp",
                    "Toque em 🔒 pra fechar o mês (congela os números; dá pra "
                    "reabrir). Use 🖨️ pra imprimir/PDF."),
            }
        });

        m.insert(QStringLiteral("tab_entradas"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Entradas"),
            QCoreApplication::translate("TabHelp",
                "💵 Aqui você registra o dinheiro que recebe — salário, freela, "
                "aluguel e outras fontes. O app calcula o líquido descontando "
                "IRPF (imposto de renda) e INSS (previdência), se você informar."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "Clique em \"➕ Novo lançamento\"."),
                QCoreApplication::translate("TabHelp",
                    "Escolha a Fonte, a Categoria e o Valor bruto em reais."),
                QCoreApplication::translate("TabHelp",
                    "Em \"Impostos retidos\", preencha IRPF e INSS — o líquido "
                    "aparece automático."),
                QCoreApplication::translate("TabHelp",
                    "Em \"Recorrência\", escolha \"Somente este mês\" ou "
                    "\"Recorrente por X meses\"."),
                QCoreApplication::translate("TabHelp",
                    "Ctrl+← / Ctrl+→ navegam meses; \"🔃 Organizar por ▾\" ordena. "
                    "Clique pra ✏️ editar, \"🗑️ Excluir\" pra apagar."),
            }
        });

        m.insert(QStringLiteral("tab_gastos"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Gastos"),
            QCoreApplication::translate("TabHelp",
                "🧾 Aqui você anota os gastos do dia a dia que não passam no cartão "
                "— PIX, débito, dinheiro ou boleto. Também separa gastos fixos "
                "(que se repetem) dos variáveis."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "Clique em \"➕ Novo gasto\"."),
                QCoreApplication::translate("TabHelp",
                    "Preencha Descrição, Categoria, Tipo de pagamento, Valor e "
                    "Data."),
                QCoreApplication::translate("TabHelp",
                    "Se quiser, informe o Beneficiário e uma Observação."),
                QCoreApplication::translate("TabHelp",
                    "No rodapé: total de Fixos (e %) e Variáveis do mês."),
                QCoreApplication::translate("TabHelp",
                    "Ctrl+← / Ctrl+→ mudam de mês; \"🔃 Organizar por ▾\" ordena."),
            }
        });

        m.insert(QStringLiteral("tab_cartoes"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Cartões"),
            QCoreApplication::translate("TabHelp",
                "💳 Aqui você registra as compras feitas no cartão de crédito — à "
                "vista ou parceladas. O app distribui as parcelas por mês e "
                "calcula os juros e a tabela de amortização."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "Clique em <b>➕ Novo item</b> para adicionar uma compra."),
                QCoreApplication::translate("TabHelp",
                    "Escolha o <b>cartão</b>, a <b>descrição</b>, o <b>valor "
                    "total</b> e o <b>nº de parcelas</b>."),
                QCoreApplication::translate("TabHelp",
                    "💰 Se houver <b>juros</b>, informe a taxa — o app calcula o "
                    "<b>CET</b> e a <b>tabela de amortização</b> (Price)."),
                QCoreApplication::translate("TabHelp",
                    "📅 As parcelas aparecem <b>por mês</b> na tabela; navegue com "
                    "as setas <b>◀️ ▶️</b>."),
                QCoreApplication::translate("TabHelp",
                    "Quer simular antes de comprar? Use a <b>calculadora 🧮</b> "
                    "na barra superior do app."),
            }
        });

        m.insert(QStringLiteral("tab_recorrencias"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Recorrências"),
            QCoreApplication::translate("TabHelp",
                "🔁 Aqui você cadastra entradas e gastos que se repetem sempre — "
                "salário, assinatura, mensalidade. O app cria os lançamentos "
                "sozinho na data certa."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "Clique em \"➕ Adicionar\": Entrada ou Gasto, valor, "
                    "categoria e Frequência (Semanal/Quinzenal/Mensal/Anual)."),
                QCoreApplication::translate("TabHelp",
                    "Defina início (e fim, se quiser)."),
                QCoreApplication::translate("TabHelp",
                    "A coluna \"Próxima\" mostra o próximo lançamento — gerado "
                    "automático ao abrir o app."),
                QCoreApplication::translate("TabHelp",
                    "\"✏️ Editar\", \"🗑️ Excluir\" ou \"⏯️ Ativar/Pausar\" (pausa sem "
                    "apagar)."),
                QCoreApplication::translate("TabHelp",
                    "Excluir uma recorrência NÃO apaga os lançamentos já "
                    "criados."),
            }
        });

        m.insert(QStringLiteral("tab_dividas"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Dívidas"),
            QCoreApplication::translate("TabHelp",
                "📉 Aqui você acompanha dívidas que tem a pagar e empréstimos que "
                "fez a alguém. O app mostra quanto já foi pago, quanto falta "
                "(saldo) e os juros (a.m. e o equivalente a.a.)."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "\"➕ Nova\": Dívida ou Empréstimo, a Contraparte, valor "
                    "total, juros a.m. e vencimento."),
                QCoreApplication::translate("TabHelp",
                    "Conforme paga, edite o valor pago — o Saldo é recalculado."),
                QCoreApplication::translate("TabHelp",
                    "\"✏️ Editar\", \"🗑️ Excluir\" ou \"✅ Quitar\" (paga de uma vez)."),
                QCoreApplication::translate("TabHelp",
                    "Filtros 🔎 \"Tipo:\" e \"Status:\" pra ver só dívidas, "
                    "atrasados, etc."),
                QCoreApplication::translate("TabHelp",
                    "Cores: fundo avermelhado = atrasado; esverdeado = quitado. "
                    "Última linha soma os totais."),
            }
        });

        m.insert(QStringLiteral("tab_metas"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Metas"),
            QCoreApplication::translate("TabHelp",
                "🎯 Aqui você cria objetivos de poupança — uma viagem, uma reserva "
                "de emergência — e acompanha quanto já juntou em relação ao "
                "valor desejado."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "\"➕ Nova\": nome, categoria, valor a juntar e (se quiser) um "
                    "prazo."),
                QCoreApplication::translate("TabHelp",
                    "Ao guardar dinheiro, selecione a meta e \"💰 Aporte\" pra "
                    "somar."),
                QCoreApplication::translate("TabHelp",
                    "A barra de Progresso mostra a %; \"Restam\" mostra os dias "
                    "até o prazo."),
                QCoreApplication::translate("TabHelp",
                    "Ao atingir o valor, vira \"Concluída\" automaticamente."),
                QCoreApplication::translate("TabHelp",
                    "\"✏️ Editar\"/\"🗑️ Excluir\" e filtro 🔎 \"Status:\"."),
            }
        });

        m.insert(QStringLiteral("tab_planejamento"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Planejamento/Orçamento"),
            QCoreApplication::translate("TabHelp",
                "📋 Aqui você monta um orçamento mensal: define quanto pretende "
                "receber e gastar por categoria, e o app compara com o que "
                "realmente aconteceu."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "Setas ◀️ ▶️ escolhem o 📅 mês."),
                QCoreApplication::translate("TabHelp",
                    "\"➕ Novo item\": linha de Receita ou Despesa com o valor "
                    "planejado."),
                QCoreApplication::translate("TabHelp",
                    "\"Planejado\" = o que você definiu; \"Real\" = o que de fato "
                    "entrou/saiu (das abas Entradas, Gastos e Parcelas)."),
                QCoreApplication::translate("TabHelp",
                    "\"Diferença\" e \"%\" mostram se ficou dentro do plano."),
                QCoreApplication::translate("TabHelp",
                    "As cores mostram: 🟢 bem dentro do plano · 🔵 dentro · 🔴 "
                    "estourou (fora do esperado)."),
            }
        });

        m.insert(QStringLiteral("tab_tendencias"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Tendências"),
            QCoreApplication::translate("TabHelp",
                "📈 Aqui você visualiza seus gastos em gráficos, pra enxergar "
                "padrões: quanto gasta por mês, em quais categorias e por forma "
                "de pagamento."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "Escolha \"De:\" e \"Até:\" (mês/ano) e clique \"🔄 Aplicar\"."),
                QCoreApplication::translate("TabHelp",
                    "Troque os gráficos no topo: \"Mensal\", \"Por Categoria\", "
                    "\"Por Tipo de Pagamento\", \"Fixo × Variável\"."),
                QCoreApplication::translate("TabHelp",
                    "O filtro \"Categoria:\" afeta só o gráfico \"Por "
                    "Categoria\"."),
                QCoreApplication::translate("TabHelp",
                    "Resumo embaixo: Total, Média/mês, mês de Pico, Maior "
                    "categoria."),
                QCoreApplication::translate("TabHelp",
                    "O gráfico \"Mensal\" já soma as parcelas do cartão aos "
                    "gastos avulsos."),
            }
        });

        m.insert(QStringLiteral("tab_investimentos"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Investimentos"),
            QCoreApplication::translate("TabHelp",
                "📊 Aqui você registra sua carteira: quanto colocou (aporte) e "
                "quanto retirou (resgate) em cada investimento — ações, fundos, "
                "renda fixa. O app calcula o saldo líquido por mês."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "\"➕ Novo investimento\": Nome do ativo, Categoria, Tipo, "
                    "Operação (aporte ou resgate), Valor e Data."),
                QCoreApplication::translate("TabHelp",
                    "A linha \"SALDO\" de cada mês mostra o líquido (aportes − "
                    "resgates)."),
                QCoreApplication::translate("TabHelp",
                    "\"📊 Gráfico ▾\": distribuição por tipo de ativo + evolução de "
                    "12 meses."),
                QCoreApplication::translate("TabHelp",
                    "Ctrl+← / Ctrl+→ mudam de mês; \"🔃 Organizar por ▾\" ordena."),
                QCoreApplication::translate("TabHelp",
                    "Clique pra ✏️ editar; \"🗑️ Excluir\" pra apagar."),
            }
        });

        m.insert(QStringLiteral("tab_proventos"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Proventos"),
            QCoreApplication::translate("TabHelp",
                "💰 Aqui você anota os rendimentos que seus investimentos pagam — "
                "dividendos, JCP (juros sobre capital próprio) e rendimentos de "
                "fundos imobiliários (FII). Acompanhe quanto a carteira rende."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "\"➕ Novo…\": Data, Ticker (código do ativo, ex.: PETR4), Tipo "
                    "(Dividendo/JCP/Rendimento FII/Bonificação), Bruto, Líquido e "
                    "Quantidade."),
                QCoreApplication::translate("TabHelp",
                    "Filtros 🔎 \"Ticker:\" e \"Tipo:\" + \"🔎 Filtrar\"."),
                QCoreApplication::translate("TabHelp",
                    "No rodapé: total líquido recebido no ano."),
                QCoreApplication::translate("TabHelp",
                    "\"✏️ Editar\"/\"🗑️ Excluir\" (ou duplo-clique pra editar)."),
            }
        });

        m.insert(QStringLiteral("tab_bolsa"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Bolsa"),
            QCoreApplication::translate("TabHelp",
                "🏦 Aqui você busca ativos da bolsa pelo código (ticker) pra "
                "consultar cotações. A fonte dos dados é configurada nas "
                "Preferências."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "Escolha a fonte em \"Provider:\" (ou em Preferências → "
                    "Mercado)."),
                QCoreApplication::translate("TabHelp",
                    "Digite um ticker (ex.: PETR4, MGLU3) e clique \"🔍 Buscar\" "
                    "(ou Enter)."),
                QCoreApplication::translate("TabHelp",
                    "Resultados na tabela: Ticker, Nome, Último, Variação %, "
                    "Volume."),
                QCoreApplication::translate("TabHelp",
                    "Cotações em tempo real, gráficos e indicadores ainda estão "
                    "em desenvolvimento — alguns campos aparecem como \"—\"."),
            }
        });

        m.insert(QStringLiteral("tab_lembretes"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Lembretes"),
            QCoreApplication::translate("TabHelp",
                "🔔 Aqui você cadastra contas a pagar pra não esquecer nenhum "
                "vencimento — água, luz, assinaturas. O app destaca o que vence "
                "em breve ou já venceu, com cores."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "\"➕ Novo\": Descrição, Valor, Vencimento, Frequência e se é "
                    "débito automático ou Manual."),
                QCoreApplication::translate("TabHelp",
                    "Ao resolver, selecione e use \"✅ Pagar\"; \"⚠️ Não debitado\" se "
                    "o automático não caiu; \"🚫 Ignorar\" pra dispensar."),
                QCoreApplication::translate("TabHelp",
                    "Lembrete recorrente pago → o app cria o próximo vencimento "
                    "sozinho."),
                QCoreApplication::translate("TabHelp",
                    "Cores: 🔴 vencido/não debitado · ⚪ vence hoje · 🟡 vence em "
                    "N dias (N≤3) · 🟢 resolvido."),
                QCoreApplication::translate("TabHelp",
                    "Filtro 🔎 \"Mostrar:\" + \"✏️ Editar\"/\"🗑️ Excluir\"."),
            }
        });

        return m;
    }();
    return mapa;
}

const QStringList &tabHelpOrder()
{
    // Ordem visual das abas (mesma sequência dos addTab em MainWindow::buildUi).
    static const QStringList ordem = {
        QStringLiteral("tab_dashboard"),
        QStringLiteral("tab_entradas"),
        QStringLiteral("tab_gastos"),
        QStringLiteral("tab_cartoes"),
        QStringLiteral("tab_recorrencias"),
        QStringLiteral("tab_dividas"),
        QStringLiteral("tab_metas"),
        QStringLiteral("tab_planejamento"),
        QStringLiteral("tab_tendencias"),
        QStringLiteral("tab_investimentos"),
        QStringLiteral("tab_proventos"),
        QStringLiteral("tab_bolsa"),
        QStringLiteral("tab_lembretes"),
    };
    return ordem;
}

TabHelpEntry tabHelpEntry(const QString &tabKey)
{
    return tabHelp().value(tabKey);
}
