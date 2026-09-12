#pragma once

#include <QDialog>

namespace Ui {
class HelpDialog;
}

class HelpDialog : public QDialog {
    Q_OBJECT
public:
    enum class Topic { Identification, Analysis, Synthesis, Rim };

private:
    Ui::HelpDialog* ui;

public:
    explicit HelpDialog(Topic topic, QWidget* parent = nullptr);
    ~HelpDialog() override;
};
