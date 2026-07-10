#pragma once

#include <QMap>
#include <QString>
#include <functional>

class QWidget;

// LazyPageRouter (#208, PERF-02 — auditoria 2026-06-22 §3.5).
//
// Antes: MainWindow construía as 13 abas (widget de conteúdo + TabShell) no
// boot, mesmo as que o usuário nunca abre (~285ms + query storm do PERF-03).
// Agora: cada chave é registrada com uma fábrica; garantir() só invoca a
// fábrica na 1ª navegação para aquela chave (idempotente — chamadas
// seguintes reusam o widget já construído, sem reconstruir nem re-chamar
// a fábrica).
//
// SRP: só sabe "que chave já foi materializada e qual widget é". Não sabe
// nada de QStackedWidget/sidebar/glossário — quem chama a fábrica decide
// isso (MainWindow::registrarFabricasAbas). Extraído p/ ser testável sem
// arrastar a árvore inteira de widgets financeiros do app (só Qt::Core).
class LazyPageRouter {
public:
    using Fabrica = std::function<QWidget *()>;

    // Registra a fábrica de `chave`. Não constrói nada agora — só guarda o
    // callback. Chamar de novo para a mesma chave substitui a fábrica
    // (não tem efeito se a chave já foi materializada, pois garantir() nem
    // chega a olhar o mapa de fábricas nesse caso).
    void registrarFabrica(const QString &chave, Fabrica fabrica);

    // Materializa (só na 1ª chamada) e devolve o widget de `chave`.
    // Chamadas seguintes reusam o widget cacheado sem invocar a fábrica de
    // novo. Retorna nullptr se `chave` nunca foi registrada.
    QWidget *garantir(const QString &chave);

    // true se `chave` tem fábrica registrada (construída ou não).
    bool registrada(const QString &chave) const;

    // true somente se a fábrica de `chave` já rodou (widget existe).
    bool construida(const QString &chave) const;

    // Widget já construído, ou nullptr se ainda não construído/desconhecido.
    // NÃO dispara construção — uso em wiring cross-widget que deve degradar
    // graciosamente quando a aba-alvo ainda não existe (o efeito prático é
    // idêntico ao de hoje: nada para atualizar numa aba que não existe).
    QWidget *paginaExistente(const QString &chave) const;

private:
    QMap<QString, Fabrica>  m_fabricas;
    QMap<QString, QWidget *> m_paginas;
};
