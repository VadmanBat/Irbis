#include "mainwindow.h"

#include "irbis/style.hpp"
#include "irbis/util/dialog-icons.hxx"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QScreen>

MainWindow::MainWindow(QWidget* parent) : QWidget(parent), ui(new Ui::MainWindow) {
    irbis::loadFonts();
    ui->setupUi(this);
    irbis::applyStyleSheet();
    const QIcon app_icon(QStringLiteral(":/icons/irbis.ico"));
    if (!app_icon.isNull()) {
        setWindowIcon(app_icon);
        qApp->setWindowIcon(app_icon);
    }
    else {
        dialog_icons::apply(this, dialog_icons::Kind::App);
        qApp->setWindowIcon(windowIcon());
    }
    center_window();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::center_window() {
    if (auto* screen = QGuiApplication::primaryScreen()) {
        const QRect g = screen->geometry();
        setGeometry((g.width() - 1200) / 2, (g.height() - 800) / 2, 1200, 800);
    }
}
