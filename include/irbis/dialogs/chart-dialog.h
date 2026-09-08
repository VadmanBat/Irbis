#pragma once

#include <QChart>
#include <QDialog>
#include <QPen>
#include <QVector>

class QComboBox;
class QLineEdit;
class QLineSeries;
class QPushButton;
class QSpinBox;

namespace Ui {
class ChartDialog;
}

class ChartDialog : public QDialog {
    Q_OBJECT

private:
    Ui::ChartDialog* ui;
    QChart* chart_;
    QVector<QPen> init_pens_;
    QVector<QPen> current_pens_;
    QVector<QLineEdit*> series_name_edits_;
    QVector<QPushButton*> color_buttons_;
    QVector<QSpinBox*> width_spin_boxes_;
    QVector<QComboBox*> style_combo_boxes_;
    QVector<QLineSeries*> line_series_;

    void build_series_editors();
    void change_series_color(int index);
    void change_series_style(int index);
    [[nodiscard]] QAbstractAxis* axis(Qt::Orientation orientation) const;
    static Qt::PenStyle pen_style_from_index(int index);
    static int index_from_pen_style(Qt::PenStyle style);

private slots:
    void applyChanges();
    void restoreChart();

public:
    explicit ChartDialog(QChart* chart, QWidget* parent = nullptr);
    ~ChartDialog() override;
};
