#include "LazyPageRouter.h"

void LazyPageRouter::registrarFabrica(const QString &chave, Fabrica fabrica)
{
    m_fabricas.insert(chave, std::move(fabrica));
}

QWidget *LazyPageRouter::garantir(const QString &chave)
{
    const auto itPagina = m_paginas.constFind(chave);
    if (itPagina != m_paginas.constEnd())
        return itPagina.value();

    const auto itFabrica = m_fabricas.constFind(chave);
    if (itFabrica == m_fabricas.constEnd())
        return nullptr;

    QWidget *widget = (*itFabrica)();
    m_paginas.insert(chave, widget);
    return widget;
}

bool LazyPageRouter::registrada(const QString &chave) const
{
    return m_fabricas.contains(chave) || m_paginas.contains(chave);
}

bool LazyPageRouter::construida(const QString &chave) const
{
    return m_paginas.contains(chave);
}

QWidget *LazyPageRouter::paginaExistente(const QString &chave) const
{
    return m_paginas.value(chave, nullptr);
}
