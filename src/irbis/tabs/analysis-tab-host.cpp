#include "irbis/tabs/analysis-tab.h"

#include "irbis/util/tf-builder.hpp"
#include "ui_analysis-tab.h"

bool AnalysisTab::showPlant(std::vector<double> num, std::vector<double> den, const double tau) {
    if (!apply_plant(std::move(num), std::move(den), tau))
        return false;
    if (ui->charts->empty())
        addTransferFunction();
    else
        replaceTransferFunction();
    return hasPlant();
}

bool AnalysisTab::hasPlant() const {
    const auto* display = panel_ ? panel_->display() : nullptr;
    return display && !display->isEmpty() && tf_builder::validInput(display->numerator(), display->denominator());
}

std::vector<double> AnalysisTab::plantNumerator() const {
    return hasPlant() ? panel_->display()->numerator() : std::vector<double>{};
}

std::vector<double> AnalysisTab::plantDenominator() const {
    return hasPlant() ? panel_->display()->denominator() : std::vector<double>{};
}

double AnalysisTab::plantDelay() const {
    return hasPlant() ? panel_->display()->delay() : 0.0;
}
