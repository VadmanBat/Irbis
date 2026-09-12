#pragma once

#include <QDialog>
#include <QString>
#include <vector>

namespace Ui {
class TfInputDialog;
}

/// Specialized W(p) editor: space-separated num/den, degree-order toggle, delay τ.
class TfInputDialog : public QDialog {
    Q_OBJECT

public:
    using Vec = std::vector<double>;

private:
    Ui::TfInputDialog* ui;
    bool high_first_{false};
    Vec num_;
    Vec den_;
    double tau_{0.0};

    static bool high_first_pref_;

    [[nodiscard]] bool parse_poly(const QString& text, Vec& high_to_low, QString* error) const;
    void set_order(bool high_first);
    void fill_fields_from_value();
    void refresh_preview();
    void style_error_state(bool has_error, bool num_bad, bool den_bad);
    void show_error(const QString& message, bool num_bad, bool den_bad);
    void clear_error();
    [[nodiscard]] bool collect_valid(Vec& num, Vec& den, double& tau, QString* error, bool* num_bad,
                                     bool* den_bad) const;

private slots:
    void onFieldsChanged();
    void tryAccept();

public:
    explicit TfInputDialog(QWidget* parent = nullptr);
    ~TfInputDialog() override;

    void setSymbolHtml(const QString& html);
    void setValue(const Vec& num, const Vec& den, double tau);

    [[nodiscard]] const Vec& numerator() const noexcept { return num_; }
    [[nodiscard]] const Vec& denominator() const noexcept { return den_; }
    [[nodiscard]] double delay() const noexcept { return tau_; }

    /// Modal edit. Returns false on cancel. `symbolHtml` is W(p) or W<sub>ОУ</sub>(p).
    static bool edit(QWidget* parent, Vec& num, Vec& den, double& tau,
                     const QString& symbolHtml = QStringLiteral("W(p)"));
};
