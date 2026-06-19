#pragma once

#include "models/Categoria.h"
#include <QDialog>

class QLineEdit;
class QComboBox;

class CategoriaDialog final : public QDialog {
    Q_OBJECT
public:
    // Modo criação: tipo pré-selecionado
    explicit CategoriaDialog(const QString &tipo,
                             const QList<Categoria> &pais,
                             QWidget *parent = nullptr);
    // Modo edição
    explicit CategoriaDialog(const Categoria &cat,
                             const QList<Categoria> &pais,
                             QWidget *parent = nullptr);

    Categoria result() const;

private slots:
    void onAccept();

private:
    void build(const QString &tipo, const QList<Categoria> &pais);
    void populatePais(const QList<Categoria> &pais, const QString &tipo);

    void atualizarVisibilidadeDespesa();

    QLineEdit *m_edtNome   = nullptr;
    QComboBox *m_cbTipo    = nullptr;
    QComboBox *m_cbPai     = nullptr;
    QComboBox *m_cbDespesa = nullptr; // #121: Variável/Fixo (só para tipo "gasto")

    Categoria  m_result;
    int        m_editId   = 0;  // 0 = criação
};
