#pragma once

#include <QWidget>

class QComboBox;
class QRadioButton;
class ConfigWidget;

// Settings tab: Language, Default Currency, Cards.
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

private:
    void buildUi();
    void syncComboToCurrency(const QString &code);
    static QStringList buildCurrencyList();

    QRadioButton *m_rPtBr        = nullptr;
    QRadioButton *m_rEnIntl      = nullptr;
    QComboBox    *m_comboCurrency = nullptr;
    ConfigWidget *m_cards        = nullptr;
    bool          m_loading      = false;
};
