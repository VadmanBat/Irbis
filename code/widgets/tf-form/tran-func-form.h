#pragma once

#include "numina/classes/control/transfer-function.h"

#include <QWidget>
#include <vector>

class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;

/// Editable W(p) = N/D · e^{-τp}. Dynamic degree fields (not pure .ui).
class TranFuncForm : public QWidget {
    Q_OBJECT

private:
    class LineEdit;

    using VecLine = std::vector<LineEdit*>;
    using Vec     = std::vector<double>;

    VecLine numerator_;
    VecLine denominator_;
    QDoubleSpinBox* delay_element_{nullptr};
    QWidget* delay_group_{nullptr};
    QPushButton* copy_btn_{nullptr};
    QPushButton* paste_btn_{nullptr};
    std::vector<bool> is_active_;
    QLabel* name_label_{nullptr};
    int id_{0};
    const numina::TransferFunction* tf_{nullptr};

    QLayout* create_coeff_row(VecLine& line_edits, int factor);
    void create_label_context_menu(QLabel* label);
    void update_name_label();
    void rebuild_activity_mask();
    void set_coeff_fields(VecLine& fields, const Vec& high_to_low);
    void set_field_value(LineEdit* line_edit, double value);
    void reposition_clip_buttons();

    static void adjust_line_edit_width(QLineEdit* line_edit);
    static Vec reverse_optimize(const Vec& container);
    static Vec get_line_edit_data(const VecLine& line_edits);
    static QString correct_line(const QString& text);

private slots:
    void copyToClipboard();
    void pasteFromClipboard();

public:
    explicit TranFuncForm(std::size_t n = 6, std::size_t m = 6, const QString& title = QStringLiteral("W(p) = "),
                          QWidget* parent = nullptr);

    [[nodiscard]] std::vector<double> numerator() const;
    [[nodiscard]] std::vector<double> denominator() const;
    [[nodiscard]] bool hasDelay() const;
    [[nodiscard]] double delayTime() const;
    [[nodiscard]] QString linkName() const;

    /// Machine + human-readable text for clipboard / report.
    [[nodiscard]] QString exportText() const;
    /// Apply from clipboard text (Irbis-TF-v1, also reads legacy RegValve-TF-v1).
    bool importText(const QString& text);

    void setTransferFunction(const numina::TransferFunction* tf);
    void bindNameLabel(QLabel* label);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
};
