#pragma once

#include <QWidget>

class QComboBox;
class QRadioButton;
class QLabel;
class QCheckBox;
class QSpinBox;
class QLineEdit;
class ConfigWidget;

// Settings panel: Language, Default Currency, Save/Autosave, Cards.
// Embeds ConfigWidget for card management.
class PrefsWidget final : public QWidget {
    Q_OBJECT
public:
    explicit PrefsWidget(QWidget *parent = nullptr);

signals:
    void cartoesAlterados();   // forwarded from ConfigWidget
    void moedaAlterada();      // triggers ParcelasWidget::recarregar

private slots:
    void onLanguageToggled();
    void onCurrencyChanged(int index);
    void onAutosaveToggled(bool checked);
    void onIntervalChanged(int minutes);
    void onBrowseSavesFolder();
    void onBrowseAutosavesFolder();

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
    QLineEdit    *m_edtSavesFolder     = nullptr;
    QLineEdit    *m_edtAutosavesFolder = nullptr;
    ConfigWidget *m_cards              = nullptr;
    bool          m_loading            = false;
};
