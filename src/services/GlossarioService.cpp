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
        {QStringLiteral("CET"),  {QStringLiteral("Custo Efetivo Total: taxa anual que inclui juros, tarifas e IOF. Reflete o que voce realmente paga ao tomar um emprestimo."),
                                  QStringLiteral("= juros total ao ano (com IOF e tarifas)"),
                                  QStringLiteral("Juros total ao ano (CET)")}},
        {QStringLiteral("PMT"),  {QStringLiteral("Pagamento periodico (parcela). Quanto voce paga ou recebe a cada periodo."),
                                  QStringLiteral("= valor da parcela mensal"),
                                  QStringLiteral("Parcela (PMT)")}},
        {QStringLiteral("PV"),   {QStringLiteral("Present Value (Valor Presente). Valor de hoje. Se voce vai receber R$ 1000 daqui a 1 ano com juros 10%, hoje vale ~R$ 909."),
                                  QStringLiteral("= valor de hoje"),
                                  QStringLiteral("Valor atual (PV)")}},
        {QStringLiteral("FV"),   {QStringLiteral("Future Value (Valor Futuro). Quanto seu dinheiro vai valer em data futura, com juros aplicados."),
                                  QStringLiteral("= valor no futuro"),
                                  QStringLiteral("Valor futuro (FV)")}},
        {QStringLiteral("NPER"), {QStringLiteral("Numero de periodos da operacao financeira (parcelas, meses, anos)."),
                                  QStringLiteral("= quantas parcelas"),
                                  QStringLiteral("Numero de parcelas (NPER)")}},
        {QStringLiteral("RATE"), {QStringLiteral("Taxa de juros por periodo (% ao mes ou ao ano)."),
                                  QStringLiteral("= juros por periodo"),
                                  QStringLiteral("Taxa de juros (RATE)")}},
        {QStringLiteral("TVM"),  {QStringLiteral("Time Value of Money (Valor do Dinheiro no Tempo). R$ 100 hoje vale mais que R$ 100 amanha por causa dos juros e da inflacao."),
                                  QStringLiteral("= dinheiro vale mais hoje do que no futuro"),
                                  QStringLiteral("")}},
        {QStringLiteral("SAC"),  {QStringLiteral("Sistema de Amortizacao Constante. Parcela diminui com o tempo: amortizacao fixa + juros sobre saldo decrescente."),
                                  QStringLiteral("= parcela decresce com o tempo"),
                                  QStringLiteral("")}},
        {QStringLiteral("PRICE"),{QStringLiteral("Sistema Price (SAF). Parcela fixa do inicio ao fim; juros e amortizacao se compensam."),
                                  QStringLiteral("= parcela fixa todo mes"),
                                  QStringLiteral("")}},
        {QStringLiteral("IOF"),  {QStringLiteral("Imposto sobre Operacoes Financeiras. Cobrado em emprestimos, cartao internacional, cambio e renda fixa resgatada < 30 dias."),
                                  QStringLiteral("= imposto federal em operacoes"),
                                  QStringLiteral("Imposto federal (IOF)")}},

        // Renda Fixa
        {QStringLiteral("CDI"),  {QStringLiteral("Certificado de Deposito Interbancario. Taxa que bancos usam emprestando entre si; baliza CDB/LCI/LCA."),
                                  QStringLiteral("= taxa que os bancos pagam entre si"),
                                  QStringLiteral("Taxa CDI")}},
        {QStringLiteral("SELIC"),{QStringLiteral("Taxa basica de juros da economia brasileira, definida pelo Copom (Banco Central) a cada 45 dias."),
                                  QStringLiteral("= juros base da economia"),
                                  QStringLiteral("Taxa basica (Selic)")}},
        {QStringLiteral("CDB"),  {QStringLiteral("Certificado de Deposito Bancario. Emprestimo que voce faz ao banco; banco paga juros (% do CDI). FGC ate R$ 250 mil."),
                                  QStringLiteral("= banco te paga juros (% do CDI)"),
                                  QStringLiteral("CDB")}},
        {QStringLiteral("LCI"),  {QStringLiteral("Letra de Credito Imobiliario. Renda fixa isenta de IR para PF. Lastreada em emprestimos imobiliarios."),
                                  QStringLiteral("= renda fixa isenta de IR (imobiliario)"),
                                  QStringLiteral("LCI (isenta IR)")}},
        {QStringLiteral("LCA"),  {QStringLiteral("Letra de Credito do Agronegocio. Renda fixa isenta de IR para PF. Lastreada em credito rural."),
                                  QStringLiteral("= renda fixa isenta de IR (agro)"),
                                  QStringLiteral("LCA (isenta IR)")}},
        {QStringLiteral("CRA"),  {QStringLiteral("Certificado de Recebiveis do Agronegocio. Renda fixa isenta IR. Risco do emissor (sem FGC)."),
                                  QStringLiteral("= isento IR; sem FGC"),
                                  QStringLiteral("")}},
        {QStringLiteral("CRI"),  {QStringLiteral("Certificado de Recebiveis Imobiliarios. Renda fixa isenta IR. Risco do emissor (sem FGC)."),
                                  QStringLiteral("= isento IR; sem FGC"),
                                  QStringLiteral("")}},
        {QStringLiteral("FGC"),  {QStringLiteral("Fundo Garantidor de Creditos. Garante ate R$ 250 mil por CPF por instituicao em CDB/poupanca/LCI/LCA em caso de quebra."),
                                  QStringLiteral("= garantia ate R$ 250 mil"),
                                  QStringLiteral("")}},
        {QStringLiteral("TR"),   {QStringLiteral("Taxa Referencial. Indice de correcao da poupanca e FGTS. Tipicamente proxima de zero quando Selic baixa."),
                                  QStringLiteral("= corrige poupanca/FGTS"),
                                  QStringLiteral("")}},
        {QStringLiteral("IPCA"), {QStringLiteral("Indice Nacional de Precos ao Consumidor Amplo (IBGE). Inflacao oficial do Brasil; meta do Banco Central."),
                                  QStringLiteral("= inflacao oficial"),
                                  QStringLiteral("Inflacao (IPCA)")}},
        {QStringLiteral("INPC"), {QStringLiteral("Indice Nacional de Precos ao Consumidor (IBGE). Inflacao para familias 1-5 salarios minimos."),
                                  QStringLiteral("= inflacao salario baixo"),
                                  QStringLiteral("")}},
        {QStringLiteral("IGPM"), {QStringLiteral("Indice Geral de Precos do Mercado (FGV). Inflacao geral; correcao de alugueis e contratos."),
                                  QStringLiteral("= inflacao do mercado/alugueis"),
                                  QStringLiteral("")}},
        {QStringLiteral("IGP-M"),{QStringLiteral("Indice Geral de Precos do Mercado (FGV). Inflacao geral; correcao de alugueis e contratos."),
                                  QStringLiteral("= inflacao do mercado/alugueis"),
                                  QStringLiteral("")}},
        {QStringLiteral("PTAX"), {QStringLiteral("Taxa de cambio oficial do Banco Central (USD/BRL diaria). Usada em IRPF, contratos."),
                                  QStringLiteral("= cambio oficial BCB"),
                                  QStringLiteral("Cambio oficial (PTAX)")}},

        // Investimentos / Bolsa
        {QStringLiteral("FII"),  {QStringLiteral("Fundo de Investimento Imobiliario. Cota negociada na bolsa que paga rendimentos mensais isentos de IR para PF."),
                                  QStringLiteral("= imovel em pedaco; paga aluguel mensal"),
                                  QStringLiteral("FII (fundo imobiliario)")}},
        {QStringLiteral("ETF"),  {QStringLiteral("Exchange Traded Fund. Fundo negociado em bolsa que segue um indice (ex: BOVA11 = Ibovespa). IR 15% sobre lucro."),
                                  QStringLiteral("= fundo que copia um indice"),
                                  QStringLiteral("")}},
        {QStringLiteral("BDR"),  {QStringLiteral("Brazilian Depositary Receipt. Certificado que representa acoes de empresas estrangeiras negociadas na B3 (Apple, Google etc)."),
                                  QStringLiteral("= acoes do exterior na B3"),
                                  QStringLiteral("")}},
        {QStringLiteral("DY"),   {QStringLiteral("Dividend Yield. Quanto a acao paga em dividendos por ano dividido pelo preco. DY 6% = R$ 6 de dividendo para cada R$ 100 investidos."),
                                  QStringLiteral("= % de dividendos por ano"),
                                  QStringLiteral("Renda por dividendos (DY)")}},
        // #187: JCP IRRF atualizado de 15% para 17,5% (LC 224/2025, vigente jan/2026)
        {QStringLiteral("JCP"),  {QStringLiteral("Juros sobre Capital Proprio. Outra forma de a empresa distribuir lucro ao acionista, mas com 17,5% de IR descontado na fonte antes de chegar a voce — diferente dos dividendos, que sao isentos."),
                                  QStringLiteral("= dividendo com 17,5% IR retido"),
                                  QStringLiteral("Juros sobre capital (JCP)")}},
        {QStringLiteral("P/L"),  {QStringLiteral("Preco / Lucro. Quantos anos do lucro atual cobririam o preco da acao. Baixo = barato (geralmente)."),
                                  QStringLiteral("= anos para lucro pagar acao"),
                                  QStringLiteral("Preco/Lucro (P/L)")}},
        {QStringLiteral("P/VP"), {QStringLiteral("Preco / Valor Patrimonial. Preco da acao sobre patrimonio liquido por acao. P/VP < 1 = compra abaixo do valor contabil."),
                                  QStringLiteral("= preco vs patrimonio"),
                                  QStringLiteral("Preco/Patrimonio (P/VP)")}},
        {QStringLiteral("ROE"),  {QStringLiteral("Return on Equity. Lucro liquido / patrimonio liquido. Mede eficiencia em gerar lucro com o capital."),
                                  QStringLiteral("= eficiencia em gerar lucro"),
                                  QStringLiteral("")}},
        {QStringLiteral("ROIC"), {QStringLiteral("Return on Invested Capital. Retorno sobre todo capital (proprio + divida)."),
                                  QStringLiteral("= retorno sobre capital total"),
                                  QStringLiteral("")}},
        {QStringLiteral("EBITDA"),{QStringLiteral("Earnings Before Interest, Taxes, Depreciation and Amortization. Lucro antes de juros, impostos, depreciacao e amortizacao. Aproxima a geracao de caixa."),
                                  QStringLiteral("= lucro operacional bruto"),
                                  QStringLiteral("")}},
        {QStringLiteral("DRIP"), {QStringLiteral("Dividend Reinvestment Plan. Reinvestir dividendos comprando mais acoes do mesmo papel automaticamente."),
                                  QStringLiteral("= reinvestir dividendos comprando + acoes"),
                                  QStringLiteral("Reinvestir dividendos (DRIP)")}},
        {QStringLiteral("IPO"),  {QStringLiteral("Initial Public Offering. Primeira venda de acoes de uma empresa no mercado (abertura de capital)."),
                                  QStringLiteral("= empresa abrindo capital"),
                                  QStringLiteral("")}},

        // FIRE family
        {QStringLiteral("FIRE"), {QStringLiteral("Financial Independence, Retire Early (Independencia Financeira e Aposentadoria Antecipada). Patrimonio que paga seus gastos para sempre via 4% ao ano. Regra dos 25: precisa de 25x sua despesa anual."),
                                  QStringLiteral("= patrimonio que paga seus gastos para sempre"),
                                  QStringLiteral("Aposentadoria Antecipada (FIRE)")}},
        {QStringLiteral("LEAN FIRE"),    {QStringLiteral("FIRE com despesas baixas/minimalista. Geralmente < R$ 40k/ano. Atinge mais rapido."),
                                  QStringLiteral("= FIRE com estilo de vida economico"),
                                  QStringLiteral("Aposentadoria minimalista (Lean FIRE)")}},
        {QStringLiteral("FAT FIRE"),     {QStringLiteral("FIRE confortavel/luxo. > R$ 200k/ano. Precisa de patrimonio muito maior."),
                                  QStringLiteral("= FIRE com estilo de vida confortavel"),
                                  QStringLiteral("Aposentadoria confortavel (Fat FIRE)")}},
        {QStringLiteral("COAST FIRE"),   {QStringLiteral("Patrimonio atual que, sem novos aportes, cresce ate o FIRE tradicional na idade-alvo."),
                                  QStringLiteral("= ja juntou; deixa crescer sozinho"),
                                  QStringLiteral("FIRE em piloto automatico (Coast FIRE)")}},
        {QStringLiteral("BARISTA FIRE"), {QStringLiteral("Patrimonio parcial; complementa renda com trabalho leve part-time."),
                                  QStringLiteral("= FIRE parcial + bico"),
                                  QStringLiteral("Semi-aposentadoria (Barista FIRE)")}},

        // Fiscal
        {QStringLiteral("IRPF"), {QStringLiteral("Imposto de Renda Pessoa Fisica. Tabela progressiva mensal/anual. Aliquota 0-27,5% sobre salario, alugueis, rendimentos."),
                                  QStringLiteral("= imposto sobre salario e investimentos"),
                                  QStringLiteral("Imposto de renda (IRPF)")}},
        // #166 Peca 3: INSS sem valor nominal de teto (reajustado todo ano — nao cravar)
        {QStringLiteral("INSS"), {QStringLiteral("Instituto Nacional do Seguro Social. Contribuicao previdenciaria descontada do salario (CLT) em aliquotas progressivas — geralmente entre 7,5% e 14% conforme o salario — ate um teto mensal. Garante aposentadoria, auxilio-doenca e outros beneficios."),
                                  QStringLiteral("= previdencia obrigatoria"),
                                  QStringLiteral("Previdencia (INSS)")}},
        {QStringLiteral("FGTS"), {QStringLiteral("Fundo de Garantia por Tempo de Servico. 8% do salario depositado pelo empregador; saque demissao sem justa causa, doenca grave, casa propria."),
                                  QStringLiteral("= 8% do salario que o patrao guarda"),
                                  QStringLiteral("FGTS (poupanca do trabalhador)")}},
        {QStringLiteral("CLT"),  {QStringLiteral("Consolidacao das Leis do Trabalho. Regime de emprego formal: carteira assinada + INSS + FGTS + 13o + ferias."),
                                  QStringLiteral("= emprego com carteira assinada"),
                                  QStringLiteral("Emprego CLT")}},
        {QStringLiteral("PJ"),   {QStringLiteral("Pessoa Juridica. Contratacao via empresa propria; sem direitos CLT mas com maior liquido em troca."),
                                  QStringLiteral("= trabalho via empresa propria"),
                                  QStringLiteral("Trabalho como PJ")}},
        {QStringLiteral("MEI"),  {QStringLiteral("Microempreendedor Individual. PJ simplificada: faturamento ate R$ 81 mil/ano, DAS unico mensal (R$ 71-76)."),
                                  QStringLiteral("= PJ simplificada ate R$ 81 mil/ano"),
                                  QStringLiteral("MEI (micro empresa)")}},
        {QStringLiteral("RBT12"),{QStringLiteral("Receita Bruta Total 12 meses. Usada para enquadrar no Simples Nacional e calcular aliquota efetiva."),
                                  QStringLiteral("= faturamento dos ultimos 12 meses"),
                                  QStringLiteral("Faturamento anual (RBT12)")}},
        {QStringLiteral("DAS"),  {QStringLiteral("Documento de Arrecadacao do Simples. Boleto mensal unico com todos os impostos do MEI/Simples."),
                                  QStringLiteral("= boleto unico de impostos"),
                                  QStringLiteral("")}},
        // #166 Peca 3: DRE unificado (chave duplicada eliminada — a segunda entrada
        // em "Outros" foi removida; manter so esta, com a definicao do Grupo 9 da spec).
        {QStringLiteral("DRE"),  {QStringLiteral("Demonstracao do Resultado do Exercicio. Relatorio que mostra as receitas, as despesas e o lucro (ou prejuizo) de um periodo. O PeuBud tem um resumo simplificado do seu DRE pessoal."),
                                  QStringLiteral("= relatorio de receitas, despesas e lucro"),
                                  QStringLiteral("")}},
        {QStringLiteral("DFC"),  {QStringLiteral("Demonstracao do Fluxo de Caixa. Relatorio de entradas e saidas reais de dinheiro."),
                                  QStringLiteral("= fluxo de caixa"),
                                  QStringLiteral("")}},
        {QStringLiteral("PGBL"), {QStringLiteral("Plano Gerador de Beneficio Livre. Previdencia privada com deducao no IRPF (ate 12% da renda) mas IR sobre TOTAL no resgate."),
                                  QStringLiteral("= previdencia que abate no IR"),
                                  QStringLiteral("Previdencia privada (PGBL)")}},
        {QStringLiteral("VGBL"), {QStringLiteral("Vida Gerador de Beneficio Livre. Previdencia privada sem deducao no IR mas tributacao so no rendimento."),
                                  QStringLiteral("= previdencia sem deduzir IR"),
                                  QStringLiteral("Previdencia privada (VGBL)")}},

        // Indicadores Tecnicos
        {QStringLiteral("SMA"),  {QStringLiteral("Simple Moving Average. Media simples dos ultimos N precos. Suaviza ruido."),
                                  QStringLiteral("= media dos ultimos N dias"),
                                  QStringLiteral("Media movel (SMA)")}},
        {QStringLiteral("EMA"),  {QStringLiteral("Exponential Moving Average. Media movel exponencial; precos recentes pesam mais."),
                                  QStringLiteral("= media com mais peso no recente"),
                                  QStringLiteral("Media movel exp. (EMA)")}},
        {QStringLiteral("RSI"),  {QStringLiteral("Relative Strength Index (Wilder, 14 periodos). Mede sobrecompra (>70) ou sobrevenda (<30)."),
                                  QStringLiteral("= mede sobrecompra/sobrevenda"),
                                  QStringLiteral("Forca relativa (RSI)")}},
        {QStringLiteral("MACD"), {QStringLiteral("Moving Average Convergence Divergence. EMA12 - EMA26; cruzamento com sinal indica tendencia."),
                                  QStringLiteral("= cruzamento de medias para tendencia"),
                                  QStringLiteral("Tendencia (MACD)")}},
        {QStringLiteral("BBANDS"),{QStringLiteral("Bollinger Bands. SMA + 2 desvios padrao acima/abaixo. Mede volatilidade."),
                                  QStringLiteral("= bandas de volatilidade"),
                                  QStringLiteral("Bandas de Bollinger")}},
        {QStringLiteral("ATR"),  {QStringLiteral("Average True Range. Volatilidade media medida pelos maximos/minimos diarios."),
                                  QStringLiteral("= volatilidade media"),
                                  QStringLiteral("")}},
        {QStringLiteral("OBV"),  {QStringLiteral("On Balance Volume. Soma cumulativa de volume seguindo direcao do preco. Confirma tendencia."),
                                  QStringLiteral("= volume confirma tendencia"),
                                  QStringLiteral("")}},

        // Outros
        {QStringLiteral("TIR"),  {QStringLiteral("Taxa Interna de Retorno. A taxa de juros que faz um investimento empatar com o custo do dinheiro. Serve para comparar se um investimento vale mais a pena que outro."),
                                  QStringLiteral("= rentabilidade do projeto"),
                                  QStringLiteral("")}},
        {QStringLiteral("VPL"),  {QStringLiteral("Valor Presente Liquido. Soma todos os retornos futuros de um investimento trazendo-os a valor de hoje, descontando uma taxa minima. VPL positivo = vale a pena; negativo = nao vale."),
                                  QStringLiteral("= se vale a pena hoje"),
                                  QStringLiteral("")}},
        {QStringLiteral("ROI"),  {QStringLiteral("Return on Investment (Retorno sobre Investimento). Mostra em % quanto voce lucrou em relacao ao que investiu. ROI de 50% = ganhou metade do valor aplicado."),
                                  QStringLiteral("= % de retorno sobre investido"),
                                  QStringLiteral("Retorno (ROI)")}},
        // DRE: segunda entrada removida (#166 Peca 3 correcao #3 — chave duplicada).
        // A definicao canonica esta na secao "Fiscal" acima (Grupo 9 da spec).
        {QStringLiteral("OFX"),  {QStringLiteral("Open Financial Exchange. Formato padrao bancario para extratos eletronicos. O PeuBud importa arquivos OFX para registrar seus lancamentos automaticamente."),
                                  QStringLiteral("= formato de extrato bancario"),
                                  QStringLiteral("")}},
        {QStringLiteral("OCR"),  {QStringLiteral("Optical Character Recognition. Tecnologia que extrai texto de imagens/PDF escaneado. O app usa para extrair dados de notas fiscais ou extratos fotografados, sem voce precisar digitar na mao."),
                                  QStringLiteral("= ler texto de imagem"),
                                  QStringLiteral("")}},

        // ── Termos novos (#166 Peca 3, estrela na spec) ────────────────────────

        // Grupo 2 — Impostos e tributos
        {QStringLiteral("IRRF"), {QStringLiteral("Imposto de Renda Retido na Fonte. E o mesmo IRPF, mas descontado direto pelo empregador, banco ou corretora antes de voce receber. No ajuste anual, o que foi retido abate o que voce deve — ou vira restituicao."),
                                  QStringLiteral("= IR descontado antes de receber"),
                                  QStringLiteral("IR retido na fonte (IRRF)")}},

        // Grupo 3 — Renda fixa e garantias
        {QStringLiteral("IPCA+"),{QStringLiteral("Forma abreviada de IPCA mais uma taxa fixa. Um titulo IPCA+ 5% rende a inflacao oficial do ano mais 5% — buscando garantir ganho acima da inflacao."),
                                  QStringLiteral("= inflacao + taxa fixa de ganho real"),
                                  QStringLiteral("")}},
        {QStringLiteral("LIQUIDEZ"),{QStringLiteral("A facilidade de transformar um investimento em dinheiro disponivel. Alta liquidez = resgata rapido. Baixa liquidez = precisa esperar um prazo. CDB diario tem liquidez diaria; alguns titulos tem carencia."),
                                  QStringLiteral("= facilidade de resgatar o dinheiro"),
                                  QStringLiteral("")}},

        // Grupo 4 — Investimentos em geral
        {QStringLiteral("APORTE"),{QStringLiteral("O dinheiro que voce coloca num investimento. Pode ser unico (aporte inicial) ou recorrente (todo mes)."),
                                  QStringLiteral("= dinheiro que voce coloca no investimento"),
                                  QStringLiteral("")}},
        {QStringLiteral("RESGATE"),{QStringLiteral("Quando voce retira dinheiro de um investimento. Alguns pedem prazo de carencia antes de resgatar."),
                                  QStringLiteral("= retirada do investimento"),
                                  QStringLiteral("")}},
        {QStringLiteral("RENDIMENTO"),{QStringLiteral("O quanto um investimento cresceu. Pode ser expresso em reais ou em porcentagem sobre o valor aplicado."),
                                  QStringLiteral("= quanto o investimento cresceu"),
                                  QStringLiteral("")}},
        {QStringLiteral("CAGR"), {QStringLiteral("Crescimento anual medio composto (Compound Annual Growth Rate). CAGR de 10% em 5 anos significa que o investimento cresceu em media 10% por ano, considerando o efeito dos juros compostos."),
                                  QStringLiteral("= crescimento anual medio composto"),
                                  QStringLiteral("")}},
        {QStringLiteral("PAYBACK"),{QStringLiteral("Tempo que leva para recuperar o dinheiro investido. Payback de 3 anos = em 3 anos os retornos cobrem o valor que voce colocou. Quanto menor, mais rapido o investimento se paga."),
                                  QStringLiteral("= tempo ate recuperar o investimento"),
                                  QStringLiteral("")}},
        {QStringLiteral("CARTEIRA"),{QStringLiteral("O conjunto de todos os seus investimentos juntos — acoes, fundos, renda fixa, etc. Diversificar a carteira reduz o risco de perder tudo de uma vez."),
                                  QStringLiteral("= seus investimentos reunidos"),
                                  QStringLiteral("")}},

        // Grupo 5 — Bolsa de valores
        {QStringLiteral("TICKER"),{QStringLiteral("O codigo de identificacao de um ativo na bolsa. PETR4 = Petrobras PN; BOVA11 = ETF que copia o Ibovespa; KNRI11 = um FII. As 4 letras sao a empresa; o numero indica o tipo de ativo."),
                                  QStringLiteral("= codigo do ativo na bolsa"),
                                  QStringLiteral("")}},
        {QStringLiteral("ACAO"), {QStringLiteral("Um pedaco da propriedade de uma empresa. Quem compra acoes de uma empresa se torna socio e pode receber dividendos e valorizacao."),
                                  QStringLiteral("= pedaco de uma empresa"),
                                  QStringLiteral("")}},
        {QStringLiteral("ACAO ON"),{QStringLiteral("Acao Ordinaria. Da direito a voto nas assembleias da empresa. No Brasil, termina em 3 no ticker (VALE3)."),
                                  QStringLiteral("= acao com direito a voto"),
                                  QStringLiteral("")}},
        {QStringLiteral("ACAO PN"),{QStringLiteral("Acao Preferencial. Prioridade no recebimento de dividendos, mas geralmente sem voto. No Brasil, termina em 4 no ticker (PETR4)."),
                                  QStringLiteral("= acao preferencial em dividendos"),
                                  QStringLiteral("")}},
        {QStringLiteral("IBOVESPA"),{QStringLiteral("O principal indice da bolsa brasileira (B3), formado pelas acoes mais negociadas. Quando o Ibovespa sobe, as principais empresas do Brasil subiram em media."),
                                  QStringLiteral("= indice das principais acoes do Brasil"),
                                  QStringLiteral("")}},

        // Grupo 6 — Proventos
        {QStringLiteral("PROVENTO"),{QStringLiteral("Qualquer pagamento que um investimento faz ao seu dono: dividendos, JCP, rendimentos de FII ou bonificacoes. E a renda passiva que o ativo gera enquanto voce o mantem."),
                                  QStringLiteral("= renda passiva do investimento"),
                                  QStringLiteral("")}},
        {QStringLiteral("DIVIDENDO"),{QStringLiteral("Parte do lucro que a empresa distribui aos acionistas em dinheiro. Para pessoas fisicas no Brasil, dividendos de acoes sao atualmente isentos de Imposto de Renda."),
                                  QStringLiteral("= lucro distribuido aos acionistas (isento PF)"),
                                  QStringLiteral("")}},
        {QStringLiteral("RENDIMENTO FII"),{QStringLiteral("O valor pago periodicamente por um Fundo de Investimento Imobiliario aos seus cotistas — como se fosse o aluguel dos imoveis do fundo. Para pessoas fisicas, geralmente e isento de IR."),
                                  QStringLiteral("= aluguel mensal do FII"),
                                  QStringLiteral("")}},
        {QStringLiteral("BONIFICACAO"),{QStringLiteral("Quando a empresa distribui novas acoes (em vez de dinheiro) proporcionalmente ao que voce ja tem. Voce fica com mais acoes, mas o preco se ajusta — o valor total continua parecido. E isenta de IR no recebimento."),
                                  QStringLiteral("= acoes gratis no lugar de dinheiro"),
                                  QStringLiteral("")}},

        // Grupo 1 — Cartao de credito (termos ja existentes, revisados)
        {QStringLiteral("PARCELA"),{QStringLiteral("Cada pagamento mensal de uma compra dividida. Uma compra de R$ 300 em 3x: cada parcela custa R$ 100 (sem juros) ou mais (com juros)."),
                                  QStringLiteral("= cada pagamento mensal da compra"),
                                  QStringLiteral("Parcela mensal")}},
        {QStringLiteral("AMORTIZACAO"),{QStringLiteral("A parte da parcela que realmente diminui o que voce deve. Cada parcela tem dois pedacos: juros (custo de ter parcelado) e amortizacao (abate da divida). Quanto mais voce amortiza, menor fica o saldo devedor."),
                                  QStringLiteral("= parte da parcela que abate a divida"),
                                  QStringLiteral("")}},
        {QStringLiteral("TABELA PRICE"),{QStringLiteral("Sistema de parcelas fixas: voce paga o mesmo valor todo mes. Nos primeiros meses os juros pesam mais; com o tempo, a amortizacao cresce. E o parcelamento mais comum no varejo e cartoes."),
                                  QStringLiteral("= parcela fixa com juros decrescentes"),
                                  QStringLiteral("")}},
        {QStringLiteral("FATURA"),{QStringLiteral("O documento mensal do cartao de credito listando todas as compras do periodo e o valor total a pagar. Pagar o total evita juros rotativos."),
                                  QStringLiteral("= total de compras do mes no cartao"),
                                  QStringLiteral("")}},
        {QStringLiteral("VENCIMENTO"),{QStringLiteral("A data limite para pagar sem multa ou juros. Em cartao: pagar so depois do vencimento gera juros rotativos (os mais caros do mercado)."),
                                  QStringLiteral("= data limite para pagar sem juros"),
                                  QStringLiteral("")}},
        {QStringLiteral("JUROS A.M."),{QStringLiteral("Taxa de juros ao mes. 2% a.m. significa que a divida cresce 2% a cada mes que nao for paga. Atencao: juros compostos fazem esse valor crescer rapido em meses seguidos."),
                                  QStringLiteral("= juros por mes"),
                                  QStringLiteral("")}},
        {QStringLiteral("JUROS A.A."),{QStringLiteral("Taxa de juros ao ano. Para comparar emprestimos diferentes, use sempre a taxa anual — ou o CET, que inclui tambem IOF e tarifas."),
                                  QStringLiteral("= juros por ano"),
                                  QStringLiteral("")}},
        {QStringLiteral("JUROS COMPOSTOS"),{QStringLiteral("Juros sobre juros: no proximo mes, a taxa incide sobre a divida que ja cresceu. R$ 1.000 a 5% a.m. viram R$ 1.050 no 1o mes e R$ 1.102,50 no 2o — nao R$ 1.100."),
                                  QStringLiteral("= juros sobre juros (crescem rapido)"),
                                  QStringLiteral("")}},
        {QStringLiteral("JUROS SIMPLES"),{QStringLiteral("Juros calculados sempre sobre o valor original, sem acumular. R$ 1.000 a 5% a.m. por 2 meses = R$ 100 de juros total. Menos comum em credito — mais usado em multas fixas."),
                                  QStringLiteral("= juros sem acumular"),
                                  QStringLiteral("")}},
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
    const QString upper = texto.toUpper();
    // Procura termos cadastrados como substring delimitada por word boundary.
    // Ordena por tamanho decrescente para evitar match parcial (LCI antes de
    // LCIA, IGP-M antes de IGPM, etc).
    QStringList chaves = tabela().keys();
    std::sort(chaves.begin(), chaves.end(), [](const QString &a, const QString &b) {
        return a.size() > b.size();
    });
    for (const QString &k : chaves) {
        // UseUnicodePropertiesOption: trata letras acentuadas (í, ç, ã…) como
        // word-chars. Sem isso, "\bDAS\b" casa em "Conclu[í]DAS" (boundary falso
        // antes de letra acentuada) — falso-positivo que pintaria sublinhado/
        // tooltip de "DAS" em "Concluídas". (#166 §9.6: corrige precisão da varredura.)
        const QRegularExpression rx(
            QStringLiteral("\\b") + QRegularExpression::escape(k) + QStringLiteral("\\b"),
            QRegularExpression::UseUnicodePropertiesOption);
        if (rx.match(upper).hasMatch()) return k;
    }
    return {};
}
