#pragma once
#include <QObject>
#include <QColor>
#include <QPalette>
#include <QString>

// ─────────────────────────────────────────────────────────────────────────────
//  ThemeManager  (Signal & Slot / Observer pattern)
//
//  Maintains the current visual theme (DARK / LIGHT) and broadcasts
//  themeChanged() whenever the user switches.  All widgets connect to
//  this signal so the UI updates without any direct coupling.
// ─────────────────────────────────────────────────────────────────────────────
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum Theme { DARK, LIGHT };

    explicit ThemeManager(QObject* parent = nullptr);

    static ThemeManager* instance();

    Theme currentTheme() const { return m_theme; }
    void  setTheme(Theme t);
    void  toggleTheme();

    // Board square colors
    QColor lightSquare()    const;
    QColor darkSquare()     const;
    QColor highlightColor() const;
    QColor lastMoveColor()  const;
    QColor legalMoveColor() const;
    QColor checkColor()     const;
    QColor dragSquareColor() const;

    // UI palette applied to the whole application
    QPalette appPalette() const;

    QString themeStyleSheet() const;

signals:
    // DSA: Signal → broadcast to all connected slots (Observer pattern)
    void themeChanged(Theme newTheme);

private:
    Theme m_theme;
    static ThemeManager* s_instance;
};
