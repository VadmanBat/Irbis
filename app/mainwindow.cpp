#include "mainwindow.h"

#include "irbis/style.hpp"
#include "irbis/tabs/analysis-tab.h"
#include "irbis/tabs/synthesis-tab.h"
#include "irbis/util/dialog-icons.hxx"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QScreen>
#include <QTabBar>
#include <QToolButton>

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
    install_tab_corner();
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

void MainWindow::install_tab_corner() {
    auto* bar = ui->tabWidget->tabBar();
    bar->setExpanding(false);
    bar->setDocumentMode(false);

    auto* corner = new QWidget(ui->tabWidget);
    corner->setObjectName(QStringLiteral("tabCornerBar"));
    auto* lay    = new QHBoxLayout(corner);
    lay->setContentsMargins(10, 2, 8, 2);
    lay->setSpacing(2);

    auto* sep = new QFrame(corner);
    sep->setObjectName(QStringLiteral("tabCornerSep"));
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    lay->addWidget(sep);

    auto make_btn = [corner](const QString& name, QChar glyph, const QString& tip) {
        auto* btn = new QToolButton(corner);
        btn->setObjectName(name);
        btn->setToolTip(tip);
        btn->setAutoRaise(true);
        btn->setFocusPolicy(Qt::TabFocus);
        btn->setCursor(Qt::PointingHandCursor);
        dialog_icons::applyGlyph(btn, glyph, 11);
        return btn;
    };

    settings_btn_ = make_btn(QStringLiteral("tabSettingsButton"), QChar(0xf013),
                             tr("Параметры моделирования"));
    charts_btn_   = make_btn(QStringLiteral("tabChartsButton"), QChar(0xf201), tr("Настройка графиков"));
    lay->addWidget(settings_btn_);
    lay->addWidget(charts_btn_);

    ui->tabWidget->setCornerWidget(corner, Qt::TopRightCorner);

    connect(settings_btn_, &QToolButton::clicked, this, &MainWindow::open_model_settings);
    connect(charts_btn_, &QToolButton::clicked, this, &MainWindow::open_chart_settings);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int) { sync_tab_corner(); });
    sync_tab_corner();
}

void MainWindow::sync_tab_corner() {
    auto* page     = ui->tabWidget->currentWidget();
    const bool on  = qobject_cast<AnalysisTab*>(page) || qobject_cast<SynthesisTab*>(page);
    settings_btn_->setEnabled(on);
    charts_btn_->setEnabled(on);
}

void MainWindow::open_model_settings() {
    auto* page = ui->tabWidget->currentWidget();
    if (auto* analysis = qobject_cast<AnalysisTab*>(page))
        analysis->openSettings();
    else if (auto* synthesis = qobject_cast<SynthesisTab*>(page))
        synthesis->openSettings();
}

void MainWindow::open_chart_settings() {
    auto* page = ui->tabWidget->currentWidget();
    if (auto* analysis = qobject_cast<AnalysisTab*>(page))
        analysis->openChartSettings();
    else if (auto* synthesis = qobject_cast<SynthesisTab*>(page))
        synthesis->openChartSettings();
}
