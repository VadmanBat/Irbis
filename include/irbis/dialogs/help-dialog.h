#pragma once

#include <QDialog>

namespace Ui {
class HelpDialog;
}

class HelpDialog : public QDialog {
    Q_OBJECT
public:
    explicit HelpDialog(QWidget* parent = nullptr);
    ~HelpDialog() override;

private:
    Ui::HelpDialog* ui;
};
