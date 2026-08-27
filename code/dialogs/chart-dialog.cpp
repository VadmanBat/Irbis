#include "code/dialogs/chart-dialog.h"

#include "code/charts/utils/chart-utils.hpp"
#include "code/util/dialog-icons.hxx"
#include "code/util/secondary-text.hxx"
#include "ui_chart-dialog.h"

#include <QColorDialog>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLineSeries>
#include <QPushButton>
#include <QSpinBox>
#include <QValueAxis>

ChartDialog::ChartDialog(QChart* chart, QWidget* parent) : QDialog(parent), ui(new Ui::ChartDialog), chart_(chart) {
    ui->setupUi(this);
    dialog_icons::apply(this, dialog_icons::Kind::ChartProps);
    secondary_text::apply(ui->seriesHint);
    ui->titleEdit->setText(chart_->title());
    if (auto* ax = axis(Qt::Horizontal))
        ui->xAxisLabelEdit->setText(ax->titleText());
    if (auto* ax = axis(Qt::Vertical))
        ui->yAxisLabelEdit->setText(ax->titleText());

    build_series_editors();

    connect(ui->applyButton, &QPushButton::clicked, this, &ChartDialog::applyChanges);
    connect(ui->cancelButton, &QPushButton::clicked, this, &ChartDialog::reject);
    connect(this, &QDialog::rejected, this, &ChartDialog::restoreChart);
}

ChartDialog::~ChartDialog() {
    delete ui;
}

QAbstractAxis* ChartDialog::axis(Qt::Orientation orientation) const {
    const auto axes = chart_->axes(orientation);
    return axes.isEmpty() ? nullptr : axes.first();
}

Qt::PenStyle ChartDialog::pen_style_from_index(int index) {
    switch (index) {
        case 1:
            return Qt::DashLine;
        case 2:
            return Qt::DotLine;
        case 3:
            return Qt::DashDotLine;
        case 4:
            return Qt::DashDotDotLine;
        default:
            return Qt::SolidLine;
    }
}

int ChartDialog::index_from_pen_style(Qt::PenStyle style) {
    switch (style) {
        case Qt::DashLine:
            return 1;
        case Qt::DotLine:
            return 2;
        case Qt::DashDotLine:
            return 3;
        case Qt::DashDotDotLine:
            return 4;
        default:
            return 0;
    }
}

void ChartDialog::build_series_editors() {
    auto* layout = new QGridLayout(ui->seriesHost);
    layout->addWidget(new QLabel(tr("Название")), 0, 0);
    layout->addWidget(new QLabel(tr("Цвет")), 0, 1);
    layout->addWidget(new QLabel(tr("Толщина")), 0, 2);
    layout->addWidget(new QLabel(tr("Стиль")), 0, 3);

    int index = 0;
    for (auto* series : chart_->series()) {
        auto* line_series = qobject_cast<QLineSeries*>(series);
        if (!line_series || chart_utils::isAccessorySeries(chart_, series))
            continue;

        auto* name_edit    = new QLineEdit(line_series->name());
        auto* color_button = new QPushButton;
        auto* width_spin   = new QSpinBox;
        auto* style_combo  = new QComboBox;

        color_button->setObjectName(QStringLiteral("seriesColorButton"));
        color_button->setAutoFillBackground(true);
        {
            QPalette pal = color_button->palette();
            pal.setColor(QPalette::Button, line_series->pen().color());
            color_button->setPalette(pal);
        }
        width_spin->setRange(1, 10);
        width_spin->setValue(line_series->pen().width());
        style_combo->addItems(
            {tr("Сплошная"), tr("Штриховая"), tr("Точечная"), tr("Штрих-точка"), tr("Штрих-точка-точка")});
        style_combo->setCurrentIndex(index_from_pen_style(line_series->pen().style()));

        connect(color_button, &QPushButton::clicked, this, [this, index] { change_series_color(index); });
        connect(width_spin, QOverload<int>::of(&QSpinBox::valueChanged), this,
                [this, index] { change_series_style(index); });
        connect(style_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this, index] { change_series_style(index); });

        ++index;
        layout->addWidget(name_edit, index, 0);
        layout->addWidget(color_button, index, 1);
        layout->addWidget(width_spin, index, 2);
        layout->addWidget(style_combo, index, 3);

        series_name_edits_.append(name_edit);
        color_buttons_.append(color_button);
        width_spin_boxes_.append(width_spin);
        style_combo_boxes_.append(style_combo);
        line_series_.append(line_series);
        init_pens_.append(line_series->pen());
    }
    current_pens_ = init_pens_;
}

void ChartDialog::change_series_color(int index) {
    const QColor new_color = QColorDialog::getColor(line_series_[index]->pen().color(), this);
    if (!new_color.isValid())
        return;
    current_pens_[index].setColor(new_color);
    line_series_[index]->setPen(current_pens_[index]);
    QPalette pal = color_buttons_[index]->palette();
    pal.setColor(QPalette::Button, new_color);
    color_buttons_[index]->setPalette(pal);
}

void ChartDialog::change_series_style(int index) {
    current_pens_[index].setWidth(width_spin_boxes_[index]->value());
    current_pens_[index].setStyle(pen_style_from_index(style_combo_boxes_[index]->currentIndex()));
    line_series_[index]->setPen(current_pens_[index]);
}

void ChartDialog::applyChanges() {
    chart_->setTitle(ui->titleEdit->text());
    if (auto* ax = axis(Qt::Horizontal))
        ax->setTitleText(ui->xAxisLabelEdit->text());
    if (auto* ax = axis(Qt::Vertical))
        ax->setTitleText(ui->yAxisLabelEdit->text());
    const int n = line_series_.size();
    for (int i = 0; i < n; ++i) {
        QString name = series_name_edits_[i]->text().trimmed();
        // Reserved guide ids — must not collide with origin crosshair series.
        if (name == QLatin1String("hor-line") || name == QLatin1String("ver-line") || name.isEmpty())
            name = tr("Кривая %1").arg(i + 1);
        line_series_[i]->setName(name);
        line_series_[i]->setPen(current_pens_[i]);
    }
    accept();
}

void ChartDialog::restoreChart() {
    const int n = line_series_.size();
    for (int i = 0; i < n; ++i)
        line_series_[i]->setPen(init_pens_[i]);
}
