#pragma once

#include <QByteArray>
#include <QWidget>

class QLabel;
class QPushButton;

// ReceiptAttachmentWidget - bloco UI reutilizavel para anexar/exibir o
// comprovante (Receipt BLOB, #90) em LancamentoEntradaDialog, GastoDialog,
// AddCompraDialog e InvestimentoDialog.
//
// Renderiza:
//   [📎 Anexar]  thumbnail (clickavel para abrir viewer expandido)  [🗑]
// O dialog hospedeiro le receipt() na hora de salvar e injeta com setReceipt()
// quando edita lancamento existente.
class ReceiptAttachmentWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReceiptAttachmentWidget(QWidget *parent = nullptr);

    void              setReceipt(const QByteArray &bytes);
    const QByteArray &receipt() const { return m_receipt; }

protected:
    bool eventFilter(QObject *watched, QEvent *e) override;

private slots:
    void onAttach();
    void onRemove();
    void onViewExpanded();

private:
    void refreshThumbnail();

    QByteArray   m_receipt;
    QPushButton *m_btnAttach   = nullptr;
    QPushButton *m_btnRemove   = nullptr;
    QLabel      *m_thumbnail   = nullptr;
};
