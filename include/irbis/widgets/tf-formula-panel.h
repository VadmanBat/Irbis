#pragma once

#include "irbis/widgets/tf-display-widget.h"
#include "numina/classes/control/models/transfer-function.h"

#include <QFrame>
#include <QString>

class QContextMenuEvent;
class QEnterEvent;
class QEvent;
class QLabel;
class QObject;
class QPushButton;
class QResizeEvent;
class QScrollBar;
class QTimer;
class QWidget;

/// Formula W(p). Actions appear on hover: Изменить / Копировать / Вставить / Подробнее.
class TfFormulaPanel : public QFrame {
    Q_OBJECT

private:
    bool exact_delay_{false};
    bool card_frame_{true};
    bool edit_visible_{true};
    bool paste_visible_{true};
    QString link_name_;

    TfDisplayWidget* display_{nullptr};
    QWidget* clip_{nullptr};
    QScrollBar* hbar_{nullptr};
    QLabel* name_label_{nullptr};
    QWidget* actions_host_{nullptr};
    QPushButton* edit_btn_{nullptr};
    QPushButton* copy_btn_{nullptr};
    QPushButton* paste_btn_{nullptr};
    QPushButton* details_btn_{nullptr};
    QTimer* hide_timer_{nullptr};

    [[nodiscard]] int border_x() const;
    [[nodiscard]] int chrome_width() const;
    void sync_scroll();
    void place_hbar();
    void place_actions();
    void set_actions_open(bool on);
    void sync_actions();
    void update_link_name();
    void copy_clicked();
    void details_clicked();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

public:
    explicit TfFormulaPanel(QWidget* parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

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
