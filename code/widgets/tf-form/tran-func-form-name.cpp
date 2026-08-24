#include "code/dialogs/tran-func-dialog.h"
#include "code/widgets/tf-form/tran-func-form-line-edit.hpp"
#include "code/widgets/tf-form/tran-func-form.h"

#include <cmath>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>

void TranFuncForm::create_label_context_menu(QLabel* label) {
    auto* about_action = new QAction(tr("Подробнее"), label);
    connect(about_action, &QAction::triggered, this, [this] {
        if (!tf_)
            return;
        const double tau = exact_delay_solutions_ ? delayTime() : 0.0;
        TranFuncDialog dialog(*tf_, this, tau);
        dialog.exec();
    });
    auto* menu = new QMenu(label);
    menu->addAction(about_action);
    label->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(label, &QLabel::customContextMenuRequested, this,
            [menu, label](const QPoint& pos) { menu->exec(label->mapToGlobal(pos)); });
}

void TranFuncForm::update_name_label() {
    if (name_label_)
        name_label_->setText(linkName());
}

void TranFuncForm::bindNameLabel(QLabel* label) {
    name_label_ = label;
    for (auto* le : numerator_)
        connect(le, &QLineEdit::editingFinished, this, &TranFuncForm::update_name_label);
    for (auto* le : denominator_)
        connect(le, &QLineEdit::editingFinished, this, &TranFuncForm::update_name_label);
    if (delay_element_)
        connect(delay_element_, &QDoubleSpinBox::editingFinished, this, &TranFuncForm::update_name_label);
}

QString TranFuncForm::linkName() const {
    QString name;
    switch (id_) {
        case 1 + 1 * 64:
            name = tr("Усилительное (безынерционное)");
            break;
        case 1 + 2 * 64:
            name = tr("Идеальное интегрирующее (астатическое)");
            break;
        case 1 + 3 * 64:
            name = tr("Инерционное 1-го порядка (апериодическое)");
            break;
        case 1 + 5 * 64:
            name = tr("Вырожденное колебательное (консервативное)");
            break;
        case 1 + 6 * 64:
            name = tr("Реальное интегрирующее (инерционное)");
            break;
        case 1 + 7 * 64: {
            const double a1 = denominator_[1]->stored;
            const double a2 = denominator_[2]->stored;
            if (a2 > 0.0 && a1 / (2.0 * std::sqrt(a2)) < 1.0)
                name = tr("Колебательное");
            else
                name = tr("Инерционное 2-го порядка (апериодическое)");
            break;
        }
        case 2 + 1 * 64:
            name = tr("Идеальное дифференцирующее");
            break;
        case 2 + 3 * 64:
            name = tr("Инерционное (реальное) дифференцирующее");
            break;
        case 4 + 3 * 64:
            name = tr("Реальное дифференцирующее 2-го порядка");
            break;
        case 3 + 2 * 64:
            name = tr("Изодромное");
            break;
        case 3 + 1 * 64:
            name = tr("Форсирующее");
            break;
        case 3 + 3 * 64:
            name = tr("Инерционно-форсирующее");
            break;
        case 63 + 21 * 64:
            name = tr("Пропорционально-дифференциальное 2-го порядка");
            break;
        default:
            name = tr("Неизвестно");
            break;
    }
    if (hasDelay())
        name += tr(" с запаздыванием");
    return name;
}
