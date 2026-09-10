#pragma once

#include <QWidget>

class QToolButton;

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget {
    Q_OBJECT

    Ui::MainWindow* ui;
    QToolButton* help_btn_{nullptr};
    QToolButton* settings_btn_{nullptr};
    QToolButton* charts_btn_{nullptr};

    void center_window();
    void install_tab_corner();
    void sync_tab_corner();
    void open_help();
    void open_model_settings();
    void open_chart_settings();

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
};
