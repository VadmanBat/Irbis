#include "code/util/format.hxx"
#include "code/widgets/tf-form/tran-func-form-line-edit.hpp"
#include "code/widgets/tf-form/tran-func-form.h"

#include <algorithm>
#include <QApplication>
#include <QClipboard>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QToolTip>

namespace {
std::vector<double> parse_coeff_list(QString line) {
    line.replace(',', ' ');
    line.replace(';', ' ');
    std::vector<double> out;
    const auto parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    for (const QString& p : parts) {
        bool ok        = false;
        const double v = num_format::parse(p, &ok);
        if (ok)
            out.push_back(v);
    }
    return out;
}
} // namespace

QString TranFuncForm::exportText() const {
    const auto num   = numerator();
    const auto den   = denominator();
    const double tau = delayTime();

    QStringList num_parts;
    QStringList den_parts;
    for (double v : num)
        num_parts << num_format::format(v, num_format::STORED_DIGITS);
    for (double v : den)
        den_parts << num_format::format(v, num_format::STORED_DIGITS);

    QString human = QStringLiteral("W(p) = (%1) / (%2)")
                        .arg(num_format::polyPlainLowFirst(num), num_format::polyPlainLowFirst(den));
    if (tau != 0.0)
        human += QStringLiteral(" · e^(-%1 p)").arg(num_format::formatFull(tau));

    return QStringLiteral(
               "Irbis-TF-v1\n"
               "num: %1\n"
               "den: %2\n"
               "tau: %3\n"
               "\n"
               "%4\n")
        .arg(num_parts.join(QLatin1Char(' ')), den_parts.join(QLatin1Char(' ')), num_format::formatFull(tau), human);
}

bool TranFuncForm::importText(const QString& text) {
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    Vec num, den;
    double tau      = 0.0;
    bool has_header = false;
    bool got_num    = false;
    bool got_den    = false;

    for (QString line : lines) {
        line = line.trimmed();
        if (line.startsWith(QStringLiteral("Irbis-TF-v1"), Qt::CaseInsensitive) ||
            line.startsWith(QStringLiteral("RegValve-TF-v1"), Qt::CaseInsensitive)) {
            has_header = true;
            continue;
        }
        if (line.startsWith(QStringLiteral("num:"), Qt::CaseInsensitive)) {
            num     = parse_coeff_list(line.mid(4));
            got_num = !num.empty();
            continue;
        }
        if (line.startsWith(QStringLiteral("den:"), Qt::CaseInsensitive)) {
            den     = parse_coeff_list(line.mid(4));
            got_den = !den.empty();
            continue;
        }
        if (line.startsWith(QStringLiteral("tau:"), Qt::CaseInsensitive)) {
            bool ok = false;
            tau     = line.mid(4).trimmed().toDouble(&ok);
            if (!ok)
                tau = 0.0;
            continue;
        }
    }

    if (!has_header || !got_num || !got_den)
        return false;

    set_coeff_fields(numerator_, num);
    set_coeff_fields(denominator_, den);
    delay_element_->setValue(std::clamp(tau, 0.0, 30.0));
    rebuild_activity_mask();
    update_name_label();
    return true;
}

void TranFuncForm::copyToClipboard() {
    QApplication::clipboard()->setText(exportText());
    QToolTip::showText(copy_btn_->mapToGlobal(QPoint(0, copy_btn_->height())), tr("ПФ скопирована"), copy_btn_, QRect(),
                       1500);
}

void TranFuncForm::pasteFromClipboard() {
    if (!importText(QApplication::clipboard()->text())) {
        QMessageBox::information(this, tr("Вставка ПФ"),
                                 tr("В буфере нет данных формата Irbis-TF-v1.\n"
                                    "Скопируйте ПФ кнопкой 📋 на вкладке «Анализ» или «Синтез»."));
        return;
    }
    QToolTip::showText(paste_btn_->mapToGlobal(QPoint(0, paste_btn_->height())), tr("ПФ вставлена"), paste_btn_,
                       QRect(), 1500);
}
