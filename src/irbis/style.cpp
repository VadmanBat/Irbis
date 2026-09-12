#include "irbis/style.hpp"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QString>
#include <QStringList>

static void init_irbis_resources() {
    Q_INIT_RESOURCE(irbis_resources);
}

namespace {
bool fonts_loaded_ = false;

[[nodiscard]] QString read_qss(const QString& path) {
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
}
}

namespace irbis {
void loadFonts() {
    init_irbis_resources();
    if (fonts_loaded_)
        return;
    const QStringList candidates = {
        QStringLiteral(":/irbis/fonts/font-awesome-6-free-solid-900.otf"),
        QStringLiteral("data/fonts/font-awesome-6-free-solid-900.otf"),
        QStringLiteral("fonts/font-awesome-6-free-solid-900.otf"),
    };
    for (const QString& path : candidates) {
        if (QFontDatabase::addApplicationFont(path) >= 0) {
            fonts_loaded_ = true;
            return;
        }
    }
}

QString defaultStyleSheet() {
    init_irbis_resources();
    const QStringList candidates = {
        QStringLiteral("data/styles/app.qss"),     QStringLiteral("styles/app.qss"),
        QStringLiteral(":/irbis/styles/app.qss"),  QStringLiteral("data/styles/button-style.qss"),
        QStringLiteral("styles/button-style.qss"), QStringLiteral(":/irbis/styles/button-style.qss"),
    };
    for (const QString& path : candidates) {
        const QString qss = read_qss(path);
        if (!qss.isEmpty())
            return qss;
    }
    return {};
}

void applyStyleSheet() {
    loadFonts();
    const QString qss = defaultStyleSheet();
    if (qss.isEmpty() || !qApp)
        return;
    qApp->setStyleSheet(qss);
    QFont font = qApp->font();
    if (font.pointSize() <= 0)
        font.setPointSize(10);
    qApp->setFont(font);
}
}
