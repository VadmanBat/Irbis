#pragma once

#include "irbis/widgets/tf-display-widget.h"
#include "numina/classes/control/models/transfer-function.h"

#include <QFrame>
#include <QString>

class QLabel;
class QPushButton;
class QWidget;

/// Formula W(p) plus actions: Изменить / Копировать / Вставить / Подробнее.
class TfFormulaPanel : public QFrame {
    Q_OBJECT

private:
    bool exact_delay_{false};
    bool card_frame_{true};
    QString link_name_;

    TfDisplayWidget* display_{nullptr};
    QLabel* name_label_{nullptr};
    QWidget* actions_host_{nullptr};
    QPushButton* edit_btn_{nullptr};
    QPushButton* copy_btn_{nullptr};
    QPushButton* paste_btn_{nullptr};
    QPushButton* details_btn_{nullptr};

    void sync_actions();
    void sync_action_widths();
    void update_link_name();
    void copy_clicked();
    void details_clicked();

public:
    explicit TfFormulaPanel(QWidget* parent = nullptr);

    [[nodiscard]] TfDisplayWidget* display() const noexcept { return display_; }
    [[nodiscard]] QString linkName() const { return link_name_; }

    void setCardFrame(bool on);
    void setPasteVisible(bool on);
    void setEditVisible(bool on);
    void setExactDelaySolutions(bool on) noexcept { exact_delay_ = on; }
    void setTitle(const QString& html);
    void setTransferFunction(const numina::TransferFunction& tf, double tau = 0.0);
    void clear();

    [[nodiscard]] bool isEmpty() const noexcept;

signals:
    void editRequested();
    void pasteRequested();
    void contentsChanged();
};
