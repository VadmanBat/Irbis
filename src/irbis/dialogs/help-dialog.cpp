#include "irbis/dialogs/help-dialog.h"

#include "irbis/util/dialog-icons.hxx"
#include "ui_help-dialog.h"

HelpDialog::HelpDialog(QWidget* parent) : QDialog(parent), ui(new Ui::HelpDialog) {
    ui->setupUi(this);
    dialog_icons::apply(this, dialog_icons::Kind::Help);
}

HelpDialog::~HelpDialog() {
    delete ui;
}
