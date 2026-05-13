#include "ReceiptAttachmentWidget.h"

#include <QBuffer>
#include <QDialog>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {
constexpr int THUMBNAIL_PX        = 48;
constexpr qint64 MAX_RECEIPT_BYTES = 5 * 1024 * 1024; // 5 MB
}

ReceiptAttachmentWidget::ReceiptAttachmentWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(6);

    m_btnAttach = new QPushButton(tr("📎 Anexar"), this);
    m_btnAttach->setToolTip(tr("Anexar foto/scan do comprovante (PNG/JPG/PDF até 5 MB)"));
    connect(m_btnAttach, &QPushButton::clicked, this, &ReceiptAttachmentWidget::onAttach);

    m_thumbnail = new QLabel(this);
    m_thumbnail->setFixedSize(THUMBNAIL_PX, THUMBNAIL_PX);
    m_thumbnail->setAlignment(Qt::AlignCenter);
    m_thumbnail->setCursor(Qt::PointingHandCursor);
    m_thumbnail->setStyleSheet(QStringLiteral(
        "QLabel { border: 1px solid palette(mid); border-radius: 4px; }"));
    m_thumbnail->setVisible(false);
    connect(m_thumbnail, &QLabel::linkActivated, this,
            &ReceiptAttachmentWidget::onViewExpanded);

    m_btnRemove = new QPushButton(tr("🗑"), this);
    m_btnRemove->setToolTip(tr("Remover comprovante anexado"));
    m_btnRemove->setFixedWidth(32);
    m_btnRemove->setVisible(false);
    connect(m_btnRemove, &QPushButton::clicked, this, &ReceiptAttachmentWidget::onRemove);

    lay->addWidget(m_btnAttach);
    lay->addWidget(m_thumbnail);
    lay->addWidget(m_btnRemove);
    lay->addStretch();
}

void ReceiptAttachmentWidget::setReceipt(const QByteArray &bytes)
{
    m_receipt = bytes;
    refreshThumbnail();
}

void ReceiptAttachmentWidget::onAttach()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Anexar comprovante"),
        QString(),
        tr("Imagens e PDF (*.png *.jpg *.jpeg *.pdf);;Todos (*.*)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Anexar"),
            tr("Não foi possível abrir o arquivo: %1").arg(path));
        return;
    }
    if (f.size() > MAX_RECEIPT_BYTES) {
        QMessageBox::warning(this, tr("Anexar"),
            tr("Arquivo muito grande (%1 MB). Limite: 5 MB.")
                .arg(f.size() / (1024.0 * 1024.0), 0, 'f', 1));
        return;
    }
    m_receipt = f.readAll();
    f.close();
    refreshThumbnail();
}

void ReceiptAttachmentWidget::onRemove()
{
    m_receipt.clear();
    refreshThumbnail();
}

void ReceiptAttachmentWidget::onViewExpanded()
{
    if (m_receipt.isEmpty()) return;

    QMimeDatabase mimeDb;
    const QMimeType mt = mimeDb.mimeTypeForData(m_receipt);
    if (mt.name() == QStringLiteral("application/pdf")) {
        QMessageBox::information(this, tr("Comprovante"),
            tr("Comprovante em PDF anexado (%1 KB). Salve o lançamento e "
               "use ferramenta externa para inspecionar.")
                .arg(m_receipt.size() / 1024));
        return;
    }

    QImage img;
    if (!img.loadFromData(m_receipt)) {
        QMessageBox::warning(this, tr("Comprovante"),
            tr("Não foi possível decodificar a imagem do comprovante."));
        return;
    }

    auto *dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowModality(Qt::ApplicationModal);
    dlg->setWindowTitle(tr("Comprovante anexado"));
    dlg->resize(800, 600);
    auto *v = new QVBoxLayout(dlg);
    auto *scroll = new QScrollArea(dlg);
    auto *lbl = new QLabel;
    lbl->setPixmap(QPixmap::fromImage(img));
    lbl->setAlignment(Qt::AlignCenter);
    scroll->setWidget(lbl);
    scroll->setWidgetResizable(true);
    v->addWidget(scroll);
    auto *btnClose = new QPushButton(tr("Fechar"), dlg);
    connect(btnClose, &QPushButton::clicked, dlg, &QDialog::accept);
    v->addWidget(btnClose);
    dlg->show();
}

bool ReceiptAttachmentWidget::eventFilter(QObject *watched, QEvent *e)
{
    if (watched == m_thumbnail && e->type() == QEvent::MouseButtonRelease)
        onViewExpanded();
    return QWidget::eventFilter(watched, e);
}

void ReceiptAttachmentWidget::refreshThumbnail()
{
    const bool hasReceipt = !m_receipt.isEmpty();
    m_thumbnail->setVisible(hasReceipt);
    m_btnRemove->setVisible(hasReceipt);
    m_btnAttach->setText(hasReceipt ? tr("📎 Trocar") : tr("📎 Anexar"));

    if (!hasReceipt) {
        m_thumbnail->clear();
        return;
    }

    QMimeDatabase mimeDb;
    const QMimeType mt = mimeDb.mimeTypeForData(m_receipt);
    if (mt.name() == QStringLiteral("application/pdf")) {
        m_thumbnail->setText(QStringLiteral("PDF"));
        m_thumbnail->setToolTip(tr("Comprovante em PDF (%1 KB) — clique para detalhes")
                                   .arg(m_receipt.size() / 1024));
        m_thumbnail->installEventFilter(this);
        return;
    }

    QImage img;
    if (img.loadFromData(m_receipt)) {
        m_thumbnail->setPixmap(QPixmap::fromImage(img).scaled(
            THUMBNAIL_PX, THUMBNAIL_PX,
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_thumbnail->setToolTip(tr("Clique para visualizar em tamanho maior"));
        m_thumbnail->installEventFilter(this);
    } else {
        m_thumbnail->setText(QStringLiteral("?"));
    }
}
