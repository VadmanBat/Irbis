#include "code/app/mainwindow.h"

#include "code/util/dialog-icons.hxx"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QScreen>

MainWindow::MainWindow(QWidget* parent) : QWidget(parent), ui(new Ui::MainWindow) {
    load_fonts();
    ui->setupUi(this);
    apply_styles();
    dialog_icons::apply(this, dialog_icons::Kind::App);
    qApp->setWindowIcon(windowIcon());
    center_window();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::load_fonts() {
    const QStringList candidates = {
        QStringLiteral("data/fonts/font-awesome-6-free-solid-900.otf"),
        QStringLiteral("fonts/font-awesome-6-free-solid-900.otf"),
    };
    for (const QString& path : candidates) {
        if (QFontDatabase::addApplicationFont(path) >= 0)
            return;
    }
}

void MainWindow::apply_styles() {
    const QStringList candidates = {
        QStringLiteral("data/styles/app.qss"),
        QStringLiteral("styles/app.qss"),
        QStringLiteral("data/styles/button-style.qss"),
        QStringLiteral("styles/button-style.qss"),
    };
    for (const QString& path : candidates) {
        QFile qss(path);
        if (!qss.open(QFile::ReadOnly))
            continue;
        qApp->setStyleSheet(QString::fromUtf8(qss.readAll()));
        QFont f = qApp->font();
        if (f.pointSize() <= 0)
            f.setPointSize(10);
        qApp->setFont(f);
        return;
    }
}

void MainWindow::center_window() {
    if (auto* screen = QGuiApplication::primaryScreen()) {
        const QRect g = screen->geometry();
        setGeometry((g.width() - 1200) / 2, (g.height() - 800) / 2, 1200, 800);
    }
}
