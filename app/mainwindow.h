#pragma once

#include <QWidget>

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget {
    Q_OBJECT

private:
    Ui::MainWindow* ui;

    void center_window();

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
};
