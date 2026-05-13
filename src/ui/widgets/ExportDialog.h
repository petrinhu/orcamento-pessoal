#pragma once

#include <QDialog>
#include <QStringList>

class QListWidget;
class QComboBox;

// Dialog that lets the user choose which sections to export and in what format.
// Shown before the save-file dialog so the format extension is already known.
class ExportDialog final : public QDialog {
    Q_OBJECT
public:
    // sections       : ordered list of available section names (shown as checkboxes, all pre-checked).
    // initialFormat  : format key pre-selected in the combo (e.g. "csv", "xlsx").
    explicit ExportDialog(const QStringList &sections,
                          const QString     &initialFormat,
                          QWidget           *parent = nullptr);

    // Names of checked sections in input order.
    QStringList selectedSections() const;

    // Format key, e.g. "csv", "xlsx", "ofx".
    QString selectedFormat() const;

private:
    void buildUi(const QStringList &sections, const QString &initialFormat);

    QListWidget *m_list  = nullptr;
    QComboBox   *m_cbFmt = nullptr;
};
