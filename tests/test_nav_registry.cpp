// tests/test_nav_registry.cpp
// TDD RED → GREEN para NavRegistry (#165 redesign Fase 2 Task 2).
// Verifica: tamanho, unicidade de chaves, primeiro item, conjunto de grupos,
// e ordem dos grupos ao varrer a lista.

#include <QtTest>
#include <QSet>
#include <QStringList>

#include "ui/NavRegistry.h"

class TestNavRegistry : public QObject
{
    Q_OBJECT

private slots:
    // T1: exatamente 13 itens
    void tamanhoTotal()
    {
        QCOMPARE(navItens().size(), 13);
    }

    // T2: todas as chaves são únicas
    void chavesUnicas()
    {
        QSet<QString> vistas;
        for (const NavItem &item : navItens()) {
            QVERIFY2(!vistas.contains(item.chave),
                     qPrintable("chave duplicada: " + item.chave));
            vistas.insert(item.chave);
        }
        QCOMPARE(vistas.size(), 13);
    }

    // T3: primeiro item = tab_dashboard com grupo vazio
    void primeiroItemDashboard()
    {
        const NavItem &primeiro = navItens().first();
        QCOMPARE(primeiro.chave, QStringLiteral("tab_dashboard"));
        QCOMPARE(primeiro.grupo, QStringLiteral(""));
    }

    // T4: conjunto de grupos não-vazios é exatamente {"Dia a dia","Planejar","Investir"}
    void conjuntoDeGrupos()
    {
        QSet<QString> grupos;
        for (const NavItem &item : navItens()) {
            if (!item.grupo.isEmpty())
                grupos.insert(item.grupo);
        }
        const QSet<QString> esperado = {
            QStringLiteral("Dia a dia"),
            QStringLiteral("Planejar"),
            QStringLiteral("Investir"),
        };
        QCOMPARE(grupos, esperado);
    }

    // T5: ordem dos grupos ao varrer é Dia a dia → Planejar → Investir (sem intercalar)
    void ordemDosGrupos()
    {
        QStringList ordemVista;
        for (const NavItem &item : navItens()) {
            if (!item.grupo.isEmpty()) {
                if (ordemVista.isEmpty() || ordemVista.last() != item.grupo)
                    ordemVista.append(item.grupo);
            }
        }
        const QStringList esperada = {
            QStringLiteral("Dia a dia"),
            QStringLiteral("Planejar"),
            QStringLiteral("Investir"),
        };
        QCOMPARE(ordemVista, esperada);
    }
};

QTEST_MAIN(TestNavRegistry)
#include "test_nav_registry.moc"
