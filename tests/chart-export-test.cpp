#include "irbis/charts/utils/chart-utils.hpp"

#include <cstdio>
#include <cstdlib>

#include <QApplication>
#include <QChart>
#include <QDir>
#include <QFile>
#include <QLineSeries>
#include <QString>

namespace {
int g_failed = 0;

void expect_true(const char* name, bool cond) {
    if (!cond) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    }
    else {
        std::printf("ok   %s\n", name);
    }
}

void expect_eq(const char* name, const QString& got, const QString& expected) {
    if (got != expected) {
        std::fprintf(stderr, "FAIL %s\n--- got ---\n%s\n--- expected ---\n%s\n", name, got.toUtf8().constData(),
                     expected.toUtf8().constData());
        ++g_failed;
    }
    else {
        std::printf("ok   %s\n", name);
    }
}

[[nodiscard]] QString exported(QChart* chart) {
    const QString path = QDir::temp().filePath(QStringLiteral("irbis-chart-export-test.txt"));
    QFile::remove(path);
    if (!chart_utils::saveChartToFile(path, chart))
        return QStringLiteral("SAVE FAILED");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QString text = QString::fromUtf8(file.readAll());
    file.close();
    QFile::remove(path);
    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    return text;
}

void add_curve(QChart* chart, const QString& name, const chart_utils::VecPair& points) {
    (void)chart_utils::addRealSeries(chart, points, name);
}

void prepare(QChart* chart) {
    chart_utils::createAxes(chart, QStringLiteral("Время, с"), QStringLiteral("Положение, %"));
}
} // namespace

int main(int argc, char** argv) {
    QApplication application(argc, argv);
    (void)application;

    {
        QChart chart;
        prepare(&chart);
        add_curve(&chart, QStringLiteral("Файл"), {{0.0, 50.0}, {7.0, 40.0}});
        add_curve(&chart, QStringLiteral("S-регулятор"), {{0.0, 50.0}, {1.0, 49.0}, {2.0, 47.0}});
        add_curve(&chart, QStringLiteral("РИМ"), {{0.0, 50.0}, {1.0, 50.0}, {7.0, 48.0}});
        for (QAbstractSeries* series : chart.series()) {
            if (series->name() == QStringLiteral("РИМ"))
                series->setVisible(false);
        }
        chart_utils::updateAxes(&chart, {0.0, 10.0}, {0.0, 100.0}, chart_utils::GridMode::Tab, false, false);

        const QString text = exported(&chart);
        expect_eq("position columns", text,
                  QStringLiteral("Время, с\tФайл\tS-регулятор\n"
                                 "0\t50\t50\n"
                                 "1\t\t49\n"
                                 "2\t\t47\n"
                                 "7\t40\t\n"));
        expect_true("guides omitted", !text.contains(QStringLiteral("hor-line")) && !text.contains(QStringLiteral("ver-line")));
        expect_true("hidden curve omitted", !text.contains(QStringLiteral("РИМ")));
    }

    {
        QChart chart;
        prepare(&chart);
        add_curve(&chart, QStringLiteral("A"), {{0.0, 1.0}, {1.0, 2.0}});
        add_curve(&chart, QStringLiteral("B"), {{0.0, 3.0}, {1.0 + 1e-10, 4.0}});
        const QString text = exported(&chart);
        expect_eq("shared grid", text, QStringLiteral("Время, с\tA\tB\n0\t1\t3\n1\t2\t4\n"));
    }

    {
        QChart chart;
        prepare(&chart);
        add_curve(&chart, QStringLiteral("A"), {{0.0, 10.0}, {2.0, 12.0}});
        add_curve(&chart, QStringLiteral("B"), {{0.0, 20.0}, {1.0, 21.0}, {2.0 + 1e-9, 22.0}});
        const QString text = exported(&chart);
        expect_eq("near times merge", text,
                  QStringLiteral("Время, с\tA\tB\n"
                                 "0\t10\t20\n"
                                 "1\t\t21\n"
                                 "2\t12\t22\n"));
    }

    {
        QChart chart;
        prepare(&chart);
        add_curve(&chart, QStringLiteral("A"), {{0.0, 1.0}, {0.0, 2.0}});
        add_curve(&chart, QStringLiteral("B"), {{0.0, 3.0}});
        const QString text = exported(&chart);
        expect_eq("duplicate abscissa", text,
                  QStringLiteral("Время, с\tA\tВремя, с\tB\n"
                                 "0\t1\t0\t3\n"
                                 "0\t2\t\t\n"));
    }

    {
        QChart chart;
        prepare(&chart);
        chart_utils::updateAxes(&chart, {-1.0, 1.0}, {-1.0, 1.0}, chart_utils::GridMode::Tab, false, false);
        expect_eq("guides only", exported(&chart), QString());
    }

    expect_true("null chart", !chart_utils::saveChartToFile(QStringLiteral("unused.txt"), nullptr));

    if (g_failed != 0) {
        std::fprintf(stderr, "%d failed\n", g_failed);
        return EXIT_FAILURE;
    }
    std::printf("all ok\n");
    return EXIT_SUCCESS;
}
