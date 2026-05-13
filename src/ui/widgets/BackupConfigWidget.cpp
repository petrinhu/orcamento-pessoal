#include "BackupConfigWidget.h"

#include "core/database/DatabaseManager.h"
#include "core/session/UserSession.h"
#include "services/BackupService.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

BackupConfigWidget::BackupConfigWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    carregarConfig();
    recarregarLista();
}

void BackupConfigWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto *form = new QFormLayout;

    m_cbFreq = new QComboBox(this);
    m_cbFreq->addItem(tr("Desativado"), int(BackupService::Frequencia::Desativado));
    m_cbFreq->addItem(tr("Diária"),     int(BackupService::Frequencia::Diaria));
    m_cbFreq->addItem(tr("Semanal"),    int(BackupService::Frequencia::Semanal));
    form->addRow(tr("Frequência:"), m_cbFreq);

    auto *rowDir = new QHBoxLayout;
    m_edtDir = new QLineEdit(this);
    m_edtDir->setReadOnly(true);
    m_btnDir = new QPushButton(tr("Escolher…"), this);
    rowDir->addWidget(m_edtDir);
    rowDir->addWidget(m_btnDir);
    form->addRow(tr("Destino:"), rowDir);

    m_spnRet = new QSpinBox(this);
    m_spnRet->setRange(1, 365);
    m_spnRet->setSuffix(tr(" cópias"));
    form->addRow(tr("Retenção:"), m_spnRet);

    m_lblUltimo = new QLabel(this);
    form->addRow(tr("Último backup:"), m_lblUltimo);

    root->addLayout(form);

    auto *rowBtns = new QHBoxLayout;
    m_btnSalvar   = new QPushButton(tr("Salvar configuração"), this);
    m_btnBackupAg = new QPushButton(tr("Fazer backup agora"),  this);
    m_btnBackupAg->setObjectName(QStringLiteral("primary"));
    rowBtns->addWidget(m_btnSalvar);
    rowBtns->addWidget(m_btnBackupAg);
    rowBtns->addStretch();
    root->addLayout(rowBtns);

    root->addWidget(new QLabel(tr("<b>Histórico</b>"), this));
    m_lista = new QListWidget(this);
    m_lista->setAlternatingRowColors(true);
    root->addWidget(m_lista, 1);

    connect(m_btnSalvar,   &QPushButton::clicked, this, &BackupConfigWidget::onSalvar);
    connect(m_btnBackupAg, &QPushButton::clicked, this, &BackupConfigWidget::onBackupAgora);
    connect(m_btnDir,      &QPushButton::clicked, this, &BackupConfigWidget::onEscolherDir);
}

void BackupConfigWidget::carregarConfig()
{
    const auto c = BackupService::load();
    const int idx = m_cbFreq->findData(int(c.frequencia));
    if (idx >= 0) m_cbFreq->setCurrentIndex(idx);
    m_edtDir->setText(c.destino);
    m_spnRet->setValue(c.retencao);
    m_lblUltimo->setText(c.ultimo.isValid()
        ? c.ultimo.toString(QStringLiteral("dd/MM/yyyy HH:mm"))
        : tr("(nunca)"));
}

void BackupConfigWidget::onSalvar()
{
    BackupService::Config c = BackupService::load();
    c.frequencia = static_cast<BackupService::Frequencia>(m_cbFreq->currentData().toInt());
    c.destino    = m_edtDir->text();
    c.retencao   = m_spnRet->value();
    BackupService::save(c);
    QMessageBox::information(this, tr("Backup"),
        tr("Configuração salva. Próximo backup automático conforme frequência."));
}

void BackupConfigWidget::onBackupAgora()
{
    const QString slug = UserSession::instance().slug();
    if (slug.isEmpty()) {
        QMessageBox::warning(this, tr("Backup"),
            tr("Nenhum usuário logado."));
        return;
    }
    BackupService::Config c = BackupService::load();
    c.frequencia = static_cast<BackupService::Frequencia>(m_cbFreq->currentData().toInt());
    c.destino    = m_edtDir->text();
    c.retencao   = m_spnRet->value();

    const QString path = BackupService::runBackup(c, slug, [](const QString &dest) {
        return DatabaseManager::instance().exportToFile(dest);
    });
    if (path.isEmpty()) {
        QMessageBox::warning(this, tr("Backup"),
            tr("Falha ao gerar backup. Verifique permissões do destino."));
        return;
    }
    QMessageBox::information(this, tr("Backup"),
        tr("Backup salvo em:\n%1").arg(path));
    carregarConfig();
    recarregarLista();
}

void BackupConfigWidget::onEscolherDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this,
        tr("Diretório de backup"), m_edtDir->text());
    if (!dir.isEmpty()) {
        m_edtDir->setText(dir);
        recarregarLista();
    }
}

void BackupConfigWidget::recarregarLista()
{
    m_lista->clear();
    const QString slug = UserSession::instance().slug();
    const QStringList paths = BackupService::listarBackups(m_edtDir->text(), slug);
    QLocale loc(QLocale::Portuguese, QLocale::Brazil);
    for (const QString &p : paths) {
        const QFileInfo fi(p);
        m_lista->addItem(QStringLiteral("%1  ·  %2 KB  ·  %3")
            .arg(fi.lastModified().toString(QStringLiteral("dd/MM/yyyy HH:mm")),
                 loc.toString(fi.size() / 1024),
                 fi.fileName()));
    }
}
