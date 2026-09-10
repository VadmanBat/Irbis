#pragma once

#include <QDialog>
#include <QString>

namespace Ui {
class HelpDialog;
}

class HelpDialog : public QDialog {
    Q_OBJECT
public:
    enum class Topic { Identification, Analysis, Synthesis, Rim };

private:
    Ui::HelpDialog* ui;

    struct Page {
        QString windowTitle;
        QString heading;
        QString html;
    };

    [[nodiscard]] QString identification_html() const;
    [[nodiscard]] QString analysis_html() const;
    [[nodiscard]] QString synthesis_html() const;
    [[nodiscard]] QString rim_html() const;
    [[nodiscard]] Page page_for(Topic topic) const;
    void apply_topic(Topic topic);

public:
    explicit HelpDialog(Topic topic, QWidget* parent = nullptr);
    ~HelpDialog() override;
};
