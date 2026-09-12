#include "irbis/dialogs/help-dialog.h"

#include "irbis/util/dialog-icons.hxx"
#include "ui_help-dialog.h"

HelpDialog::HelpDialog(Topic topic, QWidget* parent) : QDialog(parent), ui(new Ui::HelpDialog) {
    ui->setupUi(this);
    dialog_icons::apply(this, dialog_icons::Kind::Help);
    const int index = static_cast<int>(topic);
    ui->pages->setCurrentIndex(index);
    switch (topic) {
        case Topic::Identification:
            setWindowTitle(tr("Справка: идентификация"));
            break;
        case Topic::Analysis:
            setWindowTitle(tr("Справка: анализ"));
            break;
        case Topic::Synthesis:
            setWindowTitle(tr("Справка: синтез регулятора"));
            break;
        case Topic::Rim:
            setWindowTitle(tr("Справка: настройка РИМ"));
            break;
    }
}

HelpDialog::~HelpDialog() {
    delete ui;
}
