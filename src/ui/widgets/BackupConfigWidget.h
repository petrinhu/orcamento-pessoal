#pragma once

#include <QWidget>

class QComboBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QLabel;

// BackupConfigWidget (#79) - secao Backup de PrefsWidget. Permite configurar
// frequencia (desativado/diaria/semanal), diretorio de destino, retencao N e
// disparar backup manual. Lista historico recente.
class BackupConfigWidget final : public QWidget {
    Q_OBJECT
public:
    explicit BackupConfigWidget(QWidget *parent = nullptr);

private slots:
    void onSalvar();
    void onBackupAgora();
    void onEscolherDir();
    void recarregarLista();

private:
    void buildUi();
    void carregarConfig();

    QComboBox    *m_cbFreq      = nullptr;
    QLineEdit    *m_edtDir      = nullptr;
    QPushButton  *m_btnDir      = nullptr;
    QSpinBox     *m_spnRet      = nullptr;
    QPushButton  *m_btnSalvar   = nullptr;
    QPushButton  *m_btnBackupAg = nullptr;
    QLabel       *m_lblUltimo   = nullptr;
    QListWidget  *m_lista       = nullptr;
};
