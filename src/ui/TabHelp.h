#pragma once

#include <QString>
#include <QStringList>
#include <QMap>

// Fonte única (DRY) do texto de ajuda de cada aba do PeuBud.
//
// Cada entrada alimenta DOIS consumidores:
//   • o TabShell da aba (descrição + instruções colapsáveis no topo);
//   • o HelpDialog (uma seção por aba, listando todas).
//
// Chaveado por tabKey estável (ex.: "tab_cartoes", "tab_entradas", …).
// Para adicionar uma aba nova: incluir uma entrada no QMap em TabHelp.cpp e
// chamar shell->setTabKey("tab_<x>") + shell->applyHelp() no MainWindow.
struct TabHelpEntry {
    QString titulo;
    QString descricao;
    // Cada item da lista é um passo. O TabShell renderiza como <ol> rich-text;
    // o HelpDialog reusa a mesma lista.
    QStringList passos;
};

// Registro central, chaveado por tabKey.
const QMap<QString, TabHelpEntry> &tabHelp();

// Ordem canônica de exibição (HelpDialog). QMap ordena por chave, então a
// ordem visual das abas vive aqui — adicione a tabKey nova nesta lista.
const QStringList &tabHelpOrder();

// Conveniência: retorna a entrada (ou uma vazia se a key não existir).
TabHelpEntry tabHelpEntry(const QString &tabKey);
