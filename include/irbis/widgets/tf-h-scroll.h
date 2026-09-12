#pragma once

#include <QWidget>

class QBoxLayout;
class QEvent;
class QObject;
class QResizeEvent;
class QScrollBar;

/// Clips a child horizontally and shows an overlay scrollbar when it overflows.
class TfHScroll : public QWidget {
    Q_OBJECT

private:
    QWidget* clip_{nullptr};
    QWidget* content_{nullptr};
    QScrollBar* hbar_{nullptr};

    [[nodiscard]] QSize content_hint() const;
    void place_hbar();
    void sync_scroll();

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

public:
    explicit TfHScroll(QWidget* content, QWidget* parent = nullptr);

    static TfHScroll* wrapInLayout(QWidget* content, QBoxLayout* layout);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;
    [[nodiscard]] QWidget* content() const noexcept { return content_; }
};

