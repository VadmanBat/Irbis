#pragma once

#include "numina/classes/control/models/transfer-function.h"

#include <QString>
#include <QWidget>
#include <vector>

class QLabel;

/// Read-only W(p)=N/D·e^{−τp}: HTML poly text (identification result).
class TfDisplayWidget : public QWidget {
    Q_OBJECT

public:
    using Vec = std::vector<double>;

private:
    bool empty_{true};
    double tau_{0.0};
    Vec num_;
    Vec den_;
    numina::TransferFunction tf_;

    QLabel* titleLabel_{nullptr};
    QLabel* numLabel_{nullptr};
    QLabel* denLabel_{nullptr};
    QLabel* delayLabel_{nullptr};
    QWidget* delayGroup_{nullptr};

    void set_polys(const Vec& num, const Vec& den, double tau);
    [[nodiscard]] QString plain_title() const;
    [[nodiscard]] QString human_text() const;
    [[nodiscard]] QString export_text() const;

public:
    /// Designer / uic: `new TfDisplayWidget(parent)`. Default title W(p) =.
    explicit TfDisplayWidget(QWidget* parent = nullptr);

    void setTitle(const QString& html);
    void setTransferFunction(const numina::TransferFunction& tf, double tau = 0.0);
    /// Symbolic W(p) = b₀+… / (1 + a₁p + …) for an empty identification result.
    void setStructureTemplate(int numDegree, int denDegree);
    void clear();

    [[nodiscard]] bool isEmpty() const noexcept { return empty_; }
    [[nodiscard]] double delay() const noexcept { return tau_; }
    [[nodiscard]] const Vec& numerator() const noexcept { return num_; }
    [[nodiscard]] const Vec& denominator() const noexcept { return den_; }
    [[nodiscard]] const numina::TransferFunction& transferFunction() const noexcept { return tf_; }

    void copyToClipboard();
    [[nodiscard]] bool importText(const QString& text);

signals:
    void contentsChanged();
};
