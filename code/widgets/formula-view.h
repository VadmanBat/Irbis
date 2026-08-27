#pragma once

#include <QLabel>

/// Read-only math HTML (numina sup/sub). Height follows wrapped content.
class FormulaView : public QLabel {
    Q_OBJECT

private:
    QString html_;

public:
    explicit FormulaView(QWidget* parent = nullptr);

    void setHtml(const QString& html);
    [[nodiscard]] QString html() const noexcept { return html_; }
};
