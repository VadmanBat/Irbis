#include "irbis/dialogs/help-dialog.h"

#include "irbis/util/dialog-icons.hxx"
#include "ui_help-dialog.h"

HelpDialog::HelpDialog(Topic topic, QWidget* parent) : QDialog(parent), ui(new Ui::HelpDialog) {
    ui->setupUi(this);
    dialog_icons::apply(this, dialog_icons::Kind::Help);
    apply_topic(topic);
}

HelpDialog::~HelpDialog() {
    delete ui;
}

void HelpDialog::apply_topic(Topic topic) {
    const Page page = page_for(topic);
    setWindowTitle(page.windowTitle);
    ui->titleLabel->setText(page.heading);
    ui->textEdit->setHtml(page.html);
}

HelpDialog::Page HelpDialog::page_for(Topic topic) const {
    switch (topic) {
        case Topic::Identification:
            return {tr("Справка: идентификация"), tr("Идентификация передаточной функции"), identification_html()};
        case Topic::Analysis:
            return {tr("Справка: анализ"), tr("Анализ передаточной функции"), analysis_html()};
        case Topic::Synthesis:
            return {tr("Справка: синтез регулятора"), tr("Синтез ПИД-регулятора"), synthesis_html()};
        case Topic::Rim:
            return {tr("Справка: настройка РИМ"), tr("Релейно-импульсный регулятор"), rim_html()};
    }
    return {};
}

