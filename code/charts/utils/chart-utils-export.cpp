#include "code/charts/utils/chart-utils.hpp"

#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QSvgGenerator>

#define CHART_TR(str) QCoreApplication::translate("chart_utils", str)

namespace chart_utils {
namespace {
enum class ExportFormat { Png, Svg, Txt };

ExportFormat format_from_suffix(const QString& path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QLatin1String("svg"))
        return ExportFormat::Svg;
    if (suffix == QLatin1String("txt"))
        return ExportFormat::Txt;
    return ExportFormat::Png;
}

ExportFormat format_from_filter(const QString& filter) {
    if (filter.contains(QLatin1String("*.svg"), Qt::CaseInsensitive))
        return ExportFormat::Svg;
    if (filter.contains(QLatin1String("*.txt"), Qt::CaseInsensitive))
        return ExportFormat::Txt;
    return ExportFormat::Png;
}

const char* suffix_for(ExportFormat fmt) {
    switch (fmt) {
        case ExportFormat::Svg:
            return ".svg";
        case ExportFormat::Txt:
            return ".txt";
        case ExportFormat::Png:
        default:
            return ".png";
    }
}

QString ensure_suffix(QString path, ExportFormat fmt) {
    if (QFileInfo(path).suffix().isEmpty())
        path += QLatin1String(suffix_for(fmt));
    return path;
}

bool save_png(QChartView* view, const QString& path) {
    return view->grab().save(path, "png");
}

bool save_svg(QChartView* view, const QString& path) {
    const QSize size = view->size();
    if (size.isEmpty())
        return false;

    QSvgGenerator generator;
    generator.setFileName(path);
    generator.setSize(size);
    generator.setViewBox(QRect(0, 0, size.width(), size.height()));
    if (QChart* chart = view->chart())
        generator.setTitle(chart->title());
    generator.setDescription(CHART_TR("График Irbis"));

    QPainter painter(&generator);
    if (!painter.isActive())
        return false;
    view->render(&painter);
    return true;
}
} // namespace

bool saveChartExport(QChartView* view, const QString& path) {
    if (!view || path.isEmpty())
        return false;

    switch (format_from_suffix(path)) {
        case ExportFormat::Svg:
            return save_svg(view, path);
        case ExportFormat::Txt:
            return saveChartToFile(path, view->chart());
        case ExportFormat::Png:
        default:
            return save_png(view, path);
    }
}

bool saveChartAsDialog(QWidget* parent, QChartView* view, const QString& suggestedName) {
    if (!view)
        return false;

    const QString filters   = CHART_TR("Рисунок PNG (*.png);;Рисунок SVG (*.svg);;Текст TXT (*.txt);;Все файлы (*)");
    QString selected_filter = CHART_TR("Рисунок PNG (*.png)");
    QString path =
        QFileDialog::getSaveFileName(parent, CHART_TR("Сохранить график"), suggestedName, filters, &selected_filter);
    if (path.isEmpty())
        return false;

    const ExportFormat fmt =
        QFileInfo(path).suffix().isEmpty() ? format_from_filter(selected_filter) : format_from_suffix(path);
    path = ensure_suffix(path, fmt);

    if (!saveChartExport(view, path)) {
        QMessageBox::warning(parent, CHART_TR("Ошибка"), CHART_TR("Не удалось сохранить график!"));
        return false;
    }
    return true;
}
} // namespace chart_utils
