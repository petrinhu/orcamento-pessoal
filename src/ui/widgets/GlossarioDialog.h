#pragma once

#include <QDialog>

class QLineEdit;
class QListWidget;
class QTextBrowser;

// GlossarioDialog (#107) - busca + definicoes do GlossarioService.
// QListWidget filtravel a esquerda + QTextBrowser exibe definicao
// completa do termo selecionado. Atalho F2 abre direto.
class GlossarioDialog final : public QDialog {
    Q_OBJECT
public:
    explicit GlossarioDialog(QWidget *parent = nullptr);

    // Seleciona um termo especifico ao abrir (ex.: clique no botao InfoIcon).
    void selecionarTermo(const QString &termo);

private slots:
    void onFiltrar(const QString &texto);
    void onSelecaoChanged();

private:
    void buildUi();
    void popularLista();

    QLineEdit    *m_edtBusca = nullptr;
    QListWidget  *m_lista    = nullptr;
    QTextBrowser *m_texto    = nullptr;
};
