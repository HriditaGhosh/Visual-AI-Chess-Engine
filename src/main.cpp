#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QStyleFactory>
#include "MainWindow.h"
#include "ThemeManager.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Chess GUI");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("ChessQt");

    // Force Fusion style — prevents Windows native style from
    // overriding our dark theme palette and stylesheet colors
    app.setStyle(QStyleFactory::create("Fusion"));

    // App icon from SVG resource
    app.setWindowIcon(QIcon(":/icons/app_icon.svg"));

    // Apply dark theme by default
    ThemeManager* tm = ThemeManager::instance();
    app.setPalette(tm->appPalette());
    app.setStyleSheet(tm->themeStyleSheet());

    MainWindow win;
    win.setWindowIcon(QIcon(":/icons/app_icon.svg"));
    win.show();

    return app.exec();
}
