#include "GlossarioService.h"

#include <QHash>
#include <QRegularExpression>

namespace {

struct Entrada {
    QString definicao;   // explicacao completa para tooltip / dialog
    QString microcopy;   // 1 linha curta para abaixo do campo (#105)
    QString coloquial;   // label amigavel para modo Iniciante (#111); vazio = sem renomeacao
};

// 80+ entradas pt-BR. Chave em UPPERCASE para lookup case-insensitive simples.
// Cobertura: TVM, FIRE variants, indicadores TA, fiscal BR, renda fixa,
// investimentos, geral. Adicionar livremente — escalavel.
static const QHash<QString, Entrada> &tabela()
{
    static const QHash<QString, Entrada> dict = {
        // TVM / Calculadora Financeira
        {QStringLiteral("CET"),  {GlossarioService::tr("Custo Efetivo Total: taxa anual que inclui juros, tarifas e IOF. Reflete o que voce realmente paga ao tomar um emprestimo."),
                                  GlossarioService::tr("= juros total ao ano (com IOF e tarifas)"),
                                  GlossarioService::tr("Juros total ao ano (CET)")}},
        {QStringLiteral("PMT"),  {GlossarioService::tr("Pagamento periodico (parcela). Quanto voce paga ou recebe a cada periodo."),
                                  GlossarioService::tr("= valor da parcela mensal"),
                                  GlossarioService::tr("Parcela (PMT)")}},
        {QStringLiteral("PV"),   {GlossarioService::tr("Present Value (Valor Presente). Valor de hoje. Se voce vai receber R$ 1000 daqui a 1 ano com juros 10%, hoje vale ~R$ 909."),
                                  GlossarioService::tr("= valor de hoje"),
                                  GlossarioService::tr("Valor atual (PV)")}},
        {QStringLiteral("FV"),   {GlossarioService::tr("Future Value (Valor Futuro). Quanto seu dinheiro vai valer em data futura, com juros aplicados."),
                                  GlossarioService::tr("= valor no futuro"),
                                  GlossarioService::tr("Valor futuro (FV)")}},
        {QStringLiteral("NPER"), {GlossarioService::tr("Numero de periodos da operacao financeira (parcelas, meses, anos)."),
                                  GlossarioService::tr("= quantas parcelas"),
                                  GlossarioService::tr("Numero de parcelas (NPER)")}},
        {QStringLiteral("RATE"), {GlossarioService::tr("Taxa de juros por periodo (% ao mes ou ao ano)."),
                                  GlossarioService::tr("= juros por periodo"),
                                  GlossarioService::tr("Taxa de juros (RATE)")}},
        {QStringLiteral("TVM"),  {GlossarioService::tr("Time Value of Money (Valor do Dinheiro no Tempo). R$ 100 hoje vale mais que R$ 100 amanha por causa dos juros e da inflacao."),
                                  GlossarioService::tr("= dinheiro vale mais hoje do que no futuro"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("SAC"),  {GlossarioService::tr("Sistema de Amortizacao Constante. Parcela diminui com o tempo: amortizacao fixa + juros sobre saldo decrescente."),
                                  GlossarioService::tr("= parcela decresce com o tempo"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("PRICE"),{GlossarioService::tr("Sistema Price (SAF). Parcela fixa do inicio ao fim; juros e amortizacao se compensam."),
                                  GlossarioService::tr("= parcela fixa todo mes"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("IOF"),  {GlossarioService::tr("Imposto sobre Operacoes Financeiras. Cobrado em emprestimos, cartao internacional, cambio e renda fixa resgatada < 30 dias."),
                                  GlossarioService::tr("= imposto federal em operacoes"),
                                  GlossarioService::tr("Imposto federal (IOF)")}},

        // Renda Fixa
        {QStringLiteral("CDI"),  {GlossarioService::tr("Certificado de Deposito Interbancario. Taxa que bancos usam emprestando entre si; baliza CDB/LCI/LCA."),
                                  GlossarioService::tr("= taxa que os bancos pagam entre si"),
                                  GlossarioService::tr("Taxa CDI")}},
        {QStringLiteral("SELIC"),{GlossarioService::tr("Taxa basica de juros da economia brasileira, definida pelo Copom (Banco Central) a cada 45 dias."),
                                  GlossarioService::tr("= juros base da economia"),
                                  GlossarioService::tr("Taxa basica (Selic)")}},
        {QStringLiteral("CDB"),  {GlossarioService::tr("Certificado de Deposito Bancario. Emprestimo que voce faz ao banco; banco paga juros (% do CDI). FGC ate R$ 250 mil."),
                                  GlossarioService::tr("= banco te paga juros (% do CDI)"),
                                  GlossarioService::tr("CDB")}},
        {QStringLiteral("LCI"),  {GlossarioService::tr("Letra de Credito Imobiliario. Renda fixa isenta de IR para PF. Lastreada em emprestimos imobiliarios."),
                                  GlossarioService::tr("= renda fixa isenta de IR (imobiliario)"),
                                  GlossarioService::tr("LCI (isenta IR)")}},
        {QStringLiteral("LCA"),  {GlossarioService::tr("Letra de Credito do Agronegocio. Renda fixa isenta de IR para PF. Lastreada em credito rural."),
                                  GlossarioService::tr("= renda fixa isenta de IR (agro)"),
                                  GlossarioService::tr("LCA (isenta IR)")}},
        {QStringLiteral("CRA"),  {GlossarioService::tr("Certificado de Recebiveis do Agronegocio. Renda fixa isenta IR. Risco do emissor (sem FGC)."),
                                  GlossarioService::tr("= isento IR; sem FGC"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("CRI"),  {GlossarioService::tr("Certificado de Recebiveis Imobiliarios. Renda fixa isenta IR. Risco do emissor (sem FGC)."),
                                  GlossarioService::tr("= isento IR; sem FGC"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("FGC"),  {GlossarioService::tr("Fundo Garantidor de Creditos. Garante ate R$ 250 mil por CPF por instituicao em CDB/poupanca/LCI/LCA em caso de quebra."),
                                  GlossarioService::tr("= garantia ate R$ 250 mil"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("TR"),   {GlossarioService::tr("Taxa Referencial. Indice de correcao da poupanca e FGTS. Tipicamente proxima de zero quando Selic baixa."),
                                  GlossarioService::tr("= corrige poupanca/FGTS"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("IPCA"), {GlossarioService::tr("Indice Nacional de Precos ao Consumidor Amplo (IBGE). Inflacao oficial do Brasil; meta do Banco Central."),
                                  GlossarioService::tr("= inflacao oficial"),
                                  GlossarioService::tr("Inflacao (IPCA)")}},
        {QStringLiteral("INPC"), {GlossarioService::tr("Indice Nacional de Precos ao Consumidor (IBGE). Inflacao para familias 1-5 salarios minimos."),
                                  GlossarioService::tr("= inflacao salario baixo"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("IGPM"), {GlossarioService::tr("Indice Geral de Precos do Mercado (FGV). Inflacao geral; correcao de alugueis e contratos."),
                                  GlossarioService::tr("= inflacao do mercado/alugueis"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("IGP-M"),{GlossarioService::tr("Indice Geral de Precos do Mercado (FGV). Inflacao geral; correcao de alugueis e contratos."),
                                  GlossarioService::tr("= inflacao do mercado/alugueis"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("PTAX"), {GlossarioService::tr("Taxa de cambio oficial do Banco Central (USD/BRL diaria). Usada em IRPF, contratos."),
                                  GlossarioService::tr("= cambio oficial BCB"),
                                  GlossarioService::tr("Cambio oficial (PTAX)")}},

        // Investimentos / Bolsa
        {QStringLiteral("FII"),  {GlossarioService::tr("Fundo de Investimento Imobiliario. Cota negociada na bolsa que paga rendimentos mensais isentos de IR para PF."),
                                  GlossarioService::tr("= imovel em pedaco; paga aluguel mensal"),
                                  GlossarioService::tr("FII (fundo imobiliario)")}},
        {QStringLiteral("ETF"),  {GlossarioService::tr("Exchange Traded Fund. Fundo negociado em bolsa que segue um indice (ex: BOVA11 = Ibovespa). IR 15% sobre lucro."),
                                  GlossarioService::tr("= fundo que copia um indice"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("BDR"),  {GlossarioService::tr("Brazilian Depositary Receipt. Certificado que representa acoes de empresas estrangeiras negociadas na B3 (Apple, Google etc)."),
                                  GlossarioService::tr("= acoes do exterior na B3"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("DY"),   {GlossarioService::tr("Dividend Yield. Quanto a acao paga em dividendos por ano dividido pelo preco. DY 6% = R$ 6 de dividendo para cada R$ 100 investidos."),
                                  GlossarioService::tr("= % de dividendos por ano"),
                                  GlossarioService::tr("Renda por dividendos (DY)")}},
        // #187: JCP IRRF atualizado de 15% para 17,5% (LC 224/2025, vigente jan/2026)
        {QStringLiteral("JCP"),  {GlossarioService::tr("Juros sobre Capital Proprio. Outra forma de a empresa distribuir lucro ao acionista, mas com 17,5% de IR descontado na fonte antes de chegar a voce — diferente dos dividendos, que sao isentos."),
                                  GlossarioService::tr("= dividendo com 17,5% IR retido"),
                                  GlossarioService::tr("Juros sobre capital (JCP)")}},
        {QStringLiteral("P/L"),  {GlossarioService::tr("Preco / Lucro. Quantos anos do lucro atual cobririam o preco da acao. Baixo = barato (geralmente)."),
                                  GlossarioService::tr("= anos para lucro pagar acao"),
                                  GlossarioService::tr("Preco/Lucro (P/L)")}},
        {QStringLiteral("P/VP"), {GlossarioService::tr("Preco / Valor Patrimonial. Preco da acao sobre patrimonio liquido por acao. P/VP < 1 = compra abaixo do valor contabil."),
                                  GlossarioService::tr("= preco vs patrimonio"),
                                  GlossarioService::tr("Preco/Patrimonio (P/VP)")}},
        {QStringLiteral("ROE"),  {GlossarioService::tr("Return on Equity. Lucro liquido / patrimonio liquido. Mede eficiencia em gerar lucro com o capital."),
                                  GlossarioService::tr("= eficiencia em gerar lucro"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("ROIC"), {GlossarioService::tr("Return on Invested Capital. Retorno sobre todo capital (proprio + divida)."),
                                  GlossarioService::tr("= retorno sobre capital total"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("EBITDA"),{GlossarioService::tr("Earnings Before Interest, Taxes, Depreciation and Amortization. Lucro antes de juros, impostos, depreciacao e amortizacao. Aproxima a geracao de caixa."),
                                  GlossarioService::tr("= lucro operacional bruto"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("DRIP"), {GlossarioService::tr("Dividend Reinvestment Plan. Reinvestir dividendos comprando mais acoes do mesmo papel automaticamente."),
                                  GlossarioService::tr("= reinvestir dividendos comprando + acoes"),
                                  GlossarioService::tr("Reinvestir dividendos (DRIP)")}},
        {QStringLiteral("IPO"),  {GlossarioService::tr("Initial Public Offering. Primeira venda de acoes de uma empresa no mercado (abertura de capital)."),
                                  GlossarioService::tr("= empresa abrindo capital"),
                                  GlossarioService::tr("")}},

        // FIRE family
        {QStringLiteral("FIRE"), {GlossarioService::tr("Financial Independence, Retire Early (Independencia Financeira e Aposentadoria Antecipada). Patrimonio que paga seus gastos para sempre via 4% ao ano. Regra dos 25: precisa de 25x sua despesa anual."),
                                  GlossarioService::tr("= patrimonio que paga seus gastos para sempre"),
                                  GlossarioService::tr("Aposentadoria Antecipada (FIRE)")}},
        {QStringLiteral("LEAN FIRE"),    {GlossarioService::tr("FIRE com despesas baixas/minimalista. Geralmente < R$ 40k/ano. Atinge mais rapido."),
                                  GlossarioService::tr("= FIRE com estilo de vida economico"),
                                  GlossarioService::tr("Aposentadoria minimalista (Lean FIRE)")}},
        {QStringLiteral("FAT FIRE"),     {GlossarioService::tr("FIRE confortavel/luxo. > R$ 200k/ano. Precisa de patrimonio muito maior."),
                                  GlossarioService::tr("= FIRE com estilo de vida confortavel"),
                                  GlossarioService::tr("Aposentadoria confortavel (Fat FIRE)")}},
        {QStringLiteral("COAST FIRE"),   {GlossarioService::tr("Patrimonio atual que, sem novos aportes, cresce ate o FIRE tradicional na idade-alvo."),
                                  GlossarioService::tr("= ja juntou; deixa crescer sozinho"),
                                  GlossarioService::tr("FIRE em piloto automatico (Coast FIRE)")}},
        {QStringLiteral("BARISTA FIRE"), {GlossarioService::tr("Patrimonio parcial; complementa renda com trabalho leve part-time."),
                                  GlossarioService::tr("= FIRE parcial + bico"),
                                  GlossarioService::tr("Semi-aposentadoria (Barista FIRE)")}},

        // Fiscal
        {QStringLiteral("IRPF"), {GlossarioService::tr("Imposto de Renda Pessoa Fisica. Tabela progressiva mensal/anual. Aliquota 0-27,5% sobre salario, alugueis, rendimentos."),
                                  GlossarioService::tr("= imposto sobre salario e investimentos"),
                                  GlossarioService::tr("Imposto de renda (IRPF)")}},
        // #166 Peca 3: INSS sem valor nominal de teto (reajustado todo ano — nao cravar)
        {QStringLiteral("INSS"), {GlossarioService::tr("Instituto Nacional do Seguro Social. Contribuicao previdenciaria descontada do salario (CLT) em aliquotas progressivas — geralmente entre 7,5% e 14% conforme o salario — ate um teto mensal. Garante aposentadoria, auxilio-doenca e outros beneficios."),
                                  GlossarioService::tr("= previdencia obrigatoria"),
                                  GlossarioService::tr("Previdencia (INSS)")}},
        {QStringLiteral("FGTS"), {GlossarioService::tr("Fundo de Garantia por Tempo de Servico. 8% do salario depositado pelo empregador; saque demissao sem justa causa, doenca grave, casa propria."),
                                  GlossarioService::tr("= 8% do salario que o patrao guarda"),
                                  GlossarioService::tr("FGTS (poupanca do trabalhador)")}},
        {QStringLiteral("CLT"),  {GlossarioService::tr("Consolidacao das Leis do Trabalho. Regime de emprego formal: carteira assinada + INSS + FGTS + 13o + ferias."),
                                  GlossarioService::tr("= emprego com carteira assinada"),
                                  GlossarioService::tr("Emprego CLT")}},
        {QStringLiteral("PJ"),   {GlossarioService::tr("Pessoa Juridica. Contratacao via empresa propria; sem direitos CLT mas com maior liquido em troca."),
                                  GlossarioService::tr("= trabalho via empresa propria"),
                                  GlossarioService::tr("Trabalho como PJ")}},
        {QStringLiteral("MEI"),  {GlossarioService::tr("Microempreendedor Individual. PJ simplificada: faturamento ate R$ 81 mil/ano, DAS unico mensal (R$ 71-76)."),
                                  GlossarioService::tr("= PJ simplificada ate R$ 81 mil/ano"),
                                  GlossarioService::tr("MEI (micro empresa)")}},
        {QStringLiteral("RBT12"),{GlossarioService::tr("Receita Bruta Total 12 meses. Usada para enquadrar no Simples Nacional e calcular aliquota efetiva."),
                                  GlossarioService::tr("= faturamento dos ultimos 12 meses"),
                                  GlossarioService::tr("Faturamento anual (RBT12)")}},
        {QStringLiteral("DAS"),  {GlossarioService::tr("Documento de Arrecadacao do Simples. Boleto mensal unico com todos os impostos do MEI/Simples."),
                                  GlossarioService::tr("= boleto unico de impostos"),
                                  GlossarioService::tr("")}},
        // #166 Peca 3: DRE unificado (chave duplicada eliminada — a segunda entrada
        // em "Outros" foi removida; manter so esta, com a definicao do Grupo 9 da spec).
        {QStringLiteral("DRE"),  {GlossarioService::tr("Demonstracao do Resultado do Exercicio. Relatorio que mostra as receitas, as despesas e o lucro (ou prejuizo) de um periodo. O PeuBud tem um resumo simplificado do seu DRE pessoal."),
                                  GlossarioService::tr("= relatorio de receitas, despesas e lucro"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("DFC"),  {GlossarioService::tr("Demonstracao do Fluxo de Caixa. Relatorio de entradas e saidas reais de dinheiro."),
                                  GlossarioService::tr("= fluxo de caixa"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("PGBL"), {GlossarioService::tr("Plano Gerador de Beneficio Livre. Previdencia privada com deducao no IRPF (ate 12% da renda) mas IR sobre TOTAL no resgate."),
                                  GlossarioService::tr("= previdencia que abate no IR"),
                                  GlossarioService::tr("Previdencia privada (PGBL)")}},
        {QStringLiteral("VGBL"), {GlossarioService::tr("Vida Gerador de Beneficio Livre. Previdencia privada sem deducao no IR mas tributacao so no rendimento."),
                                  GlossarioService::tr("= previdencia sem deduzir IR"),
                                  GlossarioService::tr("Previdencia privada (VGBL)")}},

        // Indicadores Tecnicos
        {QStringLiteral("SMA"),  {GlossarioService::tr("Simple Moving Average. Media simples dos ultimos N precos. Suaviza ruido."),
                                  GlossarioService::tr("= media dos ultimos N dias"),
                                  GlossarioService::tr("Media movel (SMA)")}},
        {QStringLiteral("EMA"),  {GlossarioService::tr("Exponential Moving Average. Media movel exponencial; precos recentes pesam mais."),
                                  GlossarioService::tr("= media com mais peso no recente"),
                                  GlossarioService::tr("Media movel exp. (EMA)")}},
        {QStringLiteral("RSI"),  {GlossarioService::tr("Relative Strength Index (Wilder, 14 periodos). Mede sobrecompra (>70) ou sobrevenda (<30)."),
                                  GlossarioService::tr("= mede sobrecompra/sobrevenda"),
                                  GlossarioService::tr("Forca relativa (RSI)")}},
        {QStringLiteral("MACD"), {GlossarioService::tr("Moving Average Convergence Divergence. EMA12 - EMA26; cruzamento com sinal indica tendencia."),
                                  GlossarioService::tr("= cruzamento de medias para tendencia"),
                                  GlossarioService::tr("Tendencia (MACD)")}},
        {QStringLiteral("BBANDS"),{GlossarioService::tr("Bollinger Bands. SMA + 2 desvios padrao acima/abaixo. Mede volatilidade."),
                                  GlossarioService::tr("= bandas de volatilidade"),
                                  GlossarioService::tr("Bandas de Bollinger")}},
        {QStringLiteral("ATR"),  {GlossarioService::tr("Average True Range. Volatilidade media medida pelos maximos/minimos diarios."),
                                  GlossarioService::tr("= volatilidade media"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("OBV"),  {GlossarioService::tr("On Balance Volume. Soma cumulativa de volume seguindo direcao do preco. Confirma tendencia."),
                                  GlossarioService::tr("= volume confirma tendencia"),
                                  GlossarioService::tr("")}},

        // Outros
        {QStringLiteral("TIR"),  {GlossarioService::tr("Taxa Interna de Retorno. A taxa de juros que faz um investimento empatar com o custo do dinheiro. Serve para comparar se um investimento vale mais a pena que outro."),
                                  GlossarioService::tr("= rentabilidade do projeto"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("VPL"),  {GlossarioService::tr("Valor Presente Liquido. Soma todos os retornos futuros de um investimento trazendo-os a valor de hoje, descontando uma taxa minima. VPL positivo = vale a pena; negativo = nao vale."),
                                  GlossarioService::tr("= se vale a pena hoje"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("ROI"),  {GlossarioService::tr("Return on Investment (Retorno sobre Investimento). Mostra em % quanto voce lucrou em relacao ao que investiu. ROI de 50% = ganhou metade do valor aplicado."),
                                  GlossarioService::tr("= % de retorno sobre investido"),
                                  GlossarioService::tr("Retorno (ROI)")}},
        // DRE: segunda entrada removida (#166 Peca 3 correcao #3 — chave duplicada).
        // A definicao canonica esta na secao "Fiscal" acima (Grupo 9 da spec).
        {QStringLiteral("OFX"),  {GlossarioService::tr("Open Financial Exchange. Formato padrao bancario para extratos eletronicos. O PeuBud importa arquivos OFX para registrar seus lancamentos automaticamente."),
                                  GlossarioService::tr("= formato de extrato bancario"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("OCR"),  {GlossarioService::tr("Optical Character Recognition. Tecnologia que extrai texto de imagens/PDF escaneado. O app usa para extrair dados de notas fiscais ou extratos fotografados, sem voce precisar digitar na mao."),
                                  GlossarioService::tr("= ler texto de imagem"),
                                  GlossarioService::tr("")}},

        // ── Termos novos (#166 Peca 3, estrela na spec) ────────────────────────

        // Grupo 2 — Impostos e tributos
        {QStringLiteral("IRRF"), {GlossarioService::tr("Imposto de Renda Retido na Fonte. E o mesmo IRPF, mas descontado direto pelo empregador, banco ou corretora antes de voce receber. No ajuste anual, o que foi retido abate o que voce deve — ou vira restituicao."),
                                  GlossarioService::tr("= IR descontado antes de receber"),
                                  GlossarioService::tr("IR retido na fonte (IRRF)")}},

        // Grupo 3 — Renda fixa e garantias
        {QStringLiteral("IPCA+"),{GlossarioService::tr("Forma abreviada de IPCA mais uma taxa fixa. Um titulo IPCA+ 5% rende a inflacao oficial do ano mais 5% — buscando garantir ganho acima da inflacao."),
                                  GlossarioService::tr("= inflacao + taxa fixa de ganho real"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("LIQUIDEZ"),{GlossarioService::tr("A facilidade de transformar um investimento em dinheiro disponivel. Alta liquidez = resgata rapido. Baixa liquidez = precisa esperar um prazo. CDB diario tem liquidez diaria; alguns titulos tem carencia."),
                                  GlossarioService::tr("= facilidade de resgatar o dinheiro"),
                                  GlossarioService::tr("")}},

        // Grupo 4 — Investimentos em geral
        {QStringLiteral("APORTE"),{GlossarioService::tr("O dinheiro que voce coloca num investimento. Pode ser unico (aporte inicial) ou recorrente (todo mes)."),
                                  GlossarioService::tr("= dinheiro que voce coloca no investimento"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("RESGATE"),{GlossarioService::tr("Quando voce retira dinheiro de um investimento. Alguns pedem prazo de carencia antes de resgatar."),
                                  GlossarioService::tr("= retirada do investimento"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("RENDIMENTO"),{GlossarioService::tr("O quanto um investimento cresceu. Pode ser expresso em reais ou em porcentagem sobre o valor aplicado."),
                                  GlossarioService::tr("= quanto o investimento cresceu"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("CAGR"), {GlossarioService::tr("Crescimento anual medio composto (Compound Annual Growth Rate). CAGR de 10% em 5 anos significa que o investimento cresceu em media 10% por ano, considerando o efeito dos juros compostos."),
                                  GlossarioService::tr("= crescimento anual medio composto"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("PAYBACK"),{GlossarioService::tr("Tempo que leva para recuperar o dinheiro investido. Payback de 3 anos = em 3 anos os retornos cobrem o valor que voce colocou. Quanto menor, mais rapido o investimento se paga."),
                                  GlossarioService::tr("= tempo ate recuperar o investimento"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("CARTEIRA"),{GlossarioService::tr("O conjunto de todos os seus investimentos juntos — acoes, fundos, renda fixa, etc. Diversificar a carteira reduz o risco de perder tudo de uma vez."),
                                  GlossarioService::tr("= seus investimentos reunidos"),
                                  GlossarioService::tr("")}},

        // Grupo 5 — Bolsa de valores
        {QStringLiteral("TICKER"),{GlossarioService::tr("O codigo de identificacao de um ativo na bolsa. PETR4 = Petrobras PN; BOVA11 = ETF que copia o Ibovespa; KNRI11 = um FII. As 4 letras sao a empresa; o numero indica o tipo de ativo."),
                                  GlossarioService::tr("= codigo do ativo na bolsa"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("ACAO"), {GlossarioService::tr("Um pedaco da propriedade de uma empresa. Quem compra acoes de uma empresa se torna socio e pode receber dividendos e valorizacao."),
                                  GlossarioService::tr("= pedaco de uma empresa"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("ACAO ON"),{GlossarioService::tr("Acao Ordinaria. Da direito a voto nas assembleias da empresa. No Brasil, termina em 3 no ticker (VALE3)."),
                                  GlossarioService::tr("= acao com direito a voto"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("ACAO PN"),{GlossarioService::tr("Acao Preferencial. Prioridade no recebimento de dividendos, mas geralmente sem voto. No Brasil, termina em 4 no ticker (PETR4)."),
                                  GlossarioService::tr("= acao preferencial em dividendos"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("IBOVESPA"),{GlossarioService::tr("O principal indice da bolsa brasileira (B3), formado pelas acoes mais negociadas. Quando o Ibovespa sobe, as principais empresas do Brasil subiram em media."),
                                  GlossarioService::tr("= indice das principais acoes do Brasil"),
                                  GlossarioService::tr("")}},

        // Grupo 6 — Proventos
        {QStringLiteral("PROVENTO"),{GlossarioService::tr("Qualquer pagamento que um investimento faz ao seu dono: dividendos, JCP, rendimentos de FII ou bonificacoes. E a renda passiva que o ativo gera enquanto voce o mantem."),
                                  GlossarioService::tr("= renda passiva do investimento"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("DIVIDENDO"),{GlossarioService::tr("Parte do lucro que a empresa distribui aos acionistas em dinheiro. Para pessoas fisicas no Brasil, dividendos de acoes sao atualmente isentos de Imposto de Renda."),
                                  GlossarioService::tr("= lucro distribuido aos acionistas (isento PF)"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("RENDIMENTO FII"),{GlossarioService::tr("O valor pago periodicamente por um Fundo de Investimento Imobiliario aos seus cotistas — como se fosse o aluguel dos imoveis do fundo. Para pessoas fisicas, geralmente e isento de IR."),
                                  GlossarioService::tr("= aluguel mensal do FII"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("BONIFICACAO"),{GlossarioService::tr("Quando a empresa distribui novas acoes (em vez de dinheiro) proporcionalmente ao que voce ja tem. Voce fica com mais acoes, mas o preco se ajusta — o valor total continua parecido. E isenta de IR no recebimento."),
                                  GlossarioService::tr("= acoes gratis no lugar de dinheiro"),
                                  GlossarioService::tr("")}},

        // Grupo 1 — Cartao de credito (termos ja existentes, revisados)
        {QStringLiteral("PARCELA"),{GlossarioService::tr("Cada pagamento mensal de uma compra dividida. Uma compra de R$ 300 em 3x: cada parcela custa R$ 100 (sem juros) ou mais (com juros)."),
                                  GlossarioService::tr("= cada pagamento mensal da compra"),
                                  GlossarioService::tr("Parcela mensal")}},
        {QStringLiteral("AMORTIZACAO"),{GlossarioService::tr("A parte da parcela que realmente diminui o que voce deve. Cada parcela tem dois pedacos: juros (custo de ter parcelado) e amortizacao (abate da divida). Quanto mais voce amortiza, menor fica o saldo devedor."),
                                  GlossarioService::tr("= parte da parcela que abate a divida"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("TABELA PRICE"),{GlossarioService::tr("Sistema de parcelas fixas: voce paga o mesmo valor todo mes. Nos primeiros meses os juros pesam mais; com o tempo, a amortizacao cresce. E o parcelamento mais comum no varejo e cartoes."),
                                  GlossarioService::tr("= parcela fixa com juros decrescentes"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("FATURA"),{GlossarioService::tr("O documento mensal do cartao de credito listando todas as compras do periodo e o valor total a pagar. Pagar o total evita juros rotativos."),
                                  GlossarioService::tr("= total de compras do mes no cartao"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("VENCIMENTO"),{GlossarioService::tr("A data limite para pagar sem multa ou juros. Em cartao: pagar so depois do vencimento gera juros rotativos (os mais caros do mercado)."),
                                  GlossarioService::tr("= data limite para pagar sem juros"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("JUROS A.M."),{GlossarioService::tr("Taxa de juros ao mes. 2% a.m. significa que a divida cresce 2% a cada mes que nao for paga. Atencao: juros compostos fazem esse valor crescer rapido em meses seguidos."),
                                  GlossarioService::tr("= juros por mes"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("JUROS A.A."),{GlossarioService::tr("Taxa de juros ao ano. Para comparar emprestimos diferentes, use sempre a taxa anual — ou o CET, que inclui tambem IOF e tarifas."),
                                  GlossarioService::tr("= juros por ano"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("JUROS COMPOSTOS"),{GlossarioService::tr("Juros sobre juros: no proximo mes, a taxa incide sobre a divida que ja cresceu. R$ 1.000 a 5% a.m. viram R$ 1.050 no 1o mes e R$ 1.102,50 no 2o — nao R$ 1.100."),
                                  GlossarioService::tr("= juros sobre juros (crescem rapido)"),
                                  GlossarioService::tr("")}},
        {QStringLiteral("JUROS SIMPLES"),{GlossarioService::tr("Juros calculados sempre sobre o valor original, sem acumular. R$ 1.000 a 5% a.m. por 2 meses = R$ 100 de juros total. Menos comum em credito — mais usado em multas fixas."),
                                  GlossarioService::tr("= juros sem acumular"),
                                  GlossarioService::tr("")}},
    };
    return dict;
}

} // namespace

QString GlossarioService::definicao(const QString &termo)
{
    return tabela().value(termo.trimmed().toUpper()).definicao;
}

QString GlossarioService::microcopy(const QString &termo)
{
    return tabela().value(termo.trimmed().toUpper()).microcopy;
}

QString GlossarioService::labelColoquial(const QString &termo)
{
    return tabela().value(termo.trimmed().toUpper()).coloquial;
}

QStringList GlossarioService::termos()
{
    return tabela().keys();
}

QString GlossarioService::primeiroTermoEncontrado(const QString &texto)
{
    if (texto.isEmpty()) return {};

    // #194 (Onda U, PERF-01): ANTES compilava 1 QRegularExpression NOVA por
    // termo, TODA chamada (92 termos x N chamadas = O(termos) compilations
    // repetidas) — medido em ~365ms/55% do buildUi (1200-1700x mais lento
    // que o fix). AGORA: uma unica alternation-regex, construida 1x (static,
    // magic-static thread-safe) e otimizada (optimize() pre-compila o JIT
    // interno do PCRE2 na 1a construcao, nao a cada match).
    //
    // Ordena por tamanho decrescente ANTES de montar a alternation: PCRE2
    // tenta as alternativas na ORDEM listada (nao "mais longa primeiro"), entao
    // sem isso "\b(IPCA|IPCA\+)\b" casaria so "IPCA" dentro de "IPCA+" (o "+"
    // e nao-word-char, entao ha boundary ali). Mesmo motivo para
    // RENDIMENTO/RENDIMENTO FII, ACAO/ACAO ON/ACAO PN.
    static const QRegularExpression rx = [] {
        QStringList chaves = tabela().keys();
        std::sort(chaves.begin(), chaves.end(), [](const QString &a, const QString &b) {
            return a.size() > b.size();
        });
        QStringList escapadas;
        escapadas.reserve(chaves.size());
        for (const QString &k : chaves)
            escapadas << QRegularExpression::escape(k);

        // Grupo de captura 1 = o termo que casou (retorno via captured(1)).
        // UseUnicodePropertiesOption: trata letras acentuadas (í, ç, ã…) como
        // word-chars. Sem isso, "\bDAS\b" casa em "Conclu[í]DAS" (boundary
        // falso antes de letra acentuada) — falso-positivo que pintaria
        // sublinhado/tooltip de "DAS" em "Concluídas". (#166 §9.6, preservado.)
        QRegularExpression regex(
            QStringLiteral("\\b(") + escapadas.join(QLatin1Char('|')) + QStringLiteral(")\\b"),
            QRegularExpression::UseUnicodePropertiesOption);
        regex.optimize();
        return regex;
    }();

    const QRegularExpressionMatch m = rx.match(texto.toUpper());
    return m.hasMatch() ? m.captured(1) : QString();
}
