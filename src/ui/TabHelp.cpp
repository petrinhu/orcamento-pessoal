#include "TabHelp.h"

#include <QCoreApplication>

// POR ORA: só a entrada de Cartões (piloto aprovado). As outras 12 abas
// entram aqui depois, uma a uma. Tanto o TabShell quanto o HelpDialog crescem
// automaticamente conforme novas entradas forem adicionadas.
const QMap<QString, TabHelpEntry> &tabHelp()
{
    static const QMap<QString, TabHelpEntry> mapa = [] {
        QMap<QString, TabHelpEntry> m;

        m.insert(QStringLiteral("tab_cartoes"), TabHelpEntry{
            QCoreApplication::translate("TabHelp", "Cartões"),
            QCoreApplication::translate("TabHelp",
                "Aqui você registra as compras feitas no cartão de crédito — à "
                "vista ou parceladas. O app distribui as parcelas por mês e "
                "calcula os juros e a tabela de amortização."),
            QStringList{
                QCoreApplication::translate("TabHelp",
                    "Clique em <b>➕ Novo item</b> para adicionar uma compra."),
                QCoreApplication::translate("TabHelp",
                    "Escolha o <b>cartão</b>, a <b>descrição</b>, o <b>valor "
                    "total</b> e o <b>nº de parcelas</b>."),
                QCoreApplication::translate("TabHelp",
                    "Se houver <b>juros</b>, informe a taxa — o app calcula o "
                    "<b>CET</b> e a <b>tabela de amortização</b> (Price)."),
                QCoreApplication::translate("TabHelp",
                    "As parcelas aparecem <b>por mês</b> na tabela; navegue com "
                    "as setas <b>◀ ▶</b>."),
                QCoreApplication::translate("TabHelp",
                    "Quer simular antes de comprar? Use a <b>calculadora 🧮</b>."),
            }
        });

        return m;
    }();
    return mapa;
}

const QStringList &tabHelpOrder()
{
    static const QStringList ordem = {
        QStringLiteral("tab_cartoes"),
        // tab_dashboard, tab_entradas, tab_gastos, … (adicionar quando aprovado)
    };
    return ordem;
}

TabHelpEntry tabHelpEntry(const QString &tabKey)
{
    return tabHelp().value(tabKey);
}
