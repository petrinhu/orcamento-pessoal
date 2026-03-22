#include "Theme.h"

#include <QApplication>
#include <QFontDatabase>
#include <QStyleHints>
#include <QPalette>

namespace Theme {

namespace Color {
    const QString BackgroundLight = QStringLiteral("#F5F4F0");
    const QString BackgroundDark  = QStringLiteral("#18181A");
    const QString SurfaceLight    = QStringLiteral("#FFFFFF");
    const QString SurfaceDark     = QStringLiteral("#222225");
    const QString Accent          = QStringLiteral("#166F4A");
    const QString AccentDark      = QStringLiteral("#22A367");
    const QString Positive        = QStringLiteral("#166F4A");
    const QString PositiveDark    = QStringLiteral("#22A367");
    const QString Negative        = QStringLiteral("#C94040");
    const QString NegativeDark    = QStringLiteral("#E85555");
    const QString TextLight       = QStringLiteral("#1A1A1A");
    const QString TextDark        = QStringLiteral("#EDEDED");
    const QString SubtextLight    = QStringLiteral("#6B6B6B");
    const QString SubtextDark     = QStringLiteral("#9A9A9A");
}

static bool isDark()
{
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

QString acento()     { return isDark() ? Color::AccentDark    : Color::Accent;    }
QString negativo()   { return isDark() ? Color::NegativeDark  : Color::Negative;  }
QString positivo()   { return isDark() ? Color::PositiveDark  : Color::Positive;  }
QString background() { return isDark() ? Color::BackgroundDark: Color::BackgroundLight; }
QString surface()    { return isDark() ? Color::SurfaceDark   : Color::SurfaceLight;  }
QString texto()      { return isDark() ? Color::TextDark      : Color::TextLight;  }

static void carregarFontes()
{
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter-Regular.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter-Medium.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter-SemiBold.ttf"));
}

static QString gerarQss()
{
    const bool dark = isDark();
    const QString bg   = dark ? Color::BackgroundDark  : Color::BackgroundLight;
    const QString surf = dark ? Color::SurfaceDark     : Color::SurfaceLight;
    const QString acc  = dark ? Color::AccentDark      : Color::Accent;
    const QString txt  = dark ? Color::TextDark        : Color::TextLight;
    const QString sub  = dark ? Color::SubtextDark     : Color::SubtextLight;
    const QString neg  = dark ? Color::NegativeDark    : Color::Negative;

    return QStringLiteral(R"(
        QWidget {
            font-family: "Inter";
            font-size: 13px;
            color: %1;
            background-color: %2;
        }
        QMainWindow, QDialog {
            background-color: %2;
        }
        QFrame#surface, QGroupBox {
            background-color: %3;
            border-radius: 8px;
            border: none;
        }
        QPushButton#primary {
            background-color: %4;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px 20px;
            font-weight: 500;
        }
        QPushButton#primary:hover { background-color: %4; opacity: 0.85; }
        QPushButton#primary:pressed { background-color: %4; }
        QPushButton {
            background-color: transparent;
            border: 1px solid %5;
            border-radius: 6px;
            padding: 6px 16px;
            color: %1;
        }
        QPushButton:hover { border-color: %4; color: %4; }
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QDateEdit {
            background-color: %3;
            border: 1px solid #D0D0D0;
            border-radius: 6px;
            padding: 5px 8px;
            color: %1;
        }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus,
        QComboBox:focus, QDateEdit:focus {
            border-color: %4;
        }
        QTableWidget {
            background-color: %3;
            alternate-background-color: %2;
            gridline-color: transparent;
            border: none;
        }
        QTableWidget::item { padding: 4px 8px; min-height: 36px; }
        QTableWidget::item:selected {
            background-color: %4;
            color: white;
        }
        QHeaderView::section {
            background-color: %2;
            border: none;
            border-bottom: 2px solid %4;
            padding: 6px 8px;
            font-weight: 600;
            color: %1;
        }
        QHeaderView::section:hover { color: %4; }
        QLabel#title { font-size: 16px; font-weight: 600; }
        QLabel#subtitle { font-size: 12px; color: %5; }
        QStatusBar { background-color: %2; color: %5; }
        QScrollBar:vertical, QScrollBar:horizontal {
            background: %2; width: 8px; height: 8px;
        }
        QScrollBar::handle { background: %5; border-radius: 4px; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; }
    )")
    .arg(txt, bg, surf, acc, sub, neg);
}

void aplicar()
{
    carregarFontes();

    QFont font(QStringLiteral("Inter"), 13);
    QApplication::setFont(font);

    qApp->setStyleSheet(gerarQss());

    // Re-apply on scheme change (light ↔ dark)
    QObject::connect(
        QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
        qApp, [](Qt::ColorScheme) { qApp->setStyleSheet(gerarQss()); }
    );
}

} // namespace Theme
