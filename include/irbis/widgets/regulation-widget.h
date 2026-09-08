#pragma once

#include <QWidget>
#include <utility>
#include <vector>

class QGridLayout;
class QLabel;
class QLineEdit;

class RegulationWidget : public QWidget {
    Q_OBJECT

private:
    QGridLayout* layout_{nullptr};
    std::vector<QLabel*> labels_;
    std::vector<QLineEdit*> line_edits_;
    std::vector<double> last_values_;
    std::vector<std::pair<int, int>> colors_; ///< per-cell: flash index when value down / up (0..2)

    void apply_default_style();
    [[nodiscard]] int color_index(std::size_t index, double old_value, double new_value) const;
    void update_cell_style(std::size_t index, double new_value);
    static QString format_double(double value);

public:
    explicit RegulationWidget(int rows = 3, int cols = 2, QWidget* parent = nullptr);

    /// Short HTML labels; optional tooltips (full metric names) on label + value field.
    void setLabels(const std::vector<QString>& labelNames, const std::vector<QString>& tooltips = {});
    void setColors(const std::vector<std::pair<int, int>>& valueColors);
    void updateValues(const std::vector<double>& values);
};
