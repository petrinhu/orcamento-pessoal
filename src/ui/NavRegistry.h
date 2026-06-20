// src/ui/NavRegistry.h
// Registro central da ordem fixa e agrupada das abas da sidebar (#165).
// Consumido pelo MainWindow e pela SidebarNav. Ordem imutável em runtime.

#pragma once

#include <QList>
#include <QString>

struct NavItem {
    QString chave;   // ex.: "tab_dashboard" — mesma chave usada no TabHelp
    QString emoji;   // ex.: "🏠"
    QString titulo;  // ex.: "Dashboard"
    QString grupo;   // "" (sem grupo, para o Dashboard) ou "Dia a dia" / "Planejar" / "Investir"
};

// Retorna a lista de navegação em ordem fixa, inicializada uma única vez.
const QList<NavItem> &navItens();
