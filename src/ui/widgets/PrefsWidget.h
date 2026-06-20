#pragma once

#include <QWidget>

class QComboBox;
class QRadioButton;
class QLabel;
class QCheckBox;
class QSpinBox;
class QLineEdit;
class ConfigCategoriasWidget;
class ConfigFontesWidget;
class ConfigPagadoresWidget;
class ConfigWidget;

// Settings panel: Language, Default Currency, Save/Autosave, Cards, Income Sources, Beneficiários.
class PrefsWidget final : public QWidget {
    Q_OBJECT
public:
    explicit PrefsWidget(QWidget *parent = nullptr);

signals:
    void cartoesAlterados();     // forwarded from ConfigWidget
    void moedaAlterada();        // triggers ParcelasWidget::recarregar
    void fontesAlteradas();      // forwarded from ConfigFontesWidget
    void pagadoresAlterados();   // forwarded from ConfigPagadoresWidget
    void categoriasAlteradas();  // forwarded from ConfigCategoriasWidget

private slots:
    void onLanguageToggled();
    void onCurrencyChanged(int index);
    void onAutosaveToggled(bool checked);
    void onIntervalChanged(int minutes);

private:
    void buildUi();
    void syncComboToCurrency(const QString &code);
    static QStringList buildCurrencyList();

    QRadioButton *m_rPtBr              = nullptr;
    QRadioButton *m_rEnIntl            = nullptr;
    QLabel       *m_lblRestartHint     = nullptr;
    QComboBox    *m_comboCurrency      = nullptr;
    QCheckBox    *m_chkAutosave        = nullptr;
    QSpinBox     *m_spnInterval        = nullptr;
    // (#181) m_edtSavesFolder / m_edtAutosavesFolder removidos — paths são fixos.
    QComboBox    *m_comboInitialTab    = nullptr;
    QCheckBox    *m_chkIniciante       = nullptr;
    ConfigCategoriasWidget*m_categorias= nullptr;
    ConfigFontesWidget   *m_fontes    = nullptr;
    ConfigPagadoresWidget*m_pagadores = nullptr;
    ConfigWidget         *m_cards     = nullptr;
    bool                  m_loading  = false;
};
