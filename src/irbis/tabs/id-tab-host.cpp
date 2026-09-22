#include "irbis/tabs/id-tab.h"

#include "irbis/util/tf-builder.hpp"
#include "ui_id-tab.h"

#include <QComboBox>
#include <QFileInfo>

bool IdTab::loadExperiment(const QString& path, const int method, const IdSettings::PlantKind kind) {
    if (path.isEmpty())
        return false;
    ui->methodCombo->setCurrentIndex(method);
    ui->plantKindCombo->setCurrentIndex(static_cast<int>(kind));
    file_path_ = path;
    ui->fileLabel->setToolTip(path);
    if (!preview_loaded_file()) {
        file_path_.clear();
        has_data_ = false;
        ui->fileLabel->setText(tr("Файл не выбран"));
        ui->fileLabel->setToolTip({});
        return false;
    }
    ui->fileLabel->setText(QFileInfo(path).fileName());
    return true;
}

void IdTab::setPlantKind(const IdSettings::PlantKind kind) {
    ui->plantKindCombo->setCurrentIndex(static_cast<int>(kind));
}

void IdTab::showPlant(std::vector<double> num, std::vector<double> den, const double tau) {
    if (!tf_builder::validInput(num, den))
        return;
    panel_->setTransferFunction(tf_builder::plant(std::move(num), std::move(den)), tau < 0.0 ? 0.0 : tau);
}

bool IdTab::hasIdentifiedPlant() const {
    const auto* display = panel_ ? panel_->display() : nullptr;
    return display && !display->isEmpty() && tf_builder::validInput(display->numerator(), display->denominator());
}

std::vector<double> IdTab::plantNumerator() const {
    return hasIdentifiedPlant() ? panel_->display()->numerator() : std::vector<double>{};
}

std::vector<double> IdTab::plantDenominator() const {
    return hasIdentifiedPlant() ? panel_->display()->denominator() : std::vector<double>{};
}

double IdTab::plantDelay() const {
    return hasIdentifiedPlant() ? panel_->display()->delay() : 0.0;
}
