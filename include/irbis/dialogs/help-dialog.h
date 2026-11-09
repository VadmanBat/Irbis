#pragma once
// Created by Vadim 25.07.2026.

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
