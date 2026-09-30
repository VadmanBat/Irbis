#include "irbis/charts/utils/chart-utils.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <QFile>
#include <QList>
#include <QLocale>
#include <QPointF>
#include <QStringConverter>
#include <QTextStream>
#include <QValueAxis>
#include <QXYSeries>

namespace chart_utils {
namespace {
struct Column {
    QString name;
    QList<QPointF> points;
};

[[nodiscard]] QString plain(QString text) {
    text.replace(QLatin1Char('\t'), QLatin1Char(' '));
    text.replace(QLatin1Char('\n'), QLatin1Char(' '));
    text.replace(QLatin1Char('\r'), QLatin1Char(' '));
    return text;
}

[[nodiscard]] QString abscissa_title(const QChart* chart) {
    const auto axes = chart->axes(Qt::Horizontal);
    if (auto* axis = qobject_cast<QValueAxis*>(axes.value(0))) {
        const QString title = plain(axis->titleText());
        if (!title.isEmpty())
            return title;
    }
    return QStringLiteral("x");
}

[[nodiscard]] QList<Column> data_columns(QChart* chart) {
    QList<Column> columns;
    const auto series_list = chart->series();
    columns.reserve(series_list.size());
    for (QAbstractSeries* series : series_list) {
        if (isAccessorySeries(chart, series))
            continue;
        auto* xy = qobject_cast<QXYSeries*>(series);
        if (xy == nullptr || xy->count() == 0)
            continue;
        columns.push_back(Column{plain(series->name()), xy->points()});
    }
    return columns;
}

/// Время растёт, повторных абсцисс нет — кривую можно разложить по общему столбцу времени.
[[nodiscard]] bool monotonic_unique(const QList<QPointF>& points) {
    for (qsizetype i = 1; i < points.size(); ++i) {
        if (!(points.at(i).x() > points.at(i - 1).x()))
            return false;
    }
    return true;
}

[[nodiscard]] double min_step(const QList<Column>& columns) {
    double step = std::numeric_limits<double>::infinity();
    for (const Column& column : columns) {
        const QList<QPointF>& points = column.points;
        for (qsizetype i = 1; i < points.size(); ++i)
            step = std::min(step, points.at(i).x() - points.at(i - 1).x());
    }
    return step;
}

[[nodiscard]] bool same_grid(const QList<Column>& columns, double tolerance) {
    const QList<QPointF>& base = columns.front().points;
    for (qsizetype c = 1; c < columns.size(); ++c) {
        const QList<QPointF>& points = columns.at(c).points;
        if (points.size() != base.size())
            return false;
        for (qsizetype i = 0; i < base.size(); ++i) {
            if (std::fabs(points.at(i).x() - base.at(i).x()) > tolerance)
                return false;
        }
    }
    return true;
}

void write_names(QTextStream& out, const QString& x_title, const QList<Column>& columns, bool own_abscissa) {
    for (qsizetype c = 0; c < columns.size(); ++c) {
        if (c > 0)
            out << '\t';
        if (own_abscissa || c == 0)
            out << x_title << '\t';
        out << columns.at(c).name;
    }
    out << '\n';
}

void write_shared(QTextStream& out, const QString& x_title, const QList<Column>& columns) {
    write_names(out, x_title, columns, false);
    const qsizetype n = columns.front().points.size();
    for (qsizetype i = 0; i < n; ++i) {
        out << columns.front().points.at(i).x();
        for (const Column& column : columns)
            out << '\t' << column.points.at(i).y();
        out << '\n';
    }
}

void write_aligned(QTextStream& out, const QString& x_title, const QList<Column>& columns, double tolerance) {
    std::vector<double> samples;
    std::size_t total = 0;
    for (const Column& column : columns)
        total += static_cast<std::size_t>(column.points.size());
    samples.reserve(total);
    for (const Column& column : columns) {
        for (const QPointF& point : column.points)
            samples.push_back(point.x());
    }
    std::sort(samples.begin(), samples.end());

    std::vector<double> times;
    times.reserve(samples.size());
    for (double x : samples) {
        if (times.empty() || x > times.back() + tolerance)
            times.push_back(x);
    }

    std::vector<qsizetype> cursor(static_cast<std::size_t>(columns.size()), 0);
    write_names(out, x_title, columns, false);
    for (double time : times) {
        out << time;
        for (qsizetype c = 0; c < columns.size(); ++c) {
            out << '\t';
            const QList<QPointF>& points = columns.at(c).points;
            qsizetype& index             = cursor[static_cast<std::size_t>(c)];
            while (index < points.size() && points.at(index).x() < time - tolerance)
                ++index;
            if (index < points.size() && std::fabs(points.at(index).x() - time) <= tolerance) {
                out << points.at(index).y();
                ++index;
            }
        }
        out << '\n';
    }
}

/// Кривая не является функцией времени: у каждой остаётся своя пара «абсцисса, значение».
void write_paired(QTextStream& out, const QString& x_title, const QList<Column>& columns) {
    write_names(out, x_title, columns, true);
    qsizetype rows = 0;
    for (const Column& column : columns)
        rows = std::max(rows, column.points.size());
    for (qsizetype i = 0; i < rows; ++i) {
        for (qsizetype c = 0; c < columns.size(); ++c) {
            if (c > 0)
                out << '\t';
            const QList<QPointF>& points = columns.at(c).points;
            if (i < points.size())
                out << points.at(i).x() << '\t' << points.at(i).y();
            else
                out << '\t';
        }
        out << '\n';
    }
}
} // namespace

bool saveChartToFile(const QString& fileName, QChart* chart) {
    if (chart == nullptr)
        return false;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out.setGenerateByteOrderMark(true);
    out.setLocale(QLocale::c());

    const QList<Column> columns = data_columns(chart);
    if (columns.isEmpty())
        return true;

    const QString x_title = abscissa_title(chart);
    const bool functional = std::all_of(columns.cbegin(), columns.cend(), [](const Column& column) {
        return monotonic_unique(column.points);
    });
    if (!functional) {
        write_paired(out, x_title, columns);
        return true;
    }

    // Доля минимального шага: 7 и 7+eps попадают в одну строку, соседние отсчёты не сливаются.
    const double step      = min_step(columns);
    const double tolerance = std::isfinite(step) ? step * 1e-6 : 0.0;
    if (same_grid(columns, tolerance))
        write_shared(out, x_title, columns);
    else
        write_aligned(out, x_title, columns, tolerance);
    return true;
}
} // namespace chart_utils
