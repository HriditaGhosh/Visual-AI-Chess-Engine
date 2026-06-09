#include "ThemeManager.h"
#include <QApplication>
#include <QStyleFactory>

ThemeManager* ThemeManager::s_instance = nullptr;

ThemeManager* ThemeManager::instance()
{
    if (!s_instance)
        s_instance = new ThemeManager();
    return s_instance;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent), m_theme(DARK)
{}

void ThemeManager::setTheme(Theme t)
{
    if (m_theme == t) return;
    m_theme = t;
    qApp->setStyle(QStyleFactory::create("Fusion"));
    qApp->setPalette(appPalette());
    qApp->setStyleSheet(themeStyleSheet());
    emit themeChanged(t);
}

void ThemeManager::toggleTheme()
{
    setTheme(m_theme == DARK ? LIGHT : DARK);
}

// ── Board colors ────────────────────────────────────────────────────────────

QColor ThemeManager::lightSquare() const
{
    return m_theme == DARK
        ? QColor(0xB8, 0x8B, 0x6E)   // warm tan (dark theme)
        : QColor(0xF0, 0xD9, 0xB5);  // classic cream (light theme)
}

QColor ThemeManager::darkSquare() const
{
    return m_theme == DARK
        ? QColor(0x6B, 0x3F, 0x2A)   // deep brown
        : QColor(0xB5, 0x88, 0x63);  // medium brown
}

QColor ThemeManager::highlightColor() const
{
    return QColor(0xFF, 0xF2, 0x7A, 180);   // yellow glow, semi-transparent
}

QColor ThemeManager::lastMoveColor() const
{
    return QColor(0x6F, 0xBD, 0x6F, 150);   // soft green
}

QColor ThemeManager::legalMoveColor() const
{
    return QColor(0x00, 0x00, 0x00, 70);    // dark circle dots
}

QColor ThemeManager::checkColor() const
{
    return QColor(0xE5, 0x30, 0x30, 200);   // danger red
}

QColor ThemeManager::dragSquareColor() const
{
    return QColor(0x44, 0xA8, 0xE0, 130);   // blue highlight for drag source
}

// ── Application palette ─────────────────────────────────────────────────────

QPalette ThemeManager::appPalette() const
{
    QPalette p;
    if (m_theme == DARK) {
        p.setColor(QPalette::Window,          QColor(0x23, 0x23, 0x2E));
        p.setColor(QPalette::WindowText,      QColor(0xE0, 0xE0, 0xE8));
        p.setColor(QPalette::Base,            QColor(0x1A, 0x1A, 0x24));
        p.setColor(QPalette::AlternateBase,   QColor(0x2A, 0x2A, 0x38));
        p.setColor(QPalette::Text,            QColor(0xE0, 0xE0, 0xE8));
        p.setColor(QPalette::Button,          QColor(0x30, 0x30, 0x40));
        p.setColor(QPalette::ButtonText,      QColor(0xE0, 0xE0, 0xE8));
        p.setColor(QPalette::Highlight,       QColor(0x6F, 0x9B, 0xD6));
        p.setColor(QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF));
        p.setColor(QPalette::Link,            QColor(0x8A, 0xB8, 0xF0));
        p.setColor(QPalette::Mid,             QColor(0x3A, 0x3A, 0x4E));
        p.setColor(QPalette::Dark,            QColor(0x15, 0x15, 0x1E));
    } else {
        p.setColor(QPalette::Window,          QColor(0xF2, 0xF0, 0xEB));
        p.setColor(QPalette::WindowText,      QColor(0x1A, 0x1A, 0x2E));
        p.setColor(QPalette::Base,            QColor(0xFF, 0xFF, 0xFF));
        p.setColor(QPalette::AlternateBase,   QColor(0xE8, 0xE5, 0xDF));
        p.setColor(QPalette::Text,            QColor(0x1A, 0x1A, 0x2E));
        p.setColor(QPalette::Button,          QColor(0xD8, 0xD5, 0xCE));
        p.setColor(QPalette::ButtonText,      QColor(0x1A, 0x1A, 0x2E));
        p.setColor(QPalette::Highlight,       QColor(0x4A, 0x7C, 0xC0));
        p.setColor(QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF));
        p.setColor(QPalette::Link,            QColor(0x2A, 0x5C, 0xA0));
        p.setColor(QPalette::Mid,             QColor(0xBB, 0xB8, 0xB2));
        p.setColor(QPalette::Dark,            QColor(0x9A, 0x97, 0x91));
    }
    return p;
}

QString ThemeManager::themeStyleSheet() const
{
    if (m_theme == DARK) {
        return R"(
QMainWindow, QDialog {
    background-color: #23232E;
}
QMenuBar {
    background-color: #1A1A24;
    color: #E0E0E8;
    border-bottom: 1px solid #3A3A4E;
}
QMenuBar::item {
    color: #E0E0E8;
    background-color: transparent;
    padding: 4px 10px;
}
QMenuBar::item:selected { background-color: #30304A; color: #FFFFFF; }
QMenuBar::item:pressed  { background-color: #25254A; color: #FFFFFF; }
QMenu {
    background-color: #23232E;
    color: #E0E0E8;
    border: 1px solid #3A3A4E;
}
QMenu::item {
    color: #E0E0E8;
    background-color: transparent;
    padding: 5px 24px 5px 16px;
}
QMenu::item:selected {
    background-color: #3A3A5A;
    color: #FFFFFF;
}
QMenu::item:disabled { color: #666680; }
QPushButton {
    background-color: #30304A;
    color: #E0E0E8;
    border: 1px solid #4A4A6A;
    border-radius: 6px;
    padding: 6px 14px;
    font-size: 13px;
}
QPushButton:hover  { background-color: #3A3A5A; border-color: #6F9BD6; }
QPushButton:pressed { background-color: #25254A; }
QListWidget, QTextEdit, QPlainTextEdit {
    background-color: #1A1A24;
    color: #E0E0E8;
    border: 1px solid #3A3A4E;
    border-radius: 4px;
}
QLabel { color: #E0E0E8; }
QStatusBar { background-color: #1A1A24; color: #B0B0C0; }
QToolBar { background-color: #1A1A24; border-bottom: 1px solid #3A3A4E; spacing: 4px; }
QComboBox {
    background-color: #30304A; color: #E0E0E8;
    border: 1px solid #4A4A6A; border-radius: 4px; padding: 4px 8px;
}
QScrollBar:vertical {
    background: #1A1A24; width: 10px; border-radius: 5px;
}
QScrollBar::handle:vertical { background: #4A4A6A; border-radius: 5px; min-height: 20px; }
QGroupBox { color: #E0E0E8; border: 1px solid #3A3A4E; border-radius: 6px; margin-top: 8px; padding-top: 4px; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
)";
    } else {
        return R"(
QMainWindow, QDialog { background-color: #F2F0EB; }
QMenuBar { background-color: #E8E5DF; color: #1A1A2E; border-bottom: 1px solid #BBBBB0; }
QMenuBar::item:selected { background-color: #D0CEC8; }
QMenu { background-color: #F2F0EB; color: #1A1A2E; border: 1px solid #BBBBB0; }
QMenu::item:selected { background-color: #D0CEC8; }
QPushButton {
    background-color: #D8D5CE; color: #1A1A2E;
    border: 1px solid #AAAAAA; border-radius: 6px; padding: 6px 14px; font-size: 13px;
}
QPushButton:hover { background-color: #C8C5BE; border-color: #4A7CC0; }
QPushButton:pressed { background-color: #B8B5AE; }
QListWidget, QTextEdit, QPlainTextEdit {
    background-color: #FFFFFF; color: #1A1A2E;
    border: 1px solid #BBBBB0; border-radius: 4px;
}
QLabel { color: #1A1A2E; }
QStatusBar { background-color: #E8E5DF; color: #3A3A4E; }
QToolBar { background-color: #E8E5DF; border-bottom: 1px solid #BBBBB0; spacing: 4px; }
QComboBox {
    background-color: #E8E5DF; color: #1A1A2E;
    border: 1px solid #AAAAAA; border-radius: 4px; padding: 4px 8px;
}
QScrollBar:vertical { background: #E8E5DF; width: 10px; border-radius: 5px; }
QScrollBar::handle:vertical { background: #AAAAAA; border-radius: 5px; min-height: 20px; }
QGroupBox { color: #1A1A2E; border: 1px solid #BBBBB0; border-radius: 6px; margin-top: 8px; padding-top: 4px; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
)";
    }
}
