#include "mainwindow.h"

#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Irbis"));
    QIcon app_icon(QStringLiteral(":/icons/irbis.svg"));
    if (app_icon.isNull())
        app_icon = QIcon(QStringLiteral(":/icons/irbis.ico"));
    QApplication::setWindowIcon(app_icon);

    QLocale::setDefault(QLocale(QLocale::Russian));

    QFont app_font = QApplication::font();
    if (app_font.pointSize() <= 0)
        app_font.setPointSize(10);
    QApplication::setFont(app_font);

    QTranslator translator;
    if (translator.load(QStringLiteral("qt_ru"), QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QApplication::installTranslator(&translator);

    MainWindow window;
    window.show();
    return QApplication::exec();
}
/*
num: 2.5
den: 12000 1600 70 1
tau: 7
*/